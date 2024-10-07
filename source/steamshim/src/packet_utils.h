
#pragma once

#include "os.h"

static inline bool unpack_string_array_null( char *buf, size_t bufSize,char *str[], size_t len )
{
	if( len == 0 ) {
		return true;
	}
	size_t strIdx = 0;
	size_t cursor = 0;
	str[strIdx++] = buf;
	for( ; cursor < bufSize && strIdx < len; cursor++ ) {
		if( cursor + 1 >= bufSize ) {
			str[strIdx++] = NULL;
		}
		if( buf[cursor] == '\0' ) {
			str[strIdx++] = buf + cursor + 1;
		}
	}
	return true;
}

static inline size_t pack_cstr_null_terminated( char *buf, size_t bufSize, const char *values[], size_t len )
{
	size_t cursor = 0;
	for( size_t i = 0; i < len; i++ ) {
		for( const char *c = values[i]; *c; c++ ) {
			buf[cursor++] = *c;
			if( cursor > bufSize ) {
				return 0;
			}
		}
		buf[cursor++] = '\0';
		if( cursor > bufSize ) {
			return 0;
		}
	}
	return cursor;
}
