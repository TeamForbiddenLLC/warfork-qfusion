#ifndef R_BSP_HEADER_H
#define R_BSP_HEADER_H

#include "../gameshared/q_arch.h"

enum bsp_format_def_e {
	BSP_VERSION_1,
	BSP_VERSION_2,
	BSP_VERSION_3
};

enum bsp_flag_3_e {
	BSP_3_NONE        = 0,
	BSP_3_RAVEN       = 1,
	BSP_3_NOAREAS     = 2
};

struct bsp_format_def_s{
	enum bsp_format_def_e type;
	union {
		struct {
			int lightmapWidth;
			int lightmapHeight;
			enum bsp_flag_3_e flags;
		} v3;
	};
};

struct bsp_format_def_s guessFormatDef(const char* buffer, size_t len);

struct lump_s {
	int fileofs, filelen;
};

#define Q3_IDBSPHEADER	  "IBSP"
#define Q3_RBSPHEADER	    "RBSP"
#define Q3_QFBSPHEADER	  "FBSP"

#define Q3_BSPVERSION	    46
#define Q3_RTCWBSPVERSION	    47
#define Q3_RBSPVERSION	    1
#define Q3_QFBSPVERSION	    1

// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	Q3_MAX_MAP_MODELS	    0x400
#define	Q3_MAX_MAP_BRUSHES	    0x8000
#define	Q3_MAX_MAP_ENTITIES    0x800
#define	Q3_MAX_MAP_ENTSTRING   0x40000
#define	Q3_MAX_MAP_SHADERS	    0x400

#define	Q3_MAX_MAP_AREAS	    0x100
#define	Q3_MAX_MAP_FOGS	    0x100
#define	Q3_MAX_MAP_PLANES	    0x20000
#define	Q3_MAX_MAP_NODES	    0x20000
#define	Q3_MAX_MAP_BRUSHSIDES  0x30000
#define	Q3_MAX_MAP_LEAFS	    0x20000
#define	Q3_MAX_MAP_VERTEXES    0x80000
#define	Q3_MAX_MAP_FACES	    0x20000
#define	Q3_MAX_MAP_LEAFFACES   0x20000
#define	Q3_MAX_MAP_LEAFBRUSHES 0x40000
#define	Q3_MAX_MAP_PORTALS	    0x20000
#define	Q3_MAX_MAP_INDICES	    0x80000
#define	Q3_MAX_MAP_LIGHTING    0x800000
#define	Q3_MAX_MAP_VISIBILITY  0x200000

// lightmaps
#define Q3_MAX_LIGHTMAPS		4

#define Q3_LIGHTMAP_BYTES		3

#define	Q3_LIGHTMAP_WIDTH		128
#define	Q3_LIGHTMAP_HEIGHT		128
#define Q3_LIGHTMAP_SIZE		( LIGHTMAP_WIDTH*LIGHTMAP_HEIGHT*LIGHTMAP_BYTES )

#define QF_LIGHTMAP_WIDTH	512
#define QF_LIGHTMAP_HEIGHT	512
#define QF_LIGHTMAP_SIZE	( QF_LIGHTMAP_WIDTH*QF_LIGHTMAP_HEIGHT*LIGHTMAP_BYTES )

// key / value pair sizes

enum q3_lumps_e {
	Q3_LUMP_ENTITIES = 0,
	Q3_LUMP_SHADERREFS = 1,
	Q3_LUMP_PLANES = 2,
	Q3_LUMP_NODES = 3,
	Q3_LUMP_LEAFS = 4,
	Q3_LUMP_LEAFFACES = 5,
	Q3_LUMP_LEAFBRUSHES = 6,
	Q3_LUMP_MODELS = 7,
	Q3_LUMP_BRUSHES = 8,
	Q3_LUMP_BRUSHSIDES = 9,
	Q3_LUMP_VERTEXES = 10,
	Q3_LUMP_ELEMENTS = 11,
	Q3_LUMP_FOGS = 12,
	Q3_LUMP_FACES = 13,
	Q3_LUMP_LIGHTING = 14,
	Q3_LUMP_LIGHTGRID = 15,
	Q3_LUMP_VISIBILITY = 16,
	Q3_LUMP_LIGHTARRAY = 17,

	Q3_HEADER_SIZE = 18
};

struct q3bsp_header_s {
	char magic[4];
	int version;
	struct lump_s lumps[Q3_HEADER_SIZE];
};

#define Q1_BSPVERSION       29
#define Q1_MAX_MAP_HULLS    4

enum q1_lumps_e {
	Q1_LUMP_ENTITIES = 0,
	Q1_LUMP_PLANES = 1,
	Q1_LUMP_TEXTURES = 2,
	Q1_LUMP_VERTEXES = 3,
	Q1_LUMP_VISIBILITY = 4,
	Q1_LUMP_NODES = 5,
	Q1_LUMP_TEXINFO = 6,
	Q1_LUMP_FACES = 7,
	Q1_LUMP_LIGHTING = 8,
	Q1_LUMP_CLIPNODES = 9,
	Q1_LUMP_LEAFS = 10,
	Q1_LUMP_MARKSURFACES = 11,
	Q1_LUMP_EDGES = 12,
	Q1_LUMP_SURFEDGES = 13,
	Q1_LUMP_MODELS = 14,
	
  Q1_LUMP_SIZE = 15
};

struct q2bsp_header_s {
	char magic[4];
	int version;
	struct lump_s lumps[Q1_LUMP_SIZE];
};

struct q1bsp_header_s {
	int version;
	struct lump_s lumps[Q1_LUMP_SIZE];
};

#endif
