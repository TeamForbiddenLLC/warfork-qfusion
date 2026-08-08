/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cm_q3bsp.c -- Q3 BSP model loading

#include "qcommon.h"
#include "cm_local.h"
#include "patch.h"

#define MAX_FACET_PLANES 32

// per-axis limit on a tessellated patch, well above anything q3map2 emits
#define CM_MAX_PATCH_SIZE 256

/*
* CM_CreateFacetFromPoints
*/
static int CM_CreateFacetFromPoints( cmodel_state_t *cms, cbrush_t *facet, vec3_t *verts, int numverts, cshaderref_t *shaderref, cplane_t *brushplanes )
{
	int i, j;
	int axis, dir;
	vec3_t normal, mins, maxs;
	float d, dist;
	cplane_t mainplane;
	vec3_t vec, vec2;
	int numbrushplanes;

	// set default values for brush
	facet->numsides = 0;
	facet->brushsides = NULL;
	facet->contents = shaderref->contents;

	// calculate plane for this triangle
	PlaneFromPoints( verts, &mainplane );
	if( ComparePlanes( mainplane.normal, mainplane.dist, vec3_origin, 0 ) )
		return 0;

	// test a quad case
	if( numverts > 3 )
	{
		d = DotProduct( verts[3], mainplane.normal ) - mainplane.dist;
		if( d < -0.1 || d > 0.1 )
			return 0;

		if( 0 )
		{
			vec3_t v[3];
			cplane_t plane;

			// try different combinations of planes
			for( i = 1; i < 4; i++ )
			{
				VectorCopy( verts[i], v[0] );
				VectorCopy( verts[( i+1 )%4], v[1] );
				VectorCopy( verts[( i+2 )%4], v[2] );
				PlaneFromPoints( v, &plane );

				if( fabs( DotProduct( mainplane.normal, plane.normal ) ) < 0.9 )
					return 0;
			}
		}
	}

	numbrushplanes = 0;

	// add front plane
	SnapPlane( mainplane.normal, &mainplane.dist );
	VectorCopy( mainplane.normal, brushplanes[numbrushplanes].normal );
	brushplanes[numbrushplanes].dist = mainplane.dist; numbrushplanes++;

	// calculate mins & maxs
	ClearBounds( mins, maxs );
	for( i = 0; i < numverts; i++ )
		AddPointToBounds( verts[i], mins, maxs );

	// add the axial planes
	for( axis = 0; axis < 3; axis++ )
	{
		for( dir = -1; dir <= 1; dir += 2 )
		{
			for( i = 0; i < numbrushplanes; i++ )
			{
				if( brushplanes[i].normal[axis] == dir )
					break;
			}

			if( i == numbrushplanes )
			{
				VectorClear( normal );
				normal[axis] = dir;
				if( dir == 1 )
					dist = maxs[axis];
				else
					dist = -mins[axis];

				VectorCopy( normal, brushplanes[numbrushplanes].normal );
				brushplanes[numbrushplanes].dist = dist; numbrushplanes++;
			}
		}
	}

	// add the edge bevels
	for( i = 0; i < numverts; i++ )
	{
		j = ( i + 1 ) % numverts;

		VectorSubtract( verts[i], verts[j], vec );
		if( VectorNormalize( vec ) < 0.5 )
			continue;

		SnapVector( vec );
		for( j = 0; j < 3; j++ )
		{
			if( vec[j] == 1 || vec[j] == -1 )
				break; // axial
		}
		if( j != 3 )
			continue; // only test non-axial edges

		// try the six possible slanted axials from this edge
		for( axis = 0; axis < 3; axis++ )
		{
			for( dir = -1; dir <= 1; dir += 2 )
			{
				// construct a plane
				VectorClear( vec2 );
				vec2[axis] = dir;
				CrossProduct( vec, vec2, normal );
				if( VectorNormalize( normal ) < 0.5 )
					continue;
				dist = DotProduct( verts[i], normal );

				for( j = 0; j < numbrushplanes; j++ )
				{
					// if this plane has already been used, skip it
					if( ComparePlanes( brushplanes[j].normal, brushplanes[j].dist, normal, dist ) )
						break;
				}
				if( j != numbrushplanes )
					continue;

				// if all other points are behind this plane, it is a proper edge bevel
				for( j = 0; j < numverts; j++ )
				{
					if( j != i )
					{
						d = DotProduct( verts[j], normal ) - dist;
						if( d > 0.1 )
							break; // point in front: this plane isn't part of the outer hull
					}
				}
				if( j != numverts )
					continue;

				// add this plane
				VectorCopy( normal, brushplanes[numbrushplanes].normal );
				brushplanes[numbrushplanes].dist = dist; numbrushplanes++;
				if( numbrushplanes == MAX_FACET_PLANES )
					break;
			}
		}
	}

	return ( facet->numsides = numbrushplanes );
}

/*
* CM_CreatePatch
*/
static void CM_CreatePatch( cmodel_state_t *cms, cface_t *patch, cshaderref_t *shaderref, vec3_t *verts, int *patch_cp )
{
	int step[2], size[2], flat[2];
	vec3_t *patchpoints;
	int i, j, k ,u, v;
	int numsides, totalsides;
	cbrush_t *facets, *facet;
	vec3_t *points;
	vec3_t tverts[4];
	uint8_t *data;
	cplane_t *brushplanes;

	// find the degree of subdivision in the u and v directions
	Patch_GetFlatness( CM_SUBDIV_LEVEL, ( vec_t * )verts[0], 3, patch_cp, flat );

	step[0] = 1 << flat[0];
	step[1] = 1 << flat[1];
	size[0] = ( patch_cp[0] >> 1 ) * step[0] + 1;
	size[1] = ( patch_cp[1] >> 1 ) * step[1] + 1;
	if( size[0] <= 0 || size[1] <= 0 )
		return;

	// the allocation below is quadratic in the tessellated size and each facet
	// costs 32 planes, so an adversarial control grid turns one patch into
	// gigabytes of work
	if( size[0] > CM_MAX_PATCH_SIZE || size[1] > CM_MAX_PATCH_SIZE )
		return;

	patchpoints = Mem_TempMalloc( size[0] * size[1] * sizeof( vec3_t ) );
	Patch_Evaluate( vec_t, 3, verts[0], patch_cp, step, patchpoints[0], 0 );
	Patch_RemoveLinearColumnsRows( patchpoints[0], 3, &size[0], &size[1], 0, NULL, NULL );

	// vec3_t and cbrush_t are both 4-byte sized/aligned but cbrush_t holds a
	// pointer, so each sub-array has to start on a pointer boundary
	{
		size_t pointsSize = ALIGN( size[0] * size[1] * sizeof( vec3_t ), sizeof( void * ) );
		size_t facetsSize = ALIGN( ( size[0]-1 ) * ( size[1]-1 ) * 2 * sizeof( cbrush_t ), sizeof( void * ) );
		size_t planesSize = ( size[0]-1 ) * ( size[1]-1 ) * 2 * MAX_FACET_PLANES * sizeof( cplane_t );

		data = Mem_Alloc( cms->mempool, pointsSize + facetsSize + planesSize );

		points = ( vec3_t * )data; data += pointsSize;
		facets = ( cbrush_t * )data; data += facetsSize;
		brushplanes = ( cplane_t * )data;
	}

	// fill in
	memcpy( points, patchpoints, size[0] * size[1] * sizeof( vec3_t ) );
	Mem_TempFree( patchpoints );

	totalsides = 0;
	patch->numfacets = 0;
	patch->facets = NULL;
	ClearBounds( patch->mins, patch->maxs );

	// create a set of facets
	for( v = 0; v < size[1]-1; v++ )
	{
		for( u = 0; u < size[0]-1; u++ )
		{
			i = v * size[0] + u;
			VectorCopy( points[i], tverts[0] );
			VectorCopy( points[i + size[0]], tverts[1] );
			VectorCopy( points[i + size[0] + 1], tverts[2] );
			VectorCopy( points[i + 1], tverts[3] );

			for( i = 0; i < 4; i++ )
				AddPointToBounds( tverts[i], patch->mins, patch->maxs );

			// try to create one facet from a quad
			numsides = CM_CreateFacetFromPoints( cms, &facets[patch->numfacets], tverts, 4, shaderref, brushplanes + totalsides );
			if( !numsides )
			{	// create two facets from triangles
				VectorCopy( tverts[3], tverts[2] );
				numsides = CM_CreateFacetFromPoints( cms, &facets[patch->numfacets], tverts, 3, shaderref, brushplanes + totalsides );
				if( numsides )
				{
					totalsides += numsides;
					patch->numfacets++;
				}

				VectorCopy( tverts[2], tverts[0] );
				VectorCopy( points[v *size[0] + u + size[0] + 1], tverts[2] );
				numsides = CM_CreateFacetFromPoints( cms, &facets[patch->numfacets], tverts, 3, shaderref, brushplanes + totalsides );
			}

			if( numsides )
			{
				totalsides += numsides;
				patch->numfacets++;
			}
		}
	}

	if( patch->numfacets )
	{
		uint8_t *data;

		// cbrush_t is all ints, but cbrushside_t holds a pointer, so the facet
		// array has to be padded out or every side access is misaligned
		size_t facetsSize = ALIGN( patch->numfacets * sizeof( cbrush_t ), sizeof( void * ) );

		// keep the sides and planes in two contiguous runs rather than
		// interleaving them per facet: cplane_t is only 4-aligned, so an
		// interleaved layout leaves every other cbrushside_t misaligned
		size_t sidesSize = ALIGN( totalsides * sizeof( cbrushside_t ), sizeof( void * ) );
		cbrushside_t *sides;
		cplane_t *planes;

		data = Mem_Alloc( cms->mempool, facetsSize + sidesSize + totalsides * sizeof( cplane_t ) );

		patch->facets = ( cbrush_t * )data;
		sides = ( cbrushside_t * )( data + facetsSize );
		planes = ( cplane_t * )( data + facetsSize + sidesSize );

		memcpy( patch->facets, facets, patch->numfacets * sizeof( cbrush_t ) );
		for( i = 0, k = 0, facet = patch->facets; i < patch->numfacets; i++, facet++ )
		{
			cbrushside_t *s;

			facet->brushsides = sides;

			for( j = 0, s = sides; j < facet->numsides; j++, s++ )
			{
				planes[j] = brushplanes[k++];

				s->plane = &planes[j];
				SnapPlane( s->plane->normal, &s->plane->dist );
				CategorizePlane( s->plane );
				s->surfFlags = shaderref->flags;
			}

			sides += facet->numsides;
			planes += facet->numsides;
		}

		patch->contents = shaderref->contents;

		for( i = 0; i < 3; i++ )
		{
			// spread the mins / maxs by a pixel
			patch->mins[i] -= 1;
			patch->maxs[i] += 1;
		}
	}

	Mem_Free( points );
}

/*
===============================================================================

MAP LOADING

===============================================================================
*/

/*
* CMod_LumpCount
*
* Validates a lump against the size of the file it came from and returns the
* number of elemSize-sized records it holds. The .bsp is attacker-controlled
* (the map is whatever the server hands out), so no lump offset may be turned
* into a pointer before it has been range-checked.
*/
static int CMod_LumpCount( cmodel_state_t *cms, const lump_t *l, size_t elemSize, const char *lumpName )
{
	if( elemSize == 0 )
		Com_Error( ERR_DROP, "CMod_LumpCount: zero-sized %s record", lumpName );

	if( l->fileofs < 0 || l->filelen < 0 )
		Com_Error( ERR_DROP, "CMod_LumpCount: negative %s lump (ofs %i, len %i)",
			lumpName, l->fileofs, l->filelen );

	// written as a subtraction so that fileofs + filelen cannot itself overflow
	if( (size_t)l->fileofs > cms->cmod_length || (size_t)l->filelen > cms->cmod_length - (size_t)l->fileofs )
		Com_Error( ERR_DROP, "CMod_LumpCount: %s lump out of bounds (ofs %i, len %i, file %u)",
			lumpName, l->fileofs, l->filelen, (unsigned)cms->cmod_length );


	// every record in these lumps is built from 4-byte ints and floats, and the
	// loader reads them by casting mod_base+fileofs straight to a struct
	// pointer. q3map2 always aligns its lumps; a hostile file need not, and an
	// unaligned cast is undefined behaviour (and a fault on strict-alignment
	// targets).
	if( elemSize > 1 )
	{
		size_t align = elemSize >= 4 ? 4 : elemSize;
		if( (size_t)l->fileofs & ( align - 1 ) )
			Com_Error( ERR_DROP, "CMod_LumpCount: %s lump offset %i is misaligned", lumpName, l->fileofs );
	}

	if( l->filelen % elemSize )
		Com_Error( ERR_DROP, "CMod_LumpCount: funny %s lump size", lumpName );

	return l->filelen / (int)elemSize;
}

/*
* CMod_LumpCountMin
*
* As CMod_LumpCount, but rejects an empty lump and counts above the format limit.
*/
static int CMod_LumpCountMin( cmodel_state_t *cms, const lump_t *l, size_t elemSize, const char *lumpName, int maxCount )
{
	int count = CMod_LumpCount( cms, l, elemSize, lumpName );
	if( count < 1 )
		Com_Error( ERR_DROP, "CMod_LumpCount: map with no %s", lumpName );
	if( count > maxCount )
		Com_Error( ERR_DROP, "CMod_LumpCount: too many %s (%i > %i)", lumpName, count, maxCount );
	return count;
}

/*
* CMod_CheckFloat
*
* Rejects NaN and +-infinity. Tested against the bit pattern rather than with
* isfinite() because release builds are compiled with -ffast-math, which lets
* the compiler assume no NaNs exist and fold the usual predicates away.
*
* Non-finite geometry is not merely ugly: Patch_FlatnessTest subdivides until a
* segment is flat enough and every comparison against a NaN is false, and
* SnapPlane feeds the value to Q_rint, whose float-to-int conversion is
* undefined outside the representable range.
*/
static inline float CMod_CheckFloat( float f )
{
	uint32_t u;

	memcpy( &u, &f, sizeof( u ) );
	if( ( u & 0x7f800000u ) == 0x7f800000u )
		Com_Error( ERR_DROP, "CM_LoadQ3BrushModel: non-finite value in map geometry" );
	return f;
}

/*
* CMod_LumpData
*/
static inline void *CMod_LumpData( cmodel_state_t *cms, const lump_t *l )
{
	return cms->cmod_base + l->fileofs;
}

/*
* CMod_LoadSurfaces
*/
static void CMod_LoadSurfaces( cmodel_state_t *cms, lump_t *l )
{
	int i;
	int count;
	char *buffer;
	size_t len, bufLen, bufSize;
	dshaderref_t *in;
	cshaderref_t *out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "shaders", MAX_MAP_FACES );
	in = CMod_LumpData( cms, l );

	out = cms->map_shaderrefs = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numshaderrefs = count;

	buffer = NULL;
	bufLen = bufSize = 0;

	for( i = 0; i < count; i++, in++, out++, bufLen += len + 1 )
	{
		// in->name is a fixed-size field straight out of the file and carries no
		// guaranteed terminator, so measure it without running off the end
		for( len = 0; len < sizeof( in->name ) && in->name[len]; len++ ) ;
		if( bufLen + len >= bufSize )
		{
			bufSize = bufLen + len + 128;
			if( buffer )
				buffer = Mem_Realloc( buffer, bufSize );
			else
				buffer = Mem_Alloc( cms->mempool, bufSize );
		}

		// Vic: ZOMG, this is so nasty, perfectly valid in C though
		out->name = ( char * )( ( void * )bufLen );
		memcpy( buffer + bufLen, in->name, len );
		buffer[bufLen + len] = '\0';
		out->flags = LittleLong( in->flags );
		out->contents = LittleLong( in->contents );
	}

	for( i = 0; i < count; i++ )
		cms->map_shaderrefs[i].name = buffer + ( size_t )( ( void * )cms->map_shaderrefs[i].name );
}

/*
* CMod_LoadVertexes
*/
static void CMod_LoadVertexes( cmodel_state_t *cms, lump_t *l )
{
	int i;
	int count;
	dvertex_t *in;
	vec3_t *out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "vertexes", MAX_MAP_VERTEXES );
	in = CMod_LumpData( cms, l );

	out = cms->map_verts = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numvertexes = count;

	for( i = 0; i < count; i++, in++ )
	{
		out[i][0] = CMod_CheckFloat( LittleFloat( in->point[0] ) );
		out[i][1] = CMod_CheckFloat( LittleFloat( in->point[1] ) );
		out[i][2] = CMod_CheckFloat( LittleFloat( in->point[2] ) );
	}
}

/*
* CMod_LoadVertexes_RBSP
*/
static void CMod_LoadVertexes_RBSP( cmodel_state_t *cms, lump_t *l )
{
	int i;
	int count;
	rdvertex_t *in;
	vec3_t *out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "vertexes", MAX_MAP_VERTEXES );
	in = CMod_LumpData( cms, l );

	out = cms->map_verts = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numvertexes = count;

	for( i = 0; i < count; i++, in++ )
	{
		out[i][0] = CMod_CheckFloat( LittleFloat( in->point[0] ) );
		out[i][1] = CMod_CheckFloat( LittleFloat( in->point[1] ) );
		out[i][2] = CMod_CheckFloat( LittleFloat( in->point[2] ) );
	}
}

/*
* CMod_LoadFace
*/
static inline void CMod_LoadFace( cmodel_state_t *cms, cface_t *out, int shadernum, int firstvert, int numverts, int *patch_cp )
{
	cshaderref_t *shaderref;

	shadernum = LittleLong( shadernum );
	if( shadernum < 0 || shadernum >= cms->numshaderrefs )
		return;

	shaderref = &cms->map_shaderrefs[shadernum];
	if( !shaderref->contents || ( shaderref->flags & SURF_NONSOLID ) )
		return;

	patch_cp[0] = LittleLong( patch_cp[0] );
	patch_cp[1] = LittleLong( patch_cp[1] );
	if( patch_cp[0] <= 0 || patch_cp[1] <= 0 )
		return;

	firstvert = LittleLong( firstvert );
	if( numverts <= 0 || firstvert < 0 || firstvert >= cms->numvertexes )
		return;

	// CM_CreatePatch reads the full patch_cp[0]*patch_cp[1] control grid.
	// Bezier grids are 2n+1 on each axis: Patch_GetFlatness and Patch_Evaluate
	// step by two and read p+2*cp[0]+2, which is the last control point when
	// both dimensions are odd and a full row past the end when they are not.
	if( patch_cp[0] < 3 || patch_cp[1] < 3 ||
		!( patch_cp[0] & 1 ) || !( patch_cp[1] & 1 ) ||
		patch_cp[0] > MAX_PATCH_CP || patch_cp[1] > MAX_PATCH_CP )
		return;
	if( patch_cp[0] * patch_cp[1] > cms->numvertexes - firstvert )
		return;

	CM_CreatePatch( cms, out, shaderref, cms->map_verts + firstvert, patch_cp );
}

/*
* CMod_LoadFaces
*/
static void CMod_LoadFaces( cmodel_state_t *cms, lump_t *l )
{
	int i, count;
	dface_t	*in;
	cface_t	*out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "faces", MAX_MAP_FACES );
	in = CMod_LumpData( cms, l );

	out = cms->map_faces = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numfaces = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->contents = 0;
		out->numfacets = 0;
		out->facets = NULL;
		if( LittleLong( in->facetype ) != FACETYPE_PATCH )
			continue;
		CMod_LoadFace( cms, out, in->shadernum, in->firstvert, in->numverts, in->patch_cp );
	}
}

/*
* CMod_LoadFaces_RBSP
*/
static void CMod_LoadFaces_RBSP( cmodel_state_t *cms, lump_t *l )
{
	int i, count;
	rdface_t *in;
	cface_t	*out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "faces", MAX_MAP_FACES );
	in = CMod_LumpData( cms, l );

	out = cms->map_faces = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numfaces = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->contents = 0;
		out->numfacets = 0;
		out->facets = NULL;
		if( LittleLong( in->facetype ) != FACETYPE_PATCH )
			continue;
		CMod_LoadFace( cms, out, in->shadernum, in->firstvert, in->numverts, in->patch_cp );
	}
}

/*
* CMod_LoadSubmodels
*/
static void CMod_LoadSubmodels( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	dmodel_t *in;
	cmodel_t *out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "models", MAX_MAP_SUBMODELS );
	in = CMod_LumpData( cms, l );

	out = cms->map_cmodels = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numcmodels = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		int firstface = LittleLong( in->firstface );
		int firstbrush = LittleLong( in->firstbrush );

		out->nummarkfaces = LittleLong( in->numfaces );
		out->nummarkbrushes = LittleLong( in->numbrushes );

		if( out->nummarkfaces < 0 || firstface < 0 || out->nummarkfaces > cms->numfaces - firstface )
			Com_Error( ERR_DROP, "CMod_LoadSubmodels: submodel %i has bad face range (first %i, count %i, of %i)",
				i, firstface, out->nummarkfaces, cms->numfaces );
		if( out->nummarkbrushes < 0 || firstbrush < 0 || out->nummarkbrushes > cms->numbrushes - firstbrush )
			Com_Error( ERR_DROP, "CMod_LoadSubmodels: submodel %i has bad brush range (first %i, count %i, of %i)",
				i, firstbrush, out->nummarkbrushes, cms->numbrushes );

		out->markfaces = Mem_Alloc( cms->mempool, out->nummarkfaces * sizeof( cface_t * ) );
		out->markbrushes = Mem_Alloc( cms->mempool, out->nummarkbrushes * sizeof( cbrush_t * ) );

		for( j = 0; j < out->nummarkfaces; j++ )
			out->markfaces[j] = cms->map_faces + firstface + j;
		for( j = 0; j < out->nummarkbrushes; j++ )
			out->markbrushes[j] = cms->map_brushes + firstbrush + j;

		for( j = 0; j < 3; j++ )
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat( in->mins[j] ) - 1;
			out->maxs[j] = LittleFloat( in->maxs[j] ) + 1;
		}
	}
}

/*
* CMod_LoadNodes
*/
static void CMod_LoadNodes( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	dnode_t	*in;
	cnode_t	*out;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "nodes", MAX_MAP_NODES );
	in = CMod_LumpData( cms, l );

	out = cms->map_nodes = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numnodes = count;

	for( i = 0; i < 3; i++ )
	{
		cms->world_mins[i] = LittleFloat( in->mins[i] );
		cms->world_maxs[i] = LittleFloat( in->maxs[i] );
	}

	for( i = 0; i < count; i++, out++, in++ )
	{
		int planenum = LittleLong( in->planenum );

		if( planenum < 0 || planenum >= cms->numplanes )
			Com_Error( ERR_DROP, "CMod_LoadNodes: node %i has bad plane %i", i, planenum );
		out->plane = cms->map_planes + planenum;

		for( j = 0; j < 2; j++ )
		{
			int p = LittleLong( in->children[j] );

			// negative numbers are -(leafs+1); leafs are loaded before nodes so
			// both sides can be checked here
			if( p >= 0 )
			{
				if( p >= cms->numnodes )
					Com_Error( ERR_DROP, "CMod_LoadNodes: node %i has bad child node %i", i, p );
			}
			else
			{
				if( p == INT_MIN || ( -1 - p ) >= cms->numleafs )
					Com_Error( ERR_DROP, "CMod_LoadNodes: node %i has bad child leaf %i", i, -1 - p );
			}
			out->children[j] = p;
		}
	}
}

/*
* CMod_LoadMarkFaces
*/
static void CMod_LoadMarkFaces( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	cface_t	**out;
	int *in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "leaffaces", MAX_MAP_LEAFFACES );
	in = CMod_LumpData( cms, l );

	out = cms->map_markfaces = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->nummarkfaces = count;

	for( i = 0; i < count; i++ )
	{
		j = LittleLong( in[i] );
		if( j < 0 || j >= cms->numfaces )
			Com_Error( ERR_DROP, "CMod_LoadMarkFaces: bad surface number" );
		out[i] = cms->map_faces + j;
	}
}

/*
* CMod_LoadLeafs
*/
static void CMod_LoadLeafs( cmodel_state_t *cms, lump_t *l )
{
	int i, j, k;
	int count;
	cleaf_t	*out;
	dleaf_t	*in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "leafs", MAX_MAP_LEAFS );
	in = CMod_LumpData( cms, l );

	out = cms->map_leafs = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numleafs = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		int firstleafbrush = LittleLong( in->firstleafbrush );
		int firstleafface = LittleLong( in->firstleafface );

		out->contents = 0;
		out->cluster = LittleLong( in->cluster );
		out->area = LittleLong( in->area );
		out->nummarkbrushes = LittleLong( in->numleafbrushes );
		out->nummarkfaces = LittleLong( in->numleaffaces );

		if( out->nummarkbrushes < 0 || firstleafbrush < 0 ||
			out->nummarkbrushes > cms->nummarkbrushes - firstleafbrush )
			Com_Error( ERR_DROP, "CMod_LoadLeafs: leaf %i has bad leafbrush range (first %i, count %i, of %i)",
				i, firstleafbrush, out->nummarkbrushes, cms->nummarkbrushes );
		if( out->nummarkfaces < 0 || firstleafface < 0 ||
			out->nummarkfaces > cms->nummarkfaces - firstleafface )
			Com_Error( ERR_DROP, "CMod_LoadLeafs: leaf %i has bad leafface range (first %i, count %i, of %i)",
				i, firstleafface, out->nummarkfaces, cms->nummarkfaces );

		// -1 means "no area"; only the upper bound feeds cms->numareas, which
		// sizes the area/areaportal tables
		if( out->area >= MAX_MAP_AREAS )
			Com_Error( ERR_DROP, "CMod_LoadLeafs: leaf %i has bad area %i", i, out->area );

		out->markbrushes = cms->map_markbrushes + firstleafbrush;
		out->markfaces = cms->map_markfaces + firstleafface;

		// OR brushes' contents
		for( j = 0; j < out->nummarkbrushes; j++ )
			out->contents |= out->markbrushes[j]->contents;

		// exclude markfaces that have no facets
		// so we don't perform this check at runtime
		for( j = 0; j < out->nummarkfaces; )
		{
			k = j;
			if( !out->markfaces[j]->facets )
			{
				for(; ( ++j < out->nummarkfaces ) && !out->markfaces[j]->facets; ) ;
				if( j < out->nummarkfaces )
					memmove( &out->markfaces[k], &out->markfaces[j], ( out->nummarkfaces - j ) * sizeof( *out->markfaces ) );
				out->nummarkfaces -= j - k;

			}
			j = k + 1;
		}

		// OR patches' contents
		for( j = 0; j < out->nummarkfaces; j++ )
			out->contents |= out->markfaces[j]->contents;

		if( out->area >= cms->numareas )
			cms->numareas = out->area + 1;
	}
}

/*
* CMod_LoadPlanes
*/
static void CMod_LoadPlanes( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	cplane_t *out;
	dplane_t *in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "planes", MAX_MAP_PLANES );
	in = CMod_LumpData( cms, l );

	out = cms->map_planes = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numplanes = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->signbits = 0;
		out->type = PLANE_NONAXIAL;

		for( j = 0; j < 3; j++ )
		{
			out->normal[j] = CMod_CheckFloat( LittleFloat( in->normal[j] ) );
			if( out->normal[j] < 0 )
				out->signbits |= ( 1 << j );
			if( out->normal[j] == 1.0f )
				out->type = j;
		}

		out->dist = CMod_CheckFloat( LittleFloat( in->dist ) );
	}
}

/*
* CMod_LoadMarkBrushes
*/
static void CMod_LoadMarkBrushes( cmodel_state_t *cms, lump_t *l )
{
	int i;
	int count;
	cbrush_t **out;
	int *in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "leafbrushes", MAX_MAP_LEAFBRUSHES );
	in = CMod_LumpData( cms, l );

	out = cms->map_markbrushes = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->nummarkbrushes = count;

	for( i = 0; i < count; i++, in++ )
	{
		int j = LittleLong( *in );

		if( j < 0 || j >= cms->numbrushes )
			Com_Error( ERR_DROP, "CMod_LoadMarkBrushes: bad brush number %i", j );
		out[i] = cms->map_brushes + j;
	}
}

/*
* CMod_LoadBrushSides
*/
static void CMod_LoadBrushSides( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	cbrushside_t *out;
	dbrushside_t *in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "brushsides", MAX_MAP_BRUSHSIDES );
	in = CMod_LumpData( cms, l );

	out = cms->map_brushsides = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numbrushsides = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		j = LittleLong( in->planenum );
		if( j < 0 || j >= cms->numplanes )
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: brushside %i has bad plane %i", i, j );
		out->plane = cms->map_planes + j;

		j = LittleLong( in->shadernum );
		if( j < 0 || j >= cms->numshaderrefs )
			Com_Error( ERR_DROP, "Bad brushside texinfo" );
		out->surfFlags = cms->map_shaderrefs[j].flags;
	}
}

/*
* CMod_LoadBrushSides_RBSP
*/
static void CMod_LoadBrushSides_RBSP( cmodel_state_t *cms, lump_t *l )
{
	int i, j;
	int count;
	cbrushside_t *out;
	rdbrushside_t *in;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "brushsides", MAX_MAP_BRUSHSIDES );
	in = CMod_LumpData( cms, l );

	out = cms->map_brushsides = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numbrushsides = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		j = LittleLong( in->planenum );
		if( j < 0 || j >= cms->numplanes )
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: brushside %i has bad plane %i", i, j );
		out->plane = cms->map_planes + j;

		j = LittleLong( in->shadernum );
		if( j < 0 || j >= cms->numshaderrefs )
			Com_Error( ERR_DROP, "Bad brushside texinfo" );
		out->surfFlags = cms->map_shaderrefs[j].flags;
	}
}

/*
* CMod_LoadBrushes
*/
static void CMod_LoadBrushes( cmodel_state_t *cms, lump_t *l )
{
	int i;
	int count;
	dbrush_t *in;
	cbrush_t *out;
	int shaderref;

	count = CMod_LumpCountMin( cms, l, sizeof( *in ), "brushes", MAX_MAP_BRUSHSIDES );
	in = CMod_LumpData( cms, l );

	out = cms->map_brushes = Mem_Alloc( cms->mempool, count * sizeof( *out ) );
	cms->numbrushes = count;

	for( i = 0; i < count; i++, out++, in++ )
	{
		int firstside;

		shaderref = LittleLong( in->shadernum );
		if( shaderref < 0 || shaderref >= cms->numshaderrefs )
			Com_Error( ERR_DROP, "CMod_LoadBrushes: brush %i has bad shader %i", i, shaderref );
		out->contents = cms->map_shaderrefs[shaderref].contents;

		out->numsides = LittleLong( in->numsides );
		firstside = LittleLong( in->firstside );
		if( out->numsides < 0 || firstside < 0 || out->numsides > cms->numbrushsides - firstside )
			Com_Error( ERR_DROP, "CMod_LoadBrushes: brush %i has bad side range (first %i, count %i, of %i)",
				i, firstside, out->numsides, cms->numbrushsides );
		out->brushsides = cms->map_brushsides + firstside;
	}
}

/*
* CMod_LoadVisibility
*/
static void CMod_LoadVisibility( cmodel_state_t *cms, lump_t *l )
{
	cms->map_visdatasize = CMod_LumpCount( cms, l, 1, "visibility" );
	if( !cms->map_visdatasize )
	{
		cms->map_pvs = NULL;
		return;
	}

	if( cms->map_visdatasize < (int)sizeof( dvis_t ) )
		Com_Error( ERR_DROP, "CMod_LoadVisibility: visibility lump is %i bytes, header needs %i",
			cms->map_visdatasize, (int)sizeof( dvis_t ) );
	cms->map_pvs = Mem_Alloc( cms->mempool, cms->map_visdatasize );
	memcpy( cms->map_pvs, CMod_LumpData( cms, l ), cms->map_visdatasize );

	cms->map_pvs->numclusters = LittleLong( cms->map_pvs->numclusters );
	cms->map_pvs->rowsize = LittleLong( cms->map_pvs->rowsize );

	// every consumer indexes this as data[cluster*rowsize .. +rowsize), so the
	// whole table has to fit inside what we just copied
	if( cms->map_pvs->numclusters < 0 || cms->map_pvs->rowsize < 0 )
		Com_Error( ERR_DROP, "CMod_LoadVisibility: negative vis dimensions" );
	if( cms->map_pvs->rowsize &&
		cms->map_pvs->numclusters > ( cms->map_visdatasize - (int)offsetof( dvis_t, data ) ) / cms->map_pvs->rowsize )
		Com_Error( ERR_DROP, "CMod_LoadVisibility: %i clusters of %i bytes do not fit in %i bytes",
			cms->map_pvs->numclusters, cms->map_pvs->rowsize, cms->map_visdatasize );
}

/*
* CMod_LoadEntityString
*/
static void CMod_LoadEntityString( cmodel_state_t *cms, lump_t *l )
{
	cms->numentitychars = CMod_LumpCount( cms, l, 1, "entities" );
	if( !cms->numentitychars )
		return;

	// callers treat this as a C string; the file carries no guaranteed terminator
	cms->map_entitystring = Mem_Alloc( cms->mempool, cms->numentitychars + 1 );
	memcpy( cms->map_entitystring, CMod_LumpData( cms, l ), cms->numentitychars );
	cms->map_entitystring[cms->numentitychars] = '\0';
}

/*
* CM_LoadQ3BrushModel
*/
void CM_LoadQ3BrushModel( cmodel_state_t *cms, void *parent, void *buf, size_t bufferLen, bspFormatDesc_t *format )
{
	int i;
	dheader_t header;

	cms->cmap_bspFormat = format;

	if( bufferLen < sizeof( dheader_t ) )
		Com_Error( ERR_DROP, "CM_LoadQ3BrushModel: file is truncated (%u bytes, header needs %u)",
			(unsigned)bufferLen, (unsigned)sizeof( dheader_t ) );

	header = *(dheader_t *)buf;
	for( i = 0; i < sizeof( dheader_t ) / 4; i++ )
		( (int *)&header )[i] = LittleLong( ( (int *)&header )[i] );
	cms->cmod_base = ( uint8_t * )buf;
	cms->cmod_length = bufferLen;

	// load into heap
	CMod_LoadSurfaces( cms, &header.lumps[LUMP_SHADERREFS] );
	CMod_LoadPlanes( cms, &header.lumps[LUMP_PLANES] );
	if( cms->cmap_bspFormat->flags & BSP_RAVEN )
		CMod_LoadBrushSides_RBSP( cms, &header.lumps[LUMP_BRUSHSIDES] );
	else
		CMod_LoadBrushSides( cms, &header.lumps[LUMP_BRUSHSIDES] );
	CMod_LoadBrushes( cms, &header.lumps[LUMP_BRUSHES] );
	CMod_LoadMarkBrushes( cms, &header.lumps[LUMP_LEAFBRUSHES] );
	if( cms->cmap_bspFormat->flags & BSP_RAVEN )
	{
		CMod_LoadVertexes_RBSP( cms, &header.lumps[LUMP_VERTEXES] );
		CMod_LoadFaces_RBSP( cms, &header.lumps[LUMP_FACES] );
	}
	else
	{
		CMod_LoadVertexes( cms, &header.lumps[LUMP_VERTEXES] );
		CMod_LoadFaces( cms, &header.lumps[LUMP_FACES] );
	}
	CMod_LoadMarkFaces( cms, &header.lumps[LUMP_LEAFFACES] );
	CMod_LoadLeafs( cms, &header.lumps[LUMP_LEAFS] );
	CMod_LoadNodes( cms, &header.lumps[LUMP_NODES] );
	CMod_LoadSubmodels( cms, &header.lumps[LUMP_MODELS] );
	CMod_LoadVisibility( cms, &header.lumps[LUMP_VISIBILITY] );
	CMod_LoadEntityString( cms, &header.lumps[LUMP_ENTITIES] );

	// leaf clusters index the vis rows, which are only known after the
	// visibility lump has been read
	for( i = 0; i < cms->numleafs; i++ )
	{
		cleaf_t *leaf = cms->map_leafs + i;

		if( leaf->cluster < 0 )
			continue;
		if( !cms->map_pvs || leaf->cluster >= cms->map_pvs->numclusters )
			leaf->cluster = -1;
	}

	FS_FreeFile( buf );

	if( cms->numvertexes )
		Mem_Free( cms->map_verts );
}
