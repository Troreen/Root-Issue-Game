#pragma once
#include <tge/math/Vector.h>
#include <tge/scene/ScenePropertyTypes.h>
#include <nlohmann/json.hpp>
#include <tge/stringRegistry/StringRegistry.h>
#include "tge/math/Vector2.h"
#include "tge/math/color.h"
#include "string.h"
#include <vector>
#include <variant>
#include <tge/text/text.h>
#include <tge/sprite/sprite.h>
#include <tge/text/TextService.h>

using json = nlohmann::json;

namespace UIDefaults
{
    static const char* Image = "DefaultUIAssets/KnightRotAd.dds";

    static const char* ButtonBackground = "DefaultUIAssets/DefaultUIButtonBackground.dds";

    static const char* ToggleBackground = "DefaultUIAssets/DefaultUIToggleBackground.dds";
    static const char* ToggleCheckmark = "DefaultUIAssets/DefaultUIToggleCheckmark.dds";

    static const char* SliderBackground = "DefaultUIAssets/DefaultUISliderBackground.ddS";
    static const char* SliderFill = "DefaultUIAssets/DefaultUISliderFill.dds";
    static const char* SliderHandle = "DefaultUIAssets/DefaultUISliderHandle.dds";

    static const char* TextFont = "DefaultUIAssets/arial.ttf";
}

struct GeneralUIProperties
{
    bool hide = false;
    char name[128] = "New UI";
    Tga::Vector2f pos;
    Tga::Vector2f anchorPoint;
    Tga::Vector2f size = { 100.0f, 100.0f };
    bool scaleUniformly = true;
    int renderOrder = 0;
    Tga::Vector2f pivot;
    int hiercharyDisplayOrder = 0;
    int groupIndex = -1;
};

struct SelectableUIProperties
{
    int navigation[4] = { -1, -1, -1, -1 };
    bool customMouseSelectionBounds = false;
    Tga::Vector2f mouseSelectionBounds = { 100.0f, 100.0f };
};

struct UIImage
{
    Tga::CopyOnWriteWrapper<Tga::SceneReference> sceneReference;
    Tga::Color tint = Tga::Color(1, 1, 1, 1);
    Tga::SpriteSharedData shared;
    Tga::Sprite2DInstanceData instance;
};

enum class HorizontalAlign
{
    Left,
    Center,
    Right
};

enum class VerticalAlign
{
    Top,
    Middle,
    Bottom
};

struct UIText
{
    char text[128] = "Hello World!";
    int fontSize = 30;
    float fontScale = 1.0f;
    HorizontalAlign horizontalAlign = HorizontalAlign::Left;
    VerticalAlign verticalAlign = VerticalAlign::Top;
    Tga::CopyOnWriteWrapper<Tga::SceneReference> sceneReference;
    Tga::Color tint = Tga::Color(1, 1, 1, 1);
    Tga::Text textObject;
};

struct UIButton
{
    UIText buttonText;
    UIImage buttonImage;
    SelectableUIProperties selectable;
};

struct UIToggle
{
    bool defaultOn = true;
    bool isOn;
    UIImage backgroundImage;
    UIImage checkmarkImage;
    SelectableUIProperties selectable;
};

struct UISlider
{
    float defaultValue = .5f;
    float currentValue;
    float minValue = 0.f;
    float maxValue = 1.f;
    UIImage backgroundImage;
    UIImage fillImage;
    UIImage handleBarImage;
    SelectableUIProperties selectable;
};

struct UIElementGroup
{

};

using UIElementProperties = std::variant<UIImage, UIText, UIButton, UIToggle, UISlider, UIElementGroup>;

enum class UIElementType
{
    Image,
    Text,
    Button,
    Toggle,
    Slider,
    ElementGroup,
    Count
};

inline const char* ToString(UIElementType type)
{
    switch (type)
    {
    case UIElementType::Image: return "Image";
    case UIElementType::Text: return "Text";
    case UIElementType::Button: return "Button";
    case UIElementType::Toggle: return "Toggle";
    case UIElementType::Slider: return "Slider";
    case UIElementType::ElementGroup: return "ElementGroup";
    default: return "Unknown";
    }
}

inline UIElementType UIElementTypeFromString(const std::string& str)
{
    if (str == "Image") return UIElementType::Image;
    if (str == "Text") return UIElementType::Text;
    if (str == "Button") return UIElementType::Button;
    if (str == "Toggle") return UIElementType::Toggle;
    if (str == "Slider") return UIElementType::Slider;
    if (str == "ElementGroup") return UIElementType::ElementGroup;

    assert(false && "Invalid UIElementType string in JSON");
    return UIElementType::Image;
}

struct UIElement
{
    UIElementType elementType;
    GeneralUIProperties generalProperties;
    UIElementProperties uiElementProperties;
};

static void SerializeGeneral(json& j, const GeneralUIProperties& p)
{
    j["position"] = { p.pos.x, p.pos.y };
    j["size"] = { p.size.x, p.size.y };
    j["anchor"] = { p.anchorPoint.x, p.anchorPoint.y };
    j["pivot"] = { p.pivot.x, p.pivot.y };
    j["renderOrder"] = p.renderOrder;
    j["name"] = p.name;
    j["hiercharyDisplayOrder"] = p.hiercharyDisplayOrder;
    j["scaleUniformly"] = p.scaleUniformly;
    j["groupIndex"] = p.groupIndex;
    j["hide"] = p.hide;
}

static void DeserializeGeneral(const json& j, GeneralUIProperties& p)
{
    if (j.contains("position"))
        p.pos = { j["position"][0], j["position"][1] };
    if (j.contains("size"))
        p.size = { j["size"][0], j["size"][1] };
    if (j.contains("anchor"))
        p.anchorPoint = { j["anchor"][0], j["anchor"][1] };
    if (j.contains("pivot"))
        p.pivot = { j["pivot"][0], j["pivot"][1] };
    if (j.contains("renderOrder"))
        p.renderOrder = j["renderOrder"];
    if (j.contains("name"))
        std::snprintf(p.name, sizeof(p.name), "%s", j["name"].get<std::string>().c_str());
    if (j.contains("hiercharyDisplayOrder"))
        p.hiercharyDisplayOrder = j["hiercharyDisplayOrder"];
    if (j.contains("scaleUniformly"))
        p.scaleUniformly = j["scaleUniformly"];
    if (j.contains("groupIndex"))
        p.groupIndex = j["groupIndex"];
    if (j.contains("hide"))
        p.hide = j["hide"];
}

static void SerializeSelectable(json& j, const SelectableUIProperties& p)
{
    j["navigation"] = { p.navigation[0], p.navigation[1], p.navigation[2], p.navigation[3] };
    j["customMouseSelectionBounds"] = p.customMouseSelectionBounds;
    j["mouseSelectionBounds"] = { p.mouseSelectionBounds.x, p.mouseSelectionBounds.y };
}

static void DeserializeSelectable(const json& j, SelectableUIProperties& p)
{
    if (j.contains("navigation"))
    {
        for (int i = 0; i < 4; ++i)
            p.navigation[i] = j["navigation"][i];
    }
    if (j.contains("customMouseSelectionBounds"))
        p.customMouseSelectionBounds = j["customMouseSelectionBounds"];
    if (j.contains("mouseSelectionBounds"))
        p.mouseSelectionBounds = { j["mouseSelectionBounds"][0], j["mouseSelectionBounds"][1] };
}

static void SerializeUIImage(json& j, UIImage& img)
{
    j["tint"] = { img.tint.r, img.tint.g, img.tint.b, img.tint.a };
    j["sceneReference"] = img.sceneReference.Get().path.GetString();
}

static void DeserializeUIImage(const json& j, UIImage& img)
{
    if (j.contains("tint"))
        img.tint = { j["tint"][0], j["tint"][1], j["tint"][2], j["tint"][3] };
    if (j.contains("sceneReference"))
        img.sceneReference.Edit().path =
        Tga::StringRegistry::RegisterOrGetString(j["sceneReference"].get<std::string>().c_str());
}

static void SerializeUIText(json& j, UIText& txt)
{
    j["text"] = txt.text;
    j["fontScale"] = txt.fontScale;
    j["fontSize"] = txt.fontSize;
    j["horizontalAlign"] = (int)txt.horizontalAlign;
    j["verticalAlign"] = (int)txt.verticalAlign;
    j["tint"] = { txt.tint.r, txt.tint.g, txt.tint.b, txt.tint.a };
    j["sceneReference"] = txt.sceneReference.Get().path.GetString();
}

static void InitializeUITextTextObject(UIText& txt)
{
    if (!txt.sceneReference.Get().path.IsEmpty())
    {
        txt.textObject = Tga::Text();
        txt.textObject.SetFont(
            txt.sceneReference.Get().path.GetString(),
            static_cast<Tga::FontSize>(txt.fontSize),
            0
        );

        txt.textObject.SetText(txt.text);
        txt.textObject.SetScale(txt.fontScale);
        txt.textObject.SetColor(txt.tint);
    }
}

static void DeserializeUIText(const json& j, UIText& txt)
{
    if (j.contains("text"))
        std::snprintf(txt.text, sizeof(txt.text), "%s", j["text"].get<std::string>().c_str());
    if (j.contains("fontSize"))
        txt.fontSize = j["fontSize"];
    if (j.contains("fontScale"))
        txt.fontScale = j["fontScale"];
    if (j.contains("horizontalAlign"))
        txt.horizontalAlign = (HorizontalAlign)j["horizontalAlign"].get<int>();
    if (j.contains("verticalAlign"))
        txt.verticalAlign = (VerticalAlign)j["verticalAlign"].get<int>();
    if (j.contains("tint"))
        txt.tint = { j["tint"][0], j["tint"][1], j["tint"][2], j["tint"][3] };
    if (j.contains("sceneReference"))
        txt.sceneReference.Edit().path =
        Tga::StringRegistry::RegisterOrGetString(j["sceneReference"].get<std::string>().c_str());

    InitializeUITextTextObject(txt);
}

static void SerializeUIButton(json& j, UIButton& btn)
{
    SerializeUIImage(j["image"], btn.buttonImage);
    SerializeUIText(j["text"], btn.buttonText);
}

static void DeserializeUIButton(const json& j, UIButton& btn)
{
    DeserializeUIImage(j["image"], btn.buttonImage);
    DeserializeUIText(j["text"], btn.buttonText);
}

static void SerializeUIToggle(json& j, UIToggle& t)
{
    j["defaultOn"] = t.defaultOn;
    SerializeUIImage(j["background"], t.backgroundImage);
    SerializeUIImage(j["checkmark"], t.checkmarkImage);
}

static void DeserializeUIToggle(const json& j, UIToggle& t)
{
    t.defaultOn = j.value("defaultOn", false);
    DeserializeUIImage(j["background"], t.backgroundImage);
    DeserializeUIImage(j["checkmark"], t.checkmarkImage);
}

static void SerializeUISlider(json& j, UISlider& s)
{
    j["defaultValue"] = s.defaultValue;
    j["minValue"] = s.minValue;
    j["maxValue"] = s.maxValue;
    SerializeUIImage(j["background"], s.backgroundImage);
    SerializeUIImage(j["fill"], s.fillImage);
    SerializeUIImage(j["handle"], s.handleBarImage);
}

static void DeserializeUISlider(const json& j, UISlider& s)
{
    s.defaultValue = j.value("defaultValue", 0.0f);
    s.minValue = j.value("minValue", 0.0f);
    s.maxValue = j.value("maxValue", 1.0f);
    DeserializeUIImage(j["background"], s.backgroundImage);
    DeserializeUIImage(j["fill"], s.fillImage);
    DeserializeUIImage(j["handle"], s.handleBarImage);
    s.currentValue = s.defaultValue;
}

static json SerializeUIElement(UIElement& e)
{
    json j;
    SerializeGeneral(j, e.generalProperties);
    j["type"] = ToString(e.elementType);

    std::visit([&](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, UIImage>)
            {
                SerializeUIImage(j["data"], arg);
            }
            else if constexpr (std::is_same_v<T, UIText>)
            {
                SerializeUIText(j["data"], arg);
            }
            else if constexpr (std::is_same_v<T, UIButton>)
            {
                SerializeUIButton(j["data"], arg);
                SerializeSelectable(j["data"], arg.selectable);
            }
            else if constexpr (std::is_same_v<T, UIToggle>)
            {
                SerializeUIToggle(j["data"], arg);
                SerializeSelectable(j["data"], arg.selectable);
            }
            else if constexpr (std::is_same_v<T, UISlider>)
            {
                SerializeUISlider(j["data"], arg);
                SerializeSelectable(j["data"], arg.selectable);
            }
        }, e.uiElementProperties);

    return j;
}

static UIElement DeserializeUIElement(const json& j)
{
    UIElement e{};

    DeserializeGeneral(j, e.generalProperties);

    if (!j.contains("type"))
    {
        assert(false && "UIElement missing type!");
        return e;
    }

    e.elementType = UIElementTypeFromString(j["type"].get<std::string>());

    if (!j.contains("data"))
    {
        return e;
    }

    const json& data = j["data"];

    switch (e.elementType)
    {
    case UIElementType::Image:
    {
        UIImage img;
        DeserializeUIImage(data, img);
        e.uiElementProperties = img;
        break;
    }
    case UIElementType::Text:
    {
        UIText txt;
        DeserializeUIText(data, txt);
        e.uiElementProperties = txt;
        break;
    }
    case UIElementType::Button:
    {
        UIButton btn;
        DeserializeUIButton(data, btn);
        DeserializeSelectable(data, btn.selectable);
        e.uiElementProperties = btn;
        break;
    }
    case UIElementType::Toggle:
    {
        UIToggle t;
        DeserializeUIToggle(data, t);
        DeserializeSelectable(data, t.selectable);
        e.uiElementProperties = t;
        break;
    }
    case UIElementType::Slider:
    {
        UISlider s;
        DeserializeUISlider(data, s);
        DeserializeSelectable(data, s.selectable);
        e.uiElementProperties = s;
        break;
    }
    }

    return e;
}

