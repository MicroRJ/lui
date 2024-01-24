


float lui_getTextWidth(lui_Font *font, char const *lpStr, int length) {
	float result = 0.;
	float spaceW = font->spaceWidth;
	for (int i=0; i<length; i+=1) {
		int chr = lpStr[i];
		if E_IS_BLANK(chr) {
			if (chr == '\t') {
				result += spaceW * 3;
			} else {
				result += spaceW;
			}
		} else {
			lui_FontGlyph *lpGlyph = lui_getFontGlyphByUnicode(font,chr);
			result += lpGlyph->walking_x;
		}
	}
	return result;
}

lui_FontGlyph *lui_getFontGlyphByIndex(lui_Font *font, int glyph_index, int utf32);

lui_FontGlyph *lui_getFontGlyphByUnicode(lui_Font *font, int utf32) {
	if (utf32 >= 0 && utf32 < LUI_GLYPH_TABLE_LENGTH) {
		if (font->glyph_table[utf32].utf32 == utf32) {
			return font->glyph_table + utf32;
		}
	}

	int index = FT_Get_Char_Index(font->freetype.face,utf32);
	if (index == 0) {
		lui_logError("'%i': <%c> glyph not found!",utf32,utf32);
	}
	return lui_getFontGlyphByIndex(font,index,utf32);
}


void lui_genFontTextures(lui_Font *font) {
	if (font->needs_texture_reupload != FALSE) {
		if (font->texture != NULL) {
			lgi_updateTexture(font->texture,font->packer.bitmap);
		} else {
			font->texture = lgi_uploadImage(font->packer.bitmap);
		}
		font->needs_texture_reupload = FALSE;
	}
}

void lui_packFontGlyph(lui_Font *font, int itemwidth, int itemheight, int itemstride, unsigned char *itemmemory, int *outx, int *outy) {
	//
	// NOTE: Do Very Advanced Packing Algorithm!
	//


	int xcapacity = font->packer.bitmap.size_x;
	int ycapacity = font->packer.bitmap.size_y;

	if (font->packer.offset_x + itemwidth > xcapacity) {
		font->packer.offset_x = 0;
		font->packer.offset_y += font->packer.linebump;
	}

	if (itemheight > font->packer.linebump) {
		font->packer.linebump = itemheight;
	}

	// TODO: If ran out of space allocate new texture!
	if (font->packer.offset_y + itemheight > ycapacity) {
		lui_logError("Out Of Memory!");
	}

	int packx = font->packer.offset_x;
	int packy = font->packer.offset_y;
	*outx = packx;
	*outy = packy;

	font->packer.offset_x += itemwidth;

	int stride = font->packer.bitmap.stride;
	unsigned char *memory = font->packer.bitmap.memory;

	for (int y = 0; y < itemheight; y += 1) {
		lui_memcopy(memory + stride * (y + packy) + packx, itemmemory + itemstride * y, itemstride);
	}
	font->needs_texture_reupload = TRUE;
}

lui_FontGlyph *lui_getFontGlyphByIndex(lui_Font *font, int glyph_index, int utf32) {
	// lui_logError("Get Font Glyph By Index: %i",glyph_index);
	//
	// NOTE: Always allocate the glyph, to avoid calling this function again!
	//

	lui_FontGlyph *glyph = font->glyph_table + utf32;
	glyph->index = glyph_index;
	glyph->utf32 = utf32;

	if (glyph_index == 0) {
		goto L_leave;
	}


	FT_Face ftface = font->freetype.face;

	if (FT_Load_Glyph(ftface,glyph_index,FT_LOAD_DEFAULT)) {
		goto L_leave;
	}

	if (FT_Render_Glyph(ftface->glyph,FT_RENDER_MODE_LCD)) {
		goto L_leave;
	}

	FT_GlyphSlot ftslot = ftface->glyph;

	int glyph_width = ftslot->bitmap.width;
	int glyph_height = ftslot->bitmap.rows;
	int glyph_stride = ftslot->bitmap.pitch;
	unsigned char *glyph_memory = ftslot->bitmap.buffer;

	if (glyph_memory != NULL) {
		int x0,y0;
		lui_packFontGlyph(font,glyph_width,glyph_height,glyph_stride,glyph_memory,&x0,&y0);
		glyph->x0 = x0;
		glyph->y0 = y0;
		glyph->x1 = x0 + glyph_width;
		glyph->y1 = y0 + glyph_height;
		glyph->offset_x = ftslot->bitmap_left;
		glyph->offset_y = ftslot->bitmap_top - glyph_height;
		glyph->walking_x = ftslot->advance.x / 64;
	}

	L_leave:
	return glyph;
}


lui_Font *lui_loadFont(char const *fileName, float height) {
	FT_Library ftlib;
	if (FT_Init_FreeType(&ftlib)) {
		lui_logError("Failed To Initialize FreeType");
		return lgi_Null;
	}
	FT_Face ftface;
	if (FT_New_Face(ftlib,fileName,0,&ftface)) {
		lui_logError("Failed To Load File");
		return lgi_Null;
	}
	if (FT_Set_Pixel_Sizes(ftface,0,height)) {
		lui_logError("Invalid Font Height");
		return lgi_Null;
	}

	/* 16.16 to 24.6 to ..32.. */
	float units_to_pixels = ftface->size->metrics.y_scale / 65536. / 64.;
	float lineGap = ftface->height - (ftface->ascender - ftface->descender);

	lui_Font *font = lgi__allocate_typeof(lui_Font);
	lgi__clear_typeof(font);

	font->is_subpixel = TRUE;
	font->is_sdf = FALSE;
	font->fpath = fileName;
	font->ascent = ftface->ascender * units_to_pixels;
	font->descent = ftface->descender * units_to_pixels;
	font->lineGap = lineGap * units_to_pixels;
	font->line_height = height;
	font->char_height = height;
	font->freetype.face = ftface;

	font->packer.bitmap = lgi_makeImage(1024,1024,lgi_Format_R8_UNORM);

	lui_FontGlyph *underscoreGlyph = lui_getFontGlyphByUnicode(font,'_');
	lui_FontGlyph *spaceGlyph = lui_getFontGlyphByUnicode(font,' ');
	if (spaceGlyph == NULL) {
		spaceGlyph = underscoreGlyph;
	}
	font->spaceWidth = spaceGlyph->walking_x;

	lui_getFontGlyphByUnicode(font,'A');


	return font;
}

float lgi_getFontKerning(lui_Font *lpFont, int prev, int code) {
	float result = .0;
	if (FT_HAS_KERNING(lpFont->freetype.face)) {
		// TODO: Instead Use The Indexes We've Stored Already!
		FT_Vector kerning2;
		FT_Get_Kerning(lpFont->freetype.face
		, FT_Get_Char_Index(lpFont->freetype.face,prev)
		, FT_Get_Char_Index(lpFont->freetype.face,code),FT_KERNING_DEFAULT,&kerning2);
		result += kerning2.x / 64.;
	}

	return result;
}


// TODO:
// I actually had no idea text shaping was so freaking involved,
// I'll have to take some time aside to do this manually!
// I just wanted the simple programming ligatures, maybe someone
// out there has a way to do this!
int lui_nextFontLigature(char const *s, int *u) {
	*u = *s;
	return 1;
}

/* I don't want to do things this way, we should have a cache buffer, which holds a maximum
	number of glyphs, when rendering, we add the glyphs to this buffer, and we store the
	draw quad, when we run out of space we flush the buffer */
void lgi_drawText( lui_Draw_Config *lpConfig ) {


	lui_Font *lpFont = lpConfig->lpFont;
	// TODO: 1 Frame Latency!
	lui_genFontTextures(lpFont);


	if (lpConfig->char_height == 0.) {
		lpConfig->char_height = lpFont->char_height;
	}
	if (lpConfig->line_height == 0.) {
		lpConfig->line_height = lpFont->line_height;
	}

	float scale = lpConfig->char_height / lpFont->char_height;

	float line_height = lpConfig->line_height;

	float unwiden = 1. / (lpFont->is_subpixel ? 3. : 1.);

	unsigned char *colors = lpConfig->colors;

	char const *string = lpConfig->string;

	lgi_Color color = lpConfig->color;

	lgi_Color *color_table = lpConfig->color_table;


	lui_TextLine single_line;
	single_line.offset = 0;
	single_line.length = lpConfig->length;
	if(lpConfig->line_array == NULL) {
		lpConfig->line_array = & single_line;
		lpConfig->line_count = 1;
	}

	//
	// TODO: Can We Just Attach The Program To The Shader?
	//
	if (lpFont->is_subpixel) {
		lgi_bindProgram(lgi.lcdTextProgram);
	} else
	if (lpFont->is_sdf) {
		lgi_bindProgram(lgi.sdfTextProgram);
	}

	lui_TextLine *line_array = lpConfig->line_array;
	int line_count = lpConfig->line_count;

	float y = lpConfig->y;

	lui_TextLine *it;
	for (it = line_array; it < line_array + line_count; it += 1) {
		__int64 offset = it->offset;
		__int64 length = it->length;
		if (length < 0) {
			break;
		}
		if (offset < 0) {
			break;
		}
		if (offset + length > lpConfig->length) {
			break;
		}
		float x = lpConfig->x;

		for(int iii = 0; iii < length;) {
			/* todo */

			/* todo: this won't work properly with colors */
			int xchar = iii;

			int utf32;
			iii += lui_nextFontLigature(string+(offset+iii),&utf32);
			lui_FontGlyph *glyph = lui_getFontGlyphByUnicode(lpFont,utf32);
			if (glyph == NULL) {
				goto L_skip_rendering;
			}
			if E_IS_BLANK(utf32) {
				goto L_skip_rendering;
			}
			if (glyph->walking_x == 0.) {
				goto L_skip_rendering;
			}

#if 0
			lui_GlyphCol *pallet = glyph->pallet;
			if (pallet == NULL) {
				continue;
			}

			/* you should do this whole thing first, checking for all the glyphs that need
			  to be added and then finally update the texture #todo #urgent */
			if (pallet->dirty) {
				pallet->dirty = FALSE;
				if(pallet->texture == NULL) {
					pallet->texture = lgi_uploadImage(pallet->storage);
				} else {
					lgi_updateTexture(pallet->texture,pallet->storage);
				}
			}
			if (pallet->texture == NULL) {
				continue;
			}
#endif

			lgi_Texture *texture = lpFont->texture;
			lgi_bindTexture(0,texture,FALSE);

			if (color_table != NULL) {
				color = color_table[colors[it->offset + xchar]];
			}

			float x0 = x + glyph->offset_x * unwiden;
			float y0 = y + glyph->offset_y;
			x0 *= scale;
			y0 *= scale;
			float x1 = x0 + (glyph->x1 - glyph->x0) * scale * unwiden;
			float y1 = y0 + (glyph->y1 - glyph->y0) * scale;

			float xnor = 1. / texture->size_x;
			float ynor = 1. / texture->size_y;

			lgi_beginVertexArray(6,4);
			lgi.State.attr.rgba = color;
			lgi_addIndicesV(6, 0,1,2, 0,2,3);
			lgi_addVerticesV(4,
			rxvtx_xyuv(x0,y0, glyph->x0 * xnor, glyph->y1 * ynor),
			rxvtx_xyuv(x0,y1, glyph->x0 * xnor, glyph->y0 * ynor),
			rxvtx_xyuv(x1,y1, glyph->x1 * xnor, glyph->y0 * ynor),
			rxvtx_xyuv(x1,y0, glyph->x1 * xnor, glyph->y1 * ynor));
			lgi_endVertexArray();

			L_skip_rendering:

			if (glyph == NULL || glyph->walking_x == 0.) {
				glyph = lui_getFontGlyphByUnicode(lpFont,'_');
			}
			float walking_x = glyph->walking_x;

			if (utf32 == '\t') {
				walking_x *= 3;
			}

			x += walking_x * scale;
		}

		y -= line_height;
	}
}
