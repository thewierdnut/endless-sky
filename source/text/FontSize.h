/* FontSize.h
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

#ifndef FONT_SIZE_H_INCLUDED
#define FONT_SIZE_H_INCLUDED

#include <cstddef>


// Track font sizes without using the old numbers, which don't really match any
// useful metric.
enum class FontSize: int
{
	NORMAL,        // Old 14 font size, 32 unscaled pixel size.
	LARGE,         // Old 18 font size, 36 unscaled pixel size.
	FONT_SIZE_MAX
};


// The base font sizes used by default. This will be scaled by the current
// zoom level.
constexpr int BASE_FONT_SIZES[] = {14, 18};
static_assert(sizeof(BASE_FONT_SIZES) / sizeof(*BASE_FONT_SIZES) == static_cast<int>(FontSize::FONT_SIZE_MAX),
	"Need to define a base font size for each FontSize enum");


// // We render everything using a scaled screen metric, which doesn't match
// // actual screen pixel sizes. This means that if we render and shape a 14 pixel
// // font, which is then scaled to 200%, it will look bad. This FONT_SCALE will
// // change the level of detail at which a font is rendered, but does not change
// // the screen size of the rendered text.
// constexpr float FONT_SCALE = 1.0;



#endif
