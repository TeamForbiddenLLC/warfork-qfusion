/* Bstr 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
 * Paul Hsieh in 2002-2015
 * Michael Pollind 2024-* <mpollind at gmail dot com>
 * 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include "qstr.h"
#include "qarch.h"

#define   upcase(c) (toupper ((unsigned char) c))
#define downcase(c) (tolower ((unsigned char) c))
#define   wspace(c) (isspace ((unsigned char) c))

#define CHAR_TABLE_LEN (1 << CHAR_BIT)
#define TFSTR_MAX_PREALLOC (1024*1024)

static inline bool charToDigit(uint8_t c , uint8_t base, uint8_t* res) {
  uint8_t val = 0;
  if(c >= '0' && c <= '9')
    val = c - '0';
  if(c >= 'a' && c <= 'z')
    val = c - 'a' + 10;
  if(c >= 'A' && c <= 'Z') 
    val = c - 'A' + 10;
  if(val >= base) 
    return false;
  (*res) = val;
  return true;
}

typedef bool (*qStrCmpHandle)(struct QStrSpan s0, struct QStrSpan s1);

void qStrUpper(struct QStrSpan slice)
{
    for (size_t i = 0; i < slice.len; i++)
    {
        slice.buf[i] = toupper(slice.buf[i]);
    }
}

void qStrLower(struct QStrSpan slice)
{
    for (size_t i = 0; i < slice.len; i++)
    {
        slice.buf[i] = tolower(slice.buf[i]);
    }
}

struct QStrSpan qStrTrim(struct QStrSpan slice) { return qStrLTrim(qStrRTrim(slice)); }

struct QStrSpan qStrRTrim(struct QStrSpan slice)
{
    if (qStrEmpty(slice))
        return (struct QStrSpan){ slice.buf, 0 };
    for (size_t i = slice.len - 1;; i--)
    {
        if (!isspace(slice.buf[i]))
        {
            return (struct QStrSpan){ slice.buf, i + 1 };
        }
        if (i == 0)
            break;
    }
    return (struct QStrSpan){ slice.buf, 0 };
}

struct QStrSpan qStrLTrim(struct QStrSpan slice)
{
    if (qStrEmpty(slice))
        return (struct QStrSpan){ slice.buf, 0 };
    for (size_t i = 0; i < slice.len; i++)
    {
        if (!isspace(slice.buf[i]))
        {
            return (struct QStrSpan){ slice.buf + i, slice.len - i };
        }
    }
    return (struct QStrSpan){ slice.buf, 0 };
}

bool qStrAssign(struct QStr* str, struct QStrSpan slice)
{
    // set the length of the string alloc will
    if (!qStrSetLen(str, slice.len))
        return false;
    // slices can potentially overlap to the dest string
    //    trimming and assigning
    memmove(str->buf, slice.buf, slice.len);
    str->buf[str->len] = '\0';
    return true;
}

bool bstrresize(struct QStr* str, size_t len)
{
    str->buf = (char*)realloc(str->buf, len + 1);
    if (str->buf == NULL)
        return false;

    str->len = len;
    str->alloc = len + 1;
    str->buf[str->len] = '\0';

    return true;
}

bool qStrSetLen(struct QStr* str, size_t len)
{
    if (len > str->alloc)
    {
        assert(len > str->len);
        size_t reqSize = len;
        if (reqSize < TFSTR_MAX_PREALLOC)
        {
            reqSize *= 2;
        }
        else
        {
            reqSize += TFSTR_MAX_PREALLOC;
        }
        str->buf = (char*)realloc(str->buf, reqSize);
        if (str->buf == NULL)
            return false;
        str->alloc = reqSize;
    }
    str->len = len;
    return true;
}

bool qStrUpdateLen(struct QStr* str)
{
    size_t len = strlen(str->buf);
    str->len = len;
    assert(str->len < str->alloc); // the buffer has overrun
    return true;
}

bool qStrClear(struct QStr* str)
{
    if (!qStrSetLen(str, 0))
        return false;
    if(str->buf != NULL) {
        str->buf[0] = 0;
    }
    return true;
}

void qStrFree(struct QStr* str)
{
	if( str->buf ) {
		free( str->buf );
	}
	str->len = 0;
    str->alloc = 0;
    str->buf = NULL;
}

bool qStrSetResv(struct QStr* str, size_t reserveLen)
{
    if (reserveLen > str->alloc)
    {
        str->alloc = reserveLen;
        str->buf = (char*)realloc(str->buf, str->alloc);
        if (str->buf == NULL)
            return false;
    }
    return true;
}

struct QStr qStrDup(const struct QStr* str)
{
    assert(str);
    struct QStr result = { 0 };
    if (str->buf == NULL)
        return result;
    result.buf = (char*)malloc(str->len + 1);
    if (result.buf == NULL)
        return result;
    memcpy(result.buf, str->buf, str->len);
    result.buf[result.len] = str->len;
    return result;
}

bool qStrAppendSlice(struct QStr* str, const struct QStrSpan slice)
{
    if(slice.len == 0)
        return true;
    if (!qStrMakeRoomFor(str, slice.len + 1))
        return false;
    memmove(str->buf + str->len, slice.buf, slice.len);
    str->len += slice.len;
    str->buf[str->len] = '\0'; // don't expect the user to use memory in the avaliable range
    return true;
}

int qstrfmtll(struct QStrSpan slice, long long value)
{
    unsigned long long v;
    /* Generate the string representation, this method produces
     * a reversed string. */
    if (value < 0)
    {
        /* Since v is unsigned, if value==LLONG_MIN then
         * -LLONG_MIN will overflow. */
        if (value != LLONG_MIN)
        {
            v = -value;
        }
        else
        {
            v = ((unsigned long long)LLONG_MAX) + 1;
        }
    }
    else
    {
        v = value;
    }
    char* p = slice.buf;
    do
    {
        // early escape if we write past the end of the slice
        if (p + 1 >= slice.buf + slice.len)
            return -1;
        *p++ = '0' + (v % 10);
        v /= 10;
    } while (v);
    if (value < 0)
        *p++ = '-';

    /* Compute length and add null term. */
    const int len = p - slice.buf;

    /* Reverse the string. */
    p--;
    char* s = slice.buf;
    while (s < p)
    {
        const char aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return len;
}

int qstrfmtull(struct QStrSpan slice, unsigned long long value)
{
    /* Generate the string representation, this method produces
     * a reversed string. */
    char* p = slice.buf;
    do
    {
        // early escape if we write past the end of the slice
        if (p + 1 >= slice.buf + slice.len)
            return -1;
        *p++ = '0' + (value % 10);
        value /= 10;
    } while (value);

    /* Compute length and add null term. */
    const int len = p - slice.buf;

    /* Reverse the string. */
    p--;
    char* s = slice.buf;
    while (s < p)
    {
        const char aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return len;
}

bool qstrcatfmt(struct QStr* s, char const* fmt, ...)
{
    const char* f = fmt;
    va_list     ap;

    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least two times the format string itself. It's not the
     * best heuristic but seems to work in practice. */
    if (!qStrMakeRoomFor(s, strlen(fmt) * 2))
        return false; // we failed to make room for
    va_start(ap, fmt);
    f = fmt; /* Next format specifier byte to process. */
    while (*f)
    {
        char      next;
        long long num;

        /* Make sure there is always space for at least 1 char. */
        if (qStrAvailLen(*s) == 0)
        {
            if (!qStrMakeRoomFor(s, 1))
                return false;
        }
        assert(s->len <= s->alloc);

        switch (*f)
        {
        case '%':
            next = *(f + 1);
            if (next == '\0')
                break;
            f++;
            switch (next)
            {
            case 'c':
                num = va_arg(ap, int);
                s->buf[s->len++] = num;
                break;
			case 's': {
				char *str = va_arg( ap, char * );
				const size_t len = strlen( str );
				if( !qStrMakeRoomFor( s, len ) )
					return false;
				memcpy( s->buf + s->len, str, len );
				s->len += len;
				break;
			}
			case 'S':
            {
                const struct QStrSpan str = va_arg(ap, struct QStrSpan);
                if (!qStrMakeRoomFor(s, str.len))
                    return false;
                memcpy(s->buf + s->len, str.buf, str.len);
                s->len += str.len;
                break;
            }
            case 'i':
            case 'I':
            case 'l': {
                if (next == 'i')
                {
                    num = va_arg(ap, int);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                else if (next == 'l')
                {
                    num = va_arg(ap, long);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                else
                {
                    num = va_arg(ap, long long);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                const int len = qstrfmtll(qStrAvailSpan(*s), num);
                if (len == -1)
                    return false;
                s->len += len;

                break;
            }
            case 'u':
            case 'U':
            case 'L':
            {
                unsigned long long unum;
                if (next == 'u')
                {
                    unum = va_arg(ap, unsigned int);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                else if (next == 'L')
                {
                    unum = va_arg(ap, unsigned long);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                else
                {
                    unum = va_arg(ap, unsigned long long);
                    if (!qStrMakeRoomFor(s, QSTR_LSTR_SIZE))
                        return false;
                }
                const int len = qstrfmtull(qStrAvailSpan(*s), unum);
                if (len == -1)
                    return false;
                s->len += len;

                break;
            }
            default: /* Handle %% and generally %<unknown>. */
                s->buf[s->len++] = next;
                break;
            }
            break;
        default:
            s->buf[s->len++] = *f;
            break;
        }
        f++;
    }
    va_end(ap);

    if (!qStrMakeRoomFor(s, 1))
        return false;
    /* Add null-term */
    s->buf[s->len] = '\0';
    return s;
}

static inline size_t readNumberSign(const char* buf, size_t pos, size_t len, int* result_sign)
{
    assert(result_sign);
    if (pos + 1 < len)
    {
        switch (buf[pos])
        {
        case '+':
            (*result_sign) = 1;
            return 1;
        case '-':
            (*result_sign) = -1;
            return 1;
        }
    }
    return 0;
}

static inline size_t readNumberBase(const char* buf, size_t pos, size_t len, int* base)
{
    assert(base);
    (*base) = 10;
    if (pos + 2 < len && buf[pos] == '0')
    {
        switch (downcase(buf[pos + 1]))
        {
        case 'b':
            (*base) = 2;
            return 2;
        case 'o':
            (*base) = 8;
            return 2;
        case 'x':
            (*base) = 16;
            return 2;
        }
    }
    return 0;
}

bool qStrReadull(struct QStrSpan slice, unsigned long long* result)
{
    assert(result);
    if (qStrEmpty(slice))
        return false;

    size_t pos = 0;
    int    numBase = 0;
    pos += readNumberBase(slice.buf, pos, slice.len, &numBase);
    unsigned long long val = 0;
    for (; pos < slice.len; pos++)
    {
        uint8_t digit = 0;
        if (!charToDigit(slice.buf[pos], numBase, &digit))
            return false;
        if (val != 0)
        {
            val *= numBase;
        }
        val += digit;
    }
    (*result) = val;
    return true;
}
bool qStrReadll(struct QStrSpan slice, long long* result)
{
    assert(result);
    if (qStrEmpty(slice))
        return false;

    size_t pos = 0;
    int    sign = 1;
    int    numBase = 0;
    pos += readNumberSign(slice.buf, pos, slice.len, &sign);
    pos += readNumberBase(slice.buf, pos, slice.len, &numBase);
    int64_t val = 0;
    for (; pos < slice.len; pos++)
    {
        uint8_t digit = 0;
        if (!charToDigit(slice.buf[pos], numBase, &digit))
            return false;

        if (val != 0)
        {
            val *= numBase;
        }
        val += (digit * sign);
    }
    (*result) = val;
    return true;
}

bool qStrReadFloat(struct QStrSpan slice, float* result) {
    assert(result);
    if (qStrEmpty(slice))
        return false;
    size_t pos = 0;
    int    sign = 1;
    pos += readNumberSign(slice.buf, pos, slice.len, &sign);
    float val = 0;
    for (; pos < slice.len; pos++)
    {
        uint8_t digit = 0;
        if(slice.buf[pos] == '.') {
            pos++;
            size_t pos2 = slice.len - 1;
            float fract = 0.0f;
            while (pos <= pos2) {
                if (!charToDigit(slice.buf[pos2], 10, &digit))
                    return false;
                fract += (digit * sign);
                fract /= 10.0f;
                pos2--;
            }
            val += fract;
            break;
        }
        if (!charToDigit(slice.buf[pos], 10, &digit))
            return false;
        if (val != 0)
        {
            val *= 10;
        }
        val += (digit * sign);
    }
    (*result) = val;
    return true;
}
bool qStrReadDouble(struct QStrSpan slice, double* result) {
    assert(result);
    if (qStrEmpty(slice))
        return false;
    size_t pos = 0;
    int    sign = 1;
    pos += readNumberSign(slice.buf, pos, slice.len, &sign);
    double val = 0;
    for (; pos < slice.len; pos++)
    {
        uint8_t digit = 0;
        if(slice.buf[pos] == '.') {
            pos++;
            size_t pos2 = slice.len - 1;
            double fract = 0.0f;
            while (pos <= pos2) {
                if (!charToDigit(slice.buf[pos2], 10, &digit))
                    return false;
                fract += (digit * sign);
                fract /= 10.0;
                pos2--;
            }
            val += fract;
            break;
        }
        if (!charToDigit(slice.buf[pos], 10, &digit))
            return false;
        if (val != 0)
        {
            val *= 10;
        }
        val += (digit * sign);
    }
    (*result) = val;
    return true;
}

int qstrsscanf(struct QStrSpan slice, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int result = qstrvsscanf(slice, fmt, ap);
    va_end(ap);
    return result;
}

int qstrvsscanf(struct QStrSpan slice, const char* fmt, va_list ap)
{
    va_list cpy;
    char    staticbuf[1024], *buf = staticbuf;
    if ((slice.len + 1) >= 1024)
    {
        buf = (char*)malloc(slice.len + 1);
    }
    memcpy(buf, slice.buf, slice.len);
    buf[slice.len] = 0;

    va_copy(cpy, ap);
    const int res = vsscanf(buf, fmt, ap);
    va_end(cpy);
    if (buf != staticbuf)
        free(buf);
    return res;
}

bool qstrcatvprintf(struct QStr* str, const char* fmt, va_list ap)
{
    va_list cpy;
    char    staticbuf[1024], *buf = staticbuf;
    size_t  buflen = strlen(fmt) * 2;
    int     bufstrlen;

    /* We try to start using a static buffer for speed.
     * If not possible we revert to heap allocation. */
    if (buflen > sizeof(staticbuf))
    {
        buf = (char*)malloc(buflen);
        if (buf == NULL)
            return false;
    }
    else
    {
        buflen = sizeof(staticbuf);
    }

    /* Alloc enough space for buffer and \0 after failing to
     * fit the string in the current buffer size. */
    while (1)
    {
        va_copy(cpy, ap);
        bufstrlen = vsnprintf(buf, buflen, fmt, cpy);
        va_end(cpy);
        if (bufstrlen < 0)
        {
            if (buf != staticbuf)
                free(buf);
            return false;
        }
        if (((size_t)bufstrlen) >= buflen)
        {
            if (buf != staticbuf)
                free(buf);
            buflen = ((size_t)bufstrlen) + 1;
            buf = (char*)malloc(buflen);
            if (buf == NULL)
                return false;
            continue;
        }
        break;
    }

    /* Finally concat the obtained string to the bstr string and return it. */
    qStrAppendSlice(str, (struct QStrSpan){ buf, (size_t)bufstrlen });
    if (buf != staticbuf)
        free(buf);
    return true;
}

bool qstrcatprintf(struct QStr* s, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const bool result = qstrcatvprintf(s, fmt, ap);
    va_end(ap);
    return result;
}

bool bstrInserSlice(struct QStr* str, size_t offset, const struct QStrSpan slice)
{
    assert(offset <= str->len);
    if (!qStrMakeRoomFor(str, slice.len + 1))
        return false;
    memmove(str->buf + offset + slice.len, str->buf + offset, str->len - offset);
    for (size_t i = 0; i < slice.len; i++)
    {
        str->buf[offset + i] = slice.buf[i];
    }
    str->len += slice.len;
    str->buf[str->len] = '\0';
    return true;
}

bool bstrAppendChar(struct QStr* str, char b) {
    return qStrAppendSlice(str, (struct QStrSpan){ &b,  1 }); 
}

bool bstrInsertChar(struct QStr* str, size_t i, char b)
{
    return bstrInserSlice(str, i, (struct QStrSpan){ &b,  1 });
}

bool qStrMakeRoomFor(struct QStr* str, size_t addlen)
{
    const size_t avail = str->alloc - str->len;
    if (avail >= addlen)
        return true;

    size_t reqSize = str->len + addlen;
    if (reqSize < TFSTR_MAX_PREALLOC)
    {
        reqSize *= 2;
    }
    else
    {
        reqSize += TFSTR_MAX_PREALLOC;
    }
    str->buf = (char*)realloc(str->buf, reqSize);
    if (!str->buf)
        return false;
    str->alloc = reqSize;
    return true;
}

struct QStrSpan qStrSplitIter(struct qStrSplitIterable* iterable)
{
    assert(!qStrEmpty(iterable->buffer));
    assert(!qStrEmpty(iterable->delim));
    if (iterable->cursor == iterable->buffer.len)
        return (struct QStrSpan){ 0 };

    const int offset = qStrIndexOfOffset(iterable->buffer, iterable->cursor, iterable->delim);
    if (offset == -1)
    {
        struct QStrSpan res = qSubStrSpan(iterable->buffer, iterable->cursor, iterable->buffer.len);
        iterable->cursor = iterable->buffer.len; // move the cursor to the very end of the buffer
        return res;
    }
    struct QStrSpan res = qSubStrSpan(iterable->buffer, iterable->cursor, offset);
    iterable->cursor = offset + iterable->delim.len;
    return res;
}

struct QStrSpan qStrSplitRevIter(struct qStrSplitIterable* iterable)
{
    assert(!qStrEmpty(iterable->buffer));
    assert(!qStrEmpty(iterable->delim));
    if (iterable->cursor == 0)
        return (struct QStrSpan) { 0 };

    const int offset = qStrLastIndexOfOffset(iterable->buffer, iterable->cursor - 1, iterable->delim);
    if (offset == -1)
    {
        struct QStrSpan res = qSubStrSpan(iterable->buffer, 0, iterable->cursor);
        iterable->cursor = 0; // move the cursor to the very beginning of the buffer
        return res;
    }
    assert(offset + iterable->delim.len <= iterable->cursor);
    struct QStrSpan res = qSubStrSpan(iterable->buffer, offset + iterable->delim.len, iterable->cursor);
    iterable->cursor = offset;
    return res;
}

int qStrCaselessCompare(const struct QStrSpan b0, const struct QStrSpan b1)
{
    size_t i0 = 0;
    size_t i1 = 0;
    if (b0.buf == b1.buf)
    {
        goto same_str; // same string but length is different
    }

    for (; i0 < b0.len && i1 < b1.len; i0++, i1++)
    {
        const int v = (int)downcase(b0.buf[i0]) - (int)downcase(b1.buf[i1]);
        if (0 != v)
            return v;
    }

same_str:
    if (b0.len > b1.len)
        return 1;
    if (b1.len > b0.len)
        return -1;
    return 0;
}

int qStrCompare(const struct QStrSpan b0, const struct QStrSpan b1)
{
    size_t i0 = 0;
    size_t i1 = 0;
    if (b0.buf == b1.buf)
    {
        goto same_str; // same string but length is different
    }

    for (; i0 < b0.len && i1 < b1.len; i0++, i1++)
    {
        const int v = (int)b0.buf[i0] - (int)b1.buf[i1];
        if (0 != v)
        {
            return v;
        }
    }

same_str:
    if (b0.len > b1.len)
        return 1;
    if (b1.len > b0.len)
        return -1;
    return 0;
}

bool qStrCaselessEqual(const struct QStrSpan b0, const struct QStrSpan b1)
{
    if (b0.len != b1.len)
        return 0;

    size_t i0 = 0;
    size_t i1 = 0;
    for (; i0 < b0.len && i1 < b1.len; i0++, i1++)
    {
        if ((char)downcase(b0.buf[i0]) != (char)downcase(b1.buf[i1]))
        {
            return 0;
        }
    }
    return 1;
}

bool qStrEqual(const struct QStrSpan b0, const struct QStrSpan b1)
{
    // printf("EQ: \"%.*s\" -- \"%.*s\"\n", (int)b0.len, b0.buf, (int)b1.len, b1.buf);
    if (b0.len != b1.len)
        return 0;
    size_t i0 = 0;
    size_t i1 = 0;
    for (; i0 < b0.len && i1 < b1.len; i0++, i1++)
    {
        if (b0.buf[i0] != b1.buf[i1])
        {
            return false;
        }
    }
    return true;
}

static inline int qStrIndexOfCmp(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle,
                                 qStrCmpHandle handle)
{
    if (needle.len > haystack.len || needle.len == 0)
        return -1;

    if (haystack.len < 52 && needle.len <= 4)
    {
        for (size_t i = offset; i < (haystack.len - (needle.len - 1)); i++)
        {
            for (size_t j = 0; j < needle.len; j++)
            {
                if (handle((struct QStrSpan){  haystack.buf + i, needle.len }, needle))
                {
                    return i;
                }
            }
        }
        return -1;
    }

    // Boyer–Moore–Horspool_algorithm
    char skip_table[CHAR_TABLE_LEN] = { 0 };
    for (size_t i = 0; i < CHAR_TABLE_LEN; i++)
    {
        skip_table[i] = needle.len;
    }
    for (size_t i = 0; i < needle.len - 1; i++)
    {
        skip_table[(unsigned char)needle.buf[i]] = needle.len - i - 1;
    }

    size_t i = offset;
    while (i <= haystack.len - needle.len)
    {
        if (qStrEqual((struct QStrSpan){ haystack.buf + i,  needle.len }, needle))
        {
            return i;
        }
        i += skip_table[(unsigned char)haystack.buf[i + needle.len - 1]];
    }
    return -1;
}

static inline int bstrLastIndexOfCmp(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle,
                                     qStrCmpHandle handle)
{
    if (needle.len > haystack.len || needle.len == 0)
        return -1;

    assert(offset <= haystack.len);
    const size_t startIndex = (offset <= (haystack.len - needle.len)) ? offset : (haystack.len - needle.len);

    if (haystack.len < 52 && needle.len <= 4)
    {
        for (size_t i = startIndex;; i--)
        {
            assert(i < haystack.len);
            for (size_t j = 0; j < needle.len; j++)
            {
                if (handle((struct QStrSpan){ haystack.buf + i, needle.len }, needle))
                {
                    return i;
                }
            }
            if (i == 0)
                break;
        }
        return -1;
    }

    // Boyer–Moore–Horspool_algorithm
    // skip table to speed up searching for substrings
    char skip_table[256] = { 0 };
    for (size_t i = 0; i < 256; i++)
    {
        skip_table[i] = needle.len;
    }

    for (size_t i = needle.len - 1;; i--)
    {
        skip_table[(unsigned char)needle.buf[i]] = i;
        if (i == 1)
            break;
    }

    size_t i = startIndex;
    while (1)
    {
        if (handle((struct QStrSpan){ haystack.buf + i, needle.len }, needle))
        {
            return i;
        }
        const size_t skip = skip_table[(unsigned char)haystack.buf[i]];
        if (skip > i)
            break;
        i -= skip;
    }
    return -1;
}

int qStrIndexOfOffset(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle)
{
    return qStrIndexOfCmp(haystack, offset, needle, qStrEqual);
}

int qStrIndexOf(const struct QStrSpan haystack, const struct QStrSpan needle)
{
    return qStrIndexOfCmp(haystack, 0, needle, qStrEqual);
}

int qStrIndexOfCaseless(const struct QStrSpan haystack, const struct QStrSpan needle)
{
    return qStrIndexOfCmp(haystack, 0, needle, qStrCaselessEqual);
}

int qStrIndexOfCaselessOffset(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle)
{
    return qStrIndexOfCmp(haystack, offset, needle, qStrCaselessEqual);
}

int qStrLastIndexOf(const struct QStrSpan haystack, const struct QStrSpan needle)
{
    return bstrLastIndexOfCmp(haystack, haystack.len, needle, qStrEqual);
}

int qStrLastIndexOfOffset(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle)
{
    return bstrLastIndexOfCmp(haystack, offset, needle, qStrEqual);
}

int qStrLastIndexOfCaseless(const struct QStrSpan haystack, const struct QStrSpan needle)
{
    return bstrLastIndexOfCmp(haystack, haystack.len, needle, qStrCaselessEqual);
}

int qStrLastIndexOfCaselessOffset(const struct QStrSpan haystack, size_t offset, const struct QStrSpan needle)
{
    return bstrLastIndexOfCmp(haystack, offset, needle, qStrCaselessEqual);
}

int qStrIndexOfAny(const struct QStrSpan haystack, const struct QStrSpan characters)
{
    for (size_t i = 0; i < haystack.len; i++)
    {
        for (size_t j = 0; characters.len; j++)
        {
            if (haystack.buf[i] == characters.buf[j])
            {
                return i;
            }
        }
    }
    return false;
}

bool qstrcatjoin(struct QStr* str, struct QStrSpan* slices, size_t numSlices, struct QStrSpan sep)
{
    {
        assert(str);
        assert(slices);
        if (numSlices == 0)
            return true; // there is nothing to join just exit

        // calculating the reserve length
        size_t reserveLen = 0;
        for (size_t i = 0; i < numSlices; i++)
        {
            reserveLen += slices[i].len;
            reserveLen += sep.len;
        }
        reserveLen += 1; // space for the null terminator
        if (!qStrMakeRoomFor(str, reserveLen))
            return false;
    }
    for (size_t i = 0; i < numSlices; i++)
    {
        memcpy(str->buf + str->len, slices[i].buf, slices[i].len);
        str->len += slices[i].len;
        if (numSlices > 1 && i < numSlices - 1 && sep.len >= 1)
        {
            memcpy(str->buf + str->len, sep.buf, sep.len);
            str->len += sep.len;
        }
    }
    str->buf[str->len] = '\0';

    assert(str->len <= str->alloc); // we've overrun the buffer
    return true;
}

bool qstrcatjoinCStr(struct QStr* str, const char** argv, size_t argc, struct QStrSpan sep)
{
    for (size_t i = 0; i < argc; i++)
    {
        const size_t argLen = strlen(argv[i]);
        if (!qStrMakeRoomFor(str, argLen + sep.len))
            return false;
        memcpy(str->buf + str->len, argv[i], argLen);
        str->len += argLen;
        if (argc > 1 && i < argc - 1 && sep.len >= 1)
        {
            memcpy(str->buf + str->len, sep.buf, sep.len);
            str->len += sep.len;
        }
    }
    str->buf[str->len] = '\0';
    assert(str->len <= str->alloc); // we've overrun the buffer
    return true;
}

int qStrLastIndexOfAny(const struct QStrSpan haystack, const struct QStrSpan characters)
{
    for (size_t i = haystack.len;; i--)
    {
        for (size_t j = 0; characters.len; j++)
        {
            if (haystack.buf[i] == characters.buf[j])
            {
                return i;
            }
        }
    }
    return false;
}

// minimum length is 7 + precision
static int doubleToShorQStr(struct QStrSpan str, double d, int precision)
{
    const int len = snprintf(str.buf, str.len, "%.*f", precision, d);
    if(len < 0 || len == str.len) 
        return -1;
    // remove trailing zeroes
    int       end = len;
    for (int i = len - 1; i >= 0; i--)
    {
        if (str.buf[i] == '0')
        {
            if (end == i + 1)
                end = i;
            continue;
        }

        if (str.buf[i] != '.')
            continue;

        if (end == i + 1)
            end = i;
        str.buf[end] = 0;
        break;
    }
    return end > 0 ? (size_t)end : 0;
}

int qPrettyPrintBytes(struct QStrSpan slice, size_t numBytes)
{
    const struct QStrSpan strs[] = {
        qCToStrRef("B"), qCToStrRef("KB"), qCToStrRef("MB"), qCToStrRef("GB"), qCToStrRef("TB"),
    };
    double   value = (double)numBytes;
    uint64_t i = 0;
    while ((value < 0 ? -value : value) >= 1024 && i < sizeof(strs) / sizeof(*strs) - 1)
    {
        value /= 1024;
        ++i;
    }
    size_t pos = doubleToShorQStr(slice, value, 1);
    if (pos == -1)
        return -1;
    struct QStrSpan subStr = qSubStrSpan(slice, pos, slice.len);

    size_t len = Q_MIN(strs[i].len, subStr.len);
    if (len < strs[i].len)
        return -1;
    memcpy(slice.buf + pos, strs[i].buf, len);
    pos += len;
    return pos;
}

int qPrettyPrintDuration(struct QStrSpan slice, double ns)
{
    const struct  QStrSpan strs[] = { qCToStrRef("ns"), qCToStrRef("ms"), qCToStrRef("s"), qCToStrRef("m"), qCToStrRef("h"), qCToStrRef("d") };

    uint64_t i = 0;

    double capacity = 1000;
    while ((ns < 0 ? -ns : ns) >= capacity && i < sizeof(strs) / sizeof(*strs) - 1)
    {
        ns /= capacity;
        ++i;

        switch (i)
        {
        case 0:
        case 1:
        case 2:
            capacity = 1000;
            break;
        case 3:
            capacity = 60;
            break;
        case 4:
            capacity = 24;
            break;
        }
    }

    const int maxPrecision = 7;

    int    precision = 1;
    double v = ns;
    while (v < 100 && precision < maxPrecision)
    {
        v *= 100;
        ++precision;
    }

    size_t pos = doubleToShorQStr(slice, ns, precision);
    if (pos == -1)
        return -1;
    struct QStrSpan subStr = qSubStrSpan(slice, pos, slice.len);

    size_t len = Q_MIN(strs[i].len, subStr.len);
    if (len < strs[i].len)
        return -1;
    memcpy(slice.buf + pos, strs[i].buf, len);
    pos += len;
    return pos;
}

