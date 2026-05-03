#ifndef R_FTLIB_MODULE_H
#define R_FTLIB_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"

struct shader_s;
struct qfontface_s;

#ifndef _FTLIB_PUBLIC_H_
typedef void ( *fdrawchar_t )( int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, const struct shader_s *shader );
#endif

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

// font registration / lifecycle
DECLARE_TYPEDEF_METHOD( void, FTLIB_PrecacheFonts, bool verbose );
DECLARE_TYPEDEF_METHOD( struct qfontface_s *, FTLIB_RegisterFont, const char *family, const char *fallback, int style, unsigned int size );
DECLARE_TYPEDEF_METHOD( void, FTLIB_TouchFont, struct qfontface_s *qfont );
DECLARE_TYPEDEF_METHOD( void, FTLIB_TouchAllFonts, void );
DECLARE_TYPEDEF_METHOD( void, FTLIB_FreeFonts, bool verbose );

// font metrics
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_FontSize, struct qfontface_s *font );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_FontHeight, struct qfontface_s *font );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_StringWidth, const char *str, struct qfontface_s *font, size_t maxlen, int flags );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_StrlenForWidth, const char *str, struct qfontface_s *font, size_t maxwidth, int flags );
DECLARE_TYPEDEF_METHOD( int, FTLIB_FontUnderline, struct qfontface_s *font, int *thickness );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_FontAdvance, struct qfontface_s *font );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_FontXHeight, struct qfontface_s *font );

// drawing
DECLARE_TYPEDEF_METHOD( void, FTLIB_DrawRawChar, int x, int y, wchar_t num, struct qfontface_s *font, vec4_t color );
DECLARE_TYPEDEF_METHOD( void, FTLIB_DrawClampChar, int x, int y, wchar_t num, int xmin, int ymin, int xmax, int ymax, struct qfontface_s *font, vec4_t color, vec4_t bgcolor );
DECLARE_TYPEDEF_METHOD( void, FTLIB_DrawClampString, int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct qfontface_s *font, vec4_t color, int flags );
DECLARE_TYPEDEF_METHOD( size_t, FTLIB_DrawRawString, int x, int y, const char *str, size_t maxwidth, int *width, struct qfontface_s *font, vec4_t color, int flags );
DECLARE_TYPEDEF_METHOD( int, FTLIB_DrawMultilineString, int x, int y, const char *str, int halign, int maxwidth, int maxlines, struct qfontface_s *font, vec4_t color, int flags );
DECLARE_TYPEDEF_METHOD( fdrawchar_t, FTLIB_SetDrawCharIntercept, fdrawchar_t intercept );

#undef DECLARE_TYPEDEF_METHOD

struct ftlib_import_s {
	FTLIB_PrecacheFontsFn FTLIB_PrecacheFonts;
	FTLIB_RegisterFontFn FTLIB_RegisterFont;
	FTLIB_TouchFontFn FTLIB_TouchFont;
	FTLIB_TouchAllFontsFn FTLIB_TouchAllFonts;
	FTLIB_FreeFontsFn FTLIB_FreeFonts;
	FTLIB_FontSizeFn FTLIB_FontSize;
	FTLIB_FontHeightFn FTLIB_FontHeight;
	FTLIB_StringWidthFn FTLIB_StringWidth;
	FTLIB_StrlenForWidthFn FTLIB_StrlenForWidth;
	FTLIB_FontUnderlineFn FTLIB_FontUnderline;
	FTLIB_FontAdvanceFn FTLIB_FontAdvance;
	FTLIB_FontXHeightFn FTLIB_FontXHeight;
	FTLIB_DrawRawCharFn FTLIB_DrawRawChar;
	FTLIB_DrawClampCharFn FTLIB_DrawClampChar;
	FTLIB_DrawClampStringFn FTLIB_DrawClampString;
	FTLIB_DrawRawStringFn FTLIB_DrawRawString;
	FTLIB_DrawMultilineStringFn FTLIB_DrawMultilineString;
	FTLIB_SetDrawCharInterceptFn FTLIB_SetDrawCharIntercept;
};

#if FTLIB_DEFINE_INTERFACE_IMPL
static struct ftlib_import_s ftlib_import;

void FTLIB_PrecacheFonts( bool verbose ) { ftlib_import.FTLIB_PrecacheFonts( verbose ); }
struct qfontface_s *FTLIB_RegisterFont( const char *family, const char *fallback, int style, unsigned int size ) { return ftlib_import.FTLIB_RegisterFont( family, fallback, style, size ); }
void FTLIB_TouchFont( struct qfontface_s *qfont ) { ftlib_import.FTLIB_TouchFont( qfont ); }
void FTLIB_TouchAllFonts( void ) { ftlib_import.FTLIB_TouchAllFonts(); }
void FTLIB_FreeFonts( bool verbose ) { ftlib_import.FTLIB_FreeFonts( verbose ); }
size_t FTLIB_FontSize( struct qfontface_s *font ) { return ftlib_import.FTLIB_FontSize( font ); }
size_t FTLIB_FontHeight( struct qfontface_s *font ) { return ftlib_import.FTLIB_FontHeight( font ); }
size_t FTLIB_StringWidth( const char *str, struct qfontface_s *font, size_t maxlen, int flags ) { return ftlib_import.FTLIB_StringWidth( str, font, maxlen, flags ); }
size_t FTLIB_StrlenForWidth( const char *str, struct qfontface_s *font, size_t maxwidth, int flags ) { return ftlib_import.FTLIB_StrlenForWidth( str, font, maxwidth, flags ); }
int FTLIB_FontUnderline( struct qfontface_s *font, int *thickness ) { return ftlib_import.FTLIB_FontUnderline( font, thickness ); }
size_t FTLIB_FontAdvance( struct qfontface_s *font ) { return ftlib_import.FTLIB_FontAdvance( font ); }
size_t FTLIB_FontXHeight( struct qfontface_s *font ) { return ftlib_import.FTLIB_FontXHeight( font ); }
void FTLIB_DrawRawChar( int x, int y, wchar_t num, struct qfontface_s *font, vec4_t color ) { ftlib_import.FTLIB_DrawRawChar( x, y, num, font, color ); }
void FTLIB_DrawClampChar( int x, int y, wchar_t num, int xmin, int ymin, int xmax, int ymax, struct qfontface_s *font, vec4_t color, vec4_t bgcolor ) { ftlib_import.FTLIB_DrawClampChar( x, y, num, xmin, ymin, xmax, ymax, font, color, bgcolor ); }
void FTLIB_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct qfontface_s *font, vec4_t color, int flags ) { ftlib_import.FTLIB_DrawClampString( x, y, str, xmin, ymin, xmax, ymax, font, color, flags ); }
size_t FTLIB_DrawRawString( int x, int y, const char *str, size_t maxwidth, int *width, struct qfontface_s *font, vec4_t color, int flags ) { return ftlib_import.FTLIB_DrawRawString( x, y, str, maxwidth, width, font, color, flags ); }
int FTLIB_DrawMultilineString( int x, int y, const char *str, int halign, int maxwidth, int maxlines, struct qfontface_s *font, vec4_t color, int flags ) { return ftlib_import.FTLIB_DrawMultilineString( x, y, str, halign, maxwidth, maxlines, font, color, flags ); }
fdrawchar_t FTLIB_SetDrawCharIntercept( fdrawchar_t intercept ) { return ftlib_import.FTLIB_SetDrawCharIntercept( intercept ); }

static inline void Q_ImportFtlibModule( struct ftlib_import_s *mod ) {
	ftlib_import = *mod;
}

#endif

#endif
