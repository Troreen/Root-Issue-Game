#include "SpeechBubbleComponent.h"

#include <tge/texture/TextureManager.h>
#include <tge/graphics/DX11.h>
#include <tge/engine.h>

#include <algorithm>

std::vector<SpeechBubbleComponent*> SpeechBubbleComponent::ourRenderQueue;

SpeechBubbleComponent::SpeechBubbleComponent(bool aLifetime, bool aFollowState) :
myActive(false),
myFollowTarget(aFollowState),
myTriggerInteract(false),
myLifetime(aLifetime),
myCurrentTime(0.0f),
myTime(0.0f),
myTarget(nullptr),
myText("Text/Evil Bible.ttf", Tga::FontSize_24),
myOffset(0, 250, 0),
myTextOffset(0.0f, 0.0f)
{
}

SpeechBubbleComponent::~SpeechBubbleComponent()
{
RemoveFromRenderQueue();
}

void SpeechBubbleComponent::OnStart()
{
myActive = false;

Tga::Vector3f pos;
if (myTarget != nullptr)
{
	pos = myTarget->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
}
else
{
	pos = GetOwner()->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
}

auto& engine = *Tga::Engine::GetInstance();
Tga::Vector2f resolution = { static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().x), static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().y) };

std::string sprite = "sprites/error.dds";
if (myBubbleSharedData.myTexture == nullptr)
{
	myBubbleSharedData.myTexture = engine.GetTextureManager().GetTexture(sprite.c_str());
}

if (myBubbleInstance.myPosition == Tga::Vector2f{ 0.5f, 0.5f })
{
	myBubbleInstance.myPivot = Tga::Vector2f{ 0.5f, 0.55f };
}

if (myBubbleInstance.myPosition == Tga::Vector2f{ 0.0f, 0.0f })
{
	myBubbleInstance.myPosition = Tga::Vector2f{ pos.x, pos.y };
}

if (myBubbleInstance.mySize == Tga::Vector2f{ 1.0f, 1.0f })
{
	myBubbleInstance.mySize = Tga::Vector2f{ 0.15f, 0.15f } *resolution.y;
}
myBubbleInstance.mySize.x += myText.GetWidth();
myBubbleInstance.mySize.y += myText.GetHeight();

myText.SetPosition({ -0.5f * myText.GetWidth() + pos.x + myTextOffset.x, 0.0f + pos.y + myTextOffset.y });
}

void SpeechBubbleComponent::OnUpdate(float aDeltaTime)
{
if (myActive == false)
{
	return;
}

if (myLifetime == true && myCurrentTime >= 0)
{
	myCurrentTime -= aDeltaTime;
}
else if (myCurrentTime < 0)
{
	myActive = false;
}

if (myFollowTarget == false)
{
	return;
}

Tga::Vector3f pos = { 0,0,0 };
if (myFollowTarget == true && myTarget != nullptr)
{
	pos = myTarget->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
}

myText.SetPosition({ -0.5f * myText.GetWidth() + pos.x + myTextOffset.x, 0.0f + pos.y + myTextOffset.y });
myBubbleInstance.myPosition = Tga::Vector2f{ pos.x, pos.y };
}

void SpeechBubbleComponent::OnDisable()
{
myActive = false;
RemoveFromRenderQueue();
}

void SpeechBubbleComponent::OnScriptDestroy()
{
RemoveFromRenderQueue();
}

void SpeechBubbleComponent::Render()
{
if (myActive == false)
{
	return;
}

ourRenderQueue.push_back(this);
}

void SpeechBubbleComponent::FlushQueuedRender()
{
if (ourRenderQueue.empty())
{
	return;
}

for (SpeechBubbleComponent* bubble : ourRenderQueue)
{
	if (!bubble)
	{
		continue;
	}

	bubble->RenderImmediate();
}

ourRenderQueue.clear();
}

void SpeechBubbleComponent::RenderImmediate()
{
if (myActive == false)
{
	return;
}

Tga::DX11::BackBuffer->SetAsActiveTarget();


auto& engine = *Tga::Engine::GetInstance();
Tga::Vector2f resolution = { static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().x), static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().y) };
Tga::SpriteDrawer& spriteDrawer(engine.GetGraphicsEngine().GetSpriteDrawer());

spriteDrawer.Draw(myBubbleSharedData, myBubbleInstance);

myText.Render();

Tga::DX11::BackBuffer->SetAsActiveTarget(Tga::DX11::DepthBuffer);
}

void SpeechBubbleComponent::SetActive(const bool aState)
{
myActive = aState;
}

bool SpeechBubbleComponent::GetActive() const
{
return myActive;
}

void SpeechBubbleComponent::SetSpriteBubble(const std::string& aFileName)
{
auto& engine = *Tga::Engine::GetInstance();
std::string sprite = "sprites/" + aFileName;
myBubbleSharedData.myTexture = engine.GetTextureManager().GetTexture(sprite.c_str());
}

void SpeechBubbleComponent::SetSizeBubble(Vector2f aSize)
{
Tga::Vector2f resolution = { static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().x), static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().y) };
myBubbleInstance.mySize = Tga::Vector2f{ aSize.x, aSize.y } *resolution.y;
}

void SpeechBubbleComponent::SetFollow(const bool aState)
{
myFollowTarget = aState;
}

void SpeechBubbleComponent::SetOffset(Vector3f aOffset)
{
myOffset = aOffset;
}

void SpeechBubbleComponent::SetTarget(GameObject* aTarget)
{
myTarget = aTarget;
Tga::Vector3f pos = myTarget->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
myText.SetPosition({ -0.5f * myText.GetWidth() + pos.x, 0.0f + pos.y });
myBubbleInstance.myPosition = Tga::Vector2f{ pos.x, pos.y };
}

void SpeechBubbleComponent::SetTextPosition(Vector2f aPosition)
{
myTextOffset = aPosition;
}

void SpeechBubbleComponent::SetText(const std::string& aText)
{
myText.SetText(aText);

Tga::Vector2f resolution = { static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().x), static_cast<float>(Tga::Engine::GetInstance()->GetRenderSize().y) };

myBubbleInstance.mySize = Tga::Vector2f{ 0.15f, 0.15f } *resolution.y;
myBubbleInstance.mySize.x += myText.GetWidth();
myBubbleInstance.mySize.y += myText.GetHeight();

Tga::Vector3f pos;
if (myTarget != nullptr)
{
	pos = myTarget->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
}
else
{
	pos = GetOwner()->GetTransform().GetPosition().ToTga() + myOffset.ToTga();
}
myText.SetPosition({ -0.5f * myText.GetWidth() + pos.x, 0.0f + pos.y });
}

void SpeechBubbleComponent::SetTime(float aTime)
{
myTime = aTime;
myCurrentTime = aTime;
}

void SpeechBubbleComponent::ResetTime()
{
myCurrentTime = myTime;
}

Vector2f SpeechBubbleComponent::GetTextSize()
{
return { myText.GetWidth(), myText.GetHeight() };
}

void SpeechBubbleComponent::SetTriggerInteract(const bool aState)
{
myTriggerInteract = aState;
}

bool SpeechBubbleComponent::GetTriggerInteract() const
{
return myTriggerInteract;
}

void SpeechBubbleComponent::RemoveFromRenderQueue()
{
ourRenderQueue.erase(
	std::remove(ourRenderQueue.begin(), ourRenderQueue.end(), this),
	ourRenderQueue.end());
}
