#pragma once

#include <tge/animation/Pose.h>
#include <tge/animation/Skeleton.h>
#include <tge/model/Model.h>
#include <tge/stringRegistry/StringRegistry.h>

#include <vector>

namespace Tga
{
	struct EmittedAnimationEvent
	{
		StringId id;
		StringId clipPath;
		float time = 0.f;
	};

	struct PoseGenerationContext
	{
		std::shared_ptr<Model> model;

		uint32_t frameNumber;
		float deltaTime;
		float syncedPlaybackTime; // always between 0 and 1
		std::vector<EmittedAnimationEvent>* emittedEvents = nullptr;
	};

	class PoseGenerator
	{
	public:
		virtual void GeneratePose(PoseGenerationContext& context, LocalSpacePose& outputPose) 
		{ 
			outputPose = context.model->GetSkeleton()->localBindPose;
			return;
		}

		virtual void GenerateRootMotionDelta(PoseGenerationContext& context, Vector3f& outRootMotionPositionDelta, Quatf& outRootMotionRotationDelta)
		{
			context;
			outRootMotionPositionDelta = Vector3f{};
			outRootMotionRotationDelta = Quatf{};
		}
	};

	// posegenerators will be things like:
	// play animation, blend animation etc
	// student can add things like IK generators and so on

	// this will be used as a property type in nodes/objects
	struct PoseAndMotion
	{
		PoseGenerator* poseGenerator = nullptr;

		// when animations are synced, desired playback rates are accumulated to figure out an average speed to step animations
		// currently only one synced rate is allowed, but could be expanded to multiple for more advanced use cases
		float desiredSyncedPlaybackRate = 0.f;
		float desiredSyncedPlaybackRateWeight = 0.f;
	};

	// animation graph nodes will produce PoseAndMotions
	
	// after a graph is evaluated you can then use the desiredSyncedPlaybackRate to set up a PoseGenerationContext
	// then generate root motion, and resolve root motion against the game world if needed
	// then run pose generation
	// it is done as three steps to be able to adjust play back rates first, 
	// then adjust world location, 
	// so pose generators can do spatial queries at correct location if needed
	

	// node ideas
	// AdjustSpeed
	// requests pose with modified delta time
	// and multiplies desiredSyncedPlaybackRate

	// override sync speed
	// animation input
	// and sync reference input



}