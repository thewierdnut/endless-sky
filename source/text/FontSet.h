/* FontSet.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ES_TEXT_FONT_SET_H_
#define ES_TEXT_FONT_SET_H_

#include "FontSize.h"

#include <memory>
#include <string>
#include <vector>

class Font;
class DataNode;
class GlyphCache;


// Class for getting the Font object for a given point size. Each font must be
// based on a glyph image; right now only point sizes 14 and 18 exist.
class FontSet {
public:
	struct FontInfo
	{
		int priority;
		FontSize size;
		int pixelSize;
		float scale;
		std::vector<std::pair<uint32_t, uint32_t>> codepointRanges;
		std::shared_ptr<struct FT_FaceRec_> face;
		std::shared_ptr<struct hb_font_t> hbFont;
		std::unique_ptr<GlyphCache> cache;

		FontInfo(FontInfo&&) = default;
		FontInfo& operator=(FontInfo&&) = default;
		~FontInfo();
		bool HasUnicodeChar(uint32_t c) const;
	};

	static void Add(const std::string &path, int size);
	static const Font &Get(int size);

	static void Load(const DataNode &node);

	static const FontInfo* FontForUnicodeChar(uint32_t c, FontSize size);

	// Resize all the fonts to match the current zoom level.
	static void ResizeFonts(double zoom);
};



#endif
