#ifndef RI_PIPELINE_H
#define RI_PIPELINE_H

// Pipeline / draw-state enums: primitive topology, depth/stencil compare, cull mode, color write
// mask, blend factors, and index type. Consumed by the pipeline description types (elsewhere).

#include "ri_prelude.h"

enum RITopology_e {
	RI_TOPOLOGY_POINT_LIST,
	RI_TOPOLOGY_LINE_LIST,
	RI_TOPOLOGY_LINE_STRIP,
	RI_TOPOLOGY_TRIANGLE_LIST,
	RI_TOPOLOGY_TRIANGLE_STRIP,
	RI_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
	RI_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
	RI_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
	RI_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
	RI_TOPOLOGY_PATCH_LIST
};

// R - fragment's depth or stencil reference
// D - depth or stencil buffer
enum RICompareFunc_e {
	RI_COMPARE_NONE,		 // test is disabled
	RI_COMPARE_ALWAYS,		 // true
	RI_COMPARE_NEVER,		 // false
	RI_COMPARE_EQUAL,		 // R == D
	RI_COMPARE_NOT_EQUAL,	 // R != D
	RI_COMPARE_LESS,		 // R < D
	RI_COMPARE_LESS_EQUAL,	 // R <= D
	RI_COMPARE_GREATER,		 // R > D
	RI_COMPARE_GREATER_EQUAL // R >= D
};

enum RICullMode_e { RI_CULL_MODE_NONE = 0, RI_CULL_MODE_FRONT = 0x1, RI_CULL_MODE_BACK = 0x2, RI_CULL_MODE_BOTH = RI_CULL_MODE_FRONT | RI_CULL_MODE_BACK };

// Reversed winding swaps which face is front, so the cull mode has to swap FRONT<->BACK with it. NONE and
// BOTH are winding-independent and must pass through untouched -- note this is NOT a bitwise NOT: the enum
// is consumed as a mask, so ~NONE would set both bits (culling everything) and ~BOTH would clear them.
static inline enum RICullMode_e RI_FlipCullMode( enum RICullMode_e mode )
{
	if( mode == RI_CULL_MODE_FRONT )
		return RI_CULL_MODE_BACK;
	if( mode == RI_CULL_MODE_BACK )
		return RI_CULL_MODE_FRONT;
	return mode;
}

enum RIIndexType_e { RI_INDEX_TYPE_16, RI_INDEX_TYPE_32 };

enum RIColorWriteMask_e {
	RI_COLOR_WRITE_NONE = 0,
	RI_COLOR_WRITE_R = 0x1,
	RI_COLOR_WRITE_G = 0x2,
	RI_COLOR_WRITE_B = 0x4,
	RI_COLOR_WRITE_A = 0x8,

	RI_COLOR_WRITE_RGB = RI_COLOR_WRITE_R | RI_COLOR_WRITE_G | RI_COLOR_WRITE_B,

	RI_COLOR_WRITE_RGBA = RI_COLOR_WRITE_R | RI_COLOR_WRITE_G | RI_COLOR_WRITE_B | RI_COLOR_WRITE_A
};

// S0 - source color 0
// S1 - source color 1
// D - destination color
// C - blend constants, set by "CmdSetBlendConstants"
enum RIBlendFactor_e {				   // RGB                               ALPHA
	RI_BLEND_ZERO,					   // 0                                 0
	RI_BLEND_ONE,					   // 1                                 1
	RI_BLEND_SRC_COLOR,				   // S0.r, S0.g, S0.b                  S0.a
	RI_BLEND_ONE_MINUS_SRC_COLOR,	   // 1 - S0.r, 1 - S0.g, 1 - S0.b      1 - S0.a
	RI_BLEND_DST_COLOR,				   // D.r, D.g, D.b                     D.a
	RI_BLEND_ONE_MINUS_DST_COLOR,	   // 1 - D.r, 1 - D.g, 1 - D.b         1 - D.a
	RI_BLEND_SRC_ALPHA,				   // S0.a                              S0.a
	RI_BLEND_ONE_MINUS_SRC_ALPHA,	   // 1 - S0.a                          1 - S0.a
	RI_BLEND_DST_ALPHA,				   // D.a                               D.a
	RI_BLEND_ONE_MINUS_DST_ALPHA,	   // 1 - D.a                           1 - D.a
	RI_BLEND_CONSTANT_COLOR,		   // C.r, C.g, C.b                     C.a
	RI_BLEND_ONE_MINUS_CONSTANT_COLOR, // 1 - C.r, 1 - C.g, 1 - C.b         1 - C.a
	RI_BLEND_CONSTANT_ALPHA,		   // C.a                               C.a
	RI_BLEND_ONE_MINUS_CONSTANT_ALPHA, // 1 - C.a                           1 - C.a
	RI_BLEND_SRC_ALPHA_SATURATE,	   // min(S0.a, 1 - D.a)                1
	RI_BLEND_SRC1_COLOR,			   // S1.r, S1.g, S1.b                  S1.a
	RI_BLEND_ONE_MINUS_SRC1_COLOR,	   // 1 - S1.r, 1 - S1.g, 1 - S1.b      1 - S1.a
	RI_BLEND_SRC1_ALPHA,			   // S1.a                              S1.a
	RI_BLEND_ONE_MINUS_SRC1_ALPHA	   // 1 - S1.a                          1 - S1.a
};

#endif
