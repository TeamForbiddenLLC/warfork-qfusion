/*
Copyright (C) 2025 Warfork contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// gen_seed.c -- writes minimal but structurally valid .bsp files
//
// A fuzzer starting from random bytes almost never gets past the ident/version
// check, let alone into the per-lump parsers. These seeds give it something
// well-formed to mutate: one of each face type, a fog volume, a patch, and a
// small BSP tree, in each of the three Q3-family formats the engine accepts.
//
// Shipping real maps as a corpus is not an option - the smallest one in the
// repo is 6 MB, which is far too large for useful mutation.
//
//   gen_seed <outdir>

#include "../../../gameshared/q_arch.h"
#include "../../../gameshared/q_math.h"
#include "../../../gameshared/q_shared.h"
#include "../../../qcommon/qfiles.h"

#define NUM_VERTS	32
#define NUM_ELEMS	24
#define NUM_PLANES	8
#define NUM_LEAFS	3
#define NUM_NODES	2
#define NUM_FOGS	2
#define PATCH_CP_W	3
#define PATCH_CP_H	3

// must be a power of two; _lightmapimagesize rejects anything else
#define SEED_LIGHTMAP_SIZE	32

typedef struct
{
	uint8_t *data;
	size_t len, capacity;
} buffer_t;

static void Buf_Reserve( buffer_t *b, size_t extra )
{
	if( b->len + extra <= b->capacity )
		return;
	b->capacity = ( b->len + extra ) * 2 + 4096;
	b->data = realloc( b->data, b->capacity );
	if( !b->data )
		abort();
}

static size_t Buf_Append( buffer_t *b, const void *src, size_t size )
{
	size_t at;

	// keep every lump 4-byte aligned, matching what q3map2 emits
	while( b->len & 3 )
	{
		Buf_Reserve( b, 1 );
		b->data[b->len++] = 0;
	}

	at = b->len;
	Buf_Reserve( b, size );
	memcpy( b->data + at, src, size );
	b->len += size;
	return at;
}

static void SetLump( dheader_t *header, int lump, size_t ofs, size_t len )
{
	header->lumps[lump].fileofs = (int)ofs;
	header->lumps[lump].filelen = (int)len;
}

/*
* WriteSeed
*
* raven: emit rdface_t/rdvertex_t/rdbrushside_t and the RBSP lightgrid, i.e.
* what BSP_RAVEN selects in the loader.
*/
static void WriteSeed( const char *path, const char *ident, int version, bool raven, int lightmapSize )
{
	buffer_t buf = { 0 };
	dheader_t header;
	size_t ofs;
	int i, j;
	FILE *f;

	memset( &header, 0, sizeof( header ) );
	memcpy( &header.ident, ident, 4 );
	header.version = version;

	// the header is patched in at the end, but its space has to be claimed first
	Buf_Reserve( &buf, sizeof( header ) );
	memset( buf.data, 0, sizeof( header ) );
	buf.len = sizeof( header );

	// --- entities ---
	//
	// _lightmapimagesize overrides the format's default lightmap dimensions
	// (Mod_LoadEntities runs before Mod_LoadLighting), which keeps the seeds
	// small enough to be useful fuzzer inputs - FBSP would otherwise need a
	// 512x512 lightmap and weigh in at ~800 KB.
	{
		char ents[512];
		size_t entsLen;

		entsLen = 1 + (size_t)snprintf( ents, sizeof( ents ),
			"{\n"
			"\"classname\" \"worldspawn\"\n"
			"\"gridsize\" \"64 64 128\"\n"
			"\"_color\" \"1 1 1\"\n"
			"\"_ambient\" \"20\"\n"
			"\"_lightmapimagesize\" \"%i\"\n"
			"}\n", lightmapSize );

		ofs = Buf_Append( &buf, ents, entsLen );
		SetLump( &header, LUMP_ENTITIES, ofs, entsLen );
	}

	// --- shader refs ---
	{
		dshaderref_t shaders[2];

		memset( shaders, 0, sizeof( shaders ) );
		strcpy( shaders[0].name, "textures/fuzz/wall" );
		shaders[0].contents = CONTENTS_SOLID;
		strcpy( shaders[1].name, "textures/fuzz/fog" );
		shaders[1].contents = CONTENTS_FOG;

		ofs = Buf_Append( &buf, shaders, sizeof( shaders ) );
		SetLump( &header, LUMP_SHADERREFS, ofs, sizeof( shaders ) );
	}

	// --- planes ---
	{
		dplane_t planes[NUM_PLANES];

		memset( planes, 0, sizeof( planes ) );
		for( i = 0; i < NUM_PLANES; i++ )
		{
			planes[i].normal[i % 3] = ( i & 1 ) ? -1.0f : 1.0f;
			planes[i].dist = ( i & 1 ) ? -64.0f : 64.0f;
		}

		ofs = Buf_Append( &buf, planes, sizeof( planes ) );
		SetLump( &header, LUMP_PLANES, ofs, sizeof( planes ) );
	}

	// --- vertexes ---
	if( raven )
	{
		rdvertex_t verts[NUM_VERTS];

		memset( verts, 0, sizeof( verts ) );
		for( i = 0; i < NUM_VERTS; i++ )
		{
			verts[i].point[0] = (float)( ( i % 8 ) * 16 );
			verts[i].point[1] = (float)( ( i / 8 ) * 16 );
			verts[i].point[2] = (float)( i & 1 ? 8 : 0 );
			verts[i].normal[2] = 1.0f;
			verts[i].tex_st[0] = (float)i / NUM_VERTS;
			for( j = 0; j < MAX_LIGHTMAPS; j++ )
			{
				verts[i].lm_st[j][0] = (float)i / NUM_VERTS;
				memset( verts[i].color[j], 255, 4 );
			}
		}

		ofs = Buf_Append( &buf, verts, sizeof( verts ) );
		SetLump( &header, LUMP_VERTEXES, ofs, sizeof( verts ) );
	}
	else
	{
		dvertex_t verts[NUM_VERTS];

		memset( verts, 0, sizeof( verts ) );
		for( i = 0; i < NUM_VERTS; i++ )
		{
			verts[i].point[0] = (float)( ( i % 8 ) * 16 );
			verts[i].point[1] = (float)( ( i / 8 ) * 16 );
			verts[i].point[2] = (float)( i & 1 ? 8 : 0 );
			verts[i].normal[2] = 1.0f;
			verts[i].tex_st[0] = (float)i / NUM_VERTS;
			verts[i].lm_st[0] = (float)i / NUM_VERTS;
			memset( verts[i].color, 255, 4 );
		}

		ofs = Buf_Append( &buf, verts, sizeof( verts ) );
		SetLump( &header, LUMP_VERTEXES, ofs, sizeof( verts ) );
	}

	// --- elements ---
	{
		int elems[NUM_ELEMS];

		for( i = 0; i < NUM_ELEMS; i++ )
			elems[i] = i % NUM_VERTS;

		ofs = Buf_Append( &buf, elems, sizeof( elems ) );
		SetLump( &header, LUMP_ELEMENTS, ofs, sizeof( elems ) );
	}

	// --- faces: one planar, one patch, one trisurf, one foliage ---
	{
		struct
		{
			int facetype, firstvert, numverts, firstelem, numelems, cp0, cp1;
		} spec[] = {
			{ FACETYPE_PLANAR,  0,  8,  0, 12, 0, 0 },
			{ FACETYPE_PATCH,   8,  PATCH_CP_W * PATCH_CP_H, 0, 0, PATCH_CP_W, PATCH_CP_H },
			{ FACETYPE_TRISURF, 17, 8,  12, 12, 0, 0 },
			{ FACETYPE_FOLIAGE, 25, 0,  0,  6,  4, 4 },
		};
		const int numFaces = (int)( sizeof( spec ) / sizeof( spec[0] ) );

		if( raven )
		{
			rdface_t faces[4];

			memset( faces, 0, sizeof( faces ) );
			for( i = 0; i < numFaces; i++ )
			{
				faces[i].shadernum = 0;
				faces[i].fognum = -1;
				faces[i].facetype = spec[i].facetype;
				faces[i].firstvert = spec[i].firstvert;
				faces[i].numverts = spec[i].numverts;
				faces[i].firstelem = spec[i].firstelem;
				faces[i].numelems = spec[i].numelems;
				faces[i].patch_cp[0] = spec[i].cp0;
				faces[i].patch_cp[1] = spec[i].cp1;
				faces[i].normal[2] = 1.0f;
				faces[i].maxs[0] = faces[i].maxs[1] = faces[i].maxs[2] = 64.0f;
				for( j = 0; j < MAX_LIGHTMAPS; j++ )
				{
					faces[i].lm_texnum[j] = ( j == 0 ) ? 0 : -1;
					faces[i].lightmapStyles[j] = ( j == 0 ) ? 0 : 255;
					faces[i].vertexStyles[j] = ( j == 0 ) ? 0 : 255;
				}
			}

			ofs = Buf_Append( &buf, faces, sizeof( faces ) );
			SetLump( &header, LUMP_FACES, ofs, sizeof( faces ) );
		}
		else
		{
			dface_t faces[4];

			memset( faces, 0, sizeof( faces ) );
			for( i = 0; i < numFaces; i++ )
			{
				faces[i].shadernum = 0;
				faces[i].fognum = -1;
				faces[i].facetype = spec[i].facetype;
				faces[i].firstvert = spec[i].firstvert;
				faces[i].numverts = spec[i].numverts;
				faces[i].firstelem = spec[i].firstelem;
				faces[i].numelems = spec[i].numelems;
				faces[i].patch_cp[0] = spec[i].cp0;
				faces[i].patch_cp[1] = spec[i].cp1;
				faces[i].normal[2] = 1.0f;
				faces[i].maxs[0] = faces[i].maxs[1] = faces[i].maxs[2] = 64.0f;
				faces[i].lm_texnum = 0;
			}

			ofs = Buf_Append( &buf, faces, sizeof( faces ) );
			SetLump( &header, LUMP_FACES, ofs, sizeof( faces ) );
		}
	}

	// --- brushes and brush sides (fog volumes reference these) ---
	{
		dbrush_t brushes[NUM_FOGS];

		memset( brushes, 0, sizeof( brushes ) );
		for( i = 0; i < NUM_FOGS; i++ )
		{
			brushes[i].firstside = i * 6;
			brushes[i].numsides = 6;
			brushes[i].shadernum = 1;
		}

		ofs = Buf_Append( &buf, brushes, sizeof( brushes ) );
		SetLump( &header, LUMP_BRUSHES, ofs, sizeof( brushes ) );
	}

	if( raven )
	{
		rdbrushside_t sides[NUM_FOGS * 6];

		memset( sides, 0, sizeof( sides ) );
		for( i = 0; i < NUM_FOGS * 6; i++ )
		{
			sides[i].planenum = i % NUM_PLANES;
			sides[i].shadernum = 1;
		}

		ofs = Buf_Append( &buf, sides, sizeof( sides ) );
		SetLump( &header, LUMP_BRUSHSIDES, ofs, sizeof( sides ) );
	}
	else
	{
		dbrushside_t sides[NUM_FOGS * 6];

		memset( sides, 0, sizeof( sides ) );
		for( i = 0; i < NUM_FOGS * 6; i++ )
		{
			sides[i].planenum = i % NUM_PLANES;
			sides[i].shadernum = 1;
		}

		ofs = Buf_Append( &buf, sides, sizeof( sides ) );
		SetLump( &header, LUMP_BRUSHSIDES, ofs, sizeof( sides ) );
	}

	// --- fogs ---
	{
		dfog_t fogs[NUM_FOGS];

		memset( fogs, 0, sizeof( fogs ) );
		for( i = 0; i < NUM_FOGS; i++ )
		{
			strcpy( fogs[i].shader, "textures/fuzz/fog" );
			fogs[i].brushnum = i;
			fogs[i].visibleside = 0;
		}

		ofs = Buf_Append( &buf, fogs, sizeof( fogs ) );
		SetLump( &header, LUMP_FOGS, ofs, sizeof( fogs ) );
	}

	// --- leaf faces ---
	{
		int leaffaces[4] = { 0, 1, 2, 3 };

		ofs = Buf_Append( &buf, leaffaces, sizeof( leaffaces ) );
		SetLump( &header, LUMP_LEAFFACES, ofs, sizeof( leaffaces ) );
	}

	// --- leaf brushes ---
	{
		int leafbrushes[NUM_FOGS] = { 0, 1 };

		ofs = Buf_Append( &buf, leafbrushes, sizeof( leafbrushes ) );
		SetLump( &header, LUMP_LEAFBRUSHES, ofs, sizeof( leafbrushes ) );
	}

	// --- leafs ---
	{
		dleaf_t leafs[NUM_LEAFS];

		memset( leafs, 0, sizeof( leafs ) );
		for( i = 0; i < NUM_LEAFS; i++ )
		{
			leafs[i].cluster = i - 1;   // leaf 0 is the solid leaf
			leafs[i].area = 0;
			leafs[i].mins[0] = leafs[i].mins[1] = leafs[i].mins[2] = -128;
			leafs[i].maxs[0] = leafs[i].maxs[1] = leafs[i].maxs[2] = 128;
			leafs[i].firstleafface = 0;
			leafs[i].numleaffaces = 4;
			leafs[i].firstleafbrush = 0;
			leafs[i].numleafbrushes = NUM_FOGS;
		}

		ofs = Buf_Append( &buf, leafs, sizeof( leafs ) );
		SetLump( &header, LUMP_LEAFS, ofs, sizeof( leafs ) );
	}

	// --- nodes ---
	{
		dnode_t nodes[NUM_NODES];

		memset( nodes, 0, sizeof( nodes ) );
		// node 0 splits into node 1 and leaf 0; node 1 into leaf 1 and leaf 2
		nodes[0].planenum = 0;
		nodes[0].children[0] = 1;
		nodes[0].children[1] = -1;
		nodes[1].planenum = 1;
		nodes[1].children[0] = -2;
		nodes[1].children[1] = -3;
		for( i = 0; i < NUM_NODES; i++ )
		{
			nodes[i].mins[0] = nodes[i].mins[1] = nodes[i].mins[2] = -128;
			nodes[i].maxs[0] = nodes[i].maxs[1] = nodes[i].maxs[2] = 128;
		}

		ofs = Buf_Append( &buf, nodes, sizeof( nodes ) );
		SetLump( &header, LUMP_NODES, ofs, sizeof( nodes ) );
	}

	// --- submodels ---
	{
		dmodel_t models[1];

		memset( models, 0, sizeof( models ) );
		models[0].mins[0] = models[0].mins[1] = models[0].mins[2] = -128.0f;
		models[0].maxs[0] = models[0].maxs[1] = models[0].maxs[2] = 128.0f;
		models[0].firstface = 0;
		models[0].numfaces = 4;
		models[0].firstbrush = 0;
		models[0].numbrushes = NUM_FOGS;

		ofs = Buf_Append( &buf, models, sizeof( models ) );
		SetLump( &header, LUMP_MODELS, ofs, sizeof( models ) );
	}

	// --- lighting: one lightmap, sized by the format descriptor ---
	{
		size_t size = (size_t)lightmapSize * lightmapSize * LIGHTMAP_BYTES;
		uint8_t *lm = malloc( size );

		if( !lm )
			abort();
		memset( lm, 0x40, size );
		ofs = Buf_Append( &buf, lm, size );
		SetLump( &header, LUMP_LIGHTING, ofs, size );
		free( lm );
	}

	// --- light grid ---
	if( raven )
	{
		rdgridlight_t grid[4];

		memset( grid, 0, sizeof( grid ) );
		for( i = 0; i < 4; i++ )
			memset( grid[i].ambient, 0x20, sizeof( grid[i].ambient ) );

		ofs = Buf_Append( &buf, grid, sizeof( grid ) );
		SetLump( &header, LUMP_LIGHTGRID, ofs, sizeof( grid ) );

		// RBSP indirects the grid through a light array
		{
			unsigned short lightarray[4] = { 0, 1, 2, 3 };
			ofs = Buf_Append( &buf, lightarray, sizeof( lightarray ) );
			SetLump( &header, LUMP_LIGHTARRAY, ofs, sizeof( lightarray ) );
		}
	}
	else
	{
		dgridlight_t grid[4];

		memset( grid, 0, sizeof( grid ) );
		for( i = 0; i < 4; i++ )
			memset( grid[i].ambient, 0x20, sizeof( grid[i].ambient ) );

		ofs = Buf_Append( &buf, grid, sizeof( grid ) );
		SetLump( &header, LUMP_LIGHTGRID, ofs, sizeof( grid ) );
	}

	// --- visibility ---
	{
		struct
		{
			int numclusters;
			int rowsize;
			uint8_t data[2];
		} vis;

		vis.numclusters = 2;
		vis.rowsize = 1;
		vis.data[0] = 0xff;
		vis.data[1] = 0xff;

		ofs = Buf_Append( &buf, &vis, sizeof( vis ) );
		SetLump( &header, LUMP_VISIBILITY, ofs, sizeof( vis ) );
	}

	memcpy( buf.data, &header, sizeof( header ) );

	f = fopen( path, "wb" );
	if( !f )
	{
		fprintf( stderr, "gen_seed: cannot write %s\n", path );
		exit( 1 );
	}
	fwrite( buf.data, 1, buf.len, f );
	fclose( f );
	free( buf.data );

	printf( "gen_seed: wrote %s (%u bytes)\n", path, (unsigned)buf.len );
}

int main( int argc, char **argv )
{
	const char *outdir;
	char path[1024];

	// this lands in the same output directory as the test binaries, and CI
	// execs everything it finds there - so do nothing unless asked
	if( argc < 2 )
	{
		printf( "usage: gen_bsp_seed <outdir>\n" );
		return 0;
	}
	outdir = argv[1];

	// the three entries of q3BSPFormats in qcommon/bsp.c
	snprintf( path, sizeof( path ), "%s/seed_ibsp.bsp", outdir );
	WriteSeed( path, IDBSPHEADER, Q3BSPVERSION, false, SEED_LIGHTMAP_SIZE );

	snprintf( path, sizeof( path ), "%s/seed_rbsp.bsp", outdir );
	WriteSeed( path, RBSPHEADER, RBSPVERSION, true, SEED_LIGHTMAP_SIZE );

	snprintf( path, sizeof( path ), "%s/seed_fbsp.bsp", outdir );
	WriteSeed( path, QFBSPHEADER, QFBSPVERSION, true, SEED_LIGHTMAP_SIZE );

	return 0;
}
