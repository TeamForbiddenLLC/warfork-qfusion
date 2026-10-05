textures/36_others/36sky2
{
	qer_editorimage textures/36_others/36sky2.png
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_surfacelight 20
	q3map_sun 1 1 1 40 110 75
	skyparmssides env/36sky rt rt rt env/36sky_up rt 1500 -
	{
		map textures/cha0s_ws/ch_sky_night_front.png
		blendfunc add
		tcMod scale 2 2
		tcMod scroll 0.01 0.01
	}
}

textures/36_others/36sky3
{
	qer_editorimage textures/36_others/36sky3.png
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nomarks
	skyparmssides env/36sky rt rt rt rt rt 1500 -
}

textures/36_others/36tele2
{
	q3map_lightimage textures/36_colours/FFFFFF.png
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm noimpact
	surfaceparm nodlight
	q3map_surfacelight 150

	portal
	{
		depthwrite
		map $whiteimage
		blendfunc filter
	}
}

textures/36_lights/light-FFFFFF
{
	surfaceparm nolightmap
	surfaceparm nomarks
	q3map_lightimage textures/36_colours/FFFFFF.png
	qer_editorimage textures/36_lights/light-FFFFFF.png
	q3map_surfacelight 800
	q3map_lightsubdivide 16
	q3map_backsplash 15 64
	{
		map textures/36_colours/FFFFFF.png
	}
}
