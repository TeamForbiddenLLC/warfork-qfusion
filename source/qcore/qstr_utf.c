#include "qstr_utf.h"
#include "qarch.h"
#include "qstr.h"

struct QStrSpan qStrUtf8CodePointIter(struct qStrUTFIterable_s* iter){
  assert(iter);
  const size_t startCursor = iter->cursor;
  if(startCursor >= iter->buffer.len)
    return (struct QStrSpan) {0};
  unsigned char c0 = iter->buffer.buf[iter->cursor];
  if (c0 < 0xC0 || c0 > 0xFD) {
    if(iter->cursor + 1 > iter->buffer.len)
      return (struct QStrSpan) {0};
    iter->cursor += 1;
    return qSubStrSpan(iter->buffer, startCursor, iter->cursor);
  } else if (c0 < 0xE0) {
    if(iter->cursor + 2 > iter->buffer.len)
      return (struct QStrSpan) {0};
    iter->cursor += 2;
    return qSubStrSpan(iter->buffer, startCursor, iter->cursor);
  } else if (c0 < 0xF0) {
    if(iter->cursor + 3 > iter->buffer.len)
      return (struct QStrSpan) {0};
    iter->cursor += 3;
    return qSubStrSpan(iter->buffer, startCursor, iter->cursor);
  } else if (c0 < 0xF8) {
    if(iter->cursor + 4 > iter->buffer.len)
      return (struct QStrSpan) {0};
    iter->cursor += 4;
    return qSubStrSpan(iter->buffer, startCursor, iter->cursor);
  }
  iter->cursor++;
  for (; iter->cursor < iter->buffer.len; iter->cursor++) {
    if ((iter->buffer.buf[iter->cursor] & 0xC0) != 0x80)
      break;
  }
  return qSubStrSpan(iter->buffer, startCursor, iter->cursor);
}

struct QStrSpan bstrUtf16CodePointIter( struct qStrUTFIterable_s *iter )
{
	assert( iter );
	const size_t startCursor = iter->cursor;
	if( iter->cursor + 2 > iter->buffer.len )
		return ( struct QStrSpan ){ 0 };
	const uint16_t w1 = ( (unsigned)iter->buffer.buf[1] << 8 ) | (unsigned)iter->buffer.buf[0];
	if( w1 < 0xD800 || w1 > 0xDFFF )
		return qSubStrSpan( iter->buffer, startCursor, iter->cursor + 2 );
	if( iter->cursor + 4 > iter->buffer.len )
		return ( struct QStrSpan  ){ 0 };
	return qSubStrSpan( iter->buffer, startCursor, iter->cursor + 4 );
}

struct qStrUtfResult_s qStrUtf8NextCodePoint(struct qStrUTFIterable_s* iter) {
  assert(iter);
  if(qStrEmpty(iter->buffer)) 
    return (struct qStrUtfResult_s) {
      .codePoint = 0,
      .finished = true,
      .invalid = true
    };
  const size_t remainingLen = iter->buffer.len - iter->cursor;
  
  if(remainingLen < 1) goto invalid_token;
  const unsigned char c0 = iter->buffer.buf[iter->cursor];
  if (c0 < 0xC0 || c0 > 0xFD) {
  	iter->cursor += 1;
  	if (c0 >= 0x80) goto invalid_token_overflow; 
  	return (struct qStrUtfResult_s) {
  	  .codePoint = c0,
  	  .finished = remainingLen == 1,
  	  .invalid = false
  	};
  } else if (c0 < 0xE0) {
    if(remainingLen < 2) goto invalid_token_overflow;
    const unsigned char c1 = (unsigned char) ((unsigned) iter->buffer.buf[iter->cursor + 1] - 0x080); 
  	iter->cursor += 2;
  	const uint32_t codePoint = ((c0 << 6u) - (0x0C0 << 6u)) + c1;
  	if (c1 >= 0x40 || codePoint < 0x80) goto invalid_token;
    return (struct qStrUtfResult_s) {
		  .codePoint = codePoint,
		  .finished = remainingLen == 2,
		  .invalid = false 
		};
  } else if (c0 < 0xF0) {
    if(remainingLen < 3) goto invalid_token_overflow;
    const unsigned char c1 = (unsigned char) ((unsigned) iter->buffer.buf[iter->cursor + 1] - 0x080); 
    const unsigned char c2 = (unsigned char) ((unsigned) iter->buffer.buf[iter->cursor + 2] - 0x080); 
  	const uint32_t codePoint = ((c0 << 12) - (0x0E0 << 12u)) + (c1 << 6u) + c2;
  	iter->cursor += 3;
  	if ((c1|c2) >= 0x40 || codePoint < 0x80 || !isLegalUnicodeCodePoint (codePoint)) goto invalid_token;
    return (struct qStrUtfResult_s) {
		  .codePoint = codePoint,
		  .finished = remainingLen == 3,
		  .invalid = false
		};
  } else if (c0 < 0xF8) {
    if(remainingLen < 4) goto invalid_token_overflow;
  	const unsigned char c1 = (unsigned char) ((unsigned)  iter->buffer.buf[iter->cursor + 1] - 0x080);
  	const unsigned char c2 = (unsigned char) ((unsigned)  iter->buffer.buf[iter->cursor + 2] - 0x080);
  	const unsigned char c3 = (unsigned char) ((unsigned)  iter->buffer.buf[iter->cursor + 3] - 0x080);
  	iter->cursor += 4;
    const uint32_t codePoint = ((c0 << 18) - (0x0F0 << 18u)) + (c1 << 12u) + (c2 << 6u) + c3; 
  	if ((c1|c2|c3) >= 0x40 || codePoint < 0x10000 || !isLegalUnicodeCodePoint (codePoint)) goto invalid_token;
    return (struct qStrUtfResult_s) {
		  .codePoint = codePoint,
		  .finished = remainingLen == 4,
		  .invalid = false
		};
  }
invalid_token_overflow:
  iter->cursor = iter->buffer.len; 
invalid_token:
    return (struct qStrUtfResult_s) {
		  .codePoint = 0,
		  .finished = remainingLen == 0,
		  .invalid = true
		};

}

struct qStrUtfResult_s qStrUtf16NextCodePoint(struct qStrUTFIterable_s* iter) {
  assert(iter);
  if(qStrEmpty(iter->buffer)) 
    return (struct qStrUtfResult_s) {
      .codePoint = 0,
      .finished = true,
      .invalid = true
    };
  const size_t remainingLen = iter->buffer.len - iter->cursor;
  if(remainingLen < 2) goto invalid_token_overflow;
  const uint16_t w1 = ((unsigned)iter->buffer.buf[iter->cursor + 1] << 8) | (unsigned)iter->buffer.buf[iter->cursor];
  iter->cursor += 2;
  if (w1 < 0xD800 || w1 > 0xDFFF)
    return (struct qStrUtfResult_s) {
      .codePoint = w1,
      .invalid = false,
      .finished = remainingLen == 2
    };
  if(remainingLen < 4) goto invalid_token_overflow;
  const uint16_t w2 = ((unsigned)iter->buffer.buf[iter->cursor + 1] << 8) | (unsigned)iter->buffer.buf[iter->cursor];
  iter->cursor += 2;
  if (w2 < 0xDC00 || w2 > 0xDFFF) goto invalid_token; 
  return (struct qStrUtfResult_s) {
		.codePoint = (((w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000,
		.finished = remainingLen == 4,
		.invalid = true
	};


invalid_token_overflow:
  iter->cursor = iter->buffer.len; 
invalid_token:
    return (struct qStrUtfResult_s) {
		  .codePoint = 0,
		  .finished = iter->cursor == iter->buffer.len,
		  .invalid = true
		};
}
