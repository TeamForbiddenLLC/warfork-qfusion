textures/factory/blx_wt3_sky_orange_shader_reverse
{
	qer_editorimage textures/factory/blx_wt3_sky_orange.png
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 20
	q3map_sun 1 1 0.75 155 350 70
	skyParms - - -

	{
		map textures/factory/blx_wt3_sky_orange.png
		tcMod scale 2 2
		tcMod scroll 0.005 0.005
		depthWrite
	}
	{
		map textures/factory/blx_wt3_sky_orange.png
		blendfunc add
		tcMod scale 3 3
		tcMod scroll 0.01 0.01
	}
}


textures/factory/blx_wt3_sky_orange_shader
{
	qer_editorimage textures/factory/blx_wt3_sky_orange.png
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 20
	q3map_sun 1 1 0.75 155 170 70
	skyParms - - -

	{
		map textures/factory/blx_wt3_sky_orange.png
		tcMod scale 2 2
		tcMod scroll 0.005 0.005
		depthWrite
	}
	{
		map textures/factory/blx_wt3_sky_orange.png
		blendfunc add
		tcMod scale 3 3
		tcMod scroll 0.01 0.01
	}
}

textures/factory/scroll_beam
{	
	nopicmip
	qer_editorimage textures/factory/beam_beam.png
	polygonOffset 
	surfaceparm nolightmap
	sutfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm noimpact
	qer_trans 0.75
	{	
		map textures/factory/beam_beam.png
		tcmod scroll 0 4
		blendfunc add
	}
}


textures/factory/deadly_lasers
{
	q3map_globaltexture
	surfaceparm nonsolid
	nopicmip
	surfaceparm trans
	surfaceparm nolightmap
	q3map_surfacelight 250
	qer_editorimage textures/factory/laser_beam.png
	qer_trans 0.25
        {
		map textures/factory/laser_beam.png
		blendfunc add
		tcmod scroll 0 -2
                blendFunc add
	}
	{
		map textures/factory/laser_beam.png
          	blendfunc add
		tcmod scroll 0 2
	}
}


textures/factory/blx_glow2sides
{	
	nopicmip
	qer_editorimage textures/blx/blx_glow2.png
	cull none
	surfaceparm nonsolid
	{	
		map textures/blx/blx_glow2.png
		blendfunc add
		tcmod scale 1 0.98
	}
}


textures/factory/pillar03_fx
{
	qer_editorimage textures/factory/pillar03_fx.png
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/factory/pillar03_fx.png
		blendFunc add
		tcmod scroll 0 2
	}
}


textures/factory/wave_fx_scroll
{
	qer_editorimage textures/factory/wave_fx.png
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/factory/wave_fx.png
		blendFunc add
		tcmod scroll .5 0
	}
}


textures/factory/wave_fx_scroll-
{
	qer_editorimage textures/factory/wave_fx.png
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/factory/wave_fx.png
		blendFunc add
		tcmod scroll -.5 0
	}
}


textures/factory/pillar03_fx3
{
	nopicmip
	qer_editorimage textures/factory/pillar03.png
	
if deluxe
	{
		material textures/factory/pillar03.png $blankbumpimage - textures/factory/pillar03_alpha.png
	}
endif

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/factory/pillar03.png
		blendfunc filter
	}
	{
		map textures/factory/pillar03_alpha.png
		blendfunc add
	}
endif
	{
		map textures/factory/pillar03_fx.png
		blendFunc add
		tcmod scroll 0 2
		tcmod scale 1 10
	}
}


textures/factory/wall01_shader
{
	nopicmip
	qer_editorimage textures/factory/wall01.png

if deluxe
	{
		material textures/factory/wall01.png $blankbumpimage - textures/factory/wall01_alpha.png
	}
endif

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/factory/wall01.png
		blendfunc filter
	}
	{
		map textures/factory/wall01_alpha.png
		blendfunc add
	}
endif
}

textures/factory/blx_wt3_floor4_shader
{
	qer_editorimage textures/blx_wtest3/blx_wt3_floor4.png

	{
		map textures/blx_wtest3/blx_wt3_floor4.png
	}
	{
		map textures/factory/chrome4.png
		blendfunc add
		tcGen environment
		tcmod scale 2 2
	}
}


textures/factory/trim01_fx
{
	nopicmip
	qer_editorimage textures/factory/trim01.png

if deluxe
	{
		material textures/factory/trim01.png $blankbumpimage - textures/factory/trim01_alpha.png
	}
endif

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/factory/trim01.png
		blendfunc filter
	}
	{
		map textures/factory/trim01_alpha.png
		blendfunc add
	}
endif
	{
		map textures/factory/trim01_glow.png
		blendfunc add
		rgbGen wave sin 0.5 -1 0 0.5
	}
	{
		map textures/factory/trim01_fx.png
		blendFunc add
		tcmod scroll 1.5 0
	}
}


textures/factory/trim01_fx_alpha
{
	qer_editorimage textures/factory/trim01_fx.png
	nopicmip
	surfaceparm nolightmap
	sutfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	qer_trans 0.50
	{
		map textures/factory/trim01_fx.png
		blendFunc add
		tcmod scroll 1.5 0
		rgbgen teamcolor 2
	}
}


textures/factory/trim01_fx_beta
{
	qer_editorimage textures/factory/trim01_fx.png
	nopicmip
	surfaceparm nolightmap
	sutfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	qer_trans 0.50
	{
		map textures/factory/trim01_fx.png
		blendFunc add
		tcmod scroll 1.5 0
		rgbgen teamcolor 3
	}
}


textures/factory/trim01
{
	nopicmip
	qer_editorimage textures/factory/trim01.png

if deluxe
	{
		material textures/factory/trim01.png $blankbumpimage - textures/factory/trim01_alpha.png
	}
endif

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/factory/trim01.png
		blendfunc filter
	}
	{
		map textures/factory/trim01_alpha.png
		blendfunc add
	}
endif	
	{
		map textures/factory/trim01_glow.png
		blendfunc add
		rgbGen wave sin 0.5 -1 0 0.5
	}
}


textures/factory/orange_border_trans
{
	qer_editorimage textures/factory/orange_border_trans.png
	cull none
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/orange_border_trans.png
		blendfunc add
	}
	{
		map textures/factory/orange_border_glow.png
		blendfunc add
	}
	{
		map textures/factory/orange_border_alpha.png
		blendfunc filter
	}
}


textures/factory/elec_floor
{
	nopicmip
	qer_editorimage textures/factory/elec_floor.png

if deluxe
	{
		material textures/factory/elec_floor.png $blankbumpimage - textures/factory/elec_floor_alpha.png
		rgbGen wave sin 0.5 -1 0 0.25
	}
endif

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/factory/elec_floor.png
		blendfunc filter
	}
	{
		map textures/factory/elec_floor_alpha.png
		blendfunc add
		rgbGen wave sin 0.5 -1 0 0.25
	}
endif
}


textures/factory/orange_squares_scroll
{
	qer_editorimage textures/factory/orange_border_glow.png
	cull none
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/orange_border_glow.png
		blendfunc add
		tcmod scroll 0 0.5
	}
}


textures/factory/base
{
	nopicmip
	cull none
	surfaceparm nonsolid
	surfaceparm trans
	qer_editorimage textures/factory/base.png
	qer_trans 0.75
//	{
//		map $lightmap
//		rgbGen identity
//	}
	{
		map textures/factory/base.png
		blendfunc add
	}
	{
		map textures/factory/base_fx.png
		blendfunc add
		tcmod rotate 270
	}
	{
		map textures/factory/base_stripes_out.png
		blendfunc add
		tcmod rotate 180
		tcmod scale 1 1
	}
	{
		map textures/factory/base_stripes_in.png
		blendfunc add
		tcmod rotate -180
		tcmod scale 1 1
	}
}

textures/factory/weapon_base
{
	qer_editorimage textures/factory/weapon_base.png
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/weapon_base.png
		blendfunc blend
	}
}

textures/factory/ammo_base
{
	qer_editorimage textures/factory/ammo_base.png
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/ammo_base.png
		blendfunc blend
	}
}

textures/factory/health_base
{
	qer_editorimage textures/factory/health_base.png
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/health_base.png
		blendfunc blend
	}
}

textures/factory/powerup_base
{
	qer_editorimage textures/factory/powerup_base.png
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/powerup_base.png
		blendfunc blend
	}
}

textures/factory/armor_base
{
	qer_editorimage textures/factory/armor_base.png
	polygonOffset
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/factory/armor_base.png
		blendfunc blend
	}
}

textures/factory/blx_spacer_2
{
	qer_editorimage textures/blx/blx_spacer_2.png	
	surfaceparm nomarks

	{
		map textures/blx/blx_spacer_2.png
		tcmod scroll .4 0
	}
	{
		map textures/blx/blx_spacer_alpha.png
		blendfunc add
		tcmod scroll 1 .25
	}
}

textures/factory/blx_spacer_2_light_soft
{
	q3map_lightimage textures/blx/blx_spacer_2.png
	qer_editorimage textures/blx/blx_spacer_2.png
	q3map_surfacelight 256
	q3map_lightsubdivide 32
	surfaceparm nomarks

	{
		map textures/blx/blx_spacer_2.png
		tcmod scroll .4 0
	}
	{
		map textures/blx/blx_spacer_alpha.png
		blendfunc add
		tcmod scroll 1 .25
	}
}

textures/factory/glass
{
	qer_editorimage textures/factory/glass.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 
	cull disable

	{
		map textures/factory/glass.png
		tcGen environment
		blendFunc gl_one gl_one
	}
}

textures/factory/glass_singlesided_distancebloom
{
	qer_editorimage textures/factory/glass.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/factory/glass.png
		tcGen environment
		blendFunc gl_one gl_one
	}

	{
		map textures/factory/glass.png
		tcGen environment
		blendFunc gl_one gl_one
		//blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
		rgbgen wave distanceramp 0 1 200 500
	}
}

textures/factory/glass_softadd
{
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/factory/glass.png
		tcGen environment
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
}




textures/factory/trim_04
{
	qer_editorimage textures/factory/trim_04_decal.png
	surfaceparm nomarks
	surfaceparm nonsolid
	nopicmip

	{
		material textures/factory/trim_04_decal.png
		blendFunc blend
	}
}

textures/factory/trim_04_decal
{
	qer_editorimage textures/factory/trim_04_decal.png
	surfaceparm nomarks
	polygonOffset
	surfaceparm nonsolid
	nopicmip

	{
		material textures/factory/trim_04_decal
		blendFunc blend
	}
}

textures/factory/factory_wall_grey
{
	qer_editorimage textures/factory/factory_wall_grey.png

	{
		material textures/factory/factory_wall_grey.png textures/factory/factory_wall_norm.png
	}
}

// CTF

textures/factory/factory_wall_alpha
{
	qer_editorimage textures/factory/factory_wall_team.png

	{
		material textures/factory/factory_wall_team.png
	}
}

textures/factory/factory_wall_beta
{
	qer_editorimage textures/factory/factory_wall_team.png

	{
		material textures/factory/factory_wall_team.png
	}
}

textures/factory/factory_wall_rust_alpha
{
	qer_editorimage textures/factory/factory_wall_grey.png

	{
		material textures/factory/factory_wall_grey.png textures/factory/factory_wall_norm.png textures/factory/factory_wall_gloss.png
	}
}

textures/factory/factory_wall_rust_beta
{
	qer_editorimage textures/factory/factory_wall_grey.png

	{
		material textures/factory/factory_wall_grey.png textures/factory/factory_wall_norm.png textures/factory/factory_wall_gloss.png
	}
}

textures/factory/factory_wall_red
{
	qer_editorimage textures/factory/factory_wall_team.png

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/factory_wall_team.png
		blendfunc filter
	}
	{
		map textures/factory/factory_wall_team_color.png
		rgbgen const 1 0 0
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
endif

if deluxe
	{
		material textures/factory/factory_wall_team.png textures/factory/factory_wall_team_norm.png textures/factory/factory_wall_team_gloss.png textures/factory/factory_wall_team_color.png
		rgbgen const 1 0 0
	}
endif
}

textures/factory/factory_wall_green
{
	qer_editorimage textures/factory/factory_wall_team.png

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/factory_wall_team.png
		blendfunc filter
	}
	{
		map textures/factory/factory_wall_team_color.png
		rgbgen const 0 1 0
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
endif

if deluxe
	{
		material textures/factory/factory_wall_team.png textures/factory/factory_wall_team_norm.png textures/factory/factory_wall_team_gloss.png textures/factory/factory_wall_team_color.png
		rgbgen const 0 1 0
	}
endif
}

textures/factory/factory_wall_blue
{
	qer_editorimage textures/factory/factory_wall_team.png

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/factory_wall_team.png
		blendfunc filter
	}
	{
		map textures/factory/factory_wall_team_color.png
		rgbgen const 0 0 1
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
endif

if deluxe
	{
		material textures/factory/factory_wall_team.png textures/factory/factory_wall_team_norm.png textures/factory/factory_wall_team_gloss.png textures/factory/factory_wall_team_color.png
		rgbgen const 0 0 1
	}
endif
}

textures/factory/factory_wall_yellow
{
	qer_editorimage textures/factory/factory_wall_team.png

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/factory_wall_team.png
		blendfunc filter
	}
	{
		map textures/factory/factory_wall_team_color.png
		rgbgen const 1 0.85 0
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
endif

if deluxe
	{
		material textures/factory/factory_wall_team.png textures/factory/factory_wall_team_norm.png textures/factory/factory_wall_team_gloss.png textures/factory/factory_wall_team_color.png
		rgbgen const 1 0.85 0
	}
endif
}

textures/factory/factory_wall_violet
{
	qer_editorimage textures/factory/factory_wall_team.png

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/factory_wall_team.png
		blendfunc filter
	}
	{
		map textures/factory/factory_wall_team_color.png
		rgbgen const 0.65 0 0.65
		blendFunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
endif

if deluxe
	{
		material textures/factory/factory_wall_team.png textures/factory/factory_wall_team_norm.png textures/factory/factory_wall_team_gloss.png textures/factory/factory_wall_team_color.png
		rgbgen const 0.35 0 0.65
	}
endif
}
