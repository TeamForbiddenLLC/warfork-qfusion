textures/wdm1/sky
{
	qer_editorimage textures/world/sh/quicksky.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	q3map_lightimage textures/cha0s_ws/ch_sky_night_back.png
	q3map_surfacelight 280
	q3map_sun 0.27 0.4 0.51 190 170 33

 if textureCubeMap

	skyparms env/wdm1env - -

 endif

 if ! textureCubeMap

	skyParms - - -
	{
		map textures/cha0s_ws/ch_sky_night_back.png
		tcMod scale 2 2
		tcMod scroll 0.01 0.01
		depthWrite
	}

 endif

	{
		map textures/cha0s_ws/ch_sky_night_front.png
		blendfunc add
		tcMod scale 3 3
		tcMod scroll 0.02 0.02
	}
}
