#pragma once
#include "ScriptComponent.h"

#include <tge/animation/AnimationPlayer.h>
//#include "State.h"

class AnimatedMeshComponent;

class Animator : public ScriptComponent
{
	public:

		Animator() = default;
		~Animator() = default;

		/// Queue an animation to be loaded during OnStart.
		/// Call this after construction but before the first frame.
		Tga::AnimationPlayer& GetAnimation(const int anAnimationIndex);
		Tga::AnimationPlayer& GetCurrentAnimation();
		void AddAnimation(const std::string& aFilePath, bool aIsLooping = true);

		const int GetCurrentAnimationIndex() const;
		/// Switch to a different animation. Stops the old one and plays the new one.
		void SetCurrentAnimationIndex(const int aNewAnimationIndex);

		/*void StartCurrentAnimation(const int anAnimationIndex);
		void StopCurrentAnimation();*/


		/*virtual void SetState(State* aState);
		State* GetCurrentState() const;*/

	protected:
		/// Queued animation file paths + looping flag, consumed in OnStart.
		struct AnimationEntry
		{
			std::string filePath;
			bool isLooping = true;
		};
		std::vector<AnimationEntry> myAnimationEntries;

		std::vector<Tga::AnimationPlayer> myAnimationPlayers;
		AnimatedMeshComponent* myAnimatedMesh = nullptr;

		int myCurrentAnimationIndex = 0;

		//State* myCurrentState;
};