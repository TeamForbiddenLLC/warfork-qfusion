textures/wdm7/light_tile
{
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_lightRGB 1 0.88 0.47
   	qer_editorimage   textures/wdm7/light_tile.png
   	q3map_surfacelight   5000
  	q3map_lightsubdivide   16
	{
		map textures/wdm7/light_tile.png
	}
}


textures/wdm7/bounce
{
	q3map_lightimage textures/wdm7/cx_ecel_bounce_blend.png
	qer_editorimage textures/wdm7/cx_ecel_bounce_blend.png
	surfaceparm nolightmap
	surfaceparm nodamage
	surfaceparm nomarks
	forceoverlays
	polygonOffset 
	nopicmip
	q3map_surfacelight 400
	{
		map textures/wdm7/cx_ecel_bounce.png
		blendfunc filter
	}
	{
		map textures/wdm7/cx_ecel_bounce_blend.png
		blendfunc add
		rgbGen wave sin 0.5 0.5 0 1.5 
	}
	{
		clampmap textures/wdm7/cx_ecel_bounce_small.png
		blendfunc add
		rgbGen wave square 0.5 0.5 0.25 1.5 
		tcMod stretch sin 1.2 0.8 0 1.5 
	}
}


textures/wdm7/oil
{
	qer_editorimage textures/wdm7/oil.png
	surfaceparm noimpact
	surfaceparm slime
	tesssize 64
	deformVertexes wave 64 sin 1 1 0.25 0.6
	{
		map textures/wdm7/oil.png
		tcMod turb 0 .1 0 .1	//texture 'waves'
		tcmod scroll .02 .01
	}
	{
		map textures/wdm7/oil.png
		blendfunc add
		tcMod turb 0 0.1 0 .1	//texture 'waves'
		tcmod scroll .01 .02
	}
}

textures/wdm7/sky
{
	qer_editorimage textures/Factory/blx_wt3_sky_orange.png
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 80
	q3map_sun 1 1 0.75 155 170 70
	skyParms - - -

	{
		map textures/Factory/blx_wt3_sky_orange.png
		tcMod scale 2 2
		tcMod scroll 0.005 0.005
	}
	{
		map textures/Factory/blx_wt3_sky_orange.png
		blendfunc add
		tcMod scale 3 3
		tcMod scroll 0.01 0.01
	}
}
