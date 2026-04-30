#ifndef QGL_TYPES_H
#define QGL_TYPES_H

/* GL primitive type aliases — stable C types, no GL headers needed. */
#ifndef GL_TYPEDEFS_DEFINED
#define GL_TYPEDEFS_DEFINED
typedef float          GLfloat;
typedef int            GLint;
typedef unsigned int   GLuint;
typedef unsigned short GLhalfARB;
#endif

/* GL pixel format/type enum values — stable across all GL implementations. */
#ifndef GL_FLOAT
#define GL_FLOAT                    0x1406
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT               0x140B
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE            0x1401
#endif
#ifndef GL_ALPHA
#define GL_ALPHA                    0x1906
#endif
#ifndef GL_RGB
#define GL_RGB                      0x1907
#endif
#ifndef GL_RGBA
#define GL_RGBA                     0x1908
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE                0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA          0x190A
#endif
#ifndef GL_BGR_EXT
#define GL_BGR_EXT                  0x80E0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                 0x80E1
#endif
#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4   0x8033
#endif
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1   0x8034
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5     0x8363
#endif
#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES            0x8D64
#endif

#endif /* QGL_TYPES_H */
