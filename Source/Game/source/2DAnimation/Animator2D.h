#pragma once
#include <tge/texture/TextureManager.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/sprite/sprite.h>
#include <tge/drawers/SpriteDrawer.h>
#include <vector>
#include <string>
#include <functional>

struct UV
{
	public:
		UV() = default;
		UV(Tga::Vector2f aStart, Tga::Vector2f aEnd) { myStart = aStart; myEnd = aEnd; }
		Tga::Vector2f myStart;
		Tga::Vector2f myEnd;
};

struct Animation2D
{
	public:
		Animation2D(const std::string& someAnimation2DName, const std::vector<int>& someFrameIndexes, const bool& aLoop, const std::function<void()>& aCallback = std::function<void()>())
		{
			Animation2DName = someAnimation2DName;
			FrameIndexes = someFrameIndexes;
			for (int i = 0; i < someFrameIndexes.size(); i++)
			{
				FrameIndexes[i] = someFrameIndexes[i] - 1;
			}
			Loop = aLoop;
			Callback = aCallback;
		}

		std::string Animation2DName;
		std::vector<int> FrameIndexes;	
		bool Loop;
		std::function<void()> Callback;
};

class Animator2D
{
	public:
		Animator2D() {}
		Animator2D(Tga::Sprite3DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<::Animation2D> someAnimation2Ds);
		Animator2D(Tga::Sprite3DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, Tga::Vector2i aFrameSize, std::vector<::Animation2D> someAnimation2Ds);
		Animator2D(Tga::Sprite2DInstanceData* aSpriteInstance, Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<::Animation2D> someAnimation2Ds);
		void BaseConstructorValues(Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize, std::vector<::Animation2D> someAnimation2Ds);
		void BaseConstructorValues(Tga::SpriteSharedData* aSharedSprite, const int aFps, Tga::Vector2i aFrameSize, std::vector<::Animation2D> someAnimation2Ds);

		void Update();
		void Render();

		void Render(Tga::Sprite3DInstanceData* aSprite);

		void SetSpriteAnimation2D(Tga::Sprite3DInstanceData* aSprite, Tga::SpriteSharedData* aSharedData);
		void AddAnimation2D(const Animation2D& anAnimation2D);

		void PlayAnimation2D(std::string Animation2DName);
		void PauseAnimation2D();
		void StopAnimation2D();

		int GetCurrentFrame() const;
		bool IsPlaying() const;


		//JW TEST RESIZE
		//void BaseResize(Tga::SpriteSharedData* aSharedSprite, const int aFps, int aSpriteSize);


		//JW TEST RESIZE

	private:
		Tga::Sprite3DInstanceData* mySpriteInstance;
		Tga::Sprite2DInstanceData* mySpriteInstance2D;
		Tga::SpriteSharedData* mySharedSprite;
		std::vector<::Animation2D> myAnimation2Ds;
		std::vector<UV> frameUVs;
		int myCurrentAnimation2DIndex;
		int myCurrentFrameIndex;
		float myLastFrameTime;
		float myLastPausedTime;
		int myFps;
		bool myIsPlaying;
		bool myIsPaused;




};
