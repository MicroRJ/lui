/*
**
** Copyright(c) 2023 - Dayan Rodriguez - Dyr
**
** -+- lui -+-
**
*/

#ifndef _lui_
#define _lui_

// NOTE: If You Are Compiling From The Command Line Use '/LIBPATH:'
// To Tell The Compiler Where To Find FreeType!
#pragma comment(lib,"freetype")

#include <ft2build.h>
#include FT_FREETYPE_H

#define TUI_LINE_HEIGHT ((lui.font->line_height*1.2f))

#define ELUI_COLOR_BACKGROUND   lgi_RGBA_U(11,53,62,255)
#define ELUI_COLOR_FOREGROUND   lgi_RGBA_U(9,63,73,255)
#define ELUI_COLOR_PRIMARY      lgi_RGBA_U(111,139,149,255)
#define ELUI_COLOR_SECONDARY 	  lgi_RGBA_U(111,139,149,255)
#define ELUI_COLOR_DANGER 		  lgi_RGBA_U(180+45,60+45,63+45,255)
#define ELUI_COLOR_HAPPY 		  lgi_RGBA_U(79,191,138,255)
#define ELUI_COLOR_ACCENT 		  lgi_RGBA_U(134,151,38,255)

#define ELUI_DEFAULT_ROUNDINESS 2.5


#ifndef _MSC_VER
typedef signed long long int __int64;
typedef signed int 			  __int32;
#endif

/* TODO: */
#ifndef lui__debugassert
#define lui__debugassert lgi_ASSERT
# endif
#ifndef lui__reallocaligned
#define lui__reallocaligned(length,memory) _aligned_realloc(memory,length,16)
# endif
#ifndef lui__freealigned
#define lui__freealigned(memory) _aligned_free(memory);
# endif
#ifndef lui_memmove
#define lui_memmove memmove
# endif
#ifndef lui_memcopy
#define lui_memcopy memcpy
# endif
#ifndef lui_memset
#define lui_memset memset
# endif

// TODO: REMOVE BEGIN
#ifndef E_IS_ALPNUM
#define E_IS_ALPNUM(r) (isWithin(r,'a','z')||isWithin(r,'A','Z')||isWithin(r,'0','9')||(r)== '_')
# endif
#ifndef E_IS_BLANK
#define E_IS_BLANK(chr) ((chr ==  ' ')||(chr == '\t')||(chr == '\r')||(chr == '\n'))
# endif
#ifndef E_IS_WORD_DELI
#define E_IS_WORD_DELI(r) (!E_IS_ALPNUM(r))
# endif
# ifndef E_CURSOR_PHASE
# define E_CURSOR_PHASE(E,time_since_last_blink,blinks_per_second) (.5 + .5 * cos(lgi_PI * time_since_last_blink / blinks_per_second));
#include <math.h>
#  endif
#ifndef E_CURSOR_COLOR
#define E_CURSOR_COLOR(E) E_MK_COLOR_RGBA_UNORM(148,232,148,255)
# endif
#ifndef E_COLOR
#define E_COLOR lgi_Color
# endif
#ifndef E_MK_COLOR_RGBA_UNORM
#define E_MK_COLOR_RGBA_UNORM lgi_RGBA_U
# endif
// TODO: REMOVE END

#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4101)
#pragma warning(disable:4706) /* todo */
#pragma warning(disable:4267)
#pragma warning(disable:4305)
#pragma warning(disable:4018)
#pragma warning(disable:4201)
#pragma warning(disable:4324)
#pragma warning(disable:4244)
#pragma warning(disable:4189)

enum { lui_left, lui_right, lui_top, lui_bottom };

typedef struct lui_Box {
	float x0,y0,x1,y1;
} lui_Box;

//
// Rendering Functions, To Be Implemented By Backend!
//
void lui__drawText(lui_Box box, char const *string);
void lui__drawRoundBox(lui_Box box, lgi_Color color, float cornerRadius);
void lui__drawBox(lui_Box rect, lgi_Color color);


#define lui_text(xx) lui__drawText(*lui.box,xx)
#define lui_texf(ff,...) lui__drawText(*lui.box,elCS_tmpFormat(ff,__VA_ARGS__))

//
// Editor:
//

typedef enum {
	E_MOD_NONE,
	E_MOD_SHIFT_BIT = 1 << 0,
	E_MOD_CTRL_BIT = 1 << 1,
	E_MOD_ALT_BIT = 1 << 2,
	E_MOD_CUSTOM_0_BIT = 1 << 3,
	E_MOD_CUSTOM_1_BIT = 1 << 4,
	E_MOD_CUSTOM_2_BIT = 1 << 5,
} E_MOD;

typedef enum {
	E_kNONE = 0,
	E_kMOVE_LEFT,
	E_kMOVE_UP,
	E_kMOVE_RIGHT,
	E_kMOVE_DOWN,
	E_kMOVE_WORD_LEFT,
	E_kMOVE_WORD_RIGHT,
	E_kMOVE_LINE_LEFT,
	E_kMOVE_LINE_RIGHT,
	E_kMOVE_PAGE_START,
	E_kMOVE_PAGE_END,
	E_kDELETE_BACK,
	E_kDELETE_HERE,
	E_kCUT,
	E_kCOPY,
	E_kPASTE,
	E_kLINE,
	E_kCHAR,
} E_KID;

typedef struct {
	/* .mod: any applied modifiers
		.kid: the key id
		.cur: target cursor, (-1) for all
		.num: (optional) the number of times to do it, (1) default
		.xpt: (optional) x screen space coordinates (for things like click events)
		.ypt: (optional) y screen space coordinates (for things like click events)
		.chr: (optional) utf32 code */
	E_MOD mod;
	E_KID kid;
	int cur;
	int num,chr;
	float xpt,ypt;
	void *lpUser;
} lui_EditorEvent;

typedef struct lui_GlyphCol lui_GlyphCol;

typedef struct {
	// lui_GlyphCol *pallet;
	union {
		int external_index;
		int index;
	};
	int utf32;
	short x0,y0,x1,y1;
	short offset_x,offset_y;
	float walking_x;
} lui_FontGlyph;

typedef struct lui_Font {
	char const *fpath;

	lgi_Texture *texture;
	int needs_texture_reupload;

	struct {
		lgi_Image bitmap;
		int linebump;
		int offset_x;
		int offset_y;
	} packer;

#if !defined(LUI_GLYPH_TABLE_LENGTH)
	#define LUI_GLYPH_TABLE_LENGTH 0x100
#endif

	// TODO:
	lui_FontGlyph glyph_table[LUI_GLYPH_TABLE_LENGTH];

	float char_height;
	float line_height;

	float spaceWidth;

	float ascent;
	float descent;
	float lineGap;

	int is_sdf;
	int is_subpixel;

	/* for underline effect, offset is relative to the baseline,
		height is whatever the designer thought looked nice for the
		underline's thickness */
	float underline_baseline_offset;
	float underline_thickness;

	struct {
		FT_Face face;
	} freetype;

	// struct {
	// 	stbtt_fontinfo face;
	// } stb;
} lui_Font;

typedef struct {
	/* [[TODO]]: only the offset is needed */
	int offset,length;
} lui_TextLine;

typedef struct lui_Draw_Config {
	union {
		lui_Font *font;
		lui_Font *lpFont;
	};
	float             x,y;

	int tab_size; /* in spaces */

	lgi_Color color;

	lgi_Color *color_table;

	int              length;
	char const     * string;
	unsigned char  * colors;

	/* should you choose to use .array to draw several lines,
		.string, .colors and .length must be large enough to accomodate all
		lines, set colors or null to use .color instead */
	lui_TextLine *line_array;

	int line_count;

	float line_height;

	/* optional, not meant for subpixel fonts */
	float char_height;
} lui_Draw_Config;


typedef struct {
	void *key;
	lgi_Bool toggled;
} lui_State;

typedef struct {
	char name[MAX_PATH];
	char fileName[MAX_PATH];
   /* [[todo]]: this could be replaced with single ints, the length of the line
   	isn't necessary, at least not explicitly, it can be calculated by using the
   	offset of the next line instead  */
	lui_TextLine *lcache;
	union  {
		void *memory;
		char *string;
	};

	/* [[REMOVE]]: we may want to open a file multiple encodings and or formattings */
	unsigned char  *colors;
	__int32 modified;
	__int64 extent;
	__int64 length;
	__int32 isReadonly;
} lui_Buffer;

void lui_Buffer_setName(lui_Buffer *, char const *);
void lui_Buffer_setFileName(lui_Buffer *, char const *);
char *lui_Buffer_allocSize(lui_Buffer *, __int64 reserve, __int64 commit);
char *lui_Buffer_insertSize(lui_Buffer *, __int64 offset, __int64 length);
void lui_Buffer_initAlloc(lui_Buffer *, char const *, __int64 length);
lui_TextLine lui_Buffer_getLineAtIndex(lui_Buffer *buffer, int index);
int lui_Buffer_getLineLength(lui_Buffer *buffer, int yline);
int lui_Buffer_getLineOffset(lui_Buffer *buffer, int yline);


typedef struct {
	__int32 xchar,yline;
} lui_Cursor;

typedef struct {
	E_MOD mod;
	lui_Cursor *cursor;
	lui_Buffer buffer;
	lui_Font *font;
	float cursor_blink_speed_in_seconds;
	float cursor_blink_timer;
	__int32 lyview;
	__int32 isReadonly;
} lui_Editor;

char *egetptr(lui_Editor *, __int32);
__int32 enumcur(lui_Editor *);
__int32 lui_editor__addCursor(lui_Editor *, lui_Cursor cur);
void esetcur(lui_Editor *, __int32, lui_Cursor cur);
lui_Cursor egetcur(lui_Editor *, __int32);
__int32 egetcurx(lui_Editor *, __int32);
__int32 egetcury(lui_Editor *, __int32);
__int32 ecurloc(lui_Editor *, __int32);
char ecurchr(lui_Editor *, __int32, __int32 off);
char *ecurptr(lui_Editor *, __int32);
__int32 eputchar(lui_Editor *, __int32 index, __int32 chr);
void edelchar(lui_Editor *, __int32 index);
void eaddline(lui_Editor *, __int32 offset, __int32 number);
void eremline(lui_Editor *, __int32 offset, __int32 number);
void lui_editor__testKeys(lui_Editor *);
void Editor_keyOne(lui_Editor *, lui_EditorEvent key);
void Editor_keyAll(lui_Editor *, lui_EditorEvent key);



// TODO:!
#define lui_logError lgi_logError

lui_Font *lui_loadFont(char const *fileName, float height);
lui_FontGlyph *lui_getFontGlyphByUnicode(lui_Font *lpFont, int utf32);
void lgi_drawText( lui_Draw_Config *config );

#include <src/lui_glyphs.c>
#include <src/lui_buffer.c>
#include <src/lui_editor.c>
#include "lui.c"

#pragma warning(pop)

#endif