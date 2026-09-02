

textures/cha0s_ws/cement_2_yellow_flat
{
	qer_editorimage textures/metal/cement_2_yellow_flat

	{
		material textures/cha0s_ws/cement_2_yellow_flat $blankBumpImage
	}
}

textures/cha0s_ws/trim19
{
	qer_editorimage textures/cha0s_ws/trim19.png

	{
		material textures/cha0s_ws/trim19 textures/cha0s_ws/trim19_norm textures/cha0s_ws/trim19_gloss
	}
}



textures/cha0s_ws/trim19_black
{
	qer_editorimage textures/cha0s_ws/trim19_black.png

	{
		material textures/cha0s_ws/trim19_black textures/cha0s_ws/trim19_norm textures/cha0s_ws/trim19_gloss
	}
}


//=======================================
// LIGHTS ETC
//=======================================

textures/cha0s_ws/glass
{
	qer_editorimage textures/cha0s_ws/chrome4.png
	surfaceparm trans
	cull none
	qer_trans 0.5

	{
		map textures/cha0s_ws/chrome4.png
		blendfunc add
		tcGen environment 
		tcmod scale 2 2
	}
	{
		map textures/cha0s_ws/dirt.png
		blendfunc blend
		tcmod scale .5 .5
	}
}


//=======================================
// ALPHA
//=======================================


textures/cha0s_ws/cement_1_grimy_alpha
{
	qer_editorimage textures/cha0s_ws/cement_1_grey.png

	{
		material textures/cha0s_ws/cement_1_grey.png textures/cha0s_ws/cement_1_grimy_norm.png
		rgbgen teamcolor 2
	}
}

textures/cha0s_ws/cement-tiled_alpha
{
	qer_editorimage textures/cha0s_ws/cement-tiled_grey.png

	{
		material textures/cha0s_ws/cement-tiled_grey.png textures/cha0s_ws/cement-tiled_grey_norm.png textures/cha0s_ws/cement-tiled_grey_gloss.png
		rgbgen teamcolor 2
	}
}

textures/cha0s_ws/trim19_alpha
{
	qer_editorimage textures/cha0s_ws/trim19_grey.png

	{
		material textures/cha0s_ws/trim19_grey.png textures/cha0s_ws/trim19_norm.png textures/cha0s_ws/trim19_gloss.png
		rgbgen teamcolor 2
	}
}

textures/cha0s_ws/base_alpha
{
	qer_editorimage textures/cha0s_ws/base_grey.png

	{
		material textures/cha0s_ws/base_grey.png textures/cha0s_ws/base_grey_norm.png textures/cha0s_ws/base_grey_gloss.png
		rgbgen teamcolor 2
	}
}

textures/cha0s_ws/cement_alpha
{
	qer_editorimage textures/cha0s_ws/cement_grey.png

	{
		material textures/cha0s_ws/cement_grey.png textures/cha0s_ws/cement_3_norm.png
		rgbgen teamcolor 2
	}
}


//=======================================
// BETA
//=======================================


textures/cha0s_ws/cement_1_grimy_beta
{
	qer_editorimage textures/cha0s_ws/cement_1_grey.png

	{
		material textures/cha0s_ws/cement_1_grey.png textures/cha0s_ws/cement_1_grimy_norm.png
		rgbgen teamcolor 3
	}
}

textures/cha0s_ws/cement-tiled_beta
{
	qer_editorimage textures/cha0s_ws/cement-tiled_grey.png

	{
		material textures/cha0s_ws/cement-tiled_grey.png textures/cha0s_ws/cement-tiled_grey_norm.png textures/cha0s_ws/cement-tiled_grey_gloss.png
		rgbgen teamcolor 3
	}
}

textures/cha0s_ws/trim19_beta
{
	qer_editorimage textures/cha0s_ws/trim19_grey.png

	{
		material textures/cha0s_ws/trim19_grey.png textures/cha0s_ws/trim19_norm.png textures/cha0s_ws/trim19_gloss.png
		rgbgen teamcolor 3
	}
}

textures/cha0s_ws/base_beta
{
	qer_editorimage textures/cha0s_ws/base_grey.png

	{
		material textures/cha0s_ws/base_grey.png textures/cha0s_ws/base_grey_norm.png textures/cha0s_ws/base_grey_gloss.png
		rgbgen teamcolor 3
	}
}

textures/cha0s_ws/cement_beta
{
	qer_editorimage textures/cha0s_ws/cement_grey.png

	{
		material textures/cha0s_ws/cement_grey.png textures/cha0s_ws/cement_3_norm.png
		rgbgen teamcolor 3
	}
}

textures/cha0s_ws/cement_2
{
	qer_editorimage textures/concrete/concrete2.png

	{
		material textures/concrete/concrete2.png
	}
}
