/*
Copyright (C) 2013 Victor Luchits

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

// r_cubemaps.c - per-surface, capture-once reflection cubemaps ('map $cubemap')

#include "r_local.h"

// view angles (pitch, yaw, roll) for rendering straight into the six GL
// cube faces, derived from the engine's own envshot chain and cross-checked
// against the standard GL render-to-cubemap camera table (see
// studies/cubemap_shader_key_plan.txt, Section 4)
static const vec3_t cubemapFaceAngles[6] =
{
	{   0,   0,  90 },		// +X (px)
	{   0, 180, -90 },		// -X (nx)
	{   0,  90,   0 },		// +Y (py)
	{   0, 270, 180 },		// -Y (ny)
	{ -90, 180, -90 },		// +Z (pz)
	{  90, 180, -90 }		// -Z (nz)
};

/*
* R_BuildWorldCubemapGroups
*
* Walks the world model's own faces and groups all coplanar faces that
* share one $cubemap shader into a single cubemapGroup_t. Plane derivation
* and coplanar tolerance mirror R_AddPortalSurface (r_portals.c).
*/
void R_BuildWorldCubemapGroups( void )
{
	int i, j;
	bool overflowed = false;
	msurface_t *surf;
	shader_t *shader;
	cubemapGroup_t *group;
	cplane_t plane;
	vec3_t v[3];

	memset( rsh.cubemapGroups, 0, sizeof( rsh.cubemapGroups ) );
	rsh.numCubemapGroups = 0;

	if( !rsh.worldBrushModel || !rsh.worldBrushModel->firstmodelsurface )
		return;

	// v1: world model faces only — submodel faces belong to brush entities
	// (doors etc.) and may move; a moving capture spot would be wrong
	for( i = 0; i < (int)rsh.worldBrushModel->nummodelsurfaces; i++ )
	{
		surf = rsh.worldBrushModel->firstmodelsurface + i;
		surf->cubemap = NULL;

		shader = surf->shader;
		if( !shader || !( shader->flags & SHADER_CUBEMAP_CAPTURE ) )
			continue;
		if( !surf->mesh || surf->mesh->numElems < 3 )
			continue;

		// plane from the first triangle, exactly like r_portals.c:48-55
		for( j = 0; j < 3; j++ )
			VectorCopy( surf->mesh->xyzArray[surf->mesh->elems[j]], v[j] );
		PlaneFromPoints( v, &plane );
		plane.dist += 1; // nudge along the normal a bit (r_portals.c:54)
		CategorizePlane( &plane );

		// find a coplanar group with the same shader
		// (tolerances from r_portals.c:117-126)
		group = NULL;
		for( j = 0; j < rsh.numCubemapGroups; j++ )
		{
			if( rsh.cubemapGroups[j].shader == shader
				&& DotProduct( rsh.cubemapGroups[j].plane.normal, plane.normal ) > 0.99f
				&& fabs( rsh.cubemapGroups[j].plane.dist - plane.dist ) < 0.1f ) {
				group = &rsh.cubemapGroups[j];
				break;
			}
		}

		if( !group )
		{
			if( rsh.numCubemapGroups == MAX_CUBEMAP_IMAGES )
			{
				if( !overflowed )
				{
					Com_Printf( S_COLOR_YELLOW "Too many cubemap groups (max %i), some surfaces will have no reflection\n",
						MAX_CUBEMAP_IMAGES );
					overflowed = true;
				}
				continue;
			}
			group = &rsh.cubemapGroups[rsh.numCubemapGroups++];
			group->shader = shader;
			group->plane = plane;
			ClearBounds( group->mins, group->maxs );
		}

		AddPointToBounds( surf->mins, group->mins, group->maxs );
		AddPointToBounds( surf->maxs, group->mins, group->maxs );
		surf->cubemap = group;
	}

	// capture origin: bounds midpoint snapped onto the (nudged) plane
	for( i = 0; i < rsh.numCubemapGroups; i++ )
	{
		group = &rsh.cubemapGroups[i];

		VectorAdd( group->mins, group->maxs, group->centre );
		VectorScale( group->centre, 0.5, group->centre );
		VectorMA( group->centre,
			group->plane.dist - DotProduct( group->centre, group->plane.normal ),
			group->plane.normal, group->centre );
	}

	ri.Com_DPrintf( "R_BuildWorldCubemapGroups: %i cubemap group(s)\n", rsh.numCubemapGroups );
}

/*
* R_DrawWorldCubemaps
*
* Captures every world cubemap group once, on the first world frame after
* map load (or vid_restart). Called from R_RenderView right before
* R_DrawPortals(), outermost view only. Our capture views set
* RF_CUBEMAPVIEW (== RF_ENVVIEW), which is part of RF_NONVIEWERREF, so
* nested renders can never re-enter even before the one-shot flag is
* cleared.
*/
void R_DrawWorldCubemaps( void )
{
	int i, face;
	cubemapGroup_t *group;

	if( !rf.pendingCubemapCapture )
		return;
	if( rn.renderFlags & RF_NONVIEWERREF )   // mirrors, portals, shadowmaps, env views
		return;

	rf.pendingCubemapCapture = false;        // one-shot: clear before rendering

	// NOTE: deliberately NO r_cubemaps check here (user decision):
	// capture always runs so a mid-map 0->1 toggle finds existing cubes.
	// The cvar gates drawing only (RB_ShaderpassTex).

	for( i = 0; i < rsh.numCubemapGroups; i++ )
	{
		group = &rsh.cubemapGroups[i];

		ri.Com_DPrintf( "cubemap group %i ('%s'): origin %.1f %.1f %.1f, creating cube\n",
			i, group->shader ? group->shader->name : "?",
			group->centre[0], group->centre[1], group->centre[2] );

		group->cube = R_InitCubemapTexture( &group->cube, "r_cubemap", i,
			r_cubemaps_imagesize->integer,
			IT_SPECIAL|IT_FRAMEBUFFER|IT_DEPTHRB|( glConfig.stencilBits ? IT_STENCIL : 0 ),
			IMAGE_TAG_GENERIC, glConfig.forceRGBAFramebuffers ? 4 : 3 );
		if( !group->cube )
			continue;

		for( face = 0; face < 6; face++ )
		{
			if( !R_PushRefInst() )
				return;

			RFB_AttachCubemapFaceToObject( group->cube->fbo, group->cube, face );

			rn.renderFlags |= RF_CUBEMAPVIEW;
			rn.clipFlags = 15;
			rn.shadowGroup = NULL;
			rn.meshlist = &r_cubemaplist;
			rn.portalmasklist = NULL;
			rn.farClip = R_DefaultFarClip();
			rn.fbColorAttachment = group->cube;
			rn.fbDepthAttachment = NULL;

			VectorCopy( group->centre, rn.pvsOrigin );

			rn.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER|RDF_FLIPPED );
			rn.refdef.x = rn.refdef.y = 0;
			rn.refdef.width = rn.refdef.height = group->cube->upload_width;
			rn.refdef.fov_x = rn.refdef.fov_y = 90;
			VectorCopy( group->centre, rn.refdef.vieworg );
			AnglesToAxis( cubemapFaceAngles[face], rn.refdef.viewaxis );

			Vector4Set( rn.viewport, 0, 0, rn.refdef.width, rn.refdef.height );
			Vector4Set( rn.scissor, 0, 0, rn.refdef.width, rn.refdef.height );

			R_RenderView( &rn.refdef );

			R_PopRefInst();
		}
	}

	ri.Com_DPrintf( "world cubemaps captured\n" );
}
