
objectmodel_baseshader
{
	q3map_forceMeta
	q3map_nonPlanar
	q3map_lightmapMergable

	// lightmap quality settings
	q3map_lightmapSampleOffset 16
	q3map_lightmapsamplesize 8
}


models/mapobjects/enshotsphere/sphere
{
	qer_editorimage models/mapobjects/enshotsphere/sphere.png
	qer_trans 0.5
	nopicmip
	nomipmaps
	cull back

	{
		map env/televoid/void_env.png
		tcmod scale 4 4
	}
}

models_mapobjects_lamp_lamp
{
	qer_editorimage models/mapobjects/lamp/lamp.png
	surfaceparm nolightmap
	nopicmip
	nomipmaps
	glossExponent 96

	{
		rgbgen vertex
		material models/mapobjects/lamp/lamp.png models/mapobjects/lamp/lamp_norm.png models/mapobjects/lamp/lamp_gloss.png
	}

	{
		map models/mapobjects/lamp/lamp_alpha.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

models_mapobjects_lamp_lamp_yellow
{
	qer_editorimage models/mapobjects/lamp/lamp_yellow.png
	surfaceparm nolightmap
	nopicmip
	nomipmaps
	glossExponent 96

	{
		rgbgen vertex
		material models/mapobjects/lamp/lamp_yellow.png models/mapobjects/lamp/lamp_norm.png models/mapobjects/lamp/lamp_gloss.png
	}

	{
		map models/mapobjects/lamp/lamp_alpha.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

models_mapobjects_lamp_lamp_blue
{
	qer_editorimage models/mapobjects/lamp/lamp_blue.png
	surfaceparm nolightmap
	nopicmip
	nomipmaps
	glossExponent 96

	{
		rgbgen vertex
		material models/mapobjects/lamp/lamp_blue.png models/mapobjects/lamp/lamp_norm.png models/mapobjects/lamp/lamp_gloss.png
	}

	{
		map models/mapobjects/lamp/lamp_alpha.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

models_mapobjects_lamp_lamp_halo
{
	qer_editorimage textures/wsw_flareshalos/glow_halo_white.png
	qer_trans 0.25
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite2
	softParticle

	{
		detail
		clampmap textures/wsw_flareshalos/glow_halo_white.png
		blendfunc add
		rgbgen wave distanceramp 0 0.7 80 400
	}
}

models/mapobjects/house1/outer_alpha
{
	{
		map models/mapobjects/house1/outer_alpha.png
		blendfunc blend
	}
}

models/mapobjects/house1/inner
{
	{
		map models/mapobjects/house1/inner.png
	}
	{
		map models/mapobjects/house1/inner_light.png
		blendfunc add
	}
}

models/mapobjects/house3/house3
{
	{
		map models/mapobjects/house3/house3.png
	}
	{
		map models/mapobjects/house3/house3_light.png
		blendfunc add
	}
}

models/mapobjects/house4/house4
{
	{
		map models/mapobjects/house4/house4.png
	}
	{
		map models/mapobjects/house4/house4_light.png
		blendfunc add
	}
}

models/mapobjects/jumppad/flame
{
	cull none
	nopicmip
	surfaceparm nolightmap
	deformVertexes autosprite2

	{
		detail
		map models/mapobjects/jumppad/flame.png
		blendfunc add
		rgbGen wave sin 0.5 1 0 0.3
	}
}

models/mapobjects/jumppad/jumppad1
{
	cull none
	nopicmip
	qer_editorimage models/mapobjects/jumppad/jumppad1.png

	if deluxe
	{
		material models/mapobjects/jumppad/jumppad1_diffuse.png models/mapobjects/jumppad/jumppad1_norm.png models/mapobjects/jumppad/jumppad1_gloss.png
	}
	endif

	if ! deluxe 
	{
		map $lightmap
	}
	{
		map models/mapobjects/jumppad/jumppad1.png
		blendfunc filter
	}
	endif
	{
		map models/mapobjects/jumppad/jumppad1_light.png
		blendFunc add
	}
}

models/mapobjects/jumppad1/diffuse
{
	qer_editorimage models/mapobjects/jumppad1/diffuse.png
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip

	{
		rgbgen vertex
		material models/mapobjects/jumppad1/diffuse.png models/mapobjects/jumppad1/normal.png
	}

	{
		animmap 8 models/mapobjects/jumppad1/glow_01.png models/mapobjects/jumppad1/glow_02.png  models/mapobjects/jumppad1/glow_03.png 
		blendfunc add
	}
}

models/mapobjects/jumppad1/diffuse_a
{
	qer_editorimage models/mapobjects/jumppad1/diffuse_a.png
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip

	{
		rgbgen vertex
		material models/mapobjects/jumppad1/diffuse_a.png models/mapobjects/jumppad1/normal_a.png
	}

	{
		animmap 8 models/mapobjects/jumppad1/glow_a_01.png models/mapobjects/jumppad1/glow_a_02.png  models/mapobjects/jumppad1/glow_a_03.png 
		blendfunc add
	}
}

models/mapobjects/jumppad/u_ring
{
	cull none
	nopicmip
	surfaceparm nolightmap
	deformVertexes move 0 0 4 sin 0 1 0 0.5
	{
		map models/mapobjects/jumppad/u_ring.png
		blendfunc add
		alphaFunc GT0
	}
}

models/mapobjects/jumppad/l_ring
{
	cull none
	nopicmip
	surfaceparm nolightmap
	deformVertexes move 0 0 8 sin 0 1 0.5 0.6

	{
		map models/mapobjects/jumppad/l_ring.png
		blendfunc add
		alphaFunc GT0
	}
}


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

mapobjects_leds_iron_frame
{
	qer_editorimage models/mapobjects/lights/leds_iron_frame
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		rgbgen vertex
		material models/mapobjects/lights/leds_iron_frame.png
	}
}

mapobjects_leds_orange
{	
	qer_editorimage models/mapobjects/lights/leds_light_orange.png
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/leds_light_orange.png
		rgbgen identity
	}
}

models_mapobjects_decor_misc_powerline
{
	qer_editorimage models/mapobjects/decor_misc/powerline
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/powerline.png
	}
}

models_mapobjects_decor_misc_barrel
{
	qer_editorimage models/mapobjects/decor_misc/barrel
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/barrel.png
	}
}

models_mapobjects_decor_misc_fireextinguisher
{
	qer_editorimage models/mapobjects/decor_misc/fireextinguisher
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 64

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/fireextinguisher.png
	}
}

mapobjects_decor_misc_hammer
{
	qer_editorimage models/mapobjects/decor_misc/hammer.png
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 128

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/hammer.png $blankbumpimage models/mapobjects/decor_misc/hammer_gloss.png
	}
}

mapobjects_decor_misc_spanner
{
	qer_editorimage models/mapobjects/decor_misc/spanner.png
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 128

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/spanner.png $blankbumpimage $whiteImage
	}
}

mapobjects_decor_misc_disc
{
	qer_editorimage models/mapobjects/decor_misc/disc.png
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 128

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/disc.png $blankbumpimage
	}
}

mapobjects_decor_misc_povian
{
	qer_editorimage models/mapobjects/decor_misc/povian.png
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 128

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/povian.png $blankbumpimage $whiteImage
	}
}

models/mapobjects/teleporter/telenode
{
	nopicmip
	cull front
	qer_editorimage models/mapobjects/teleporter/telenode.png
	surfaceparm nolightmap
	surfaceparm nomarks
	glossExponent 100

	{
		rgbgen vertex
		material models/mapobjects/teleporter/telenode.png $blankbumpimage models/mapobjects/teleporter/telenode_gloss.png
	}
}

models/mapobjects/teleporter/telenodefx
{
	qer_trans 0.25
	qer_editorimage models/mapobjects/teleporter/telenodefx.png
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	nopicmip

	{
		detail
		clampmap models/mapobjects/teleporter/telenodefx.png
		blendFunc add
		rgbGen wave sin .5 .5 0 1.5
		//tcMod stretch sin 1.2 .8 0 1.5
	}
}

models/mapobjects/crates/container_red
{	
	qer_editorimage models/mapobjects/crates/container_red.png
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_red.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
	}
}

models/mapobjects/crates/container_red_trans
{	
	qer_editorimage models/mapobjects/crates/container_red.png
	qer_trans 0.8
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_red.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
		blendFunc blend
	}
}


models/mapobjects/crates/container_blue
{	
	qer_editorimage models/mapobjects/crates/container_blue.png
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_blue.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
	}
}

models/mapobjects/crates/container_blue_trans
{	
	qer_editorimage models/mapobjects/crates/container_blue.png
	qer_trans 0.8
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_blue.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
		blendFunc blend
	}
}


models/mapobjects/crates/container_green
{	
	qer_editorimage models/mapobjects/crates/container_green.png
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_green.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
	}
}

models/mapobjects/crates/container_green_trans
{	
	qer_editorimage models/mapobjects/crates/container_green.png
	qer_trans 0.8
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	q3map_forceMeta
	glossExponent 96

	{
		material models/mapobjects/crates/container_green.png models/mapobjects/crates/container_norm.png models/mapobjects/crates/container_gloss.png
		blendFunc blend
	}
}

//==================================================

models/mapobjects/decor_misc/aircondition_01
{
   	qer_editorimage models/mapobjects/decor_misc/aircondition_01.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/aircondition_01.png models/mapobjects/decor_misc/aircondition_01_norm.png models/mapobjects/decor_misc/aircondition_01_gloss.png
	}
}

models/mapobjects/decor_misc/aircondition_02
{
   	qer_editorimage models/mapobjects/decor_misc/aircondition_02.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/aircondition_02.png models/mapobjects/decor_misc/aircondition_01_norm.png models/mapobjects/decor_misc/aircondition_01_gloss.png
	}
}

models/mapobjects/decor_misc/aircondition_02a
{
   	qer_editorimage models/mapobjects/decor_misc/aircondition_02a.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/aircondition_02a.png models/mapobjects/decor_misc/aircondition_01_norm.png models/mapobjects/decor_misc/aircondition_01_gloss.png
	}
}

models/mapobjects/decor_misc/aircondition_fan_01
{
   	qer_editorimage models/mapobjects/decor_misc/aircondition_fan_01.png
	qer_trans 0.25
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/decor_misc/aircondition_fan_01.png
		blendFunc blend
		tcmod rotate 160
	}
}

//==================================================

models/mapobjects/lights/coldlight_01
{
   	qer_editorimage models/mapobjects/lights/coldlight_01.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/lights/coldlight_01.png models/mapobjects/lights/coldlight_01_norm.png models/mapobjects/lights/coldlight_01_gloss.png
	}   
}
models/mapobjects/lights/coldlight_01_refl
{
   	qer_editorimage models/mapobjects/lights/coldlight_01.png
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/coldlight_01.png
		rgbgen vertex
	}
	{
		map env/2d/map1.png 
		tcGen environment
		alphagen const 0.35
		blendFunc blend
	}
}
models/mapobjects/lights/coldlight_01_tube
{
   	qer_editorimage models/mapobjects/lights/coldlight_01.png
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/coldlight_01.png
	}
}
models/mapobjects/lights/coldlight_01_glass
{
    qer_editorimage models/mapobjects/lights/coldlight_01_glass.png
	qer_trans 0.25
      	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map models/mapobjects/lights/grad.png
		blendfunc add
		tcGen environment 		
	}
	{ 
      		map models/mapobjects/lights/coldlight_01_glass.png
		blendFunc blend
	}
}

models/mapobjects/lights/coldlight_01a
{
    qer_editorimage models/mapobjects/lights/coldlight_01a.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material models/mapobjects/lights/coldlight_01a.png models/mapobjects/lights/coldlight_01a_norm.png models/mapobjects/lights/coldlight_01a_gloss.png
	}
}
models/mapobjects/lights/coldlight_01a_refl
{
    qer_editorimage models/mapobjects/lights/coldlight_01a.png
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/coldlight_01a.png
		rgbgen vertex
	}
	{
		map env/2d/map1.png 
		tcGen environment
		alphagen const 0.35
		blendFunc blend
	}
}
models/mapobjects/lights/coldlight_01a_tube
{
    qer_editorimage models/mapobjects/lights/coldlight_01a.png
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/coldlight_01a.png
	}
}
models/mapobjects/lights/coldlight_01a_glass
{
    qer_editorimage models/mapobjects/lights/coldlight_01_glass.png
	qer_trans 0.25
    surfaceparm nolightmap
	surfaceparm nomarks

	{
		map models/mapobjects/lights/grad.png
		blendfunc add
		tcGen environment 		
	} 
	{ 
      		map models/mapobjects/lights/coldlight_01_glass.png
		blendFunc blend
	}
}

//==================================================
models/mapobjects/teleporter/teleporter_01
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01.png
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip

	{
		rgbgen vertex
		material models/mapobjects/teleporter/teleporter_01.png models/mapobjects/teleporter/teleporter_01_norm.png
	}
	{
		map models/mapobjects/teleporter/teleporter_01_shine.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .4 0 .3
	}
}

models/mapobjects/teleporter/teleporter_01_a
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01_a.png
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip

	{
		rgbgen vertex
		material models/mapobjects/teleporter/teleporter_01_a.png models/mapobjects/teleporter/teleporter_01_a_norm.png
	}
	{
		map models/mapobjects/teleporter/teleporter_01_a_shine.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .4 0 .3
	}
}

models/mapobjects/teleporter/teleporter_01_b
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01_b.png
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip

	if textureCubeMap
	{
		surroundmap env/televoid/void
		rgbgen identity
	}
	endif

	if ! textureCubeMap
	{
		map env/televoid/void_env
		tcGen environment
		rgbgen identity
	}
	endif

	{
		rgbgen vertex
		material models/mapobjects/teleporter/teleporter_01_b.png models/mapobjects/teleporter/teleporter_01_b_norm.png
		blendfunc blend
	}

	{
		map models/mapobjects/teleporter/teleporter_01_b_shine.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .4 0 5
	}
}

models/mapobjects/teleporter/teleporter_01_glow
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01_glow.png
	qer_trans 0.5
	surfaceparm trans
	nopicmip
	cull front
	{
		map models/mapobjects/teleporter/teleporter_01_glow.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .3
		tcmod scroll 0.02 0
	}
	{
		map models/mapobjects/teleporter/teleporter_01_glow.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 .3 .3
		tcmod scroll -0.02 0
	}
}
//==================================================

models/mapobjects/vehicles/forklift
{
	qer_editorimage models/mapobjects/vehicles/forklift.png
	surfaceparm nomarks
	surfaceparm nolightmap
	//surfaceparm nonsolid
	nopicmip

	{
		rgbgen vertex
		material models/mapobjects/vehicles/forklift
	}
}

//==================================================

models/mapobjects/orb/orb
{
	qer_editorimage models/mapobjects/orb/orb
	surfaceparm pointlight
	surfaceparm nolightmap
	
	{
		material models/mapobjects/orb/orb
		rgbgen vertex
	}
}
