/* GlyphCache.h
Copyright (c) 2023 thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ES_GLYPH_CACHE_H_
#define ES_GLYPH_CACHE_H_

#include "FontSet.h"
#include "GlyphString.h"

#include <unordered_map>

class Point;
class Color;


// Cache glyphs in texture memory for each font.
class GlyphCache final
{
public:
	GlyphCache(struct FT_FaceRec_ *font);

	static void Init();

	void Draw(const GlyphString::Glyph &g, const Point &pos, const Color &color) const;

private:
	struct GlyphInfo
	{
		size_t tex_id;
		//float left;
		//float top;
		int left;
		int top;
		float src_x;
		float src_y;
		float src_w;
		float src_h;
	};
	const GlyphInfo &GetGlyphInfo(uint32_t glyphId) const;


	// These variables are marked as mutable because the glyphs are rendered
	// as-needed in some of the methods that require a const signature.
	mutable std::vector<std::pair<bool, int>> textures;
	mutable std::vector<std::vector<uint8_t>> bitmaps;
	mutable unsigned int nextBitmapX = 1;
	mutable unsigned int nextBitmapY = 1;
	mutable std::unordered_map<uint32_t, GlyphInfo> cache;

	mutable int screenWidth = 0;
	mutable int screenHeight = 0;

	mutable struct FT_FaceRec_ *font;
};

#endif
