#ifndef QCORE_QMATH_H
#define QCORE_QMATH_H

#include "qtypes.h"

struct Qf32x4_s {
	union {
		struct {
			float x, y, z, w;
		};
		struct {
			float r, g, b, a;
		};
		float v[4];
	};
};
Q_COMPILE_ASSERT( sizeof( struct Qf32x4_s ) == 4 * 4 );

struct QRectf32_s {
	union {
		struct {
			float x, y, w, h;
		};
		float v[4];
	};
};
Q_COMPILE_ASSERT( sizeof( struct QRectf32_s ) == 4 * 4 );

struct Qf32x3_s {
	union {
		struct {
			float x, y, z;
		};
		struct {
			float r, g, b;
		};
		float v[3];
	};
};
Q_COMPILE_ASSERT( sizeof( struct Qf32x3_s ) == 3 * 4 );

struct Qf64x4_s {
	union {
		struct {
			double x, y, z, w;
		};
		double v[4];
	};
};
Q_COMPILE_ASSERT( sizeof( struct Qf64x4_s ) == 8 * 4 );

struct Qu8x3_s {
	union {
		struct {
			uint8_t x, y, z;
		};
		uint8_t v[3];
	};
};
Q_COMPILE_ASSERT( sizeof( struct Qu8x3_s ) == 3 );

struct Qf64x3_s {
	union {
		struct {
			double x, y, z;
		};
		double v[3];
	};
};
Q_COMPILE_ASSERT( sizeof( struct Qf64x3_s ) == 8 * 3 );
#endif
