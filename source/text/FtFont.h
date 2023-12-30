/* FtFont.h
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

#ifndef ES_FT_FONT_H_
#define ES_FT_FONT_H_

#include "Font.h"

#include <string>


// Represets a freetype font at a specific size
class FtFont: public Font
{
public:
	FtFont(const std::string& path, int size);
	virtual ~FtFont();

	virtual void DrawAliased(const std::string &str, double x, double y, const Color &color) const override;


protected:
	virtual int WidthRawString(const std::string& str, char after = ' ') const noexcept override;


private:
	const int fontSize;
};


#endif