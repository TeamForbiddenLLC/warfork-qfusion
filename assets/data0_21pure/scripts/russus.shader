//=======================================
// ALPHA
//=======================================

textures/russus/spray_alpha
{
	qer_editorimage textures/russus/spray_alpha.png
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 1
	{
		map textures/russus/spray_alpha.png
		blendfunc add
		rgbgen teamcolor 2
	}
}

textures/russus/transscreen_alpha
{
	qer_editorimage textures/russus/transscreen_alpha.png
	cull none
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/russus/transscreen_alpha.png
		blendfunc add
		rgbgen teamcolor 2
	}
}


//=======================================
// BETA
//=======================================


textures/russus/spray_beta
{
	qer_editorimage textures/russus/spray_beta.png
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 1
	{
		map textures/russus/spray_beta.png
		blendfunc add
		rgbgen teamcolor 3
	}
}

textures/russus/transscreen_beta
{
	qer_editorimage textures/russus/transscreen_beta.png
	cull none
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	qer_trans 0.75
	{
		map textures/russus/transscreen_beta.png
		blendfunc add
		rgbgen teamcolor 3
	}
}


//=======================================
// MISC
//=======================================


textures/russus/teleporter
{
	q3map_lightimage textures/36_colours/F1E7B5.png
	qer_editorimage textures/world/sh/teleenergy.png
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_surfacelight 128
	q3map_lightsubdivide 16
	qer_trans 0.5
	cull none
	nopicmip

	if portalMaps
		{
			grayScale
			alphagen const 1
			distortion $blankbumpimage
			depthWrite
		}
	endif

	if ! portalMaps
		portal

		{
			map *black
			blendFunc add
			depthWrite
		}
	endif
}

