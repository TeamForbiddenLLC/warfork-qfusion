textures/blx/blx_display_1
{
	nopicmip
	qer_editorimage textures/blx/blx_display_1.png	
	{
		map textures/blx/blx_display_1.png
		blendfunc add
	}
	{
		map textures/billboard/blx/code.png
		blendfunc add
		tcmod scroll 3 1 
	}
	{
		map gfx/colors/chrome.png
		tcgen environment
  		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
   		alphaGen constant 0.4 
	}
	{
		map textures/blx/circuits_blue_512.png
 		blendfunc add
		tcmod scroll 0 0.1
		rgbGen wave sin 0 0.5 0 .25
	}
}

textures/blx/blx_spacer
{
	qer_editorimage textures/blx/blx_spacer.png
	surfaceparm noimpact
	surfaceparm nomarks
	forceoverlays
	polygonOffset 

	{
		map textures/blx/blx_spacer.png
		tcmod scroll .4 0
	}
	{
		map textures/blx/blx_spacer_alpha.png
		blendfunc add
		tcmod scroll 1 .25
	}
}


textures/blx/blx_wall_alpha
{
	qer_editorimage textures/blx/blx_wall_alpha.png	
	forceoverlays
	polygonOffset 
	{
		map textures/blx/blx_wall_alpha.png
		blendfunc decal
	}
}

textures/blx/blx_tech_1
{
	qer_editorimage textures/blx/blx_tech_1.png	
	forceoverlays
	polygonOffset 
	{
		map textures/blx/blx_tech_1.png
		blendfunc decal
	}
}

textures/blx/blx_wall_1
{
	qer_editorimage textures/blx/blx_wall_1.png	
	
	if deluxe
	{
		material
	}
	endif
	
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_1.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_1_alpha.png
		blendFunc add
	}
}

textures/blx/blx_grid
{
	qer_editorimage textures/blx/blx_grid.png	
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_grid.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_grid_alpha.png
		blendFunc add
	}
}

textures/blx/warsow_neon
{
	nopicmip
	qer_editorimage textures/blx/warsow_neon.png
	cull none
	{
		map textures/blx/warsow_neon.png
		blendFunc add
	}
}

textures/blx/blx_tile
{
	qer_editorimage textures/blx/blx_tile.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_tile.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_tile_alpha.png
		blendFunc add
	}
}


textures/blx/blx_wall_6
{
	qer_editorimage textures/blx/blx_wall_6.png
	surfaceparm nolightmap
	{
		map textures/blx/blx_wall_6.png
	}
}

textures/blx/blx_wall_2
{
	qer_editorimage textures/blx/blx_wall_2.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_2.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_2_alpha.png
		blendFunc add
	}
}

textures/blx/blx_md_lt_B
{
	qer_editorimage textures/blx/blx_md_lt_B.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_md_lt_B.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_md_lt_B_alpha.png
		blendFunc add
	}
}

textures/blx/blx_md_lt
{
	qer_editorimage textures/blx/blx_md_lt.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_md_lt.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_md_lt_alpha.png
		blendFunc add
	}
}

textures/blx/blx_wall_7
{
	qer_editorimage textures/blx/blx_wall_7.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_7.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_7_alpha.png
		blendFunc add
	}
}


textures/blx/blx_wall_5
{
	qer_editorimage textures/blx/blx_wall_5.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_5.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_5_alpha.png
		blendFunc add
	}
}


textures/blx/blx_wall_3
{
	qer_editorimage textures/blx/blx_wall_3.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_3.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_3_alpha.png
		blendFunc add
	}
}

textures/blx/blx_wall_2a
{
	qer_editorimage textures/blx/blx_wall_2a.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/blx/blx_wall_2a.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_wall_2a_alpha.png
		blendFunc add
	}
}


textures/blx/blx_ecel_bounce
{
	q3map_lightimage textures/blx/blx_ecel_bounce_blend.png
	surfaceparm nodamage
	forceoverlays
	polygonOffset 
	q3map_surfacelight 400
	nopicmip

	{
		clampmap textures/blx/blx_ecel_bounce.png
		blendfunc filter
		rgbgen identity
	}
	{
		clampmap textures/blx/blx_ecel_bounce_blend.png
		blendfunc add
		rgbGen wave sin 0.5 0.5 0 1.5 
	}
	{
		clampmap textures/blx/blx_ecel_bounce_small.png
		blendfunc add
		rgbGen wave square 0.5 0.5 0.25 1.5 
		tcMod stretch sin 1.2 0.8 0 1.5 
	}
}

textures/blx/blx_ecel_bounce_nooffset
{
	qer_editorimage textures/blx/blx_ecel_bounce.png
	surfaceparm nodamage
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	nopicmip

	{
		clampmap textures/blx/blx_ecel_bounce.png
		blendfunc filter
		rgbgen identity
	}
	{
		clampmap textures/blx/blx_ecel_bounce_blend.png
		blendfunc add
		rgbGen wave sin 0.5 0.5 0 1.5
	}
	{
		clampmap textures/blx/blx_ecel_bounce_small.png
		blendfunc add
		rgbGen wave square 0.5 0.5 0.25 1.5
		tcMod stretch sin 1.2 0.8 0 1.5
	}
}

textures/blx/blx_glow2
{	
	nopicmip
	qer_editorimage textures/blx/blx_glow2.png
	cull back
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.5
	{	
		map textures/blx/blx_glow2.png
		blendfunc add
		tcmod scale 1 0.98
	}
}

textures/blx/blx_glow2a
{	
	nopicmip
	qer_editorimage textures/blx/blx_glow2b.png
	cull back
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.1
	{	
		map textures/blx/blx_glow2b.png
		blendfunc add
		alphaGen const .01
		tcmod scale 1 0.98
	}
}


textures/blx/blx_glow
{	
	nopicmip
	qer_editorimage textures/blx/blx_glow.png
	cull front
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.5
	{
		map textures/blx/blx_glow.png
		blendfunc add
	}
}

textures/blx/blx_white_line
{	
	qer_editorimage textures/blx/blx_white_line.png
	forceoverlays
	polygonOffset 
	surfaceparm trans
	surfaceparm nonsolid
	{
		map textures/blx/blx_white_line.png
		blendfunc add
	}
}

textures/blx/blx_dark_ws
{	
	qer_editorimage textures/blx/blx_dark_ws.png
	if deluxe
	{
		material
	}
	endif
	if ! deluxe
	{
		map $lightmap
		rbgGen identity
	}
	{
		map textures/blx/blx_dark_ws.png
		blendFunc filter
	}
	endif
	{
		map textures/blx/blx_white_square.png
		blendfunc add
	}
}

textures/blx/blx_white_edge1
{	
	qer_editorimage textures/blx/blx_white_edge1.png
	forceoverlays
	polygonOffset 
	surfaceparm trans
	surfaceparm nonsolid
	{
		map textures/blx/blx_white_edge1.png
		blendfunc add
	}
}

textures/blx/blx_white_stripe1
{	
	qer_editorimage textures/blx/blx_white_stripe1.png
	polygonOffset 
	{
		map textures/blx/blx_white_stripe1.png
		blendfunc add
	}
}

textures/blx/blx_white_stripe
{	
	qer_editorimage textures/blx/blx_white_stripe.png
	polygonOffset 
	{
		map textures/blx/blx_white_stripe.png
		blendfunc add
	}
}

textures/blx/blx_white_edge
{	
	qer_editorimage textures/blx/blx_white_edge.png
	forceoverlays
	polygonOffset 
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map textures/blx/blx_white_edge.png
		blendfunc add
	}
}

textures/blx/blx_circle
{	
	nopicmip
	qer_editorimage textures/blx/blx_circle.png
	forceoverlays
	polygonOffset 
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks

	{
		material textures/blx/blx_circle
		blendfunc blend
	}
	endif
}









textures/blx/new_sky
{
	qer_editorimage textures/blx/new_sky.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 200
	q3map_sun 0.27 0.4 0.51 155 170 33
	skyParms - - -
	{
		map textures/blx/new_sky.png
		tcMod scale 8 8
		tcMod scroll 0.0 0.0
		depthWrite
	}
	{
		map textures/cha0s_ws/ch_sky_night_front.png
		blendfunc add
		tcMod scale 2 2
		tcMod scroll 0.01 0.01
	}
}

textures/blx/elec
{
	qer_editorimage textures/blx/blx_spacer_2.png
	q3map_globaltexture
	q3map_lightimage textures/blx/circuits_red_512.png
	q3map_surfacelight 600
	surfaceparm trans	
	surfaceparm noimpact
	surfaceparm lava
	cull disable	
	tesssize 128

	{
		map textures/blx/blx_spacer_2.png
		tcmod scale 1.5 1.5
		tcmod scroll .4 0
	}
	{
		map textures/blx/circuits_red_512.png
		blendfunc blend
		tcmod scale 0.5 0.5
		tcmod scroll 0.1 0.1
	}
	{
		map textures/blx/blx_lava2.png
		tcmod scale 2 2
		blendfunc add
		tcmod scroll 1 .25
	}
}


textures/blx/circuits_blue
{
	qer_editorimage textures/blx/circuits_surface_512.png
	
	{
		map $lightmap
	}
	{
		map textures/blx/circuits_surface_512.png
		blendFunc filter
	}

	{
		map textures/blx/circuits_blue_512.png
		blendFunc blend
	}
	{
		map textures/blx/circuits_white_512.png
		rgbGen wave sin 0 0.8 0 0.2
		blendfunc blend
	}
}
