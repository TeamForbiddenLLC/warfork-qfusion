
textures/shepas/sky_s
{
	qer_editorimage env/grimmnight/grimmnight_ft.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight
	q3map_lightimage textures/sky/sky_night_front
	q3map_surfacelight 150
	q3map_skyLight 80 6

	skyparms env/grimmnight/grimmnight - -
	{
		map textures/sky/sky_night_front
		blendfunc add
		tcMod scale 2 2 
		tcMod scroll 0.01 0.01
	}

	
	
}
