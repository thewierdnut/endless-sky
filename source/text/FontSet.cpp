/* FontSet.cpp
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

#include "FontSet.h"

#include "Font.h"
#include "FontSize.h"
#include "FtFont.h"
#include "GlyphCache.h"

#include "../DataNode.h"
#include "../Files.h"

#include <SDL_log.h>
#include <harfbuzz/hb-ft.h>
#include <freetype/freetype.h>

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>
#include <mutex>

using namespace std;

namespace {
	map<int, std::unique_ptr<Font>> fonts;

	// I don't expect this to ever contain more than two fonts at each size, so
	// this is not worth indexing.
	std::vector<FontSet::FontInfo> ftFonts[static_cast<int>(FontSize::FONT_SIZE_MAX)];
	std::vector<std::pair<std::string, std::string>> fontData;
	std::shared_ptr<struct FT_LibraryRec_> freeType;
	std::mutex fontMutex; // Protect font creation and destruction.

	std::string LoadFont(const std::string& path, int priority, FontSize size, int pixelSize, std::vector<std::pair<uint32_t, uint32_t>> ranges)
	{
		std::lock_guard<std::mutex> lock(fontMutex);
		if(!freeType)
		{
			FT_Library lib;
			FT_Error err = FT_Init_FreeType(&lib);
			if(err)
				throw std::runtime_error("Freetype could not be initialized: " + std::to_string(err));
			freeType.reset(lib, FT_Done_FreeType);
		}

		auto itFontData = fontData.begin();
		for(; itFontData != fontData.end(); ++itFontData)
			if(itFontData->first == path)
				break;
		if(itFontData == fontData.end())
		{
			std::string rawData = Files::Read(Files::Images() + path);
			if(rawData.empty())
				return "Unable to load font " + path;

			fontData.emplace_back(path, std::move(rawData));
			itFontData = fontData.end() - 1;
		}

		// Load the freetype face. This does not need a smart pointer, as
		// the freeType global object will destroy all faces when
		// FT_Done_FreeType gets called.
		FT_Face face;
		FT_Error err = FT_New_Memory_Face(
			freeType.get(),
			reinterpret_cast<const FT_Byte*>(itFontData->second.data()),
			itFontData->second.size(),
			0,
			&face
		);
		if(err)
			return "Unable to load font (Freetype returned " + std::to_string(err) + ")";


		// Now that we have the face loaded, we can compute default unicode ranges
		// if they were left blank.
		if(ranges.empty())
		{
			// Iterate through the unicode charmap and construct ranges from the set
			// of characters.
			FT_Select_Charmap(face, FT_ENCODING_UNICODE);

			FT_UInt idx = 0;
			FT_ULong c = FT_Get_First_Char(face, &idx);
			std::vector<FT_ULong> availableCodes;
			while(idx != 0)
			{
				if(c)
					availableCodes.push_back(c);
				c = FT_Get_Next_Char(face, c, &idx);
			}
			std::sort(availableCodes.begin(), availableCodes.end());
			if(availableCodes.empty())
			{
				return "Cannot read available unicode code points from " + itFontData->first
					+ ". Please manually specify valid ranges using the 'range' keyword";
			}
			// Add an implied range of 0-32 for every font. Otherwise we add a
			// bunch of separate ranges for just tabs, newlines, and spaces.
			uint32_t left = 0;
			uint32_t right = 33;
			for(uint32_t c: availableCodes)
			{
				if(c > right)
				{
					ranges.emplace_back(left, right);
					left = c;
					right = c + 1;
				}
				else if(right == c)
					++right;
			}
			ranges.emplace_back(left, right);
		}
		else
		{
			// Sort and sanity-check the ranges
			std::sort(ranges.begin(), ranges.end());
			uint32_t right = 0;
			for(auto& r: ranges)
			{
				if(r.first < right)
					return "Overlapping ranges";
				right = r.second;
			}
		}

		FontSet::FontInfo info{};
		info.priority = priority;
		info.size = size;
		info.codepointRanges = ranges;
		info.pixelSize = pixelSize;
		info.scale = 1.0;

		FT_Set_Pixel_Sizes(face, 0, pixelSize);
		info.face = face;
		info.hbFont.reset(hb_ft_font_create(info.face, nullptr), hb_font_destroy);
		//hb_ft_font_set_funcs(info.hbFont.get());

		info.cache = std::make_unique<GlyphCache>(info.face);

		// keep the fonts sorted by priority
		auto &fontSet = ftFonts[static_cast<int>(size)];
		fontSet.emplace_back(std::move(info));
		std::sort(fontSet.begin(), fontSet.end(), [](const FontSet::FontInfo &a, const FontSet::FontInfo &b) {
			return b.priority < a.priority;
		});

		return "";
	}
}



// This is a stub that forces the smart pointers to destruct in a context that
// knows how to do it.
FontSet::FontInfo::~FontInfo()
{

}



void FontSet::Add(const string &path, int size)
{
	if(path.size() > 4 && path.substr(path.size() - 4) == ".ttf")
	{
		FontSize fs = size == 14 ? FontSize::NORMAL : FontSize::LARGE;
		std::string errStr = LoadFont(path, 0, fs, size, {});
		if(!errStr.empty())
			throw std::runtime_error(errStr);
		fonts[size] = std::make_unique<FtFont>(path, size);
	}
	else if(!fonts.count(size))
		fonts[size] = std::make_unique<Font>(path);
}



const Font &FontSet::Get(int size)
{
	return *fonts[size];
}



void FontSet::Load(const DataNode &node)
{

	std::string path;
	std::vector<std::pair<FontSize, int>> sizes;
	std::vector<std::pair<uint32_t, uint32_t>> ranges;
	int priority = 10;
	if(node.HasChildren())
	{
		for(const DataNode &child : node)
		{
			string key = child.Token(0);

			if(key == "path")
			{
				if(child.Size() != 2)
				{
					child.PrintTrace("Font path single argument expected:");
					continue;
				}
				if(!path.empty())
				{
					child.PrintTrace("Ignoring extra font path:");
					continue;
				}

				// Load the raw font data into memory if we haven't already.
				path = child.Token(1);
			}
			else if(key == "size")
			{
				if(child.Size() != 3)
				{
					child.PrintTrace("Expecting two arguments for size:");
					continue;
				}
				int fontRawSize = child.Value(2);
				if(child.Token(1) == "normal")
					sizes.emplace_back(FontSize::NORMAL, fontRawSize);
				else if(child.Token(1) == "large")
					sizes.emplace_back(FontSize::LARGE, fontRawSize);
				else
				{
					child.PrintTrace("Expecting 'size (normal|large) fontSize':");
					continue;
				}
			}
			else if(key == "range")
			{
				if(child.Size() != 3)
				{
					child.PrintTrace("Expecting two arguments for range:");
					continue;
				}
				ranges.emplace_back(child.Value(1), child.Value(2));
				if(ranges.back().first > ranges.back().second)
					std::swap(ranges.back().first, ranges.back().second);
			}
			else if(key == "priority")
			{
				if(child.Size() != 2)
				{
					child.PrintTrace("Font priority single argument expected:");
					continue;
				}
				priority = child.Value(1);
			}
		}
	}
	else if(node.Size() == 2)
	{
		path = node.Token(1);
	}

	if(path.empty())
	{
		node.PrintTrace("Missing font path:");
		return;
	}
	if(sizes.empty())
	{
		// Default to the sizes used by the original bitmap fonts.
		for(int i = 0; i < static_cast<int>(FontSize::FONT_SIZE_MAX); ++i)
			sizes.emplace_back(static_cast<FontSize>(i), BASE_FONT_SIZES[i]);
	}

	for(auto& s: sizes)
	{
		std::string errStr = LoadFont(path, priority, s.first, s.second, ranges);
		if(!errStr.empty())
			node.PrintTrace(errStr + ":");
	}
}



bool FontSet::FontInfo::HasUnicodeChar(uint32_t c) const
{
	auto itLeft = codepointRanges.begin();
	auto itRight = codepointRanges.end();
	while(itLeft < itRight)
	{
		auto itMid = itLeft + (itRight - itLeft) / 2;
		if(itMid->second <= c)
			itLeft = itMid + 1;
		else if(itMid->first > c)
			itRight = itMid;
		else
			return true;
	}
	return false;
}



const FontSet::FontInfo* FontSet::FontForUnicodeChar(uint32_t c, FontSize size)
{
	auto &fontSet = ftFonts[static_cast<int>(size)];

	for(const FontInfo &font: fontSet)
	{
		if(font.HasUnicodeChar(c))
			return &font;
	}
	return nullptr;
}



void FontSet::ResizeFonts(double zoom)
{
	// The reason that we manually resize all of the fonts to match the current
	// zoom level is to prevent aliasing that occurs when you draw the font at a
	// different size than it was shaped for.

	// TODO: This aliasing still occurs at 110% ~ 130%, but looks ok everywhere else

	std::lock_guard<std::mutex> lock(fontMutex);
	for(int i = 0; i < static_cast<int>(FontSize::FONT_SIZE_MAX); ++i)
	{
		for(auto &font: ftFonts[i])
		{
			// Round the font size to the nearest pixel.
			int pixelSize = font.pixelSize * zoom + .5;
			FT_Set_Pixel_Sizes(font.face, 0, pixelSize);
			hb_ft_font_changed(font.hbFont.get());
			font.cache = std::make_unique<GlyphCache>(font.face);
			// Set the scale to reflect the zoom level used.
			font.scale = static_cast<float>(pixelSize) / font.pixelSize;
		}
	}
}