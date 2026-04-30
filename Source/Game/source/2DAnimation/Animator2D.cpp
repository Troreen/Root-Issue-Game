#include "Animator2D.h"
#include "Essentials.h"

Animator2D::Animator2D(Tga::Sprite3DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<Animation2D> someAnimation2Ds)
{
	if (aSpriteInstance == nullptr || aSharedSprite == nullptr || aSharedSprite->myTexture == nullptr)
	{
		return;
	}

	mySpriteInstance = aSpriteInstance;
	mySpriteInstance2D = nullptr;

	BaseConstructorValues(aSharedSprite, aFps, aSpriteSize, someAnimation2Ds);
}

Animator2D::Animator2D(Tga::Sprite3DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, Tga::Vector2i aFrameSize, std::vector<::Animation2D> someAnimation2Ds)
{
	if (aSpriteInstance == nullptr || aSharedSprite == nullptr || aSharedSprite->myTexture == nullptr)
	{
		return;
	}

	mySpriteInstance = aSpriteInstance;
	mySpriteInstance2D = nullptr;
	BaseConstructorValues(aSharedSprite, aFps, aFrameSize, someAnimation2Ds);
}

Animator2D::Animator2D(Tga::Sprite2DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<::Animation2D> someAnimation2Ds)
{
	if (aSharedSprite == nullptr || aSpriteInstance == nullptr || aSharedSprite->myTexture == nullptr)
	{
		return;
	}

	mySpriteInstance2D = aSpriteInstance;
	mySpriteInstance = nullptr;

	BaseConstructorValues(aSharedSprite, aFps, aSpriteSize, someAnimation2Ds);
}

void Animator2D::BaseConstructorValues(Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<::Animation2D> someAnimation2Ds)
{
	mySharedSprite = aSharedSprite;

	myAnimation2Ds = someAnimation2Ds;

	Tga::Vector2ui size = mySharedSprite->myTexture->CalculateTextureSize();

	myCurrentAnimation2DIndex = 0;

	myFps = aFps;

	const Tga::Vector2i frames = { static_cast<int>(size.x) / aSpriteSize, static_cast<int>(size.y) / aSpriteSize };

	for (int y = 0; y < frames.y; ++y)
	{
		for (int x = 0; x < frames.x; ++x)
		{
			frameUVs.push_back(
				UV
				(
					Tga::Vector2f
					(
						((aSpriteSize * x) / static_cast<float>(size.x)),
						((aSpriteSize * y) / static_cast<float>(size.y))
					),
					Tga::Vector2f
					(
						(((aSpriteSize * x) + aSpriteSize) / static_cast<float>(size.x)),
						(((aSpriteSize * y) + aSpriteSize) / static_cast<float>(size.y))
					)
				)
			);
		}
	}

	myIsPlaying = false;
	myIsPaused = false;
	myCurrentFrameIndex = 0;
	myLastFrameTime = 0.0f;
	myCurrentAnimation2DIndex = 0;
}

void Animator2D::BaseConstructorValues(Tga::SpriteSharedData* aSharedSprite, const int aFps, Tga::Vector2i aFrameSize, std::vector<::Animation2D> someAnimation2Ds)
{
	mySharedSprite = aSharedSprite;

	myAnimation2Ds = someAnimation2Ds;

	Tga::Vector2ui size = mySharedSprite->myTexture->CalculateTextureSize();

	assert(size.x % aFrameSize.x == 0);
	assert(size.y % aFrameSize.y == 0);

	myCurrentAnimation2DIndex = 0;

	myFps = aFps;

	const Tga::Vector2i frames = { static_cast<int>(size.x) / aFrameSize.x, static_cast<int>(size.y) / aFrameSize.y };

	for (int y = 0; y < frames.y; ++y)
	{
		for (int x = 0; x < frames.x; ++x)
		{
			frameUVs.push_back(
				UV
				(
					Tga::Vector2f
					(
						((aFrameSize.x * x) / static_cast<float>(size.x)),
						((aFrameSize.y * y) / static_cast<float>(size.y))
					),
					Tga::Vector2f
					(
						(((aFrameSize.x * x) + aFrameSize.x) / static_cast<float>(size.x)),
						(((aFrameSize.y * y) + aFrameSize.y) / static_cast<float>(size.y))
					)
				)
			);
		}
	}

	myIsPlaying = false;
	myIsPaused = false;
	myCurrentFrameIndex = 0;
	myLastFrameTime = 0.0f;
	myCurrentAnimation2DIndex = 0;
}

void Animator2D::Update()
{
	if (myCurrentAnimation2DIndex < 0 || myCurrentAnimation2DIndex >= static_cast<int>(myAnimation2Ds.size()) || !myIsPlaying || myIsPaused)
		return;

	Animation2D anim = myAnimation2Ds[myCurrentAnimation2DIndex];

	int frameCount = 0;
	frameCount = static_cast<int>(anim.FrameIndexes.size());

	if (frameCount == 0)
		return;

	float currentTime = static_cast<float>(Essentials::GetEssentials().GetTotalTime());
	if (currentTime < myLastFrameTime + (1.0f / static_cast<float>(myFps)))
		return;

	myLastFrameTime = currentTime;
	myCurrentFrameIndex++;

	if (myCurrentFrameIndex >= frameCount)
	{
		if (anim.Loop)
			myCurrentFrameIndex = 0;
		else
		{
			myIsPlaying = false;
			if(anim.Callback != nullptr)				
				anim.Callback();
			return;
		}
	}
}

void Animator2D::Render()
{

	if (myCurrentAnimation2DIndex < 0 || myCurrentAnimation2DIndex >= static_cast<int>(myAnimation2Ds.size()) || !myIsPlaying)
	{
		if (mySpriteInstance2D != nullptr)
			mySpriteInstance2D->myTextureRect = { 0,0,0,0 };
		else
			mySpriteInstance->myTextureRect = { 0,0,0,0 };
		return;
	}

	Animation2D anim = myAnimation2Ds[myCurrentAnimation2DIndex];

	const UV& uv = frameUVs[anim.FrameIndexes[myCurrentFrameIndex]];

	if (mySpriteInstance != nullptr)
		mySpriteInstance->myTextureRect = { uv.myStart.x, uv.myStart.y, uv.myEnd.x, uv.myEnd.y };
	else
		mySpriteInstance2D->myTextureRect = { uv.myStart.x, uv.myStart.y, uv.myEnd.x, uv.myEnd.y };

}

void Animator2D::Render(Tga::Sprite3DInstanceData* aSprite)
{


	//if (myCurrentAnimation2DIndex < 0 || myCurrentAnimation2DIndex >= static_cast<int>(myAnimation2Ds.size()) || !myIsPlaying)
	//{
	//    if (mySpriteInstance2D != nullptr)
	//        mySpriteInstance2D->myTextureRect = { 0,0,0,0 };
	//    else
	//        mySpriteInstance->myTextureRect = { 0,0,0,0 };
	//    return;
	//}

	//Animation2D anim = myAnimation2Ds[myCurrentAnimation2DIndex];

	//const UV& uv = frameUVs[anim.FrameIndexes[myCurrentFrameIndex]];
	if (!aSprite || !myIsPlaying)
	{
		return;
	}

	Animation2D anim = myAnimation2Ds[myCurrentAnimation2DIndex];

	const UV& uv = frameUVs[anim.FrameIndexes[myCurrentFrameIndex]];

	aSprite->myTextureRect = { uv.myStart.x, uv.myStart.y, uv.myEnd.x, uv.myEnd.y };

}

void Animator2D::SetSpriteAnimation2D(Tga::Sprite3DInstanceData* aSprite, Tga::SpriteSharedData* aSharedData)
{
	mySpriteInstance = aSprite;
	mySharedSprite = aSharedData;
}

void Animator2D::AddAnimation2D(const Animation2D& anAnimation2D)
{
	myAnimation2Ds.push_back(anAnimation2D);
}

void Animator2D::PlayAnimation2D(std::string Animation2DName)
{
	auto it = std::find_if(
		myAnimation2Ds.begin(),
		myAnimation2Ds.end(),
		[&](const Animation2D& anim)
		{
			return anim.Animation2DName == Animation2DName;
		}
	);

	if (it == myAnimation2Ds.end())
		return;

	int Animation2DIndex = static_cast<int>(std::distance(myAnimation2Ds.begin(), it));

	if (myIsPlaying && Animation2DIndex == myCurrentAnimation2DIndex)
		return;

	myIsPlaying = true;
	myIsPaused = false;
	myCurrentFrameIndex = 0;
	myCurrentAnimation2DIndex = Animation2DIndex;
	myLastFrameTime = static_cast<float>(Essentials::GetEssentials().GetTotalTime());

	Animation2D anim = myAnimation2Ds[myCurrentAnimation2DIndex];
}

void Animator2D::PauseAnimation2D()
{
	myIsPaused = !myIsPaused;
	if (myIsPaused)
	{
		myLastPausedTime = static_cast<float>(Essentials::GetEssentials().GetTotalTime());
	}
	else
	{
		myLastFrameTime += (static_cast<float>(Essentials::GetEssentials().GetTotalTime()) - myLastPausedTime);
	}
}

void Animator2D::StopAnimation2D()
{
	bool wasPlaying = myIsPlaying;

	myIsPlaying = false;
	myIsPaused = false;
	myCurrentFrameIndex = 0;
	myLastFrameTime = 0.0f;
	myCurrentAnimation2DIndex = 0;

	if (wasPlaying)
	{
		if (myAnimation2Ds[myCurrentAnimation2DIndex].Callback != nullptr)
		{
			myAnimation2Ds[myCurrentAnimation2DIndex].Callback();
		}
	}
}

int Animator2D::GetCurrentFrame() const
{
	return myCurrentFrameIndex;
}

bool Animator2D::IsPlaying() const
{
	return myIsPlaying;
}

