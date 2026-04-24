#include <stdafx.h>
#include "BlendAnimationNode.h"

#include <tge/animation/Animation.h>
#include <tge/animation/AnimationClip.h>
#include <tge/animation/PoseGenerator.h>

#include <tge/scene/ScenePropertyTypes.h>
#include <tge/script/BaseProperties.h>

using namespace Tga;

namespace
{
	constexpr float BlendEdgeEpsilon = 0.0001f;
}

void BlendPoseGenerator::GeneratePose(PoseGenerationContext& context, LocalSpacePose& outputPose)
{
	const float t = std::clamp(blendFactor, 0.0f, 1.0f);

	if (t <= BlendEdgeEpsilon)
	{
		if (generatorA)
		{
			generatorA->GeneratePose(context, outputPose);
			return;
		}

		if (generatorB)
		{
			generatorB->GeneratePose(context, outputPose);
			return;
		}

		PoseGenerator::GeneratePose(context, outputPose);
		return;
	}

	if (t >= 1.0f - BlendEdgeEpsilon)
	{
		if (generatorB)
		{
			generatorB->GeneratePose(context, outputPose);
			return;
		}

		if (generatorA)
		{
			generatorA->GeneratePose(context, outputPose);
			return;
		}

		PoseGenerator::GeneratePose(context, outputPose);
		return;
	}

	LocalSpacePose poseA = {};
	LocalSpacePose poseB = {};

	if (generatorA)
	{
		generatorA->GeneratePose(context, poseA);
	}

	if (generatorB)
	{
		generatorB->GeneratePose(context, poseB);
	}

	if (poseA.Count == 0 && poseB.Count == 0)
	{
		PoseGenerator::GeneratePose(context, outputPose);
		return;
	}

	if (poseA.Count == 0)
	{
		outputPose = poseB;
		return;
	}

	if (poseB.Count == 0)
	{
		outputPose = poseA;
		return;
	}

	if (poseA.Count != poseB.Count)
	{
		// If clips/skeletons mismatch, fall back to A instead of hard erroring/spamming logs.
		outputPose = poseA;
		return;
	}

	outputPose.Count = poseA.Count;

	for (size_t i = 0; i < poseA.Count; i++)
	{
		const ScaleRotationTranslationf& srtA = poseA.JointTransforms[i];
		const ScaleRotationTranslationf& srtB = poseB.JointTransforms[i];
		ScaleRotationTranslationf& srtOut = outputPose.JointTransforms[i];

		srtOut.SetScale(Vector3f::Lerp(srtA.GetScale(), srtB.GetScale(), t));
		srtOut.SetRotation(Quatf::Slerp(srtA.GetRotation(), srtB.GetRotation(), t));
		srtOut.SetTranslation(Vector3f::Lerp(srtA.GetTranslation(), srtB.GetTranslation(), t));
	}
}

void BlendPoseGenerator::GenerateRootMotionDelta(PoseGenerationContext& context, Vector3f& outRootMotionPositionDelta, Quatf& outRootMotionRotationDelta)
{
	const float t = std::clamp(blendFactor, 0.0f, 1.0f);

	if (t <= BlendEdgeEpsilon)
	{
		if (generatorA)
		{
			generatorA->GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
			return;
		}

		if (generatorB)
		{
			generatorB->GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
			return;
		}

		PoseGenerator::GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
		return;
	}

	if (t >= 1.0f - BlendEdgeEpsilon)
	{
		if (generatorB)
		{
			generatorB->GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
			return;
		}

		if (generatorA)
		{
			generatorA->GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
			return;
		}

		PoseGenerator::GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
		return;
	}

	if (!generatorA && !generatorB)
	{
		PoseGenerator::GenerateRootMotionDelta(context, outRootMotionPositionDelta, outRootMotionRotationDelta);
		return;
	}

	Vector3f translationA = {};
	Vector3f translationB = {};
	Quatf rotationA = {};
	Quatf rotationB = {};

	if (generatorA)
	{
		generatorA->GenerateRootMotionDelta(context, translationA, rotationA);
	}

	if (generatorB)
	{
		generatorB->GenerateRootMotionDelta(context, translationB, rotationB);
	}

	if (!generatorA)
	{
		outRootMotionPositionDelta = translationB;
		outRootMotionRotationDelta = rotationB;
		return;
	}

	if (!generatorB)
	{
		outRootMotionPositionDelta = translationA;
		outRootMotionRotationDelta = rotationA;
		return;
	}

	outRootMotionRotationDelta = Quatf::Slerp(rotationA, rotationB, t);
	outRootMotionPositionDelta = Vector3f::Lerp(translationA, translationB, t);
}


void BlendPoseNode::Init(const ScriptCreationContext& context)
{
	{
		ScriptPin valuePin = {};
		valuePin.type = ScriptLinkType::Property;
		valuePin.dataType = GetPropertyType<PoseAndMotion>();
		valuePin.defaultValue = Property::Create<PoseAndMotion>();
		valuePin.name = "Pose A"_tgaid;
		valuePin.node = context.GetNodeId();
		valuePin.role = ScriptPinRole::Input;

		myPoseAInPin = context.FindOrCreatePin(valuePin);
	}

	{
		ScriptPin valuePin = {};
		valuePin.type = ScriptLinkType::Property;
		valuePin.dataType = GetPropertyType<PoseAndMotion>();
		valuePin.defaultValue = Property::Create<PoseAndMotion>();
		valuePin.name = "Pose B"_tgaid;
		valuePin.node = context.GetNodeId();
		valuePin.role = ScriptPinRole::Input;

		myPoseBInPin = context.FindOrCreatePin(valuePin);
	}

	{
		ScriptPin valuePin = {};
		valuePin.type = ScriptLinkType::Property;
		valuePin.dataType = GetPropertyType<float>();
		valuePin.defaultValue = Property::Create<float>();
		valuePin.name = "Blend Amount"_tgaid;
		valuePin.node = context.GetNodeId();
		valuePin.role = ScriptPinRole::Input;

		myBlendAmountPin = context.FindOrCreatePin(valuePin);
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

Property BlendPoseNode::ReadPin(ScriptExecutionContext& context, ScriptPinId) const
{
	BlendPoseGenerator& generator = GetRuntimeData(context).generator;

	PoseAndMotion inputA = *context.ReadInputPin(myPoseAInPin).Get<PoseAndMotion>();
	PoseAndMotion inputB = *context.ReadInputPin(myPoseBInPin).Get<PoseAndMotion>();

	float blendFactor = *context.ReadInputPin(myBlendAmountPin).Get<float>();


	generator.generatorA = inputA.poseGenerator;
	generator.generatorB = inputB.poseGenerator;
	generator.blendFactor = blendFactor;
	

	PoseAndMotion result = {};

	result.poseGenerator = &generator;
	
	float wrate =
		(1.f - blendFactor) * inputA.desiredSyncedPlaybackRate * inputA.desiredSyncedPlaybackRateWeight
		+ (blendFactor)*inputB.desiredSyncedPlaybackRate * inputB.desiredSyncedPlaybackRateWeight;

	float w =
		(1.f - blendFactor) * inputA.desiredSyncedPlaybackRateWeight
		+ (blendFactor) * inputB.desiredSyncedPlaybackRateWeight;

	if (w > 0.f)
	{
		result.desiredSyncedPlaybackRate = wrate / w;
		result.desiredSyncedPlaybackRateWeight = w;
	}

	return Property::Create<PoseAndMotion>(result);
}