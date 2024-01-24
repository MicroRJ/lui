
void
lui_Buffer_setName(lui_Buffer *xx, char const *name) {
	char *yy = xx->name;
	while (*name) *yy ++ = *name ++;
}

void
lui_Buffer_setFileName(lui_Buffer *xx, char const *name) {
	char *yy = xx->fileName;
	while (*name) *yy ++ = *name ++;
}

void
lui_buffer__initWithMemory(lui_Buffer *xx, char const *name, char const *file, __int64 length, void *fileContents) {
	lui_Buffer_setName(xx,name);
	lui_Buffer_setFileName(xx,file);
	xx->length=length;
	xx->extent=length;
	xx->memory=fileContents;
	xx->isReadonly=1;
}

void
lui_Buffer_initAlloc(lui_Buffer *lpBuffer, char const *tag, __int64 length) {
	lui_Buffer_setName(lpBuffer,tag);

	if(length) {

		lui_Buffer_allocSize(lpBuffer,length,length);
	}
}

void
lui_buffer__uninit(lui_Buffer *lpBuffer) {
	lui__freealigned(lpBuffer->string);
	lui__freealigned(lpBuffer->colors);
	lpBuffer->length = 0;
	lpBuffer->extent = 0;
	lpBuffer->string = 0;
	lpBuffer->colors = 0;
}

char *
lui_Buffer_allocSize(lui_Buffer *lpBuffer, __int64 reserve, __int64 commit) {
	lui__debugassert(commit <= reserve + lpBuffer->extent - lpBuffer->length);

	if(lpBuffer->extent < lpBuffer->length+reserve) {
		lpBuffer->extent <<= 1;
		if(lpBuffer->extent < lpBuffer->length + reserve) {
			lpBuffer->extent = lpBuffer->length + reserve;
		}
		lpBuffer->string = lui__reallocaligned(sizeof(*lpBuffer->string) * lpBuffer->extent, lpBuffer->string);
		lpBuffer->colors = lui__reallocaligned(sizeof(*lpBuffer->colors) * lpBuffer->extent, lpBuffer->colors);
	}

	lpBuffer->length = lpBuffer->length + commit;
	return lpBuffer->string + lpBuffer->length;
}

char *
lui_Buffer_insertSize(lui_Buffer *buffer, __int64 offset, __int64 length) {

	char *colors = (char *) buffer->colors + offset;
	char *string = (char *) buffer->string + offset;
	size_t tomove = buffer->length - offset - length;

	// printf("length: [%lli/%lli] insert: %lli tomove: %lli\n"
	// , buffer->length, buffer->extent, length, tomove);

	if (length > 0) {
		lui_Buffer_allocSize(buffer,length,length);
	}

	if (offset != buffer->length) {


		/* [[todo]]: this could be niftier */
		if (length < 0) {
			lui_memmove(string,string-length,tomove);
			lui_memmove(colors,colors-length,tomove);
		} else {
			lui_memmove(string+length,string,tomove);
			lui_memmove(colors+length,colors,tomove);
		}
	}

	if (length < 0) {
		buffer->length += length;
	}

	return buffer->string+offset;
}


/* could this be done automatically #todo */
void
lui_buffer__reformat(lui_Buffer *buffer) {

	char *cursor = buffer->string;
  /* todo */
	arrdel(buffer->lcache);
	buffer->lcache = 0;

	int indent = 0;
	for(;;) {
		lui_TextLine *line = arradd(buffer->lcache,1);
		line->offset = (cursor - buffer->string);

		while((cursor < buffer->string + buffer->length) &&
		(*cursor != '\r') &&
		(*cursor != '\n')) cursor += 1;

		line->length = (cursor - buffer->string) - line->offset;

	if(cursor < buffer->string + buffer->length)
	{
		cursor += ((cursor[0] == '\r') &&
		(cursor[1] == '\n'))  ? 2 : 1;
	} else
	{
		break;
	}
}

lgi_ASSERT(cursor == buffer->string + buffer->length);
}

inline lui_TextLine
lui_Buffer_getLineAtIndex(
lui_Buffer *buffer, int index)
{
  /* remove, this should not happen? */
	if(buffer->lcache == lgi_Null)
	{
		lui_TextLine row;
		row.length = 0;
		row.offset = 0;
		return row;
	}

	return buffer->lcache[index];
}

inline int
lui_Buffer_getLineOffset(
lui_Buffer *buffer, int yline)
{
	return lui_Buffer_getLineAtIndex(buffer,yline).offset;
}

inline int
lui_Buffer_getLineLength(
lui_Buffer *buffer, int yline)
{
	return lui_Buffer_getLineAtIndex(buffer,yline).length;
}

