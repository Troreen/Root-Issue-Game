#pragma once
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/sprite/sprite.h>
#include <tge/text/text.h>
#include <tge/graphics/Camera.h>

#include <CommonUtilities/Vector3.hpp>
#include <CommonUtilities/Camera3D.hpp>

#include "GameObject.h"
#include "ScriptComponent.h"
#include "BoxColliderComponent.h"

#include <vector>

using Vector3f = CommonUtilities::Vector3<float>;
using Vector2f = CommonUtilities::Vector2<float>;

class SpeechBubbleComponent final : public ScriptComponent
{
public:
	SpeechBubbleComponent(bool aLifetime = false, bool aFollowState = false);
	~SpeechBubbleComponent();

	void OnStart() override;
	void OnUpdate(float aDeltaTime) override;
	void OnDisable() override;
	void OnScriptDestroy() override;
	void Render() override;
	static void FlushQueuedRender();

	void SetActive(const bool aState);
	bool GetActive() const;

	void SetSpriteBubble(const std::string& aFileName);
	void SetSizeBubble(Vector2f aSize);
	void SetFollow(const bool aState);
	void SetOffset(Vector3f aOffset);
	void SetTarget(GameObject* aTarget);
	void SetTextPosition(Vector2f aPosition);
	void SetText(const std::string& aText);
	void SetTime(float aTime);
	void ResetTime();

	Vector2f GetTextSize();

	void SetTriggerInteract(const bool aState);
	bool GetTriggerInteract() const;

private:
	bool myActive;
	bool myFollowTarget;
	bool myTriggerInteract;

	bool myLifetime;
	float myCurrentTime;
	float myTime;

	GameObject* myTarget;
	
	Tga::Text myText;
	Vector3f myOffset;
	Vector2f myTextOffset;

	Tga::Sprite2DInstanceData myBubbleInstance;
	Tga::SpriteSharedData myBubbleSharedData;

	void RemoveFromRenderQueue();
	void RenderImmediate();
	static std::vector<SpeechBubbleComponent*> ourRenderQueue;
};

