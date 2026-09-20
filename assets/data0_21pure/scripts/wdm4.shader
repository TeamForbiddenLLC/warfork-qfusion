
textures/wdm4/sky4_s
{
	qer_editorimage textures/blxbis/skyturq_scroll.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	//q3map_lightimage textures/cha0s_ws/ch_sky_night_back.png
	//q3map_sunExt .9 .9 .9 70 130 75 2 32
	//q3map_skyLight 80 6
	//q3map_lightmapFilterRadius 0 2

	skyParms - 2048 -

	{
		map textures/blxbis/skyturq_scroll2.png
		tcMod scale 4 4
		tcMod scroll 0 -0.015
		rgbgen const 0.2 0.2 0.2
	}

	{
		map textures/blxbis/skyturq_scroll.png
		tcMod scale 4 4
		tcMod scroll 0 0.03
		rgbgen const 0.2 0.2 0.2
		blendFunc add
	}

	{
		map textures/blxbis/skyturq_scroll2.png
		tcMod scale 4 4
		tcMod scroll 0 0.02
		rgbgen const 0.2 0.2 0.2
		blendFunc add
	}
}

textures/wdm4/fog
{
	qer_editorimage gfx/colors/white.png

	surfaceparm fog
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm	nolightmap
	qer_nocarve
	fogparms ( 0.45 0.55 0.58 ) 1300
}

textures/wdm4/foglayer
{
	qer_editorimage textures/world/wdm4/foglayer.png

	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm	nolightmap
	//cull none

	{
		map textures/world/wdm4/foglayer.png
		tcMod scale .04 .04
		tcMod scroll 0.15 0.15
		blendfunc blend
	}

	{
		map textures/world/wdm4/foglayer.png
		tcMod scale .04 .04
		tcMod scroll -0.2 -0.2
		blendfunc blend
	}
}

textures/wdm4/wdm4_teleporter
{
	q3map_surfacelight 80
	qer_editorimage textures/blxbis/ice_01.png
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nonsolid
	nopicmip
	tesssize 256

	{
		map textures/blxbis/ice_01.png
		alphagen const 0.6
		blendfunc blend
		tcMod turb 0 .1 0 .1
	}

	{
		map textures/blxbis/ice_01.png
		rgbGen wave sin .25 .35 0 2.5
		blendfunc add
		tcMod turb .1 0.25 0 .1
	}
}

textures/wdm4/ice_01_lightprojector_mid
{
	qer_editorimage textures/blxbis/ice_01.png
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	q3map_lightimage textures/blxbis/ice_01.png
	q3map_surfacelight 800

	//{
	//	map &whiteImage
	//	alphagen const 0
	//	alphaFunc GE128
	//	blendFunc blend
	//}
}

textures/wdm4/ice_01_lightprojector_strong
{
	qer_editorimage textures/blxbis/ice_01.png
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	q3map_lightimage textures/blxbis/ice_01.png
	q3map_surfacelight 1400

	//{
	//	map &whiteImage
	//	alphagen const 0
	//	alphaFunc GE128
	//	blendFunc blend
	//}
}


// MAPOBJECTS specific to this map


mapobjects_wdm4_tubes_aluminium
{
	qer_editorimage textures/blxbis/aluminium.png
	q3map_lightmapSampleOffset 8
	q3map_nonplanar

	{
		material textures/blxbis/aluminium.png
	}
}

mapobjects_wdm4_tubes_aluminium_detail
{
	qer_editorimage textures/blxbis/aluminium.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material textures/blxbis/aluminium.png
	}
}

mapobjects_wdm4_tubes_aluminium_flat
{
	qer_editorimage textures/blxbis/aluminium.png
	q3map_shadeangle 0.001

	{
		material textures/blxbis/aluminium.png
	}
}

mapobjects_wdm4_tubes_trim
{
	qer_editorimage textures/factory/trim_02.png

	{
		material textures/factory/trim_02.png
	}
}

mapobjects_wdm4_tubes_trim_scroll
{
	qer_editorimage textures/factory/trim_02.png

	{
		material textures/factory/trim_02.png
		tcMod scroll 0.2 0
	}
}

mapobjects_wdm4_tubes_metalgreen
{
	qer_editorimage textures/exwsw/eX_surf_metaldarkgrey.png

	{
		material textures/exwsw/eX_surf_metaldarkgrey.png textures/exwsw/eX_surf_norm.png textures/exwsw/eX_surf_metaldarkgrey_gloss.png
	}
}

mapobjects_wdm4_tubes_blueline
{
	qer_editorimage textures/factory/trim_03.png
	surfaceparm nomarks

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/trim_03.png
		blendFunc filter
	}

	{
		map textures/factory/trim_03_blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/factory/trim_03.png textures/factory/trim_03_norm.png textures/factory/trim_02_verygloss.png textures/factory/trim_03_blend.png
	}
endif
}

mapobjects_wdm4_trim05_blueline
{
	qer_editorimage textures/factory/trim_05.png
	surfaceparm nomarks
	q3map_lightimage textures/factory/trim_05_blend.png
	q3map_surfacelight 150
	q3map_lightsubdivide 72
	q3map_forceMeta

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/factory/trim_05.png
		blendFunc filter
	}

	{
		map textures/factory/trim_05_blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/factory/trim_05.png textures/factory/trim_05_norm.png textures/factory/trim_05_gloss.png textures/factory/trim_05_blend.png
	}
endif
}

mapobjects_wdm4_trim05
{
	qer_editorimage textures/factory/trim_05.png
	surfaceparm nomarks

	{
		material textures/factory/trim_05.png textures/factory/trim_05_norm.png textures/factory/trim_05_gloss.png
	}
}

mapobjects_wdm4_trim05_scroll
{
	qer_editorimage textures/factory/trim_05.png
	surfaceparm nomarks

	{
		material textures/factory/trim_05.png textures/factory/trim_05_norm.png textures/factory/trim_05_gloss.png
		tcMod scroll 0 0.75
	}
}

mapobjects_wdm4_trim2chaos
{
	qer_editorimage textures\cha0s_ws\trim2.png
	surfaceparm nomarks

	{
		material textures\cha0s_ws\trim2.png
		tcMod scroll 0 1
	}
}

mapobjects_wdm4_trim2chaos_slow
{
	qer_editorimage textures\cha0s_ws\trim2.png
	surfaceparm nomarks

	{
		material textures\cha0s_ws\trim2.png
		tcMod scroll 0 0.1
	}
}

mapobjects_wdm4_tubes_blueglow
{
	qer_editorimage textures/wsw_flareshalos/trim_glow_blue.png
	surfaceparm	nolightmap
	surfaceparm	nomarks
	surfaceparm	trans
	surfaceparm	nonsolid
	nopicmip

	{
		detail
		clampmap textures/wsw_flareshalos/trim_glow_blue.png
		rgbgen const 0.5 0.5 0.5
		blendfunc add
		tcmod scroll 0.002 0
	}
}

mapobjects_wdm4_ice_axisz
{
	qer_editorimage textures/blxbis/ice_01.png
	q3map_lightimage textures/blxbis/ice_01.png
	surfaceparm slick
	surfaceparm nomarks
	q3map_surfacelight 400
	q3map_lightsubdivide 72
	q3map_lightmapaxis z
	q3map_forceMeta
	q3map_nonplanar
	q3map_bounceScale 0.1

	{
		map $lightmap
	}
	{
		map textures/blxbis/ice_02.png
		blendFunc filter
	}
	{
		map textures/blxbis/ice_01.png
		blendfunc blend
		rgbGen const ( 0.756863 1 0.756863 )
		tcMod scroll 0.01 0.02
	}
	{
		map textures/blxbis/ice_01_alpha.png
		blendfunc blend
		alphaFunc GT0
	}
}

mapobjects_wdm4_ice_grated
{
	qer_editorimage textures/blxbis/ice_01.png
	q3map_lightimage textures/blxbis/ice_01.png
	surfaceparm slick
	surfaceparm nomarks
	q3map_surfacelight 50
	q3map_lightsubdivide 72
	q3map_forceMeta

	{
		map $lightmap
	}
	{
		map textures/blxbis/ice_02.png
		blendFunc filter
	}
	{
		map textures/blxbis/ice_01.png
		blendfunc blend
		rgbGen const ( 0.756863 1 0.756863 )
		tcMod scroll 0.02 0.04
	}
	{
		map textures/blxbis/ice_01_alpha.png
		blendfunc blend
		alphaFunc GT0
		tcMod scroll 0.01 0.02
	}

	{
		map textures/hazelh/grate.png
		blendfunc blend
		alphafunc GT0
	}
}

mapobjects_wdm4_ra_spot
{
	qer_editorimage textures/blxbis/ratowerskin1_spot.png
	surfaceparm nomarks

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/blxbis/ratowerskin1_spot.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/blxbis/ratowerskin1_spot.png
	}
endif

	{
		clampmap textures/baxandall/item_indi_1.png
		blendfunc add
	}
	{
		clampmap textures/baxandall/item_indi_2_green.png
		blendfunc add
		tcmod rotate 270
	}
	{
		clampmap textures/baxandall/item_indi_3.png
		blendfunc add
		tcmod rotate 180
		tcmod scale 1 1
	}
	{
		clampmap textures/baxandall/item_indi_4.png
		blendfunc add
		tcmod rotate -180
		tcmod scale 1 1
	}
}

mapobjects_wdm4_towerskin1
{
	qer_editorimage textures/blxbis/towerskin1.png
	surfaceparm nomarks

	{
		material textures/blxbis/towerskin1.png
	}
}

mapobjects_wdm4_towerskin2
{
	qer_editorimage textures/blxbis/towerskin2.png
	surfaceparm nomarks

	{
		material textures/blxbis/towerskin2.png
	}
}

mapobjects_wdm4_scratches
{
	qer_editorimage textures/blxbis/scratches0002_tiled.png

	{
		material textures/blxbis/scratches0002_tiled.png
	}
}

mapobjects_wdm4_wt3_pillar3
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material textures/blx_wtest3/blx_wt3_pillar3.png
	}
}

