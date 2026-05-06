#pragma once
#include "tge/UI/CanvasObjectDefinition.h"
#include "tge/UI/CanvasObjectDefinitionManager.h"
#include "tge/graphics/Camera.h"
#include "tge/UI/UIElement.h"

struct RuntimeUIElement
{
    UIElement* definition;
    bool hovered = false;
    bool pressed = false;

    Tga::Vector2f min;
    Tga::Vector2f max;
};

class UICanvas
{
public:
    UICanvas();
    ~UICanvas();
    void Init(const Tga::StringId& aCanvasPath, Tga::CanvasObjectDefinitionManager& aManager);
    static void UpdateAll();
    static void RenderAll();

    static std::unique_ptr<Tga::Camera> ourUICamera;

    void SetIsHidden(bool someIsHidden);
    bool GetIsHidden();

    void SetSelectedElement(const std::string aID);
    const std::string GetSelectedElementName();
    RuntimeUIElement* GetSelectedElement();
    RuntimeUIElement* GetElement(const std::string aID);
    UIButton* GetButton(const std::string aID);
    UIToggle* GetToggle(const std::string aID);
    UISlider* GetSlider(const std::string aID);
    UIImage* GetImage(const std::string aID);
    UIText* GetText(const std::string aID);
    UIElementGroup* GetElementGroup(const std::string aID);

    bool GetElementPressed(std::string aID);
    bool GetToggleValue(std::string aID);
    float GetSliderValue(std::string aID);
    void SetIsHidden(std::string aID, const bool aIsHidden);
    bool GetIsHidden(std::string aID);

    void ResetIsFocused();

    CanvasObjectDefinition* GetCanvas();
protected:
    void Render();
    void Update();

    void NavigationControl();
    void InteractionControl();

    std::vector<RuntimeUIElement> myElements;
    std::vector<int> myRenderOrder;
    CanvasObjectDefinition* myCanvas = nullptr;
    static std::vector<UICanvas*> ourUICanvases;
    bool myIsHidden;

    float myKeyboardSliderSpeed;
    float myMouseSliderSpeed;

    float myInputDelay = 0.2f;
    float myInputTimer = 0.2f;

    RuntimeUIElement* mySelectedElement;
    bool isFocused;

    Tga::Vector2f myLastMousePosition;
    Tga::Vector2f mySelectedOriginalSize;
    float mySelectedElementSizeMultiplier;
    float mySliderFocusElementSizeMultiplier;
    float myCurrentSelectedSizeMultiplier;
    float myUISizeModificationSpeed;
    bool myMouseActive;
};
