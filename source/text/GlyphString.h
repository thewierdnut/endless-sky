/* GlyphString.h
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

#ifndef GLYPH_STRING_H_INCLUDED
#define GLYPH_STRING_H_INCLUDED

#include <string>
#include <vector>

#include "FontSize.h"
#include "FontSet.h"


class GlyphCache;


// This class represents a string that has been shaped and formatted, and is
// ready to be rendered.
class GlyphString
{
public:
	struct Glyph
	{
		const FontSet::FontInfo* font;
		int glyphId;
		bool rightToLeft;
		bool underlined;

		int offsetX;
		int offsetY;
		int advanceX;
		int advanceY;
	};

	GlyphString(const std::string& text, FontSize s);

	const std::vector<Glyph>& Glyphs() const;
	int RenderedWidth() const;

private:
	const std::vector<Glyph> glyphs;
};



#endif
