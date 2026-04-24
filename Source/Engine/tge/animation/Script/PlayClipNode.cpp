#include <stdafx.h>
#include "PlayClipNode.h"

#include <tge/animation/Animation.h>
#include <tge/animation/AnimationClip.h>
#include <tge/animation/AnimationPlayer.h>
#include <tge/animation/PoseGenerator.h>
#include <tge/model/ModelFactory.h>

#include <tge/scene/ScenePropertyTypes.h>
#include <tge/script/contexts/ScriptUpdateContext.h>

#include <algorithm>
#include <cmath>

using namespace Tga;

namespace
{
	bool TrySampleRootJointAtTime(const AnimationPlayer& aPlayer, const float aTime, ScaleRotationTranslationf& outRootTransform)
	{
		const std::shared_ptr<const Animation> animation = aPlayer.GetAnimation();
		if (!animation || animation->Frames.empty())
		{
			return false;
		}

		const float fps = (aPlayer.GetFramesPerSecond() > 0.f) ? aPlayer.GetFramesPerSecond() : animation->FramesPerSecond;
		if (fps <= 0.f)
		{
			return false;
		}

		const float frameProgress = aTime * fps;
		const unsigned int length = animation->Length;
		if (length == 0)
		{
			return false;
		}

		unsigned int frame = static_cast<unsigned int>(std::floor(frameProgress));
		const float alpha = frameProgress - static_cast<float>(frame);

		unsigned int nextFrame = frame + 1;
		if (nextFrame > animation->Length && aPlayer.GetIsLooping())
		{
			nextFrame = 0;
		}

		frame = std::clamp(frame, 0u, length - 1u);
		nextFrame = std::clamp(nextFrame, 0u, length - 1u);

		const LocalSpacePose& poseA = animation->Frames[frame];
		if (poseA.Count == 0)
		{
			return false;
		}

		const ScaleRotationTranslationf& rootA = poseA.JointTransforms[0];
		if (!aPlayer.GetIsInterpolating())
		{
			outRootTransform = rootA;
			return true;
		}

		const LocalSpacePose& poseB = animation->Frames[nextFrame];
		if (poseB.Count == 0)
		{
			outRootTransform = rootA;
			return true;
		}

		outRootTransform = ScaleRotationTranslationf::Lerp(rootA, poseB.JointTransforms[0], alpha);
		return true;
	}

	Quatf ComputeRotationDelta(const Quatf& aFrom, const Quatf& aTo)
	{
		const Quatf fromNormalized = aFrom.GetNormalized();
		const Quatf toNormalized = aTo.GetNormalized();

		Quatf delta = toNormalized * fromNormalized.GetConjugate();
		delta.Normalize();
		return delta;
	}

	bool TryComputeRootMotionDeltaForSegment(
		const AnimationPlayer& aPlayer,
		const float aStartTime,
		const float anEndTime,
		Vector3f& outTranslationDelta,
		Quatf& outRotationDelta)
	{
		ScaleRotationTranslationf rootStart;
		ScaleRotationTranslationf rootEnd;

		if (!TrySampleRootJointAtTime(aPlayer, aStartTime, rootStart)
			|| !TrySampleRootJointAtTime(aPlayer, anEndTime, rootEnd))
		{
			return false;
		}

		outTranslationDelta = rootEnd.GetTranslation() - rootStart.GetTranslation();
		outRotationDelta = ComputeRotationDelta(rootStart.GetRotation(), rootEnd.GetRotation());
		return true;
	}

	void AccumulateRootMotionDelta(
		const Vector3f& aTranslationDelta,
		const Quatf& aRotationDelta,
		Vector3f& inOutTranslationDelta,
		Quatf& inOutRotationDelta)
	{
		inOutTranslationDelta += aTranslationDelta;
		inOutRotationDelta *= aRotationDelta;
		inOutRotationDelta.Normalize();
	}

	bool IsEventTimeInClipRange(const AnimationEventMarker& anEvent, const AnimationClip& aClip)
	{
		return anEvent.time >= aClip.startTime && anEvent.time <= aClip.endTime;
	}

	bool CrossedEventForward(
		const float anEventTime,
		const float aPreviousTime,
		const float aCurrentTime,
		const bool didWrap,
		const AnimationClip& aClip)
	{
		if (!didWrap)
		{
			return anEventTime > aPreviousTime && anEventTime <= aCurrentTime;
		}

		return (anEventTime > aPreviousTime && anEventTime <= aClip.endTime)
			|| (anEventTime >= aClip.startTime && anEventTime <= aCurrentTime);
	}

	bool CrossedEventBackward(
		const float anEventTime,
		const float aPreviousTime,
		const float aCurrentTime,
		const bool didWrap,
		const AnimationClip& aClip)
	{
		if (!didWrap)
		{
			return anEventTime < aPreviousTime && anEventTime >= aCurrentTime;
		}

		return (anEventTime < aPreviousTime && anEventTime >= aClip.startTime)
			|| (anEventTime <= aClip.endTime && anEventTime >= aCurrentTime);
	}

	void EmitCrossedEvents(
		const AnimationClip& aClip,
		const float aPreviousTime,
		const float aCurrentTime,
		const bool didWrap,
		const bool isForward,
		PoseGenerationContext& context)
	{
		if (!context.emittedEvents)
		{
			return;
		}

		if (aPreviousTime == aCurrentTime || aClip.events.empty())
		{
			return;
		}

		for (const AnimationEventMarker& marker : aClip.events)
		{
			if (marker.id.IsEmpty() || !IsEventTimeInClipRange(marker, aClip))
			{
				continue;
			}

			const bool crossed = isForward
				? CrossedEventForward(marker.time, aPreviousTime, aCurrentTime, didWrap, aClip)
				: CrossedEventBackward(marker.time, aPreviousTime, aCurrentTime, didWrap, aClip);

			if (!crossed)
			{
				continue;
			}

			EmittedAnimationEvent event;
			event.id = marker.id;
			event.clipPath = aClip.animationSourcePath;
			event.time = marker.time;
			context.emittedEvents->push_back(event);
		}
	}
}

bool PlayClipGenerator::EnsureLoadedAndUpdated(PoseGenerationContext& context)
{
	if (!clip || !context.model)
		return false;

	if (lastUpdatedFrame == context.frameNumber)
	{
		return animationPlayer.GetAnimation() != nullptr;
	}

	rootMotionTranslationDelta = Vector3f{};
	rootMotionRotationDelta = Quatf{};

	if (lastUpdatedFrame == (uint32_t)-1)
	{
		animationPlayer = ModelFactory::GetInstance().GetAnimationPlayer(clip->animationSourcePath.GetString(), context.model);
	}

	if (!animationPlayer.GetAnimation())
		return false;

	const bool hasPreviousFrame = lastUpdatedFrame != (uint32_t)-1;
	const bool hasContinuousPreviousFrame = hasPreviousFrame
		&& context.frameNumber > lastUpdatedFrame
		&& (context.frameNumber - lastUpdatedFrame) == 1;
	const float previousTime = animationPlayer.GetTime();

	bool didWrap = false;
	bool playingForward = clip->playbackRate >= 0.f;
	int wrapCount = 0;

	if (clip->isSyncronized)
	{
		float factor = context.syncedPlaybackTime + clip->cycleOffsetPercentage;
		factor = factor - floor(factor);

		// closest new value for lastSyncLocation, larger than previous value, but with correct factor
		float count = ceil(lastSyncLocation - factor);
		lastSyncLocation = count + factor;

		if (clip->isLooping && clip->cycleCount > 0.f)
		{
			while (lastSyncLocation > clip->cycleCount)
			{
				lastSyncLocation -= clip->cycleCount;
				didWrap = true;
				++wrapCount;
			}
		}
		else
		{
			lastSyncLocation = std::min(lastSyncLocation, clip->cycleCount);
		}

		const float clipDuration = clip->endTime - clip->startTime;
		const float safeCycleCount = std::max(clip->cycleCount, 1e-4f);
		if (clipDuration > 0.f)
		{
			animationPlayer.SetTime(clip->startTime + clipDuration * lastSyncLocation / safeCycleCount);

			if (clip->isLooping && hasContinuousPreviousFrame && !didWrap)
			{
				didWrap = playingForward
					? (animationPlayer.GetTime() < previousTime)
					: (animationPlayer.GetTime() > previousTime);

				if (didWrap)
				{
					wrapCount = 1;
				}
			}
		}
		else
		{
			animationPlayer.SetTime(clip->startTime);
		}
	}
	else
	{
		const bool shouldRestartFromStart = !hasPreviousFrame || (!clip->isLooping && !hasContinuousPreviousFrame);

		if (shouldRestartFromStart)
		{
			animationPlayer.SetTime(clip->startTime);
		}
		else
		{
			float newTime = previousTime;

			float delta = clip->playbackRate * context.deltaTime;
			newTime += delta;

			playingForward = delta >= 0.f;

			if (clip->isLooping && clip->endTime > clip->startTime)
			{
				const float clipDuration = clip->endTime - clip->startTime;

				if (playingForward)
				{
					while (newTime > clip->endTime)
					{
						newTime -= clipDuration;
						didWrap = true;
						++wrapCount;
					}
				}
				else
				{
					while (newTime < clip->startTime)
					{
						newTime += clipDuration;
						didWrap = true;
						++wrapCount;
					}
				}
			}
			else
			{
				newTime = std::min(newTime, clip->endTime);
				newTime = std::max(newTime, clip->startTime);
			}

			animationPlayer.SetTime(newTime);
		}
	}

	if (hasContinuousPreviousFrame)
	{
		EmitCrossedEvents(*clip, previousTime, animationPlayer.GetTime(), didWrap, playingForward, context);
	}

	lastUpdatedFrame = context.frameNumber;

	animationPlayer.UpdatePose();

	if (hasContinuousPreviousFrame)
	{
		auto accumulateSegment = [&](const float aStart, const float anEnd)
			{
				Vector3f segmentTranslationDelta = {};
				Quatf segmentRotationDelta = {};
				if (TryComputeRootMotionDeltaForSegment(animationPlayer, aStart, anEnd, segmentTranslationDelta, segmentRotationDelta))
				{
					AccumulateRootMotionDelta(
						segmentTranslationDelta,
						segmentRotationDelta,
						rootMotionTranslationDelta,
						rootMotionRotationDelta);
				}
			};

		const float currentTime = animationPlayer.GetTime();
		if (clip->isLooping && didWrap && wrapCount > 0 && clip->endTime > clip->startTime)
		{
			if (playingForward)
			{
				accumulateSegment(previousTime, clip->endTime);

				Vector3f fullLoopTranslationDelta = {};
				Quatf fullLoopRotationDelta = {};
				if (TryComputeRootMotionDeltaForSegment(
					animationPlayer,
					clip->startTime,
					clip->endTime,
					fullLoopTranslationDelta,
					fullLoopRotationDelta))
				{
					for (int i = 1; i < wrapCount; ++i)
					{
						AccumulateRootMotionDelta(
							fullLoopTranslationDelta,
							fullLoopRotationDelta,
							rootMotionTranslationDelta,
							rootMotionRotationDelta);
					}
				}

				accumulateSegment(clip->startTime, currentTime);
			}
			else
			{
				accumulateSegment(previousTime, clip->startTime);

				Vector3f fullLoopTranslationDelta = {};
				Quatf fullLoopRotationDelta = {};
				if (TryComputeRootMotionDeltaForSegment(
					animationPlayer,
					clip->endTime,
					clip->startTime,
					fullLoopTranslationDelta,
					fullLoopRotationDelta))
				{
					for (int i = 1; i < wrapCount; ++i)
					{
						AccumulateRootMotionDelta(
							fullLoopTranslationDelta,
							fullLoopRotationDelta,
							rootMotionTranslationDelta,
							rootMotionRotationDelta);
					}
				}

				accumulateSegment(clip->endTime, currentTime);
			}
		}
		else
		{
			accumulateSegment(previousTime, currentTime);
		}
	}

	return true;
}
void PlayClipGenerator::GeneratePose(PoseGenerationContext& context, LocalSpacePose& outputPose)
{
	if (!EnsureLoadedAndUpdated(context))
		return;

	outputPose = animationPlayer.GetLocalSpacePose();
}

void PlayClipGenerator::GenerateRootMotionDelta(PoseGenerationContext& context, Vector3f& outRootMotionPositionDelta, Quatf& outRootMotionRotationDelta)
{
	if (!EnsureLoadedAndUpdated(context))
		return;

	outRootMotionPositionDelta = rootMotionTranslationDelta;
	outRootMotionRotationDelta = rootMotionRotationDelta;
}

void PlayClipNode::Init(const ScriptCreationContext& context)
{
	{
		ScriptPin namePin = {};
		namePin.type = ScriptLinkType::Property;
		namePin.dataType = GetPropertyType<CopyOnWriteWrapper<AnimationClipReference>>();
		namePin.defaultValue = Property::Create<CopyOnWriteWrapper<AnimationClipReference>>();
		namePin.name = "Clip"_tgaid;
		namePin.node = context.GetNodeId();
		namePin.role = ScriptPinRole::Input;
		myAnimationClipInPin = context.FindOrCreatePin(namePin);
	}

	{
		ScriptPin outputPin = {};
		outputPin.type = ScriptLinkType::Property;
		outputPin.dataType = GetPropertyType<PoseAndMotion>();
		outputPin.name = "Pose"_tgaid;
		outputPin.node = context.GetNodeId();
		outputPin.role = ScriptPinRole::Output;

		myPoseOutPin = context.FindOrCreatePin(outputPin);
	}
}

Property PlayClipNode::ReadPin(ScriptExecutionContext& context, ScriptPinId) const
{
	PlayClipGenerator& generator = GetRuntimeData(context).generator;

	if (generator.clip == nullptr)
	{
		Property clipReferenceProperty = context.ReadInputPin(myAnimationClipInPin);
		if (auto* clipReference = clipReferenceProperty.Get<CopyOnWriteWrapper<AnimationClipReference>>())
		{
			StringId path = clipReference->Get().path;
			generator.clip = GetAnimationClip(path);
		}
	}

	PoseAndMotion result = {};
	
	if (generator.clip && generator.clip->isSyncronized)
	{
		const float duration = generator.clip->endTime - generator.clip->startTime;
		if (duration > 1e-4f)
		{
			result.desiredSyncedPlaybackRateWeight = 1.f;
			result.desiredSyncedPlaybackRate =
				generator.clip->playbackRate * generator.clip->cycleCount / duration;
		}
	}

	result.poseGenerator = &generator;

	return Property::Create<PoseAndMotion>(result);
}