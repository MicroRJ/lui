/*
**
** Copyright(c) 2023 - Dayan Rodriguez - Dyr
**
** -+- elang -+-
**
*/
// little user interface (lui)

// NOTE: This Structure Contains the Current lui State!
struct {
	// The Current Font Enabled!
	lui_Font  *font;
	// All the Loaded Fonts!
	lui_Font **fontLib;

	float textHeight;
	lgi_Color textColor;

	int 		boxCount;
	lui_Box  boxStack[0x20];
	lui_Box *box;
} lgi_Global lui;


void lui__check_overflow() {
	lui.boxCount += 1;
	lgi_ASSERT(lui.box<lui.boxStack+(sizeof(lui.boxStack)/sizeof(lui.boxStack[0])));
}
void lui__check_underflow() {
	lui.boxCount -= 1;
	lgi_ASSERT(lui.box>lui.boxStack);
}

void lui__drawText(lui_Box box, char const *string);
void lui__drawRoundBox(lui_Box box, lgi_Color color, float cornerRadius);

void lui_setTextColor(lgi_Color color) {
	lui.textColor = color;
}

void lui_setTextColorBlend(lgi_Color color, float blend) {
	lui_setTextColor(vec4_mix(blend,lui.textColor,color));
}

#define lui_inibox() (lui.box = lui.boxStack)
#define lui_dupbox() (lui__check_overflow(), lui.box[1] = lui.box[0], lui.box += 1)
#define lui_popbox() (lui__check_underflow(), *(lui.box -= 1))
#define lui_getbox() (*lui.box)
#define lui_setbox(xx) (lui_getbox() = (xx))
/* this has to be a function, operand has to be evaluated first and only once */
void lui_putbox(lui_Box xx) {
	lui_dupbox();
	lui_setbox(xx);
}
/* TODO: this should clip too */
#define lui_cutbox(side,size) lui_putbox(lui_boxcut(lui.box,XFUSE(lui_,side),size))

// WHEN COME BACK FIX THIS
#define lui_hasline() ((lui_getbox().y1-lui_getbox().y0)>=TUI_LINE_HEIGHT)
#define lui_newline() lui_cutbox(top,TUI_LINE_HEIGHT)
#define lui_endline() lui_popbox()

#define lui_newcol(side,size) lui_cutbox(side,size)
#define lui_endcol() lui_popbox()


int lui_testinbox(lui_Box box, float x, float y) {
	return ((x >= box.x0) && (y >= box.y0)) && ((x <  box.x1) && (y <  box.y1));
}
int lui_clickbox(lui_Box box) {
	return lgi_isButtonPressed(0) && lui_testinbox(box,lgi.Input.Mice.xcursor,lgi.Input.Mice.ycursor);
}
int lui_unclickbox(lui_Box box) {
	return lgi_isButtonReleased(0) && lui_testinbox(box,lgi.Input.Mice.xcursor,lgi.Input.Mice.ycursor);
}

#define lui_text(xx) lui__drawText(*lui.box,xx)
#define lui_texf(ff,...) lui__drawText(*lui.box,elCS_tmpFormat(ff,__VA_ARGS__))


lui_Box lui_dobox(float x0, float y0, float x1, float y1) {
	lui_Box rect;
	rect.x0 = x0; rect.x1 = x1;
	rect.y0 = y0; rect.y1 = y1;
	return rect;
}

lui_Box lui_boxcut(lui_Box *box, int mode, float size) {
	lui_Box result = lui_dobox(0,0,0,0);
	switch(mode) {
		case lui_top: {
			result = lui_dobox(box->x0,box->y1-size,box->x1,box->y1);
			box->y1 -= size;
		} break;
		case lui_bottom: {
			result = lui_dobox(box->x0,box->y0,box->x1,box->y0+size);
			box->y0 += size;
		} break;
		case lui_right: {
			result = lui_dobox(box->x1-size,box->y0,box->x1,box->y1);
			box->x1 -= size;
		} break;
		case lui_left: {
			if (size == -1) {
				size = box->x1 - box->x0;
			}
			result = lui_dobox(box->x0,box->y0,box->x0+size,box->y1);
			box->x0 += size;
		} break;
	}
	return result;
}

#if 1
lui_Box rect_center(lui_Box parent, lui_Box child) {
	lui_Box result;
	result.x0 = parent.x0 + ((parent.x1 - parent.x0) * .5) - ((child.x1 - child.x0) * .5);
	result.y0 = parent.y0 + ((parent.y1 - parent.y0) * .5) - ((child.y1 - child.y0) * .5);
	result.x1 = result.x0 + (child.x1 - child.x0);
	result.y1 = result.y0 + (child.y1 - child.y0);
	return result;
}

lui_Box rect_padd(lui_Box rect, float xpadd, float ypadd) {
	return lui_dobox(rect.x0 + (xpadd * .5)
	, rect.y0 + (ypadd * .5)
	, rect.x1 - (xpadd * .5)
	, rect.y1 - (ypadd * .5));
}


#define euyield() Or(lui_getbox().y1<=lui_getbox().y0, lui_getbox().x1<=lui_getbox().x0)
#endif

lui_State *lui_query_state(void *key, lgi_Bool create) {
	if isNull(key) {
		return lgi_Null;
	}
	/* todo: */
	lgi_Global lui_State w[0x1000];

	lgi_longInt n = sizeof(w) / sizeof(w[0]);
	lgi_longInt h = (lgi_longInt)(key) % n;

	lui_State *lastState = w + h;

	while isNotNullAnd(lastState->key,lastState->key!=key) {
		lastState += 1;
	}

	lui_State *result = lgi_Null;

	if isTrue(lastState->key == key) {
		result = lastState;
	} else
	if isTrue(create) {
		if isNull(lastState->key) {
			result = lastState;
			result->key = key;
		} else {
			lgi_ASSERT(!"Error");
		}
	}

	return result;
}


void lui__drawText(lui_Box box, char const *string) {

	int length = 0;
	while isNeitherOf3(string[length],'\0','\r','\n') {
		length += 1;
	}

	lui_Draw_Config config;
	ZeroMemory(&config,sizeof(config));
	config.font = lui.font;
	config.x = box.x0;
	config.y = box.y1 - lui.font->line_height * 1.2f;
	config.char_height = lui.font->char_height;
	config.line_height = lui.font->line_height;
	config.tab_size = 2; /* in spaces */
	config.color = lui.textColor;
	config.color_table = NULL;
	config.length = length;
	config.string = string;
	config.colors = NULL;

	lgi_drawText( &config );
}
