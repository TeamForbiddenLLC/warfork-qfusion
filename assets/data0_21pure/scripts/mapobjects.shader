
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

//==================================================

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
	q3map_maxsamplesize 4
	q3map_minsmooth 2.0
	q3map_vertexcolor 0.988235 0.945098 0.482353
	surfaceparm nonsolid
	surfaceparm nomarks
	nopicmip
	glossExponent 75

	{
		material models/mapobjects/jumppad1/diffuse
	}

	{
		animmap 8 models/mapobjects/jumppad1/glow_01.png models/mapobjects/jumppad1/glow_02.png  models/mapobjects/jumppad1/glow_03.png 
		rgbgen vertex
		blendfunc add
	}
}

models/mapobjects/jumppad1/diffuse_a
{
	qer_editorimage models/mapobjects/jumppad1/diffuse_a.png
	q3map_maxsamplesize 4
	q3map_minsmooth 2.0
	q3map_vertexcolor 0.988235 0.945098 0.482353
	surfaceparm nonsolid
	surfaceparm nomarks
	nopicmip
	glossExponent 75

	{
		material models/mapobjects/jumppad1/diffuse_a
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


models/mapobjects/teleporter/teleporter_01
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01.png
	q3map_maxsamplesize 4
	q3map_minsmooth 1.0
	surfaceparm nonsolid
	surfaceparm nomarks
	nopicmip
	glossExponent 75

	{
		material models/mapobjects/teleporter/teleporter_01.png
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
	q3map_maxsamplesize 4
	q3map_minsmooth 1.0
	surfaceparm nonsolid
	surfaceparm nomarks
	nopicmip
	glossExponent 75

	{
		material models/mapobjects/teleporter/teleporter_01_a.png
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
	q3map_maxsamplesize 4
	q3map_minsmooth 1.0
	surfaceparm nonsolid
	surfaceparm nomarks
	nopicmip
	glossExponent 75

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
		material models/mapmodels2026/teleporter01/teleporter_01_b.png
		blendfunc blend
	}

	{
		map models/mapmodels2026/teleporter01/teleporter_01_b_shine.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .4 0 5
	}
}

models/mapobjects/teleporter/teleporter_01_glow
{
	qer_editorimage models/mapobjects/teleporter/teleporter_01_glow.png
	qer_trans 0.5
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip
	cull front
	
	{
		map models/mapmodels2026/teleporter01/teleporter_01_glow.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 0 .3
		tcmod scroll 0.02 0
	}
	{
		map models/mapmodels2026/teleporter01/teleporter_01_glow.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .5 .5 .3 .3
		tcmod scroll -0.02 0
	}
}

//==================================================


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


//============= INDUSTRIAL ========================

models/mapobjects/industrial/barrel_1/albedo
{
	qer_editorimage models/mapobjects/industrial/barrel_1/albedo
	//q3map_maxsamplesize 4
	//q3map_minsmooth 1.5
	glossExponent 80
	//glossIntensity 2

	{
		material models/mapobjects/industrial/barrel_1/albedo
	}
}


models/mapobjects/industrial/barrels/albedo
{
	qer_editorimage models/mapobjects/industrial/barrels/albedo
	//q3map_maxsamplesize 4
	//q3map_minsmooth 1.5
	glossExponent 90
	glossIntensity 2

	{
		material models/mapobjects/industrial/barrels/albedo
	}
}


models/mapobjects/industrial/crates/albedo
{
	qer_editorimage models/mapobjects/industrial/crates/albedo
	//q3map_maxsamplesize 4
	//q3map_minsmooth 1.5
	glossExponent 80
	//glossIntensity 0.75

	{
		material models/mapobjects/industrial/crates/albedo
	}
}


models/mapobjects/industrial/crates/albedovc
{
	qer_editorimage models/mapobjects/industrial/crates/albedovc
	//q3map_maxsamplesize 4
	//q3map_minsmooth 1.5
	glossExponent 100
	glossIntensity 0.5
	q3map_vertexcolor 0.858824 0.619608 0.203922

	{
		material models/mapobjects/industrial/crates/albedo models/mapobjects/industrial/crates/albedo_norm  models/mapobjects/industrial/crates/albedo_gloss 
	}
	{
		material models/mapobjects/industrial/crates/albedo_color models/mapobjects/industrial/crates/albedo_norm  models/mapobjects/industrial/crates/albedo_gloss 
		rgbgen vertex
		blendfunc blend
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



