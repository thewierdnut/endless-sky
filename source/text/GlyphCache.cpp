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

#include "GlyphCache.h"

#include "../Color.h"
#include "GlyphString.h"
#include "../Logger.h"
#include "../Point.h"
#include "../Screen.h"
#include "../Shader.h"


#include <freetype/freetype.h>


namespace
{
	constexpr int BITMAP_SIZE = 512;

	Shader shader;
	GLuint colorI = -1;
	GLuint scaleI = -1;
	GLuint glyphI = -1;
	GLuint positionI = -1;
	GLuint vao = -1;
	GLuint vbo = -1;
}



GlyphCache::GlyphCache(FT_Face font):
   font(font)
{
	bitmaps.resize(1);
	bitmaps.back().assign(BITMAP_SIZE * BITMAP_SIZE, 0);
	textures.emplace_back(false, -1);
}



void GlyphCache::Init()
{
	const char *vertexCode =
		"// vertex font shader\n"
		// "scale" maps pixel coordinates to GL coordinates (-1 to 1).
		"uniform vec2 scale;\n"
		// Target glyph rect in pixel coordinates
		"uniform vec4 position;\n"
		// Source glyph rect in texture coordinates
		"uniform vec4 glyph;\n"
		// Aspect ratio of rendered glyph (unity by default).
		//"uniform float aspect;\n"

		// Inputs from the VBO.
		"in vec2 vert;\n"

		// Output to the fragment shader.
		"out vec2 texCoord;\n"

		// Pick the proper glyph out of the texture.
		"void main() {\n"
		"  texCoord = vec2((1-vert.x)*glyph[0] + vert.x* glyph[2], (1-vert.y)*glyph[1] + vert.y * glyph[3]);\n"
		"  gl_Position = vec4((position.x + vert.x * position[2]) * scale.x, (position.y + vert.y * position[3]) * scale.y, 0.f, 1.f);\n"
		"}\n";

	const char *fragmentCode =
		"// fragment font shader\n"
		"precision mediump float;\n"
		// The user must supply a texture and a color (white by default).
		"uniform sampler2D tex;\n"
		"uniform vec4 color;\n"

		// This comes from the vertex shader.
		"in vec2 texCoord;\n"

		// Output color.
		"out vec4 finalColor;\n"

		// Multiply the texture by the user-specified color (including alpha).
		"void main() {\n"
		// "  float value = texture(tex, texCoord).a;\n"
		// "  float intensity = (color.r + color.g + color.b) / 3.0;\n"
		// "  finalColor =  vec4(color.rgb * value, min(value, intensity));\n"
		"  finalColor = texture(tex, texCoord).a * color;\n"
		"}\n";

	shader = Shader(vertexCode, fragmentCode);
	colorI = shader.Uniform("color");
	scaleI = shader.Uniform("scale");
	glyphI = shader.Uniform("glyph");
	positionI = shader.Uniform("position");

	// Create the VAO and VBO.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLfloat vertices[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	constexpr auto stride = 2 * sizeof(GLfloat);
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, stride, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}



void GlyphCache::Draw(const GlyphString::Glyph& g, const Point& pos, const Color& color) const
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	glUniform4fv(colorI, 1, color.Get());


	// Update the scale, only if the screen size has changed.
	if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		GLfloat scale[2] = {2.f / screenWidth, -2.f / screenHeight};
		glUniform2fv(scaleI, 1, scale);
	}

	//int previous = 0;
	//bool isAfterSpace = true;
	//bool underlineChar = false;
	//const int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));

	// TODO: underlines
	//char32_t prev_c = 0;

	auto& gi = GetGlyphInfo(g.glyphId);

	// textures are rendered bottom up, but we render top down, so y coordinates are switched
	glUniform4f(glyphI,
		gi.src_x, (gi.src_y + gi.src_h),
		(gi.src_x + gi.src_w), gi.src_y
	);

	// Check the texture validity. It may need to be reloaded from the cached bitmap.
	if(!textures[gi.tex_id].first)
	{
		if(textures[gi.tex_id].second == -1)
		{
			// Texture needs created.
			unsigned int texid;
			glGenTextures(1, &texid);
			glBindTexture(GL_TEXTURE_2D, texid);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			textures[gi.tex_id].second = texid;
		}
		else
		{
			// Texture has been created, but its contents are old.
			glBindTexture(GL_TEXTURE_2D, textures[gi.tex_id].second);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, BITMAP_SIZE, BITMAP_SIZE, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmaps[gi.tex_id].data());
		glBindTexture(GL_TEXTURE_2D, 0);
		textures[gi.tex_id].first = true;
	}

	glBindTexture(GL_TEXTURE_2D, textures[gi.tex_id].second);

	//if (prev_c && g.glyphId)
	//{
	//	FT_Vector delta;
	//	FT_Get_Kerning(m_face.get(), prev_c, cc.glyph_index, FT_KERNING_DEFAULT, &delta);
	//	penPos[0] += delta.x / 64.0; // may be negative;
	//}
	//prev_c = cc.glyph_index;

	glUniform4f(positionI, pos.X() + gi.left  / g.font->scale, pos.Y() - gi.top / g.font->scale, gi.src_w * (BITMAP_SIZE / g.font->scale), gi.src_h * (BITMAP_SIZE / g.font->scale));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//if(underlineChar)
	//{
	//	auto& ccu = m_char_cache.at('_');
	//	glUniform4f(m_glyph,
	//		ccu.src_x / tex_size, (ccu.src_y + ccu.src_h) / tex_size,
	//		(ccu.src_x + ccu.src_w) / tex_size, ccu.src_y / tex_size
	//	);
	//	glUniform1f(m_aspect, static_cast<float>(cc.advance_x) / ccu.advance_x);
	//	glUniform4f(m_position, penPos[0], penPos[1] - ccu.top, ccu.src_w, ccu.src_h);
	//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//	underlineChar = false;
	//
	//
	//
	////	glUniform1i(glyphI, underscoreGlyph);
	////	glUniform1f(aspectI, static_cast<float>(advance[glyph * GLYPHS] + KERN)
	////		/ (advance[underscoreGlyph * GLYPHS] + KERN));
	////
	////	glUniform2fv(positionI, 1, textPos);
	////
	////	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	////	underlineChar = false;
	//}

	//previous = glyph;

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}


const GlyphCache::GlyphInfo& GlyphCache::GetGlyphInfo(uint32_t glyphId) const
{
	auto it = cache.find(glyphId);
	if(it != cache.end())
		return it->second;

	// Since its not in the cache, load it up and render it.
	FT_Error err = FT_Load_Glyph(font, glyphId, FT_LOAD_DEFAULT);
	if (err)
	{
		// This shouldn't happen. We already swapped out invalid glyphs with
		// question marks in the shaping function.
		Logger::LogError("Failed to load glyph " + std::to_string(glyphId));
		FT_Load_Glyph(font, 0, FT_LOAD_DEFAULT); // Load the "error" glyph.
	}

	err = FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL);
	if (err)
	{
		// Not sure what to do at this point. This shouldn't ever happen.
		Logger::LogError("Failed to render glyph " + std::to_string(glyphId));
	}

	auto& bitmap = font->glyph->bitmap;

	// does our cached bitmap have enough room for another glyph?
	if (nextBitmapX + bitmap.width + 1 > BITMAP_SIZE)
	{
		const int max_height = font->size->metrics.ascender - font->size->metrics.descender;
		nextBitmapX = 1;  				 // need a pixel of padding between each glyph
		nextBitmapY += max_height + 1; // so that linear interpolation works
		if (nextBitmapY + max_height + 1 > BITMAP_SIZE)
		{
			// Clear out room for a new texture. We will allocate it when we
			// know we have a valid gl context
			textures.emplace_back(false, -1);

			// Cache a new bitmap
			bitmaps.emplace_back(std::vector<uint8_t>(BITMAP_SIZE * BITMAP_SIZE, 0));
			nextBitmapY = 1;
		}
	}

	// copy the glyph into the buffer
	for (size_t y = 0; y < bitmap.rows; ++y)
	{
		for (size_t x = 0; x < bitmap.width; ++x)
		{
			bitmaps.back()[(nextBitmapY + y) * BITMAP_SIZE + (nextBitmapX + x)] =
				bitmap.buffer[(bitmap.rows - 1 - y) * bitmap.pitch + x];
		}
	}

	textures.back().first = false; // we need to reload the texture next time it gets used
	auto result = cache.emplace(glyphId, GlyphInfo{
		bitmaps.size() - 1,
		font->glyph->bitmap_left,                                // Offset from draw position this glyph is
		font->glyph->bitmap_top,                                 // supposed to be drawn at, in pixel coords.
		nextBitmapX / static_cast<float>(BITMAP_SIZE),           // Position within the cached texture, in
		nextBitmapY / static_cast<float>(BITMAP_SIZE),           // texture coordinates.
		bitmap.width / static_cast<float>(BITMAP_SIZE),          // Size of glyph in texture coordinates.
		bitmap.rows / static_cast<float>(BITMAP_SIZE)
	});
	nextBitmapX += bitmap.width + 1;
	return result.first->second;
}
