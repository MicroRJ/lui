

void Editor_keyAll(lui_Editor *lp, lui_EditorEvent key) {
	for (key.cur=enumcur(lp)-1; key.cur>=0; key.cur-=1) {
		Editor_keyOne(lp,key);
	}
}

void
Editor_handleKey(lui_Editor *lp, E_KID kid, int num, int chr) {

	// printf("handle_key kid := %s num := %i chr := %i\n"
	// , E_KID_toStringMap[kid],num,chr);

	lui_EditorEvent key;
	key.mod = lp->mod;
	key.kid = kid;
	key.num = num;
	key.chr = chr;
	key.cur =  -1;
	Editor_keyAll(lp,key);
}

int
enumcur(lui_Editor *editor) {
	return arrlen(editor->cursor);
}

void
lui__drawBox(lui_Box rect, lgi_Color color) {
	lgi_drawQuad(color,rect.x0,rect.y0,(rect.x1-rect.x0),(rect.y1-rect.y0));
}

void
lui__drawRoundBox(lui_Box box, lgi_Color color, float cornerRadius) {
	lgi_drawBoxSDF(
	/* */vec2_xy(
	/* */box.x0 + (box.x1 - box.x0) * .5f,
	/* */box.y0 + (box.y1 - box.y0) * .5f),
	/* */vec2_xy(
	/* */.5f*(box.x1-box.x0),
	/* */.5f*(box.y1-box.y0)), color, cornerRadius, 1.);
}

int
lui__draw_editor(lui_Editor *lpEditor, lui_Box bx) {

	lui_Font *lpFont = lpEditor->font;

	float blink_duration = lpEditor->cursor_blink_speed_in_seconds;
	if (blink_duration == 0) {
		blink_duration = .250;
	}
	float blink_timer = lpEditor->cursor_blink_timer;
	blink_timer += rx.Time.delta_seconds;
	double animation = E_CURSOR_PHASE(lpEditor,blink_timer,blink_duration);

	lpEditor->cursor_blink_timer = blink_timer;

	/* todo */
	lgi_Color curcolor = E_CURSOR_COLOR(lpEditor);
	curcolor.a = clamp(animation,.1,.8);
	float lineHeight = lpFont->line_height;
	/* [[TODO]]: rename to "firstline" */
	__int32 yview = lpEditor->lyview;
	/* [[TODO]]: to become a pointer */
	lui_Buffer *lpBuffer = &lpEditor->buffer;
	lui_TextLine *lpLines = lpBuffer->lcache;
	char const *lpBufStr = lpBuffer->string;
	float spaceW = lpFont->spaceWidth;

	/* [[TODO]]: render only visible cursors */
	/* [[TODO]]: adapt the size of the cursor the size of the character */
	for (int i=0;i<enumcur(lpEditor);i+=1) {
		lui_Cursor cur = egetcur(lpEditor,i);
		lui_TextLine line = lpLines[cur.yline];
		char const *lineStart = lpBufStr + line.offset;
		float cury = bx.y1 - ((cur.yline - yview + 1) * lineHeight + lpFont->char_height * .2);
		float curx = bx.x0 + lui_getTextWidth(lpFont,lineStart,cur.xchar);
		float curw = spaceW;
		float curh = lpFont->char_height * 1.2;
		vec2 currad = vec2_xy(curw*.5,curh*.5);
		vec2 curcen = vec2_xy(curx+currad.x,cury+currad.y);
		lgi_drawBoxSDF(curcen,currad,curcolor,2.5,1.);
	}

	lui_Buffer buffer = lpEditor->buffer;

	lui_Draw_Config config;
	ZeroMemory(&config,sizeof(config));
	config.font = lpFont;
	config.x = bx.x0;
	config.y = bx.y1 - lpFont->line_height;
	config.char_height = lpFont->char_height;
	config.line_height = lpFont->line_height;
   config.tab_size = 3; /* in spaces */
	config.color = lgi_RGBA(1,1,1,.8);
	config.color_table = 0;
	config.length = buffer.length;
	config.string = buffer.memory;
	config.colors = NULL;
	config.line_array = buffer.lcache + yview;
	config.line_count = (bx.y1 - bx.y0) / lpFont->line_height + 1;

	if(config.line_count > arrlen(buffer.lcache)) {
		config.line_count = arrlen(buffer.lcache);
	}

	lgi_drawText( &config );

	return lgi_False;
}



/* todo: cursors need to be sorted and merged if occupy the same slot */


int
ecurcmp(
lui_Cursor c0, lui_Cursor c1)
{
	return
	c0.yline != c1.yline ?
	(c0.yline > c1.yline ? +1 :
	c0.yline < c1.yline ? -1 : 0) :

	(c0.xchar > c1.xchar ? +1 :
	c0.xchar < c1.xchar ? -1 : 0) ;
}

int
efndcur(
lui_Editor *editor, int xchar, int yline)
{
  /* todo */
	for(int i=0; i<enumcur(editor); i+=1)
	{ if( (egetcurx(editor,i) == xchar) &&
		(egetcury(editor,i) == yline) ) return i;
}

return - 1;
}

/* probably do not use bubble sort */
void
esrtcur(
lui_Editor *editor)
{
	int sorted;

	do
	{
		sorted = lgi_True;

		for(int i=0; i<enumcur(editor)-1; i+=1)
		{
			lui_Cursor c0 = egetcur(editor,i+0);
			lui_Cursor c1 = egetcur(editor,i+1);

			int cmp = ecurcmp(c0,c1);
			if(cmp == +1)
			{ esetcur(editor,i+0,c1);
				esetcur(editor,i+1,c0);

				sorted = lgi_False;
			}
		}
	} while(sorted != lgi_True);
}

int
lui_editor__addCursor(lui_Editor *lp, lui_Cursor cur) {
	*arradd(lp->cursor,1) = cur;
	esrtcur(lp);
	return efndcur(lp,cur.xchar,cur.yline);
}

int
ecurloc(lui_Editor *wdg, int index) {
	return lui_Buffer_getLineOffset(&wdg->buffer,egetcury(wdg,index)) + egetcurx(wdg,index);
}

char *
egetptr(lui_Editor *wdg, int index)
{
  /* todo: this should point somewhere safer */
	lgi_Global char p;

  p = 0; /* in case it was overwritten */

	if(index >= 0 && index < wdg->buffer.length)
	{
		return &wdg->buffer.string[index];
	}

	return &p;
}

inline char
egetchr(lui_Editor *wdg, int offset)
{
	return *egetptr(wdg,offset);
}

inline lui_Cursor
egetcur(
lui_Editor *editor, int index)
{
	return editor->cursor[index];
}

inline int
egetcurx(
lui_Editor *editor, int index)
{
	return egetcur(editor,index).xchar;
}

inline int
egetcury(
lui_Editor *editor, int index)
{
	return egetcur(editor,index).yline;
}

void
esetcur(lui_Editor *editor, int index, lui_Cursor cur) {
	cur.yline = iclamp(cur.yline,0,arrlen(editor->buffer.lcache)-1);
	cur.xchar = iclamp(cur.xchar,0,lui_Buffer_getLineLength(&editor->buffer,cur.yline));
	editor->cursor[index] = cur;

#if 0
	lui_Cursor old = egetcur(editor,index);
	float inf = egetinf(editor,index);

	if(cur.yline != old.yline)
	{
		old.xchar = 0;

		inf = 0;
	}

	if(cur.xchar != old.xchar)
	{
		char *ptr = egetptr2(editor,old.xchar,cur.yline);
		if(cur.xchar > old.xchar)
		{ for(int i=0;i<cur.xchar-old.xchar;i+=1)
			inf += lui_getTextWidth(editor->font,ptr[i-0]);
		} else
		if(cur.xchar < old.xchar)
		{ for(int i=0;i>cur.xchar-old.xchar;i-=1)
			inf -= lui_getTextWidth(editor->font,ptr[i-1]);
		}
	}
	editor->curinf[index] = inf;
#endif
}

void
lui_editor__setCursorX(lui_Editor *editor, int index, int xchar)
{
	lui_Cursor cur = egetcur(editor,index);

	cur.xchar = xchar;

	esetcur(editor,index,cur);
}

void
lui_editor__setCursorY(lui_Editor *editor, int index, int yline)
{
	lui_Cursor cur = egetcur(editor,index);

	cur.yline = yline;

	esetcur(editor,index,cur);
}

char
ecurchr(lui_Editor *wdg, int index, int off) {
	return egetchr(wdg,ecurloc(wdg,index) + off);
}

lui_Cursor
emovcurx(lui_Editor *editor, int cursor, int mov) {
	lui_Cursor cur = egetcur(editor,cursor);

	if(cur.xchar + mov > lui_Buffer_getLineLength(&editor->buffer,cur.yline)) {
		if(cur.yline + 1 <= arrlen(editor->buffer.lcache) - 1) {
			cur.yline += 1;
			cur.xchar  = 0;
		}
	} else
	if(cur.xchar + mov < 0) {
		if(cur.yline - 1 >= 0) {
			cur.yline -= 1;
			cur.xchar  = lui_Buffer_getLineLength(&editor->buffer,cur.yline);
		}
	} else {
		cur.xchar += mov;
	}

	esetcur(editor,cursor,cur);

	return cur;
}

void
emovcury(
lui_Editor *editor, int index, int mov)
{
	lui_editor__setCursorY(editor,index,egetcury(editor,index)+mov);
}

void
eaddchr_(lui_Editor *wdg, int cursor, int length) {
}


void
Editor_keyOne(lui_Editor *wdg, lui_EditorEvent key) {

	lui_Cursor cursor = egetcur(wdg,key.cur);
	int off = ecurloc(wdg,key.cur);
	int dir = 0;
	int cur = key.cur;
	int num = key.num;
	int chr = key.chr;
	switch(key.kid) {
		case E_kMOVE_LEFT:
		case E_kMOVE_RIGHT: {
			wdg->cursor_blink_timer = 0;
			dir = +1;
			if (key.kid == E_kMOVE_LEFT) {
				off += (dir = -1);
			}
			if (key.mod & E_MOD_CTRL_BIT) {
				if (E_IS_WORD_DELI(egetchr(wdg,off))) {
					while (egetchr(wdg,off)!=0 && E_IS_WORD_DELI(egetchr(wdg,off))) {
						emovcurx(wdg,cur,dir); off += dir;
					}
				} else {
					while (egetchr(wdg,off)!=0 && !E_IS_WORD_DELI(egetchr(wdg,off))) {
						emovcurx(wdg,cur,dir); off += dir;
					}
				}
			} else {
				emovcurx(wdg,cur,dir);
			}
		} break;
		case E_kMOVE_LINE_RIGHT: {
			lui_editor__setCursorX(wdg,cur,lui_Buffer_getLineLength(&wdg->buffer,cursor.yline));
		} break;
		case E_kMOVE_LINE_LEFT: {
			lui_editor__setCursorX(wdg,cur,0);
		} break;

		case E_kDELETE_BACK: {
			off += (dir = - 1);
		}
    	/* fall-through */
		case E_kDELETE_HERE:

		if (wdg->isReadonly) {
			return;
		}

		if (off >= 0 && off + num <= wdg->buffer.length) {
			wdg->cursor_blink_timer = 0;

			char *ptr = egetptr(wdg,off);

      	/* delete the entire line feed, as supposed to just breaking it */
			if((ptr[0 + dir] == '\r') && (ptr[1 + dir] == '\n')) {
				off += dir;
				num += 1;
			}

      /* gotta move the cursor first */
			emovcurx(wdg,cur,dir*num);

			lui_Buffer_insertSize(&wdg->buffer,off,-num);
			lui_buffer__reformat(&wdg->buffer);

		} break;
		case E_kCHAR: {
			if (wdg->isReadonly) {
				return;
			}

			wdg->cursor_blink_timer = 0;
			lui__debugassert(chr != '\r' && chr != '\n' );

			int mov = 1;
			int end = 0;

			if(chr >= 0x80) {
				chr = '?';
				end =   0;
				mov =   1;
			} else
			if (chr == '\t') {
				chr = ' ';
				end = ' ';
				mov =   2;
			} else
			switch(chr) {
				case '{': end = '}'; break;
				case '[': end = ']'; break;
				case '(': end = ')'; break;
				case '}':
				case ']':
				case ')':
				if(egetchr(wdg,off) == chr) {
					mov = 1;
					chr = 0;
					end = 0;
				} break;
			}

			if (chr != 0) {
				num = end != 0 ? 2 : 1;

				char *ptr = lui_Buffer_insertSize(&wdg->buffer,off,num);
				ptr[0] = chr;

				if (end != 0) {
					ptr[1] = end;
				}

        		/* notify of the event and then move the index */
				eaddchr_(wdg,off,num);
				lui_buffer__reformat(&wdg->buffer);
			}

			emovcurx(wdg,cur,mov);
		} break;
		case E_kLINE: {
			if (wdg->isReadonly) {
				return;
			}
      /* this is syntax specific #todo */
			if(egetchr(wdg,off-1) == '{') {
				num += 1;
			}

			char *ptr = lui_Buffer_insertSize(&wdg->buffer,off,num*2);

      /* proper line end feed #todo */
			while(num -- != 0) {
				ptr[num*2+0] = '\r';
				ptr[num*2+1] = '\n';
			}

      // eaddrow(wdg,egetcury(wdg,index),num);

			eaddchr_(wdg,cur,num*2);
			emovcury(wdg,cur,    1);
			lui_editor__setCursorX(wdg,cur,    0);

			lui_buffer__reformat(&wdg->buffer);
		} break;
	}
}


void
lui_editor__testKeys(lui_Editor *editor) {

	int mod = 0;

	if (lgi_testCtrlKey()) {
		mod |= E_MOD_CTRL_BIT;
	}
	if (lgi_testAltKey()) {
		mod |= E_MOD_ALT_BIT;
	}
	if (lgi_testShiftKey()) {
		mod |= E_MOD_SHIFT_BIT;
	}

	if(lgi_isButtonPressed(0)) {
		#if 0
		int xcursor = + lgi.Input.Mice.xcursor;
		int ycursor = - lgi.Input.Mice.ycursor + rx.Window.size_y;

		int yline = editor->lyview + ycursor / editor->font->line_height;

		lui_TextLine line = lui_Buffer_getLineAtIndex(&editor->buffer,yline);
		char *string = editor->buffer.string + line.offset;

		float xwalk = 0;
		for(int xchar=0; xchar<line.length; xchar+=1)
		{
			float width = lui_getTextWidth(editor->font,string[xchar]);

			if(xcursor >= xwalk && xcursor <= xwalk + width)
			{
				esetcur(editor,0,(lui_Cursor){xchar,yline});
				break;
			}

			xwalk += width;
		}
#endif
	} else
	if(lgi_testCtrlKey() && lgi_testKey('Z')) {
	} else
	if(lgi_testKey(rx_kESCAPE))
	{
    /* todo */
		dlb_t *dlb = ccdlb(editor->cursor);
		dlb->sze_min = 1;

	} else
	if(lgi.Input.Mice.yscroll != 0)
	{
    /* scroll up */
		editor->lyview += lgi_testShiftKey() ? 16 : - lgi.Input.Mice.yscroll;
		editor->lyview  = iclamp(editor->lyview,0,arrlen(editor->buffer.lcache)-1);
	} else
	if(lgi_testKey(rx_kHOME))
	{
		Editor_handleKey(editor,E_kMOVE_LINE_LEFT,1,0);

	} else
	if(lgi_testKey(rx_kEND))
	{
		Editor_handleKey(editor,E_kMOVE_LINE_RIGHT,1,0);
	} else
	if(lgi_testCtrlKey() && lgi_testKey('X'))
	{
		for(int i=enumcur(editor)-1;i>=0;i-=1)
		{
			lui_TextLine row = lui_Buffer_getLineAtIndex(&editor->buffer,egetcury(editor,i));

			int num = 1;

			char *ptr = egetptr(editor,row.offset+row.length);
			if((ptr[0]=='\r') &&
			(ptr[1]=='\n')) num += 1;

			lui_Buffer_insertSize(&editor->buffer,row.offset,-(row.length+num));
		lui_buffer__reformat(&editor->buffer);
	}

} else
if(lgi_testKey(rxKEY_kUP))
{
	if(lgi_testCtrlKey() && lgi_testAltKey())
	{
		lui_Cursor cur = egetcur(editor,0);
		cur.yline -= 1;
		lui_editor__addCursor(editor,cur);
	} else
	if(lgi_testCtrlKey())
	{
      /* scroll up */
		editor->lyview -= lgi_testShiftKey() ? 16 : 1;
		editor->lyview  = iclamp(editor->lyview,0,arrlen(editor->buffer.lcache)-1);
	} else
	{
      /* move to the line above */
		for(int i=enumcur(editor)-1;i>=0;i-=1)
		emovcury(editor,i,-1);
}
} else
if(lgi_testKey(rxKEY_kDOWN))
{
	if(lgi_testCtrlKey() && lgi_testAltKey())
	{
		lui_Cursor cur = egetcur(editor,enumcur(editor)-1);
		cur.yline += 1;
		lui_editor__addCursor(editor,cur);
	} else
	if(lgi_testCtrlKey())
	{
      /* scroll down */
		editor->lyview += lgi_testShiftKey() ? 16 : 1;
		editor->lyview = iclamp(editor->lyview,0,arrlen(editor->buffer.lcache)-1);
	} else
	{
      /* move to the line below */
		for(int i=enumcur(editor)-1;i>=0;i-=1)
		emovcury(editor,i,+1);
}
} else
if(lgi_testKey(rxKEY_kLEFT))
{ Editor_handleKey(editor,E_kMOVE_LEFT,   1,0);
} else
if(lgi_testKey(rxKEY_kRIGHT))
{
	Editor_handleKey(editor,E_kMOVE_RIGHT, 1,0);
} else
if(lgi_testKey(rx_kDELETE))
{ Editor_handleKey(editor,E_kDELETE_HERE,1,0);
} else
if(lgi_testKey(rx_kBCKSPC))
{ Editor_handleKey(editor,E_kDELETE_BACK,1,0);
} else
if(lgi_testKey(rxKEY_kRETURN))
{
	Editor_handleKey(editor,E_kLINE,1,0);
} else
{ if(!lgi_testCtrlKey())
	{
		if(lgi_lastChar() != 0)
		{
			Editor_handleKey(editor,E_kCHAR,1,lgi_lastChar());
		}
	}
}
}