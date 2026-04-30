/*
Use this class to create and show a text
*/

#pragma once
#include <tge/math/Color.h>
#include <tge/math/vector2.h>
#include <tge/render/RenderCommon.h>
#include <string>
#include <memory>
#include "tge/sprite/sprite.h"
#include "tge/drawers/SpriteDrawer.h"

namespace Tga
{
	enum FontSize
	{
		FontSize_6 = 6,
		FontSize_8 = 8,
		FontSize_9 = 9,
		FontSize_10 = 10,
		FontSize_11 = 11,
		FontSize_12 = 12,
		FontSize_14 = 14,
		FontSize_18 = 18,
		FontSize_24 = 24,
		FontSize_30 = 30,
		FontSize_36 = 36,
		FontSize_48 = 48,
		FontSize_60 = 60,
		FontSize_72 = 72,
		FontSize_Count
	};

	static int FontSizeToEnumIndex(const FontSize& fontSize)
	{
		switch (fontSize)
		{
		case FontSize_6:
			return 0;
		case FontSize_8:
			return 1;
		case FontSize_9:
			return 2;
		case FontSize_10:
			return 3;
		case FontSize_11:
			return 4;
		case FontSize_12:
			return 5;
		case FontSize_14:
			return 6;
		case FontSize_18:
			return 7;
		case FontSize_24:
			return 8;
		case FontSize_30:
			return 9;
		case FontSize_36:
			return 10;
		case FontSize_48:
			return 11;
		case FontSize_60:
			return 12;
		case FontSize_72:
			return 13;
		default:
			return 0;
		}
	}

	static FontSize EnumIndexToFontSize(int index)
	{
		static const FontSize fontSizes[] =
		{
			FontSize_6,
			FontSize_8,
			FontSize_9,
			FontSize_10,
			FontSize_11,
			FontSize_12,
			FontSize_14,
			FontSize_18,
			FontSize_24,
			FontSize_30,
			FontSize_36,
			FontSize_48,
			FontSize_60,
			FontSize_72
		};

		if (index < 0 || index >= sizeof(fontSizes) / sizeof(fontSizes[0]))
			return FontSize_12;

		return fontSizes[index];
	}

	class InternalTextAndFontData;
	struct Font
	{
		std::shared_ptr<const InternalTextAndFontData> myData;
	};

	class TextService;
	class SpriteShader;
	class Text
	{
		friend class TextService;
	public:
		Text(const Font& font);

		// If this is the first time creating the text, the text will be loaded in memory, dont do this runtime
		/*aPathAndName: ex. taxe/arial.ttf, */
		Text(const char* aPathAndName = "Text/arial.ttf", FontSize aFontSize = FontSize_14, unsigned char aBorderSize = 0);
		~Text();
		void Render(bool forceInstant = true);
		void Render(Tga::SpriteShader* aCustomShaderToRenderWith);
		float GetWidth();
		float GetHeight();

		void SetColor(const Color& aColor);
		Color GetColor() const;

		void SetText(const std::string& aText);
		std::string GetText() const;

		void SetPosition(const Vector2f& aPosition);
		Vector2f GetPosition() const;

		void SetFont(const char* aPathAndName, FontSize aFontSize, unsigned char aBorderSize = 0);

		void SetScale(float aScale);
		float GetScale() const;

		void SetRotation(float aRotation) { myRotation = aRotation; }
		float GetRotation() const { return myRotation; }

		std::string TruncateTextToBox(Tga::Text& text, const std::string& input, float maxWidth, float maxHeight, bool addEllipsis = true);
		static std::vector<std::string> SplitLines(const std::string& text);

		float GetLineHeight() const;
	
		Font myFont;
		TextService* myTextService;
		int myRenderOrder = 0;

		Tga::SpriteSharedData mySharedData;
		Tga::Sprite2DInstanceData myInstanceData[128];

	protected:

		std::string myText;
		Vector2f myPosition;
		float myScale;
		float myRotation;
		Color myColor;
	};
}

