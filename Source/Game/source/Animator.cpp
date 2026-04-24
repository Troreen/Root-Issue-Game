#include "Animator.h"
#include "AnimatedMeshComponent.h"

Tga::AnimationPlayer& Animator::GetAnimation(const int anAnimationIndex)
{
	return myAnimationPlayers[anAnimationIndex];
}

Tga::AnimationPlayer& Animator::GetCurrentAnimation()
{
	return myAnimationPlayers[myCurrentAnimationIndex];
}

void Animator::AddAnimation(const std::string& aFilePath, bool aIsLooping)
{
	myAnimationEntries.push_back({ aFilePath, aIsLooping });
	// std::cout << aFilePath << std::endl;
}

const int Animator::GetCurrentAnimationIndex() const
{
	return myCurrentAnimationIndex;
}

void Animator::SetCurrentAnimationIndex(const int aNewAnimationIndex)
{
	if (aNewAnimationIndex == myCurrentAnimationIndex)
	{
		return;
	}

	if (aNewAnimationIndex < 0 || aNewAnimationIndex >= static_cast<int>(myAnimationPlayers.size()))
	{
		return;
	}

	// Stop the old animation, start the new one.
	myAnimationPlayers[myCurrentAnimationIndex].Stop();
	myCurrentAnimationIndex = aNewAnimationIndex;
	myAnimationPlayers[myCurrentAnimationIndex].Play();
	//std::cout << "Looping: " << myAnimationPlayers[myCurrentAnimationIndex].GetIsLooping() << std::endl;
}

//void Animator::StartCurrentAnimation(const int anAnimationIndex)
//{
//	myCurrentAnimationIndex = anAnimationIndex;
//	myAnimationPlayers[myCurrentAnimationIndex].Play();
//}
//
//void Animator::StopCurrentAnimation()
//{
//	myAnimationPlayers[myCurrentAnimationIndex].Stop();
//}

//void Animator::SetState(State* aState)
//{
//	if (aState == myCurrentState)
//	{
//		return;
//	}
//
//	if (myCurrentState)
//	{
//		myCurrentState->Exit(*this);
//		delete myCurrentState;
//	}
//
//	myCurrentState = aState;
//
//	if (myCurrentState)
//	{
//		myCurrentState->Enter(*this);
//	}
//}
//
//State* Animator::GetCurrentState() const
//{
//	return myCurrentState;
//}
