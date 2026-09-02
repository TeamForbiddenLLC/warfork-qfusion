//==============================================
//
// Deprecated shaders.
//
// FOR 0.5 COMPATIBILITY ONLY!!!!!!
//
// Do not add this shader file to shaderlist.txt so it isn't used at new maps.
//
//==============================================



///////////////////////////////////////////////////
///////////////////////////////////////////////////
//	DEPRECATED 2026
///////////////////////////////////////////////////
///////////////////////////////////////////////////

// cha0s_wsw
textures/cha0s_ws/trim19_black
{
	surfaceparm nolightmap
	qer_editorimage textures/cha0s_ws/trim19_black.png

	{
		map textures/cha0s_ws/trim19_black.png
	}
}

textures/cha0s_ws/base_column
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const 0.117647 0.196078 0.156863
	}
}

textures/cha0s_ws/base_floor
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const 0.117647 0.196078 0.156863
	}
}

textures/cha0s_ws/base_wall
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const 0.117647 0.196078 0.156863
	}
}

textures/cha0s_ws/base_grey
{
	qer_editorimage textures/concrete/concreteclean

	{
		material textures/concrete/concreteclean textures/concrete/concreteclean_norm
	}
}

textures/cha0s_ws/ceiling_grey
{
	qer_editorimage textures/concrete/concreteclean

	{
		material textures/concrete/concreteclean textures/concrete/concreteclean_norm
	}
}

textures/cha0s_ws/cement_1_clean
{
	qer_editorimage textures/concrete/concreteclean

	{
		material textures/concrete/concreteclean textures/concrete/concreteclean_norm
	}
}

textures/cha0s_ws/cement_1_grimy
{
	qer_editorimage textures/metal/metalrust_iron6

	{
		material textures/metal/metalrust_iron6 textures/factory/factory_wall_norm
	}
}

textures/cha0s_ws/cement_1_gunky
{
	qer_editorimage textures/metal/metalrust_iron6

	{
		material textures/metal/metalrust_iron6 textures/factory/factory_wall_norm
	}
}

textures/cha0s_ws/cement_2
{
	qer_editorimage textures/concrete/concretecold01
	q3map_chamfer_concavewidth 0.5

	{
		material textures/concrete/concretecold01 textures/concrete/concretecold01_norm
		tcMod scale 3 3
	}
}

textures/cha0s_ws/cement_2_height
{
	qer_editorimage textures/concrete/concretecold01
	q3map_chamfer_concavewidth 0.5

	{
		material textures/concrete/concretecold01 textures/concrete/concretecold01_norm
		tcMod scale 3 3
	}
}

textures/cha0s_ws/cement_2_yellow
{
	qer_editorimage textures/metal/metalrust_iron6

	{
		material textures/cha0s_ws/cement_2_yellow_flat textures/factory/factory_wall_norm
	}
}

textures/cha0s_ws/cement_2
{
	qer_editorimage textures/concrete/concrete2.png

	{
		material textures/concrete/concrete2.png
	}
}

textures/cha0s_ws/concrete-4
{
	qer_editorimage textures/facility/concrete4.png

	{
		material textures/facility/concrete4.png textures/facility/concrete4_norm.png
	}
}



// refly bricks
textures/bricks_vertex/vertex_redbricks_base_01
{	
	qer_editorimage textures/bricks/redbricks_base_01
	surfaceparm nolightmap

if ! deluxe

	{
		map textures/bricks/redbricks_base_01
		rgbgen vertex
	}
endif

if deluxe
	{
		rgbgen lightingDiffuse
		material textures/bricks/redbricks_base_01
	}
endif
}


textures/refly_bricks/bricksdark1org
{	
	qer_editorimage textures/bricks/bricks_05_black
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	{
		material textures/bricks/bricks_05_black textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/bricksdark1
{	
	qer_editorimage textures/bricks/bricks_05_bricksdark1
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	glossExponent 100
	glossIntensity 0.2
	{
		material textures/bricks/bricks_05_bricksdark1 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/bricksdark1org
{	
	qer_editorimage textures/bricks/bricks_05_bricksdark1
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	glossExponent 100
	glossIntensity 0.2
	{
		material textures/bricks/bricks_05_bricksdark1 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/Bricks_Refly_DarkGrey1
{	
	qer_editorimage textures/bricks/bricks_05_darkgrey1
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	glossExponent 100
	glossIntensity 0.2
	{
		material textures/bricks/bricks_05_darkgrey1 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/bricks1
{	
	qer_editorimage textures/bricks/bricks_05_bricks1
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	glossExponent 100
	glossIntensity 0.4
	{
		material textures/bricks/bricks_05_bricks1 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/bricks2
{	
	qer_editorimage textures/bricks/bricks_05_bricks2
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	{
		material textures/bricks/bricks_05_bricks2 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}

textures/refly_bricks/bricks4
{	
	qer_editorimage textures/bricks/bricks_05_bricks4
	q3map_chamfer_convexwidth 1.75
	q3map_chamfer_concavewidth 1
	glossExponent 100
	glossIntensity 0.4
	{
		material textures/bricks/bricks_05_bricks4 textures/bricks/bricks_05_norm textures/bricks/bricks_05_gloss
	}
}







////////////////////////////////////////////////////
////////////////////////////////////////////////////
//   MOVED BEFORE 0.21
////////////////////////////////////////////////////
////////////////////////////////////////////////////

textures/ecel/ecel_met02
{
	qer_editorimage textures/cleansurface/pantone7536c.png
	glossExponent 64

	{
		material textures/cleansurface/pantone7536c $blankbumpimage textures/cleansurface/pantone7536c
	}
}

textures/ecel/ecel_met03
{
	qer_editorimage textures/cleansurface/pantonecoolgray10c.png
	glossExponent 64

	{
		material textures/cleansurface/pantonecoolgray10c $blankbumpimage textures/cleansurface/pantonecoolgray10c
	}
}

textures/ecel/ecel_des01cmtl
{
	qer_editorimage textures/cleansurface/pantonecoolgray10c.png
	glossExponent 64

	{
		material textures/cleansurface/pantonecoolgray10c $blankbumpimage textures/cleansurface/pantonecoolgray10c
	}
}

textures/exwsw/matte_ex_trim_baseboard_02
{
	qer_editorimage textures/exwsw/ex_trim_baseboard_02
	glossExponent 64

	{
		material textures/exwsw/ex_trim_baseboard_02
	}
}

textures/fakeads_hor/bigbill6
{
	qer_editorimage textures/billboard/bigbill6.png
	q3map_surfacelight   20
	q3map_forceMeta
	q3map_lightmapSampleSize 128
	surfaceparm	nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	cull none
	nopicmip

	{
		clampmap textures/billboard/bigbill6.png
	}
}

textures/fakeads_hor/small1
{
	qer_editorimage textures/billboard/small1.png
	q3map_surfacelight   20
	q3map_forceMeta
	q3map_lightmapSampleSize 128
	surfaceparm	nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	cull none
	nopicmip

	{
		clampmap textures/billboard/small1.png
	}
}

textures/fakeads_ver/neon1
{
	qer_editorimage textures/billboard/neon1.png
	q3map_surfacelight   20
	q3map_forceMeta
	q3map_lightmapSampleSize 128
	surfaceparm	nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	cull none
	nopicmip

	{
		clampmap textures/billboard/neon1.png
	}
}

textures/wsw_adverts/terebi
{
	qer_editorimage textures/billboard/terebi_07.png
	q3map_lightimage   textures/billboard/terebi_06.png
	q3map_surfacelight   20
	q3map_forceMeta
	q3map_lightmapSampleSize 128
	surfaceparm	nomarks
	surfaceparm nodlight
	surfaceparm trans
	surfaceparm nonsolid
	cull none
	nopicmip
	{
		animmap 1 textures/billboard/terebi_01.png textures/billboard/terebi_02.png textures/billboard/terebi_03.png textures/billboard/terebi_04.png textures/billboard/terebi_05.png textures/billboard/terebi_06.png textures/billboard/terebi_07.png
	}
}

//***********************************
// cha0s_wsw
//***********************************

textures/cha0s_ws/phonograph_slate_2
{
	qer_editorimage textures/lights/blacktrim01
	q3map_lightRGB 1 1 1
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/blacktrim01.png
	}
}

textures/cha0s_ws/phonograph_slate_2_mid
{
	qer_editorimage textures/lights/blacktrim01
	q3map_lightRGB 1 1 1
	q3map_surfacelight 2000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/blacktrim01.png
	}
}

textures/cha0s_ws/phonograph_slate_2_strong
{
	qer_editorimage textures/lights/blacktrim01
	q3map_lightRGB 1 1 1
	q3map_surfacelight 5000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/blacktrim01.png
	}
}

textures/cha0s_ws/sw_xlight
{
	qer_editorimage textures/lights/blacktrim02.png
	q3map_lightRGB 1 1 1
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/blacktrim02.png
	}
}


textures/cha0s_ws/phonograph_bronze_3_soft
{
	qer_editorimage textures/lights/phonograph.png
	q3map_lightRGB 0.95 0.91 0.71
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/phonograph.png
	}
}

textures/cha0s_ws/phonograph_bronze_3_mid
{
	qer_editorimage textures/lights/phonograph.png
	q3map_lightRGB 0.95 0.91 0.71
	q3map_surfacelight 2500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/phonograph.png
	}
}

textures/cha0s_ws/phonograph_bronze_3
{
	qer_editorimage textures/lights/phonograph.png
	q3map_lightRGB 0.95 0.91 0.71
	q3map_surfacelight 5000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/phonograph.png
	}
}

textures/cha0s_ws/neon1_2k
{
	qer_editorimage textures/lights/neonlamp.png
	q3map_lightimage textures/lights/neonlamp.png
	q3map_surfacelight 2500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map $lightmap
		rgbGen identity
	}

	{
		map textures/lights/neonlamp.png
		blendFunc filter
	}

	{
		map textures/lights/neonlamp.blend.png
		blendfunc add
	}
}

textures/cha0s_ws/neon1_4k
{
	qer_editorimage textures/lights/neonlamp.png
	q3map_lightimage textures/lights/neonlamp.png
	q3map_surfacelight 5000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map $lightmap
		rgbGen identity
	}

	{
		map textures/lights/neonlamp.png
		blendFunc filter
	}

	{
		map textures/lights/neonlamp.blend.png
		blendfunc add
	}
}

textures/cha0s_ws/neon2_100
{
	qer_editorimage textures/lights/01neutres.png
	q3map_lightimage textures/lights/01neutres.png
  	q3map_surfacelight 1000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/01neutres.png
	}
}

textures/cha0s_ws/neon2_medium
{
	qer_editorimage textures/lights/01neutres.png
	q3map_lightimage textures/lights/01neutres.png
  	q3map_surfacelight 2000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/01neutres.png
	}
}

textures/cha0s_ws/concrete-3-yellow
{
	qer_editorimage textures/wsw_city1/concrete3.png

	{
		material textures/wsw_city1/concrete3.png textures/wsw_city1/concrete3_norm.png
	}
}

textures/cha0s_ws/realflat_lt
{
	qer_editorimage textures/cha0s_ws/flat_lt.png

	{
		material textures/cha0s_ws/flat_lt.png
	}
}

//***********************************
// wsw_city1
//***********************************

textures/wsw_city1/chrome
{	
	qer_editorimage textures/metal/chrome
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
	}

	{
		map $whiteImage
		rgbgen vertex
		alphagen const 0.25
		blendFunc blend
	}

	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
		alphagen const 0.6
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/wsw_city1/concrete1_red
{	
	qer_editorimage textures/concrete/plasterred01.png

	{
		material textures/concrete/plasterred01.png
	}
}

textures/wsw_city1/concrete2
{	
	qer_editorimage textures/concrete/stucco01.png

	{
		material textures/concrete/stucco01.png
	}
}

textures/wsw_city1/concrete2b
{	
	qer_editorimage textures/concrete/stucco01b.png

	{
		material textures/concrete/stucco01b.png textures/concrete/stucco01_norm.png
	}
}

textures/wsw_city1/concrete2c
{	
	qer_editorimage textures/concrete/stucco01c.png

	{
		material textures/concrete/stucco01c.png textures/concrete/stucco01_norm.png
	}
}

textures/wsw_city1/concrete2e
{	
	qer_editorimage textures/concrete/stucco01e.png

	{
		material textures/concrete/stucco01e.png textures/concrete/stucco01_norm.png
	}
}

textures/wsw_city1/concrete4
{	
	qer_editorimage textures/concrete/blueplaster.png

	{
		material textures/concrete/blueplaster.png
	}
}

textures/wsw_city1/ceil02
{
	qer_editorimage textures/cleansurface/pantone432c

	{
		material textures/cleansurface/pantone432c $blankbumpimage
	}
}

textures/wsw_city1/tech_concrete1
{
	qer_editorimage textures/concrete/concretecold01

	{
		material textures/concrete/concretecold01 textures/concrete/concretecold01_norm
		tcMod scale 2 2
	}
}

textures/wsw_city1/tech_concrete2
{
	qer_editorimage textures/concrete/concretecold02

	{
		material textures/concrete/concretecold02 textures/concrete/concretecold01_norm
		tcMod scale 2 2
	}
}

textures/wsw_city1/stepup01
{
	qer_editorimage textures/cleansurface/pantone425c

	{
		material textures/cleansurface/pantone425c $blankbumpimage
	}
}

textures/wsw_city1/stepup02
{
	qer_editorimage textures/cleansurface/pantone7536c

	{
		material textures/cleansurface/pantone7536c $blankbumpimage
	}
}

textures/wsw_city1/grid2
{
	qer_editorimage textures/grates/grid02.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material textures/grates/grid02.png
		alphaFunc GE128
		depthWrite
	}
}

textures/wsw_city1/0080
{	
	qer_editorimage textures/decals/0080.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/0080.png
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/wsw_city1/window_tall01
{
	qer_editorimage textures/window/purple01.png

	{
		material textures/window/purple01
	}
}

//***********************************
// blx_wtest3
//***********************************

textures/blx_wtest3/blx_wt3_dgrey
{
	qer_editorimage textures/cleansurface/pantonecoolgray10c.png

	{
		material textures/cleansurface/pantonecoolgray10c $blankbumpimage
	}
}

textures/blx_wtest3/stairlight
{	
	qer_editorimage textures/lights/blacktrim01
	q3map_lightRGB 1 1 1
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/blacktrim01.png
	}
}

textures/blx_wtest3/blx_wt3_light
{
	qer_editorimage textures/lights/tech.png
	q3map_lightimage   textures/lights/tech.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	if ! deluxe
		{
			map $lightmap
			rgbGen identity
		}
		{
			map textures/lights/tech.png
			blendfunc filter
		}
	endif

	if deluxe
		{
			material textures/lights/tech.png
		}
	endif

	{
		map textures/lights/tech.blend.png
		blendfunc add	
	}
}

textures/blx_wtest3/blx_wt3_light_nonsolid
{
	qer_editorimage textures/lights/tech.png
	q3map_lightimage   textures/lights/tech.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	if ! deluxe
		{
			map $lightmap
			rgbGen identity
		}
		{
			map textures/lights/tech.png
			blendfunc filter
		}
	endif

	if deluxe
		{
			material textures/lights/tech.png
		}
	endif

	{
		map textures/lights/tech.blend.png
		blendfunc add	
	}
}

textures/blx_wtest3/blx_wt3_light1
{
	qer_editorimage textures/lights/panel1.png
	q3map_lightimage   textures/lights/panel1.png
	q3map_surfacelight 1500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/lights/panel1.png
	}
}


textures/blx_wtest3/blx_wt3_pillar3
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png

	{
		material textures/blx_wtest3/blx_wt3_pillar3.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
	}
}


//***********************************
// blx_ca
//***********************************

textures/blx_ca/blx_ca_light_soft
{	
	qer_editorimage textures/lights/01bluel.png
	q3map_lightimage textures/lights/01bluel.png
  	q3map_surfacelight 400
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluel.png
	}
}

textures/blx_ca/blx_ca_light
{	
	qer_editorimage textures/lights/01bluel.png
	q3map_lightimage textures/lights/01bluel.png
  	q3map_surfacelight 900
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluel.png
	}
}

textures/blx_ca/blx_ca_light_strong
{	
	qer_editorimage textures/lights/01bluel.png
	q3map_lightimage textures/lights/01bluel.png
	q3map_lightsubdivide 72
  	q3map_surfacelight 2500
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluel.png
	}
}

textures/blx_ca/blx_ca_light_short_soft
{	
	qer_editorimage textures/lights/01bluem.png
	q3map_lightimage textures/lights/01bluem.png
  	q3map_surfacelight 3000
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluem.png
	}
}


textures/blx_ca/blx_ca_light_short
{	
	qer_editorimage textures/lights/01bluem.png
	q3map_lightimage textures/lights/01bluem.png
  	q3map_surfacelight 15000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluem.png
	}
}

textures/blx_ca/blx_ca_light_square_soft
{	
	qer_editorimage textures/lights/01blues.png
	q3map_lightimage textures/lights/01blues.png
  	q3map_surfacelight 3000
	//q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01blues.png
	}
}

textures/blx_ca/blx_ca_light_square_verysoft
{	
	qer_editorimage textures/lights/01blues.png
	q3map_lightimage textures/lights/01blues.png
  	q3map_surfacelight 600
	//q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01blues.png
	}
}

textures/blx_ca/blx_ca_light_square
{	
	qer_editorimage textures/lights/01blues.png
	q3map_lightimage textures/lights/01blues.png
  	q3map_surfacelight 15000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01blues.png
	}
}

textures/blx_ca/blx_ca_light_square_ultrabright
{	
	qer_editorimage textures/lights/01blues.png
	q3map_lightimage textures/lights/01blues.png
  	q3map_surfacelight 45000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01blues.png
	}
}

textures/blx_ca/blx_ca_light_whitelight
{	
	qer_editorimage textures/lights/01bluel.png
	q3map_lightRGB 1 1 1
	q3map_lightsubdivide 72
  	q3map_surfacelight 2000
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluel.png
	}
}

textures/blx_ca/blx_ca_light_whitelight_soft
{	
	qer_editorimage textures/lights/01bluel.png
	q3map_lightRGB 1 1 1
	q3map_lightsubdivide 72
  	q3map_surfacelight 400
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/01bluel.png
	}
}

//***********************************
// blx_houses
//***********************************

textures/blx_houses/blx_light_h_team1
{
	qer_editorimage textures/lights/glowtrim.png
	q3map_lightimage   textures/lights/glowtrim.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/lights/glowtrim.png
	}
	{
		map textures/lights/glowtrim.blend.png
		blendfunc add
		rgbGen teamcolor 2
	}
}

textures/blx_houses/blx_light_h_team1
{
	qer_editorimage textures/lights/glowtrim.png
	q3map_lightimage   textures/lights/glowtrim.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/lights/glowtrim.png
	}
	{
		map textures/lights/glowtrim.blend.png
		blendfunc add
		rgbGen teamcolor 3
	}
}

//***********************************
// blx_grey
//***********************************

textures/blx_grey/blx_grey_light_rh
{
	q3map_lightimage   textures/blx_grey/blx_grey_light_rh.png
  q3map_surfacelight   800
	surfaceparm nomarks
 	surfaceparm nolightmap
	{
		map textures/blx_grey/blx_grey_light_rh.png
	}
}

textures/blx_grey/blx_grey_light_h
{
	q3map_lightimage   textures/blx_grey/blx_grey_light_h.png
  q3map_surfacelight   800
	surfaceparm nomarks
 	surfaceparm nolightmap
	{
		map textures/blx_grey/blx_grey_light_h.png
	}
}

textures/blx_grey/blx_grey_light_blue
{
	q3map_lightimage   textures/blx_grey/blx_grey_light_blue.png
  q3map_surfacelight   1500
	surfaceparm nomarks
 	surfaceparm nolightmap
	{
		map textures/blx_grey/blx_grey_light_blue.png	
	}
}

//***********************************
// blx_tech2
//***********************************

textures/blx_tech2/tech2_light_1
{
	qer_editorimage textures/lights/trim2.png
	q3map_lightimage   textures/lights/trim2.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/trim2.png
	}
endif

	{
		map textures/lights/trim2.blend.png
		blendfunc add
	}
}

//***********************************
// blx
//***********************************

textures/blx/blx_light_blue
{
	qer_editorimage textures/lights/bluetrim.png
	q3map_lightimage textures/lights/bluetrim.png
	q3map_surfacelight 2500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bluetrim.png
	}
}

textures/blx/blx_light_h
{
	qer_editorimage textures/lights/bluetrim2.png
	q3map_lightimage textures/lights/bluetrim2.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bluetrim2.png
	}
}

textures/blx/blx_light_rh
{
	qer_editorimage textures/lights/bluetrim3.png
	q3map_lightimage textures/lights/bluetrim3.png
	q3map_surfacelight 1500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bluetrim3.png
	}
}

textures/blx/blx_ultrabright
{
	qer_editorimage textures/lights/bright1.png
	q3map_lightimage textures/lights/bright1.png
	q3map_surfacelight 20000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bright1.png
	}
}

//***********************************
// blxbis
//***********************************

textures/blxbis/blxbis_light_h
{
	qer_editorimage textures/lights/bluetrim2.png
	q3map_lightimage textures/lights/bluetrim2.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bluetrim2.png
	}
}

textures/blxbis/trim_5_light
{
	qer_editorimage textures/lights/trim05.png
	q3map_lightimage textures/lights/trim05.blend.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim05.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/trim05.png textures/lights/trim05_norm.png textures/lights/trim05_gloss.png
	}
endif

	{
		map textures/lights/trim05.blend.png
		blendFunc blend
		tcMod scroll 0.05 0
	}
}


textures/blxbis/trim_06
{
	qer_editorimage textures/lights/trim06.png
	q3map_lightimage textures/lights/trim06.blend.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim06.png
		blendFunc filter
	}

	{
		map textures/lights/trim06.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/trim06.png textures/lights/trim06_norm.png textures/lights/trim06_gloss.png textures/lights/trim06.blend.png
	}
endif
}

textures/blxbis/iceline_300_s
{
	qer_editorimage textures/lights/icelight.png
	q3map_lightimage textures/lights/icelight.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/icelight.png
	}
}

textures/blxbis/iceline_50_s
{
	qer_editorimage textures/lights/icelight.png
	q3map_lightimage textures/lights/icelight.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/icelight.png
	}
}

textures/blxbis/light_turq
{
	qer_editorimage textures/lights/turqs1.png
	q3map_lightimage textures/lights/turqs1.blend.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/turqs1.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs1.png
	}
endif

	{
		map textures/lights/turqs1.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/blxbis/light_turq_medium
{
	qer_editorimage textures/lights/turqs1.png
	q3map_lightimage textures/lights/turqs1.blend.png
	q3map_surfacelight 2500
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/turqs1.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs1.png
	}
endif

	{
		map textures/lights/turqs1.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}


textures/blxbis/light_turq_strong
{
	qer_editorimage textures/lights/turqs1.png
	q3map_lightimage textures/lights/turqs1.blend.png
	q3map_surfacelight 5000
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/turqs1.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs1.png
	}
endif

	{
		map textures/lights/turqs1.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/blxbis/light_turq2
{
	qer_editorimage textures/lights/turqs2.png
	q3map_lightimage textures/lights/turqs2.blend.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	polygonOffset

if ! deluxe
	{
		map textures/lights/turqs2.png
		blendFunc blend
	}

	{
		map $lightmap
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs2.png
		blendFunc blend
	}
endif

	{
		map textures/lights/turqs2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/blxbis/light_turq2_medium
{
	qer_editorimage textures/lights/turqs2.png
	q3map_lightimage textures/lights/turqs2.blend.png
	q3map_surfacelight 2500
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	polygonOffset

if ! deluxe
	{
		map textures/lights/turqs2.png
		blendFunc blend
	}

	{
		map $lightmap
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs2.png
		blendFunc blend
	}
endif

	{
		map textures/lights/turqs2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/blxbis/light_turq2_strong
{
	qer_editorimage textures/lights/turqs2.png
	q3map_lightimage textures/lights/turqs2.blend.png
	q3map_surfacelight 5000
  	q3map_lightsubdivide 86
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	polygonOffset

if ! deluxe
	{
		map textures/lights/turqs2.png
		blendFunc blend
	}

	{
		map $lightmap
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/turqs2.png
		blendFunc blend
	}
endif

	{
		map textures/lights/turqs2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/blxbis/aluminium
{
	qer_editorimage textures/blxbis/aluminium.png
	//q3map_lightmapsamplesize 32
	q3map_lightmapscale 4

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/blxbis/aluminium.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/blxbis/aluminium.png
	}
endif
}

textures/blxbis/vertex_aluminium
{
	qer_editorimage textures/blxbis/aluminium.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material textures/blxbis/aluminium.png
	}
}

//***********************************
// factory
//***********************************

textures/factory/factory_metal_floor2
{	
	qer_editorimage textures/metal/treadplate0015g.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/metal/treadplate0015g.png
		tcmod scale 0.33 0.33
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/metal/treadplate0015g.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015f_gloss.png
		tcmod scale 0.33 0.33
	}
endif
}

textures/factory/light_soft
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightimage textures/lights/oc2.blend.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/factory/light_medium
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightimage textures/lights/oc2.blend.png
	q3map_surfacelight 1500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/factory/light_bright
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightimage textures/lights/oc2.blend.png
	q3map_surfacelight 3500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/factory/light_strong
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightimage textures/lights/oc2.blend.png
	q3map_surfacelight 3500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		blendFunc blend
	}
}

textures/factory/light_yellow_soft
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightRGB 1 1 0.75
	q3map_surfacelight 800
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
		rgbgen const 1 0.92 0.48
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		rgbgen const 1 1 0.6
		blendFunc blend
	}
}

textures/factory/light_yellow_medium
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightRGB 1 1 0.75
	q3map_surfacelight 1500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
		rgbgen const 1 0.92 0.48
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		rgbgen const 1 1 0.6
		blendFunc blend
	}
}

textures/factory/light_yellow_bright
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightRGB 1 1 0.75
	q3map_surfacelight 3500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
		rgbgen const 1 0.92 0.48
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		rgbgen const 1 1 0.6
		blendFunc blend
	}
}

textures/factory/light_yellow_strong
{
	qer_editorimage textures/lights/oc2.png
	q3map_lightRGB 1 1 0.75
	q3map_surfacelight 3500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc2.png textures/lights/oc2_norm.png
	}
endif

	{
		map textures/lights/oc2_focus.png
		blendFunc blend
		rgbgen const 1 0.92 0.48
	}

	{
		map textures/lights/oc2.blend.png
		alphaGen wave sin 0.75 0.25 0.75 1.5
		rgbgen const 1 1 0.6
		blendFunc blend
	}
}

textures/factory/trim_03
{
	qer_editorimage textures/lights/trim03.png
	q3map_lightimage textures/lights/trim03.blend.png
	q3map_surfacelight 750
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim03.png
		blendFunc filter
	}

	{
		map textures/lights/trim03.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/trim03.png textures/lights/trim03_norm.png textures/lights/trim03_gloss.png textures/lights/trim03.blend.png
	}
endif
}


textures/factory/trim_03_verygloss
{
	qer_editorimage textures/lights/trim03.png
	q3map_lightimage textures/lights/trim03.blend.png
	q3map_surfacelight 750
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim03.png
		blendFunc filter
	}

	{
		map textures/lights/trim03.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/trim03.png textures/lights/trim03_norm.png textures/lights/trim03_gloss.png textures/lights/trim03.blend.png
	}
endif
}




textures/factory/trim_05_blueline
{
	qer_editorimage textures/lights/trim04.png
	q3map_lightimage textures/lights/trim04.blend.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/trim04.png
		blendFunc filter
	}

	{
		map textures/lights/trim04.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/trim04.png textures/lights/trim04_norm.png textures/lights/trim04_gloss.png textures/lights/trim04.blend.png
	}
endif
}

textures/factory/wall_light_soft
{
	qer_editorimage textures/lights/neon2s.png
	q3map_lightimage textures/lights/neon2s.blend.png
	q3map_surfacelight 7500
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/neon2s.png
		blendFunc filter
	}

	{
		map textures/lights/neon2s.blend.png
		rgbgen const 0.4 0.4 0.4
		blendfunc add
	}
}

textures/factory/wall_light_medium
{
	qer_editorimage textures/lights/neon2s.png
	q3map_lightimage textures/lights/neon2s.blend.png
	q3map_surfacelight 15000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/neon2s.png
		blendFunc filter
	}

	{
		map textures/lights/neon2s.blend.png
		rgbgen const 0.4 0.4 0.4
		blendfunc add
	}
}

//***********************************
// format
//***********************************

textures/format/lightstrip_baby
{	
	qer_editorimage textures/lights/stripcyan.blend.png
	q3map_lightimage textures/lights/stripcyan.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/stripcyan.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/stripcyan.blend.png
	}
endif
}

textures/format/lightstrip_blue
{	
	qer_editorimage textures/lights/stripblue.blend.png
	q3map_lightimage textures/lights/stripblue.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/stripblue.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/stripblue.blend.png
	}
endif
}

textures/format/lightstrip_green
{	
	qer_editorimage textures/lights/stripgreen.blend.png
	q3map_lightimage textures/lights/stripgreen.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/stripgreen.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/stripgreen.blend.png
	}
endif
}

textures/format/lightstrip_red
{	
	qer_editorimage textures/lights/stripred.blend.png
	q3map_lightimage textures/lights/stripred.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/stripred.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/stripred.blend.png
	}
endif
}

textures/format/lightstrip_yellow
{	
	qer_editorimage textures/lights/stripyellow.blend.png
	q3map_lightimage textures/lights/stripyellow.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/stripyellow.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/stripyellow.blend.png
	}
endif
}

textures/format/lightstrip_purple
{	
	qer_editorimage textures/lights/strippurple.blend.png
	q3map_lightimage textures/lights/strippurple.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/strippurple.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/strippurple.blend.png
	}
endif
}

textures/format/lightstrip_orange
{	
	qer_editorimage textures/lights/striporange.blend.png
	q3map_lightimage textures/lights/striporange.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}

	{
		map textures/lights/striporange.blend.png
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png textures/lights/striporange.blend.png
	}
endif
}

//***********************************
// supersymmetry
//***********************************

textures/supersymmetry/lamp_01
{
	qer_editorimage textures/lights/01neutres.png
	q3map_lightimage textures/lights/01neutres.png
  	q3map_surfacelight 2000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	q3map_lightmapSampleSize 128

	{
		map textures/lights/01neutres.png
	}
}

//***********************************
// wsw_cave1
//***********************************

textures/wsw_cave1/light_grid
{
	qer_editorimage textures/lights/gridl01.png
	q3map_lightimage textures/lights/gridl01.png
  	q3map_surfacelight 20000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	nopicmip
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white
		
	{
		map textures/lights/gridl01.png
	}
}

textures/wsw_cave1/light_sphere1
{
	qer_editorimage textures/lights/oc3.png
	q3map_lightimage textures/lights/oc3.png
  	q3map_surfacelight 20000
	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white
	
	{
		map textures/lights/oc3.png
	}
}

textures/wsw_cave1/rocks1
{
	qer_editorimage textures/terrain/rocks003_512x512.png
	q3map_lightmapMergable
	q3map_nonPlanar
	q3map_shadeAngle 120

if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/terrain/rocks003_512x512
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/terrain/rocks003_512x512
	}
endif
}

//***********************************
// wsw_city1
//***********************************

textures/wsw_city1/blue_light
{
	qer_editorimage textures/lights/blue1.png
	q3map_lightimage textures/lights/blue1.png
	q3map_surfacelight 10000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/blue1.png
	}
}

textures/wsw_city1/purple_light
{
	qer_editorimage textures/lights/purple1.png
	q3map_lightimage textures/lights/purple1.png
	q3map_surfacelight 10000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/purple1.png
	}
}

textures/wsw_city1/blue_lightstripes
{
	qer_editorimage textures/lights/bluetrim4.png
	q3map_lightimage textures/lights/bluetrim4.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bluetrim4.png
	}
}

textures/wsw_city1/orange_light
{
	qer_editorimage textures/lights/oc1.png
	q3map_lightimage textures/lights/oc1.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/oc1.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/oc1.png
	}
endif
}

textures/wsw_city1/red_light_oval
{
	q3map_lightRGB   1 0 0
	q3map_surfacelight 1000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map $whiteImage
		rgbgen const 1 0 0
	}
}


textures/wsw_city1/team_red_lightstrip
{	
	qer_editorimage textures/lights/strip.blend.png
	q3map_lightimage textures/lights/strip.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png
	}

	{
		map textures/lights/strip.blend.png
		rgbgen teamcolor 2
		blendFunc blend
	}
endif
}

textures/wsw_city1/team_blue_lightstrip
{	
	qer_editorimage textures/lights/strip.blend.png
	q3map_lightimage textures/lights/strip.blend.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/lights/strip.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/lights/strip.png textures/lights/strip_norm.png textures/lights/strip_gloss.png
	}

	{
		map textures/lights/strip.blend.png
		rgbgen teamcolor 3
		blendFunc blend
	}
endif
}

textures/wsw_city1/lowbright_withflare
{
	qer_editorimage textures/lights/bright1.png
	q3map_lightimage textures/lights/bright1.png
	q3map_surfacelight 10000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white

	{
		map textures/lights/bright1.png
	}
}

textures/wsw_city1/bright_withflare
{
	qer_editorimage textures/lights/bright1.png
	q3map_lightimage textures/lights/bright1.png
	q3map_surfacelight 10000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white

	{
		map textures/lights/bright1.png
	}
}

textures/wsw_city1/ultrabright_withflare
{
	qer_editorimage textures/lights/bright1.png
	q3map_lightimage textures/lights/bright1.png
	q3map_surfacelight 20000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white

	{
		map textures/lights/bright1.png
	}
}

textures/wsw_city1/megaultrabright_withflare
{
	qer_editorimage textures/lights/bright1.png
	q3map_lightimage textures/lights/bright1.png
	q3map_surfacelight 60000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap
	light 3
	flareshader textures/wsw_flareshalos/flare_sphere_white

	{
		map textures/lights/bright1.png
	}
}

textures/blx/blx_bright_blue
{
	qer_editorimage textures/lights/bright2.png
	q3map_lightimage textures/lights/bright2.png
	q3map_surfacelight 10000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bright2.png
	}
}

textures/blx/blx_ultrabright_blue
{
	qer_editorimage textures/lights/bright2.png
	q3map_lightimage textures/lights/bright2.png
	q3map_surfacelight 20000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bright2.png
	}
}

textures/blx/blx_megaultrabright_blue
{
	qer_editorimage textures/lights/bright2.png
	q3map_lightimage textures/lights/bright2.png
	q3map_surfacelight 60000
  	q3map_lightsubdivide 72
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/lights/bright2.png
	}
}

//***********************************
// hexagons
//***********************************

textures/hexagons/blue
{
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/cleansurface/pantone2727c.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/cleansurface/pantone2727c $blankbumpimage
	}
endif
}

textures/hexagons/darkblue
{
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/cleansurface/pantone2746c.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/cleansurface/pantone2746c $blankbumpimage
	}
endif
}

textures/hexagons/hexagons2
{
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/cleansurface/pantone4685c.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/cleansurface/pantone4685c $blankbumpimage
	}
endif
}

//***********************************
// wdm7
//***********************************

textures/wdm7/grid_tile
{
	qer_editorimage textures/metal/bigtreadplateg.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/metal/bigtreadplateg.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/metal/bigtreadplateg.png
	}
endif
}

//***********************************
// refly_wood
//***********************************
textures/refly_wood/wood3
{
	qer_editorimage textures/wood/woodtables01.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/wood/woodtables01.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/wood/woodtables01.png
	}
endif
}

//***********************************
// wdm17/melee
//***********************************

textures/melee/pure001_fx
{
	qer_editorimage textures/cleansurface/pantone361c.png
	q3map_lightimage textures/cleansurface/pantone361c.png
	q3map_surfacelight 100
	{
		map textures/cleansurface/pantone361c.png
	}
	{
		map $lightmap 
		blendfunc filter
		rgbGen identity
		tcGen lightmap 
	}
	{
		map textures/cleansurface/pantone361c.png
		blendfunc add
		rgbGen wave sin 0 1 0 0.25 
	}
	{
		map env/2d/chrome1.png
		blendfunc blend
		tcGen environment 
		alphaGen constant 0.2 
	}
}

textures/melee/blacksky
{
	qer_editorimage textures/cleansurface/black.png
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	skyParms - 512 -
	{
		map textures/cleansurface/black.png
	}
}

textures/melee/glass
{
	qer_editorimage textures/glass/melee.png
	surfaceparm trans
	cull disable
	nopicmip
	qer_trans 0.75
	{
		map textures/glass/melee.png
		blendfunc add
		rgbGen identity
	}
	{
		map $lightmap 
		blendfunc filter
		rgbGen identity
		tcGen lightmap 
	}
	{
		map env/2d/chrome1.png
		blendfunc blend
		tcGen environment 
		alphaGen constant 0.2 
	}
}

textures/baxandall/dirt_01
{
	{
		detail
		map textures/decals/dirt_scratch_01.png
		blendfunc blend
	}
}

textures/baxandall/dirt_02
{
	{
		detail
		map textures/decals/dirt_scratch_02.png
		blendfunc blend
	}
}

textures/baxandall/dirt_03
{
if deluxe
	{
		detail
		map textures/decals/dirt_02.png
		blendfunc blend
		rgbgen const 0.7 0.7 0.7
		alphagen const 0.3
	}
endif
}

textures/ETR/billboard
{
	template textures/billboard/board_512_256_1
}

textures/wtest1/block01a
{	
	qer_editorimage textures/concrete/concrete2.png

	{
		material textures/concrete/concrete2
	}
}

textures/blx_wtest3/teleporter
{
	qer_editorimage textures/world/sh/telescrollup1.png
	q3map_surfaceLight 300
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	qer_trans 0.5
	cull none
	nopicmip

	{
		map textures/world/sh/telescrollup1.png
		blendfunc add
		tcmod scale .25 .25
		rgbGen wave sin .25 .25 0 2.5
		tcMod scroll 0.001 0.5
	}
	{
		map textures/world/sh/telescrollup2.png
		blendfunc add
		tcmod scale .25 .25
		rgbGen wave sin .25 .25 0.25 2.5
		tcMod scroll 0.001 0.5
	}
	{
		map textures/world/sh/teleenergy.png
		blendFunc add
		tcmod scale .25 .25
		tcMod rotate -8
		tcMod scroll 0.05 0.025
		rgbGen wave sin 0.2 0.2 0.0 2
	}
	{
		map textures/world/sh/teleenergy.png
		blendFunc add
		tcmod scale -.25 -.25
		tcMod rotate 8
		tcMod scroll 0.025 0.05
		rgbGen wave sin 0.2 0.2 0.2 2
	}	
}

textures/wdm2/ink
{
	forceWorldOutlines
}

textures/acidwdm2/ink
{
	forceWorldOutlines
}

textures/wdm2/border
{
	{
		material textures/boqu/wallbase 
	}
}

textures/wdm2/wall_olive066
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const ( 0.3 0.66 0.00 )
	}
}

textures/wdm2/wall_olive100
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const ( 0.7 1.00 0.00 )
	}
}

textures/wdm2/wall_red100
{
	{
		material $whiteimage $blankbumpimage
		rgbgen const ( 1.0 0.00 0.00 )
	}
}

textures/wdm2/wdm2_sky
{
}

//***********************************
// terrain
//***********************************

textures/terrain/rockwall01b
{
	qer_editorimage textures/terrain/rockwall01.png

	{
		material textures/terrain/rockwall01.png
	}
}
