textures/wdm9/jump_pad_1
{
	nopicmip
	noshadow
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nodlight
	cull none
	qer_editorimage textures/baxandall/jump_pad_1.png
	qer_trans 0.75

	{
		clampmap textures/baxandall/jump_pad_1.png
		blendfunc add
	}
	{
		clampmap textures/baxandall/jump_pad_2.png
		blendfunc add
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

textures/wdm9/item_indi_1
{
	nopicmip
	noshadow
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nodlight
	qer_editorimage textures/baxandall/item_indi_1.png
	qer_trans 0.75

	{
		clampmap textures/baxandall/item_indi_1.png
		blendfunc add
	}
	{
		clampmap textures/baxandall/item_indi_2.png
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

textures/wdm9/item_indi_1_2
{
	nopicmip
	noshadow
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nodlight
	qer_editorimage textures/baxandall/item_indi_1.png
	qer_trans 0.75

	{
		clampmap textures/baxandall/item_indi_1.png
		blendfunc add
	}
}

textures/wdm9/sky_s
{
	qer_editorimage textures/sky/violetsky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight
	q3map_lightimage textures/cha0s_ws/ch_sky_night_back.png
	q3map_surfacelight 150
	q3map_skyLight 80 6

	skyParms - - -

	{
		map textures/sky/violetsky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/violetsky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/violetsky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}
