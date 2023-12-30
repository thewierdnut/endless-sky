/* GlyphString.cpp
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GlyphString.h"

#include "FontSet.h"
#include "Utf8.h"

#include <fribidi/fribidi.h>
#include <harfbuzz/hb.h>

#include <memory>
#include <string>



namespace
{
	std::vector<GlyphString::Glyph> ProcessUtf8Text(const std::string& text, FontSize s)
	{
		// Step 1. Convert from utf8 to unicode code points.
		std::vector<FriBidiChar> utext;
		size_t pos = 0;
		FriBidiChar c = 0;
		while(pos != std::string::npos && (c = Utf8::DecodeCodePoint(text, pos)))
			utext.push_back(c);

		// Step 2. Apply bidi algorithm to convert the text to identical direction runs.
		std::vector<FriBidiChar> visualText(utext.size());
		// TYPE_ON means figure it out based on text. It will take its cue from
		// the first strong character it finds, and reset baseDir to indicate the
		// direction of the output text.
		FriBidiCharType baseDir = FRIBIDI_TYPE_ON;
		// TODO: should we save off the ltov/vtol offsets and process text one segment at a time?
		fribidi_log2vis(utext.data(), utext.size(), &baseDir, visualText.data(), nullptr, nullptr, nullptr);

		// Step 3. For mixed-script text, we will need to break up the text
		// according to the valid fonts supported.
		std::vector<std::pair<const FontSet::FontInfo*, std::vector<FriBidiChar>>> textRuns;
		const FontSet::FontInfo* lastFont = nullptr;
		for (FriBidiChar c: utext)
		{
			if(!lastFont || !lastFont->HasUnicodeChar(c))
			{
				if(!(lastFont = FontSet::FontForUnicodeChar(c, s)))
				{
					// No loaded font can render this character. Swap it out for a
					// replacement character.
					c = '?';
					if(!(lastFont = FontSet::FontForUnicodeChar(c, s)))
						continue; // Can't load replacement. Skip it.
				}

				textRuns.emplace_back(lastFont, std::vector<FriBidiChar>{});
			}
			textRuns.back().second.push_back(c);
		}

		// Step 4. Shape the text with harfbuzz.
		std::vector<GlyphString::Glyph> ret;
		ret.reserve(visualText.size());
		std::shared_ptr<hb_buffer_t> hbBuffer(hb_buffer_create(), hb_buffer_destroy);
		for(auto &run: textRuns)
		{
			hb_buffer_clear_contents(hbBuffer.get());
			hb_buffer_add_utf32(hbBuffer.get(), run.second.data(), run.second.size(), 0, -1);
			hb_buffer_guess_segment_properties(hbBuffer.get());
			// TODO: Top to bottom text?
			hb_buffer_set_direction(hbBuffer.get(), FRIBIDI_IS_RTL(baseDir) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
			//hb_buffer_set_script(hbBuffer.get(), HB_SCRIPT_ARABIC);

			hb_shape(run.first->hbFont.get(), hbBuffer.get(), NULL, 0);

			// Iterate through the glyphs and save them.
			unsigned int count = 0;
			hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(hbBuffer.get(), &count);
			hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(hbBuffer.get(), &count);

			for (unsigned int i = 0; i < count; ++i)
			{
				GlyphString::Glyph glyph{};
				glyph.glyphId = glyphInfo[i].codepoint;
				glyph.font = run.first;
				glyph.rightToLeft = FRIBIDI_IS_RTL(baseDir);
				glyph.advanceX = glyphPos[i].x_advance; // run.first->scale;
				glyph.advanceY = glyphPos[i].y_advance; // run.first->scale;
				glyph.offsetX = glyphPos[i].x_offset; // run.first->scale;
				glyph.offsetY = glyphPos[i].y_offset; // run.first->scale;
				ret.emplace_back(glyph);
			}
		}
		return ret;
	}
}

GlyphString::GlyphString(const std::string& text, FontSize s):
	glyphs(ProcessUtf8Text(text, s))
{

}



const std::vector<GlyphString::Glyph>& GlyphString::Glyphs() const
{
	return glyphs;
}



int GlyphString::RenderedWidth() const
{
	int ret = 0;
	for(const Glyph &g: glyphs)
	{
		ret += g.advanceX;
	}
	return ret / 64;
}
