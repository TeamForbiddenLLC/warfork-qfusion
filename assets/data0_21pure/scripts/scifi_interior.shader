// Scifi Interior set by crizis
// crizis@warsow.net

// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International
// http://creativecommons.org/licenses/by-sa/4.0/

// Glass
textures/scifi_interior/glass 
{
	surfaceparm trans

	{
		material textures/scifi_interior/glass
		rgbGen const 0.75 0.75 1.0
		blendfunc blend
		alphagen const 0.5
	}	

	{
		map textures/scifi_interior/glass_effect.png
		tcgen environment
		blendfunc blend
		alphagen const 0.25
	}

	{
		map env/2d/map1.png
		tcGen environment
		blendFunc blend
		alphagen const 0.1
	}	

}

textures/scifi_interior/blue_glass
{
	qer_editorimage textures/scifi_interior/blue_glass.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/scifi_interior/blue_glass.png
		tcGen environment
		blendfunc blend
	}

	{
		map textures/scifi_interior/blue_glass.png
		tcGen environment
		blendfunc blend
		rgbgen wave distanceramp 0 1 200 500
	}
	
}

textures/scifi_interior/blue_glass_tvscreen
{
	qer_editorimage textures/scifi_interior/blue_glass.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/scifi_interior/blue_glass.png
		tcGen environment
		blendfunc blend
	}

	{
		map textures/scifi_interior/blue_glass.png
		tcGen environment
		blendfunc blend
		rgbgen wave distanceramp 0 1 200 500
	}

	{
		map textures/scifi_interior/noise.png
		blendfunc add
		rgbGen wave sin 0.4 0.4 0 .5
		tcmod scroll 20 .15 
	}   	
}

// Panels

textures/scifi_interior/aluminium_airvent
{	
	qer_editorimage textures/scifi_interior/aluminium_airvent.png

	{
		material textures/scifi_interior/aluminium_airvent.png textures/scifi_interior/aluminium_airvent_norm.png textures/scifi_interior/aluminium_airvent_gloss.png
	}
}

textures/scifi_interior/metal_panel
{	
	qer_editorimage textures/scifi_interior/metal_panel.png
	glossExponent 25

	{
		material textures/scifi_interior/metal_panel.png textures/scifi_interior/metal_panel_norm.png textures/scifi_interior/metal_panel_gloss.png
	}
}

textures/scifi_interior/metal_panel_darkblue
{	
	qer_editorimage textures/scifi_interior/metal_panel_darkblue.png
	glossExponent 25

	{
		material textures/scifi_interior/metal_panel_darkblue.png textures/scifi_interior/metal_panel_norm.png textures/scifi_interior/metal_panel_gloss.png
	}
}


textures/scifi_interior/aluminium_clean
{	
	qer_editorimage textures/scifi_interior/aluminium_clean.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_clean.png textures/scifi_interior/aluminium_clean_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel
{	
	qer_editorimage textures/scifi_interior/aluminium_panel.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel.png textures/scifi_interior/aluminium_panel_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel_blue
{	
	qer_editorimage textures/scifi_interior/aluminium_panel_blue.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel_blue.png textures/scifi_interior/aluminium_panel_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel2_blue
{	
	qer_editorimage textures/scifi_interior/aluminium_panel2_blue.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel2_blue.png textures/scifi_interior/aluminium_panel2_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel3
{	
	qer_editorimage textures/scifi_interior/aluminium_panel3.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel3.png textures/scifi_interior/aluminium_panel_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel_orange
{	
	qer_editorimage textures/scifi_interior/aluminium_panel_orange.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel_orange.png textures/scifi_interior/aluminium_panel_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel2
{	
	qer_editorimage textures/scifi_interior/aluminium_panel2.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel2.png textures/scifi_interior/aluminium_panel2_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel4
{	
	qer_editorimage textures/scifi_interior/aluminium_panel4.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel4.png textures/scifi_interior/aluminium_panel2_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel2_orange
{	
	qer_editorimage textures/scifi_interior/aluminium_panel2_orange.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel2_orange.png textures/scifi_interior/aluminium_panel2_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel_small
{	
	qer_editorimage textures/scifi_interior/aluminium_panel_small.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel_small.png textures/scifi_interior/aluminium_panel_small_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel_small_blue
{	
	qer_editorimage textures/scifi_interior/aluminium_panel_small_blue.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel_small_blue.png textures/scifi_interior/aluminium_panel_small_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_panel_small_orange
{	
	qer_editorimage textures/scifi_interior/aluminium_panel_small_orange.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_panel_small_orange.png textures/scifi_interior/aluminium_panel_small_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/rollupmetal
{	
	qer_editorimage textures/scifi_interior/rollupmetal.png
	glossExponent 100

	{
		material textures/scifi_interior/rollupmetal.png textures/scifi_interior/rollupmetal_norm.png textures/scifi_interior/gloss.png
	}
}

// Trims

textures/scifi_interior/aluminium_trim_clean
{	
	qer_editorimage textures/scifi_interior/aluminium_trim_clean.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_trim_clean.png textures/scifi_interior/aluminium_trim_clean_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_trim_white
{	
	qer_editorimage textures/scifi_interior/aluminium_trim_white.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_trim_white.png textures/scifi_interior/aluminium_trim_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_trim_orange
{	
	qer_editorimage textures/scifi_interior/aluminium_trim_orange.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_trim_orange.png textures/scifi_interior/aluminium_trim_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/aluminium_trim_black
{	
	qer_editorimage textures/scifi_interior/aluminium_trim_black.png
	glossExponent 100

	{
		material textures/scifi_interior/aluminium_trim_black.png textures/scifi_interior/aluminium_trim_norm.png textures/scifi_interior/gloss.png
	}
}

// Base colors

textures/scifi_interior/baseblack
{	
	qer_editorimage textures/scifi_interior/baseblack.png
	glossExponent 100

	{
		material textures/scifi_interior/baseblack.png $blankbumpimage textures/scifi_interior/gloss_shiny.png
	}
}

textures/scifi_interior/basered
{	
	qer_editorimage textures/scifi_interior/basered.png
	glossExponent 100

	{
		material textures/scifi_interior/basered.png $blankbumpimage textures/scifi_interior/gloss_shiny.png
	}
}

textures/scifi_interior/baseblue
{	
	qer_editorimage textures/scifi_interior/baseblue.png
	glossExponent 100

	{
		material textures/scifi_interior/baseblue.png $blankbumpimage textures/scifi_interior/gloss_shiny.png
	}
}

textures/scifi_interior/baseorange
{	
	qer_editorimage textures/scifi_interior/baseorange.png
	glossExponent 100

	{
		material textures/scifi_interior/baseorange.png $blankbumpimage textures/scifi_interior/gloss_shiny.png
	}
}

textures/scifi_interior/basesilver
{	
	qer_editorimage textures/scifi_interior/basesilver.png
	glossExponent 100

	{
		material textures/scifi_interior/basesilver.png $blankbumpimage textures/scifi_interior/gloss_shiny.png
	}
}

textures/scifi_interior/orange_panel
{	
	qer_editorimage textures/scifi_interior/orange_panel.png
	glossExponent 100

	{
		material textures/scifi_interior/orange_panel.png textures/scifi_interior/orange_panel_norm.png textures/scifi_interior/orange_panel_gloss.png
	}
}

textures/scifi_interior/orange_panel2
{	
	qer_editorimage textures/scifi_interior/orange_panel2.png
	glossExponent 100

	{
		material textures/scifi_interior/orange_panel2.png textures/scifi_interior/orange_panel2_norm.png textures/scifi_interior/orange_panel2_gloss.png
	}
}

textures/scifi_interior/rigged_panel1
{	
	qer_editorimage textures/scifi_interior/rigged_panel1.png
	glossExponent 100

	{
		material textures/scifi_interior/rigged_panel1.png textures/scifi_interior/rigged_panel1_norm.png textures/scifi_interior/rigged_panel1_gloss.png
	}
}

textures/scifi_interior/rigged_panel1_white
{	
	qer_editorimage textures/scifi_interior/rigged_panel1_white.png
	glossExponent 100

	{
		material textures/scifi_interior/rigged_panel1_white.png textures/scifi_interior/rigged_panel1_norm.png textures/scifi_interior/rigged_panel1_gloss.png
	}
}

// Walls

textures/scifi_interior/wallmetal_grey
{	
	qer_editorimage textures/scifi_interior/wallmetal_grey.png

	{
		material textures/scifi_interior/wallmetal_grey.png $blankbumpimage textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/wallmetal_blue
{	
	qer_editorimage textures/scifi_interior/wallmetal_blue.png

	{
		material textures/scifi_interior/wallmetal_blue.png $blankbumpimage textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/wallmetal_orange
{	
	qer_editorimage textures/scifi_interior/wallmetal_orange.png

	{
		material textures/scifi_interior/wallmetal_orange.png $blankbumpimage textures/scifi_interior/gloss.png
	}
}

// Decals

textures/scifi_interior/01
{	
	qer_editorimage textures/scifi_interior/01.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/01.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/02
{	
	qer_editorimage textures/scifi_interior/02.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/02.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/03
{	
	qer_editorimage textures/scifi_interior/03.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/03.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/covenant1
{	
	qer_editorimage textures/scifi_interior/covenant1.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/covenant1.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/covenant2
{	
	qer_editorimage textures/scifi_interior/covenant2.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/covenant2.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_arrow_orange
{	
	qer_editorimage textures/scifi_interior/decal_arrow_orange.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_arrow_orange.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_comptext1
{	
	qer_editorimage textures/scifi_interior/decal_comptext1.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext1.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_comptext2
{	
	qer_editorimage textures/scifi_interior/decal_comptext2.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext2.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_comptext3
{	
	qer_editorimage textures/scifi_interior/decal_comptext3.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext3.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_up_grey
{	
	qer_editorimage textures/scifi_interior/decal_up_grey.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_up_grey.png $blankbumpimage
		blendfunc blend
	}
}

textures/scifi_interior/decal_up_orange
{	
	qer_editorimage textures/scifi_interior/decal_up_orange.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_up_orange.png $blankbumpimage
		blendfunc blend
	}
}

// Floor tile

textures/scifi_interior/floortile
{	
	qer_editorimage textures/scifi_interior/floortile.png
	glossExponent 100

	{
		material textures/scifi_interior/floortile.png textures/scifi_interior/floortile_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/floortile2
{	
	qer_editorimage textures/scifi_interior/floortile2.png
	glossExponent 50

	{
		material textures/scifi_interior/floortile2.png textures/scifi_interior/floortile2_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/floorpanel
{	
	qer_editorimage textures/scifi_interior/floorpanel.png
	glossExponent 150

	{
		material textures/scifi_interior/floorpanel.png textures/scifi_interior/floorpanel_norm.png textures/scifi_interior/gloss.png
	}
}

textures/scifi_interior/floorpanel_black
{	
	qer_editorimage textures/scifi_interior/floorpanel_black.png
	glossExponent 150

	{
		material textures/scifi_interior/floorpanel_black.png textures/scifi_interior/floorpanel_norm.png textures/scifi_interior/gloss_shiny.png
	}
}

// fences

textures/scifi_interior/grid
{
	qer_trans 0.5
	qer_editorimage textures/scifi_interior/grid.png
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
		material textures/scifi_interior/grid.png
		alphaFunc GE128
		depthWrite
	}
}

textures/scifi_interior/grid_blue
{
	qer_trans 0.5
	qer_editorimage textures/scifi_interior/grid_blue.png
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
		material textures/scifi_interior/grid_blue.png
		alphaFunc GE128
		depthWrite
	}
}

textures/scifi_interior/grid_orange
{
	qer_trans 0.5
	qer_editorimage textures/scifi_interior/grid_orange.png
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
		material textures/scifi_interior/grid_orange.png
		alphaFunc GE128
		depthWrite
	}
}

// bricks

textures/scifi_interior/bricks
{	
	qer_editorimage textures/scifi_interior/bricks.png

	{
		material textures/scifi_interior/bricks.png textures/scifi_interior/bricks_norm.png *black
	}
}

textures/scifi_interior/bricks_blue
{	
	qer_editorimage textures/scifi_interior/bricks_blue.png

	{
		material textures/scifi_interior/bricks_blue.png textures/scifi_interior/bricks_norm.png *black
	}
}

textures/scifi_interior/bricks_orange
{	
	qer_editorimage textures/scifi_interior/bricks_orange.png

	{
		material textures/scifi_interior/bricks_orange.png textures/scifi_interior/bricks_norm.png *black
	}
}

// Lights

textures/scifi_interior/basewhite_1000
{
	qer_editorimage textures/scifi_interior/basewhite.png
	q3map_lightimage   textures/scifi_interior/basewhite.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/scifi_interior/basewhite.png
	}
}

textures/scifi_interior/basered_1000
{
	qer_editorimage textures/scifi_interior/basered.png
	q3map_lightimage   textures/scifi_interior/basered.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/scifi_interior/basered.png
	}
}

textures/scifi_interior/basesilver_1000
{
	qer_editorimage textures/scifi_interior/basesilver.png
	q3map_lightimage   textures/scifi_interior/basesilver.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/scifi_interior/basesilver.png
	}
}

textures/scifi_interior/baseblue_1000
{
	qer_editorimage textures/scifi_interior/baseblue.png
	q3map_lightimage   textures/scifi_interior/baseblue.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/scifi_interior/baseblue.png
	}
}

textures/scifi_interior/baseorange_1000
{
	qer_editorimage textures/scifi_interior/baseorange.png
	q3map_lightimage   textures/scifi_interior/baseorange.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forceMeta
	surfaceparm nolightmap

	{
		map textures/scifi_interior/baseorange.png
	}
}

// Buttons
textures/scifi_interior/button_locked
{
	qer_editorimage textures/scifi_interior/button_locked.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/scifi_interior/button_locked.png
		blendfunc add
		q3map_surfacelight 50
	}

	{
		map textures/scifi_interior/noise.png
		blendfunc add
		rgbGen wave sin 0.4 0.4 0 .5
		tcmod scroll 20 .15 
	}
}

textures/scifi_interior/button_unlocked
{
	qer_editorimage textures/scifi_interior/button_unlocked.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/scifi_interior/button_unlocked.png
		blendfunc add
		q3map_surfacelight 50
	}

	{
		map textures/scifi_interior/noise.png
		blendfunc add
		rgbGen wave sin 0.4 0.4 0 .5
		tcmod scroll 20 .15 
	}
}

textures/scifi_interior/cityfog_darkblue
{
	qer_editorimage textures/sfx/fog_grey.png
	surfaceparm fog
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	fogparms ( .08 0.11 .16 ) 2500 0
}

textures/scifi_interior/scanline_red
{
	qer_editorimage textures/scifi_interior/scanline_red.png
	qer_trans .15
	surfaceparm trans
	surfaceparm nolightmap 
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nodlight

	{
		map textures/scifi_interior/scanline_red.png
		blendfunc blend
	}
}

textures/scifi_interior/scanline_green
{
	qer_editorimage textures/scifi_interior/scanline_green.png
	qer_trans .15
	surfaceparm trans
	surfaceparm nolightmap 
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nodlight

	{
		map textures/scifi_interior/scanline_green.png
		blendfunc blend
	}
}

textures/scifi_interior/noise
{
	qer_editorimage textures/scifi_interior/noise.png
	qer_trans .5
	surfaceparm trans
	surfaceparm nolightmap 

	{
		map textures/scifi_interior/basesilver.png
		tcGen environment
		blendfunc blend
	}

	{
		map textures/scifi_interior/noise.png
//		tcGen environment
		blendfunc blend
		rgbgen wave distanceramp 0 1 200 500
	}

	{
		map textures/scifi_interior/noise.png
		blendfunc add
		rgbGen wave sin 0.4 0.4 0 .5
		tcmod scroll 20 .15 
	}   	
}
