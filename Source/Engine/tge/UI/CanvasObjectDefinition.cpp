#include <stdafx.h>
#include <tge/script/JsonData.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "CanvasObjectDefinition.h"
#include <tge/settings/settings.h>
#include "tge/drawers/DebugDrawer.h"
#include "tge/graphics/GraphicsEngine.h"
#include "tge/drawers/SpriteDrawer.h"
#include "tge/graphics/TextureResource.h"
#include "tge/texture/TextureManager.h"
#include <tge/text/textfile.h>
#include <tge/text/fontfile.h>
#include <tge/text/text.h>
#include <tge/text/TextService.h>

using namespace nlohmann;
using namespace Tga;

std::unordered_map<Tga::StringId, Tga::Texture*> Tga::CanvasObjectDefinition::myTextureCache;
std::vector<QueuedSprite> Tga::CanvasObjectDefinition::ourSpriteQueue;
std::vector<Tga::Text> Tga::CanvasObjectDefinition::ourTextQueue;

void CanvasObjectDefinition::Save()
{
    if (myPath.empty())
    {
        std::cerr << "Error: Cannot save CanvasObjectDefinition, path is empty.\n";
        return;
    }

    std::filesystem::path path = Tga::Settings::GameAssetRoot() / myPath;
    if (path.extension().empty())
        path.replace_extension(".canvas");
    std::filesystem::create_directories(path.parent_path());

    json j;
    j["parent"] = myParent.GetString();
    j["showBounds"] = myShowBounds;
    j["referenceResolution"] = { myReferenceWindowResolution.x, myReferenceWindowResolution.y };
    j["uiElements"] = json::array();
    j["renderOrder"] = json::array();

    for (auto& e : myUIElements)
        j["uiElements"].push_back(SerializeUIElement(e));

    for (auto& e : myRenderOrder)
        j["renderOrder"].push_back(e);

    if (std::filesystem::exists(myPath))
    {
        fs::permissions(path, fs::perms::all);
    }

    std::ofstream out(path);
    if (!out.is_open())
    {
        std::cerr << "Failed to open file for writing: " << path << "\n";
        return;
    }

    out << j.dump(2);
    out.close();
    std::cout << "Saved CanvasObjectDefinition to: " << std::filesystem::absolute(path) << "\n";
}

void CanvasObjectDefinition::Load(const char* aPath)
{
    SetPath(aPath);
    std::filesystem::path path = Tga::Settings::GameAssetRoot() / aPath;

    json j;
    std::ifstream in(path);
    in >> j;
    in.close();

    std::string filename = std::filesystem::path(aPath).stem().string();
    SetName(StringRegistry::RegisterOrGetString(filename.c_str()));

    std::string parent;
    if (j.contains("parent") && j["parent"].is_string())
    {
        parent = j["parent"].get<std::string>();
    }

    myParent = StringRegistry::RegisterOrGetString(parent.c_str());
    myShowBounds = j.value("showBounds", true);

    if (j.contains("referenceResolution") && j["referenceResolution"].is_array())
    {
        auto& res = j["referenceResolution"];
        if (res.size() == 2)
        {
            myReferenceWindowResolution = { res[0].get<int>(), res[1].get<int>() };
        }
    }

    myUIElements.clear();
    if (j.contains("uiElements") && j["uiElements"].is_array())
    {
        for (const auto& elementJson : j["uiElements"])
        {
            UIElement loadedElement = DeserializeUIElement(elementJson);
            myUIElements.push_back(loadedElement);
        }
    }

    myRenderOrder.clear();
    if (j.contains("renderOrder") && j["renderOrder"].is_array())
    {
        for (const auto& orderJson : j["renderOrder"])
        {
            int loadedOrder = orderJson;
            myRenderOrder.push_back(loadedOrder);
        }
    }

    Tga::CanvasDrawParameters params = {
        .useIdShader = false,
        .showBounds = false,
        .resolution = Tga::Vector2i(1920, 1080)
    };

    for (int i = 0; i < myUIElements.size(); i++)
    {
        DrawCanvasElement(*this, myUIElements[i], params);
    }
}

void Tga::CanvasObjectDefinition::DrawQueued(RenderTarget* aRenderTarget)
{
    Tga::Engine* engine = Tga::Engine::GetInstance();

    aRenderTarget->SetAsActiveTarget(nullptr);

    for (const QueuedSprite& sprite : ourSpriteQueue)
    {
        engine->GetGraphicsEngine().GetSpriteDrawer().Draw(sprite.sharedData, sprite.instance);
    }

    for (Tga::Text& text : ourTextQueue)
    {
        text.Render();
    }

    aRenderTarget->SetAsActiveTarget(DX11::DepthBuffer);

    ourSpriteQueue.clear();
    ourTextQueue.clear();
}

void Tga::CanvasObjectDefinition::AddUIElement(UIElement& aUIElement)
{
    ApplyDefaults(aUIElement);
    myUIElements.push_back(aUIElement);

    int lastIndex = static_cast<int>(myUIElements.size()) - 1;
    RenameUIElement(lastIndex);
    myUIElements[lastIndex].generalProperties.hiercharyDisplayOrder = static_cast<int>(myUIElements.size()) - 1;
}

void Tga::CanvasObjectDefinition::RemoveUIElement(int& aIndex)
{
    if (myUIElements[aIndex].elementType == UIElementType::ElementGroup)
    {
        for (int i = 0; i < myUIElements.size(); i++)
        {
            if (myUIElements[i].generalProperties.groupIndex == aIndex)
                myUIElements[i].generalProperties.groupIndex = -1;
        }
    }

    myUIElements.erase(myUIElements.begin() + aIndex);
}

std::vector<UIElement>& Tga::CanvasObjectDefinition::GetUIElements()
{
    return myUIElements;
}

const std::vector<UIElement>& Tga::CanvasObjectDefinition::GetUIElements() const
{
    return myUIElements;
}

void Tga::CanvasObjectDefinition::RenameUIElement(const int aUIElementOfIndex)
{
    UIElement* uiElement = &myUIElements[aUIElementOfIndex];

    if (uiElement->generalProperties.name[0] == '\0')
    {
        snprintf(uiElement->generalProperties.name, 128, "*");
    }

    char baseName[128];
    strncpy(baseName, uiElement->generalProperties.name, 128);
    baseName[127] = '\0';

    char* bracket = strrchr(baseName, '(');
    if (bracket)
    {
        char* closing = strrchr(baseName, ')');
        if (closing && closing > bracket)
        {
            if (bracket > baseName && *(bracket - 1) == ' ')
            {
                *(bracket - 1) = '\0';
            }
            else
            {
                *bracket = '\0';
            }
        }
    }

    bool foundValidName = false;
    int nameIteration = 1;

    while (!foundValidName)
    {
        foundValidName = true;
        char candidate[128];

        if (nameIteration == 1)
        {
            snprintf(candidate, 128, "%s", baseName);
        }
        else
        {
            snprintf(candidate, 128, "%s (%d)", baseName, nameIteration);
        }

        for (int i = 0; i < myUIElements.size(); i++)
        {
            if (i == aUIElementOfIndex)
                continue;
            if (strcmp(candidate, myUIElements[i].generalProperties.name) == 0)
            {
                foundValidName = false;
                break;
            }
        }

        if (!foundValidName)
            nameIteration++;
        else
            snprintf(uiElement->generalProperties.name, 128, "%s", candidate);
    }
}

std::vector<int>& Tga::CanvasObjectDefinition::GetRenderOrder()
{
    return myRenderOrder;
}

const std::vector<int>& Tga::CanvasObjectDefinition::GetRenderOrder() const
{
    return myRenderOrder;
}

void Tga::CanvasObjectDefinition::SetRenderOrder(std::vector<int>& renderOrder)
{
    myRenderOrder = renderOrder;
}

void Tga::CanvasObjectDefinition::ApplyDefaults(UIElement& element)
{
    std::visit([&](auto& e)
        {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, UIImage>)
            {
                element.elementType = UIElementType::Image;
                if (e.sceneReference.Get().path.IsEmpty())
                    e.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::Image);
            }
            else if constexpr (std::is_same_v<T, UIButton>)
            {
                element.elementType = UIElementType::Button;
                if (e.buttonImage.sceneReference.Get().path.IsEmpty())
                    e.buttonImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::ButtonBackground);
            }
            else if constexpr (std::is_same_v<T, UIToggle>)
            {
                element.elementType = UIElementType::Toggle;
                if (e.backgroundImage.sceneReference.Get().path.IsEmpty())
                    e.backgroundImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::ToggleBackground);
                if (e.checkmarkImage.sceneReference.Get().path.IsEmpty())
                    e.checkmarkImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::ToggleCheckmark);
            }
            else if constexpr (std::is_same_v<T, UISlider>)
            {
                element.elementType = UIElementType::Slider;
                if (e.backgroundImage.sceneReference.Get().path.IsEmpty())
                    e.backgroundImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::SliderBackground);
                if (e.fillImage.sceneReference.Get().path.IsEmpty())
                    e.fillImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::SliderFill);
                if (e.handleBarImage.sceneReference.Get().path.IsEmpty())
                    e.handleBarImage.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::SliderHandle);
            }
            else if constexpr (std::is_same_v<T, UIText>)
            {
                element.elementType = UIElementType::Text;
                if (e.sceneReference.Get().path.IsEmpty())
                    e.sceneReference.Edit().path = StringRegistry::RegisterOrGetString(UIDefaults::TextFont);
            }
            else if constexpr (std::is_same_v<T, UIElementGroup>)
            {
                element.elementType = UIElementType::ElementGroup;
            }
        }, element.uiElementProperties);
}

const Tga::Vector2i& Tga::CanvasObjectDefinition::GetReferenceWindowResolution() const
{
    return myReferenceWindowResolution;
}

Tga::Vector2i& Tga::CanvasObjectDefinition::GetReferenceWindowResolution()
{
    return myReferenceWindowResolution;
}

void Tga::CanvasObjectDefinition::SetReferenceWindowResolution(const Tga::Vector2i& aReferenceWindowResolution)
{
    myReferenceWindowResolution = aReferenceWindowResolution;
}

const bool& Tga::CanvasObjectDefinition::GetShowBounds() const
{
    return myShowBounds;
}

bool& Tga::CanvasObjectDefinition::GetShowBounds()
{
    return myShowBounds;
}

void Tga::CanvasObjectDefinition::SetShowBounds(const bool someShowBounds)
{
    myShowBounds = someShowBounds;
}

bool CanvasObjectDefinition::DrawCanvasElement(const CanvasObjectDefinition& canvas, UIElement& uiElement, CanvasDrawParameters& drawParameters)
{
    if (drawParameters.useIdShader || uiElement.generalProperties.hide || (uiElement.generalProperties.groupIndex != -1 && canvas.GetUIElements()[uiElement.generalProperties.groupIndex].generalProperties.hide))
        return false;

    bool result = std::visit([&](auto&& element) -> bool
        {
            using T = std::decay_t<decltype(element)>;
            if constexpr (std::is_same_v<T, UIImage>)
            {
                DrawElementBounds(uiElement.generalProperties, canvas, drawParameters, nullptr);
                return DrawCanvasImage(element, uiElement.generalProperties, canvas, drawParameters);
            }
            else if constexpr (std::is_same_v<T, UIText>)
            {
                DrawElementBounds(uiElement.generalProperties, canvas, drawParameters, nullptr);
                return DrawCanvasText(element, uiElement.generalProperties, canvas, drawParameters);
            }
            else if constexpr (std::is_same_v<T, UIButton>)
            {
                DrawElementBounds(uiElement.generalProperties, canvas, drawParameters, &element.selectable);
                DrawCanvasText(element.buttonText, uiElement.generalProperties, canvas, drawParameters);
                DrawCanvasImage(element.buttonImage, uiElement.generalProperties, canvas, drawParameters);
                return true;
            }
            else if constexpr (std::is_same_v<T, UIToggle>)
            {
                DrawElementBounds(uiElement.generalProperties, canvas, drawParameters, &element.selectable);
                if (element.isOn)
                    DrawCanvasImage(element.checkmarkImage, uiElement.generalProperties, canvas, drawParameters, 1);
                DrawCanvasImage(element.backgroundImage, uiElement.generalProperties, canvas, drawParameters);
                return true;
            }
            else if constexpr (std::is_same_v<T, UISlider>)
            {
                DrawElementBounds(uiElement.generalProperties, canvas, drawParameters, &element.selectable);
                return DrawCanvasSlider(element, uiElement.generalProperties, canvas, drawParameters);
            }
            else
            {
                return true;
            }
        }, uiElement.uiElementProperties);

    return result;
}

void CanvasObjectDefinition::DrawElementBounds(
    const GeneralUIProperties& props,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters,
    const SelectableUIProperties* selectableProps)
{
    if (!drawParameters.showBounds)
        return;

    auto* debugDrawerPointer = &Tga::Engine::GetInstance()->GetDebugDrawer();
    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (props.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f, props.anchorPoint.y * 0.5f + 0.5f };
    Vector2f pivot01 = { -props.pivot.x * 0.5f + 0.5f, -props.pivot.y * 0.5f + 0.5f };

    Tga::Color gizmoColor;
    Vector2f boxSize;
    Vector2f topLeft;

    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            boxSize = props.size * uiScale;
            gizmoColor = Tga::Color(1.f, 1.f, 0.f, 0.5f);
            topLeft = (props.pos * uiScale) + anchorOffset;
            topLeft.x -= boxSize.x * pivot01.x;
            topLeft.y += boxSize.y * (1.0f - pivot01.y);
        }
        else if (selectableProps != nullptr && selectableProps->customMouseSelectionBounds)
        {
            topLeft.x += boxSize.x / 2.f;
            topLeft.y -= boxSize.y / 2.f;
            boxSize = selectableProps->mouseSelectionBounds * uiScale;
            topLeft.x -= boxSize.x / 2.f;
            topLeft.y += boxSize.y / 2.f;
            gizmoColor = Tga::Color(0.f, 0.f, 1.f, 0.5f);
        }
        else
            break;

        Vector2f topRight = { topLeft.x + boxSize.x, topLeft.y };
        Vector2f bottomLeft = { topLeft.x, topLeft.y - boxSize.y };
        Vector2f bottomRight = { topLeft.x + boxSize.x, topLeft.y - boxSize.y };

        // NOTE(leo): Separated components so it works in retail as well
        debugDrawerPointer->DrawLine(topLeft, topRight, { gizmoColor.r, gizmoColor.g, gizmoColor.b, gizmoColor.a });
        debugDrawerPointer->DrawLine(topRight, bottomRight, { gizmoColor.r, gizmoColor.g, gizmoColor.b, gizmoColor.a });
        debugDrawerPointer->DrawLine(bottomRight, bottomLeft, { gizmoColor.r, gizmoColor.g, gizmoColor.b, gizmoColor.a });
        debugDrawerPointer->DrawLine(bottomLeft, topLeft, { gizmoColor.r, gizmoColor.g, gizmoColor.b, gizmoColor.a });
    }

    gizmoColor = Tga::Color(1.f, 1.f, 0.f, 0.5f);
    debugDrawerPointer->DrawCircle((props.pos * uiScale) + anchorOffset, 5.f * uiScale.x, gizmoColor);
}

bool CanvasObjectDefinition::DrawCanvasImage(
    UIImage& image,
    const GeneralUIProperties& properties,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters,
    int additiveRenderOrder)
{
    UNREFERENCED_PARAMETER(additiveRenderOrder);

    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ properties.anchorPoint.x * 0.5f + 0.5f, properties.anchorPoint.y * 0.5f + 0.5f };

    const std::string path = image.sceneReference.Get().path.GetString();
    if (path.empty())
        return false;

    auto texture = GetTexture(image.sceneReference.Get().path, TextureSrgbMode::ForceSrgbFormat);
    if (!texture)
        return false;

    image.shared.myTexture = texture;
    image.instance.myPivot = { -properties.pivot.x * 0.5f + 0.5f, properties.pivot.y * 0.5f + 0.5f };

    Tga::Vector2f uiScale;
    if (properties.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    image.instance.mySize = properties.size * uiScale;
    image.instance.myPosition = (properties.pos * uiScale) + anchorOffset;
    image.instance.myColor = image.tint;

    QueueSprite(image.shared, image.instance);

    return true;
}

bool CanvasObjectDefinition::DrawCanvasText(
    UIText& text,
    const GeneralUIProperties& properties,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters)
{
    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (properties.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ properties.anchorPoint.x * 0.5f + 0.5f, properties.anchorPoint.y * 0.5f + 0.5f };

    const std::string fontPath = text.sceneReference.Get().path.GetString();
    if (fontPath.empty())
        return false;

    Tga::Text& renderText = text.textObject;
    renderText.SetFont(fontPath.c_str(), static_cast<FontSize>(text.fontSize));
    renderText.SetScale(text.fontScale * uiScale.x);
    renderText.SetColor(text.tint);

    std::string truncated = renderText.TruncateTextToBox(
        renderText, text.text, properties.size.x * uiScale.x, properties.size.y * uiScale.x, false);
    renderText.SetText(truncated);

    std::vector<std::string> lines = Tga::Text::SplitLines(truncated);
    const float lineHeight = renderText.GetLineHeight();

    float ascender = renderText.myTextService->GetAscender(renderText);
    float descender = renderText.myTextService->GetDescender(renderText);

    const float totalTextHeight = (lines.size() - 1) * lineHeight + ascender + descender;

    Vector2f boxSize = properties.size * uiScale;
    Vector2f pivot01 = { -properties.pivot.x * 0.5f + 0.5f, -properties.pivot.y * 0.5f + 0.5f };
    Vector2f boxTopLeft = (properties.pos * uiScale) + anchorOffset;
    boxTopLeft.x -= boxSize.x * pivot01.x;
    boxTopLeft.y += boxSize.y * (1.0f - pivot01.y);

    float blockYOffset = 0.0f;
    switch (text.verticalAlign)
    {
    case VerticalAlign::Top:
        blockYOffset = 0.0f;
        break;
    case VerticalAlign::Middle:
        blockYOffset = (boxSize.y - totalTextHeight) * 0.5f;
        break;
    case VerticalAlign::Bottom:
        blockYOffset = boxSize.y - totalTextHeight;
        break;
    }

    float y = boxTopLeft.y - blockYOffset - ascender;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        renderText.SetText(lines[i]);
        float lineWidth = renderText.GetWidth();
        float xOffset = 0.0f;

        switch (text.horizontalAlign)
        {
        case HorizontalAlign::Left:
            xOffset = 0.0f;
            break;
        case HorizontalAlign::Center:
            xOffset = (boxSize.x - lineWidth) * 0.5f;
            break;
        case HorizontalAlign::Right:
            xOffset = boxSize.x - lineWidth;
            break;
        }

        Vector2f linePos = { boxTopLeft.x + xOffset, y };
        renderText.SetPosition(linePos);
        ourTextQueue.push_back(renderText);
        //renderText.Render();

        y -= lineHeight;
    }

    return true;
}

bool CanvasObjectDefinition::DrawCanvasSlider(
    UISlider& slider,
    const GeneralUIProperties& props,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters)
{
    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f, props.anchorPoint.y * 0.5f + 0.5f };

    float normalized = (slider.currentValue - slider.minValue) / (slider.maxValue - slider.minValue);
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    DrawSliderHandle(slider, props, canvas, drawParameters, normalized);
    DrawCanvasImageCropped(slider.fillImage, props, canvas, drawParameters, normalized);
    DrawCanvasImage(slider.backgroundImage, props, canvas, drawParameters);

    return true;
}

bool CanvasObjectDefinition::DrawCanvasImageCropped(
    UIImage& image,
    const GeneralUIProperties& props,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters,
    float normalizedWidth)
{
    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (props.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f, props.anchorPoint.y * 0.5f + 0.5f };

    const std::string path = image.sceneReference.Get().path.GetString();
    if (path.empty())
        return false;

    auto texture = GetTexture(image.sceneReference.Get().path, TextureSrgbMode::ForceSrgbFormat);
    if (!texture)
        return false;

    image.shared.myTexture = texture;

    Vector2f fullSize = props.size * uiScale;
    image.instance.myPivot = { -props.pivot.x * 0.5f + 0.5f, props.pivot.y * 0.5f + 0.5f };

    Vector2f basePosition = (props.pos * uiScale) + anchorOffset;
    float originalLeft = basePosition.x - fullSize.x * image.instance.myPivot.x;
    float newWidth = fullSize.x * normalizedWidth;
    float newCenterX = originalLeft + newWidth * image.instance.myPivot.x;

    image.instance.mySize = { newWidth, fullSize.y };
    image.instance.myPosition = { newCenterX, basePosition.y };
    image.instance.myTextureRect = { 0.0f, 0.0f, normalizedWidth, 1.0f };
    image.instance.myColor = image.tint;

    const auto& engine = Engine::GetInstance();
    engine->GetGraphicsEngine().GetSpriteDrawer().Draw(image.shared, image.instance);

    return true;
}

void CanvasObjectDefinition::DrawSliderHandle(
    UISlider& slider,
    const GeneralUIProperties& props,
    const CanvasObjectDefinition& canvas,
    CanvasDrawParameters& drawParameters,
    float normalized)
{
    const auto resolution = drawParameters.resolution;
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (props.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f, props.anchorPoint.y * 0.5f + 0.5f };

    const std::string path = slider.handleBarImage.sceneReference.Get().path.GetString();
    if (path.empty())
        return;

    auto texture = GetTexture(slider.handleBarImage.sceneReference.Get().path, TextureSrgbMode::ForceSrgbFormat);
    if (!texture)
        return;

    slider.handleBarImage.shared.myTexture = texture;

    slider.handleBarImage.instance.myPivot = { -props.pivot.x * 0.5f + 0.5f, props.pivot.y * 0.5f + 0.5f };

    Vector2f fullSize = props.size * uiScale;
    Vector2f basePosition = (props.pos * uiScale) + anchorOffset;
    float leftEdge = basePosition.x - fullSize.x * 0.5f;
    float handleX = leftEdge + fullSize.x * normalized;

    float handleHeight = fullSize.y;

    float textureWidth = texture->mySize.x;
    float textureHeight = texture->mySize.y;

    float aspect = textureWidth / textureHeight;

    slider.handleBarImage.instance.mySize =
    {
        handleHeight * aspect,
        handleHeight
    };

    slider.handleBarImage.instance.myPosition = { handleX, basePosition.y };
    slider.handleBarImage.instance.myColor = slider.handleBarImage.tint;

    const auto& engine = Engine::GetInstance();
    engine->GetGraphicsEngine().GetSpriteDrawer().Draw(slider.handleBarImage.shared, slider.handleBarImage.instance);
}

bool Tga::CanvasObjectDefinition::PosInside(const Tga::Vector2f& point, const GeneralUIProperties& props, const CanvasObjectDefinition& canvas, const Tga::Vector2i& resolution, const SelectableUIProperties* selectableProps)
{
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (props.scaleUniformly)
        uiScale = CalculateUIScaleUniformly(resolution, reference);
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset = Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f, props.anchorPoint.y * 0.5f + 0.5f };
    Vector2f pivot01 = { -props.pivot.x * 0.5f + 0.5f, -props.pivot.y * 0.5f + 0.5f };

    Vector2f boxSize;
    Vector2f topLeft;

    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            boxSize = props.size * uiScale;
            topLeft = (props.pos * uiScale) + anchorOffset;
            topLeft.x -= boxSize.x * pivot01.x;
            topLeft.y += boxSize.y * (1.0f - pivot01.y);
        }
        else if (selectableProps != nullptr && selectableProps->customMouseSelectionBounds)
        {
            topLeft.x += boxSize.x / 2.f;
            topLeft.y -= boxSize.y / 2.f;
            boxSize = selectableProps->mouseSelectionBounds * uiScale;
            topLeft.x -= boxSize.x / 2.f;
            topLeft.y += boxSize.y / 2.f;
        }
        else
            break;
    }


    Vector2f center = { topLeft.x + (boxSize.x / 2), topLeft.y - (boxSize.y / 2) };

    return (point.x > (center.x - (boxSize.x / 2.f)) && point.x < (center.x + (boxSize.x / 2.f)) && point.y < (center.y + (boxSize.y / 2.f)) && point.y >(center.y - (boxSize.y / 2.f)));
}

Tga::Vector2f CanvasObjectDefinition::ScreenPosToUIPos(
    const Tga::Vector2f& screenPos,
    const GeneralUIProperties& props,
    const CanvasObjectDefinition& canvas,
    const Tga::Vector2i& resolution)
{
    const auto reference = canvas.GetReferenceWindowResolution();

    Tga::Vector2f uiScale;
    if (props.scaleUniformly)
        uiScale = { CalculateUIScaleUniformly(resolution, reference) };
    else
        uiScale = CalculateUIScale(resolution, reference);

    const Vector2f anchorOffset =
        Vector2f{ (float)resolution.x, (float)resolution.y } *
        Vector2f{ props.anchorPoint.x * 0.5f + 0.5f,
                  props.anchorPoint.y * 0.5f + 0.5f };

    return (screenPos - anchorOffset) / uiScale;
}

float CanvasObjectDefinition::CalculateUIScaleUniformly(const Tga::Vector2i& resolution, const Tga::Vector2i& referenceResolution)
{
    Tga::Vector2f uiScale = CalculateUIScale(resolution, referenceResolution);
    return std::min(uiScale.x, uiScale.y);
}

Tga::Vector2f CanvasObjectDefinition::CalculateUIScale(const Tga::Vector2i& resolution, const Tga::Vector2i& referenceResolution)
{
    const float scaleX = static_cast<float>(resolution.x) / static_cast<float>(referenceResolution.x);
    const float scaleY = static_cast<float>(resolution.y) / static_cast<float>(referenceResolution.y);
    return { scaleX, scaleY };
}

Texture* CanvasObjectDefinition::GetTexture(StringId path, TextureSrgbMode srgbMode)
{
    if (path.IsEmpty())
        return nullptr;

    Texture* texture = nullptr;
    auto cacheIt = myTextureCache.find(path);
    if (cacheIt != myTextureCache.end())
    {
        texture = cacheIt->second;
    }
    else
    {
        auto& engine = *Tga::Engine::GetInstance();
        auto& textureManager = engine.GetTextureManager();
        texture = textureManager.GetTexture(path.GetString(), srgbMode);
        myTextureCache[path] = texture;
    }

    return texture;
}

void CanvasObjectDefinition::QueueSprite(SpriteSharedData aSharedData, Sprite2DInstanceData aInstance)
{
    QueuedSprite queuedSprite =
    {
        .sharedData = aSharedData,
        .instance = aInstance
    };

    ourSpriteQueue.push_back(queuedSprite);
}
