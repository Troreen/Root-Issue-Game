#pragma once

#include <tge/stringRegistry/StringRegistry.h>
#include <tge/script/Property.h>
#include "UIElement.h"
#include "tge/scene/Scene.h"
#include <tge/graphics/RenderTarget.h>
#include <tge/graphics/DX11.h>

struct CanvasDrawParameters
{
	bool useIdShader;
	bool showBounds;
	Tga::Vector2i resolution;
};

struct QueuedSprite
{
	Tga::SpriteSharedData sharedData;
	Tga::Sprite2DInstanceData instance;
};

class CanvasObjectDefinition
{
public:
	void SetName(Tga::StringId name) { myName = name; }
	const Tga::StringId GetName() const { return myName; }

	void SetPath(const char* path) { myPath = path; }
	const char* GetPath() const { return myPath.c_str(); }

	void Save();
	void Load(const char* aPath);

	static void DrawQueued();

	static bool DrawCanvasElement(const CanvasObjectDefinition& canvas, UIElement& uiElement, CanvasDrawParameters& drawParameters);

	static void DrawElementBounds(const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters, const SelectableUIProperties* selectableProps);

	static bool DrawCanvasImage(UIImage& image, const GeneralUIProperties& properties, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters, int additiveRenderOrder = 0);

	static bool DrawCanvasText(UIText& text, const GeneralUIProperties& properties, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters);

	static bool DrawCanvasSlider(UISlider& slider, GeneralUIProperties& props, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters);

	static bool DrawCanvasImageCropped(UIImage& image, const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters, float normalizedWidth);

	static void DrawSliderHandle(UISlider& slider, const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, CanvasDrawParameters& drawParameters, float normalized);

	static bool PosInside(const Tga::Vector2f& point, const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, const Tga::Vector2i& resolution, const SelectableUIProperties* selectableProps);

	static Tga::Vector2f ScreenPosToUIPos(const Tga::Vector2f& screenPos, const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, const Tga::Vector2i& resolution);

	void AddUIElement(UIElement& aUIElement);
	void RemoveUIElement(int& aIndex);
	std::vector<UIElement>& GetUIElements();
	const std::vector<UIElement>& GetUIElements() const;
	void RenameUIElement(const int aUIElementOfIndex);

	std::vector<int>& GetRenderOrder();
	const std::vector<int>& GetRenderOrder() const;
	void SetRenderOrder(std::vector<int>& renderOrder);

	void ApplyDefaults(UIElement& element);

	const Tga::Vector2i& GetReferenceWindowResolution() const;
	Tga::Vector2i& GetReferenceWindowResolution();
	void SetReferenceWindowResolution(const Tga::Vector2i& aReferenceWindowResolution);

	const bool& GetShowBounds() const;
	bool& GetShowBounds();
	void SetShowBounds(const bool someShowBounds);

	static float CalculateUIScaleUniformly(const Tga::Vector2i& resolution, const Tga::Vector2i& referenceResolution);

	static Tga::Vector2f CalculateUIScale(const Tga::Vector2i& resolution, const Tga::Vector2i& referenceResolution);

	static Tga::Texture* GetTexture(Tga::StringId path, Tga::TextureSrgbMode srgbMode);


private:
	static std::unordered_map<Tga::StringId, Tga::Texture*> myTextureCache;
	static std::vector<QueuedSprite> ourSpriteQueue;
	static std::vector<Tga::Text> ourTextQueue;

	std::string myPath;
	Tga::StringId myName;

	Tga::StringId myParent;
	std::vector<UIElement> myUIElements;
	std::vector<int> myRenderOrder;

	Tga::Vector2i myReferenceWindowResolution = { 1920,1080 };
	bool myShowBounds = true;

	static void QueueSprite(Tga::SpriteSharedData aSharedData, Tga::Sprite2DInstanceData aInstance);
};
