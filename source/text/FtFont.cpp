/* FtFont.cpp
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

#include "FtFont.h"
#include <stdexcept>
#include <string>

#include "../Color.h"
#include "../FillShader.h"
#include "GlyphCache.h"
#include "GlyphString.h"
#include "../Point.h"


namespace {
	const int KERN = 2;
}

// Load a given ttf file using freetype.
FtFont::FtFont(const std::string& path, int size):
	fontSize(size)
{
	SetMetrics(size + KERN, WidthRawString(" "));
}



// Compute the rendered width of the text.
int FtFont::WidthRawString(const std::string& str, char after) const noexcept
{
	return GlyphString(str, fontSize == 14 ? FontSize::NORMAL : FontSize::LARGE).RenderedWidth();
}



// Draw the text.
void FtFont::DrawAliased(const std::string &str, double x, double y, const Color &color) const
{
	GlyphString gs(str, fontSize == 14 ? FontSize::NORMAL : FontSize::LARGE);
	Point pos(x + KERN, y + (fontSize - KERN));
	pos *= 64; // Convert to font units.
	for(auto &g: gs.Glyphs())
	{
		Point offset(g.offsetX, g.offsetY);
		g.font->cache->Draw(g, (pos + offset) / 64, color);
		if(DrawUnderlines() && g.underlined)
		{
			Point underlinePos(pos + Point(g.advanceX, g.advanceY) / 2);
			underlinePos /= 64;
			underlinePos.Y() += KERN;
			FillShader::Fill(underlinePos, Point(g.advanceX/64, 1), color);
		}
		pos += Point(g.advanceX, g.advanceY);
	}
}