/*
This class handles all texts that will be rendered, don't use this to show text, only use the Text class
*/

#pragma once

#include <tge/EngineDefines.h>
#include <tge/math/color.h>
#include <tge/text/fontfile.h>
#include <tge/text/text.h>
#include <unordered_map>
#include <vector>

namespace Tga
{
	class Texture;
	class Text;
	class TextService
	{
	public:
		TextService();
		~TextService();

		void Init();

		Font GetOrLoad(std::string aFontPathAndName, FontSize aFontSize, unsigned char aBorderSize = 0);
		bool Draw(Tga::Text& aText, Tga::SpriteShader* aCustomShaderToRenderWith = nullptr, bool forceInstant = true);
		float GetTextBlockHeight(const Text& text) const;
		float MeasureWidth(const Tga::Text& textTemplate, const std::string& str) const;
		float GetSentenceWidth(Tga::Text& aText);
		float GetSentenceHeight(Tga::Text& aText);
		float GetLineHeight(const Tga::Text& text) const;
		float GetAscender(const Tga::Text& text) const;
		float GetDescender(const Tga::Text& text) const;

	private:
		struct FT_LibraryRec_* myLibrary;

		std::unordered_map<std::string, std::weak_ptr<InternalTextAndFontData>> myFontData;
	};
}