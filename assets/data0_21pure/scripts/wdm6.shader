
textures/wdm6/kimza_crocinus_pillar2
{
	qer_editorimage textures/wdm6/kimza_crocinus_pillar2.png

	{
		material
	}
}

textures/wdm6/kimza_wt3_pillar3_orange
{
	qer_editorimage textures/wdm6/kimza_wt3_pillar3_orange.png

	{
		material
	}
}

textures/wdm6/sky_s
{
	qer_editorimage textures/blxbis/skyturq_scroll.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky

	skyParms - - -

	{
		map textures/blxbis/skyturq_scroll2.png
		tcMod scale 4 4
		tcMod scroll 0 -0.015
		rgbgen const 0.2 0.2 0.2
	}

	{
		map textures/blxbis/skyturq_scroll.png
		tcMod scale 4 4
		tcMod scroll 0 0.03
		rgbgen const 0.2 0.2 0.2
		blendFunc add
	}

	{
		map textures/blxbis/skyturq_scroll2.png
		tcMod scale 4 4
		tcMod scroll 0 0.02
		rgbgen const 0.2 0.2 0.2
		blendFunc add
	}
}

textures/wdm6/teleporter_distortion
{
	qer_editorimage textures/common/nodraw.png
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	noshadow
	qer_trans 0.25

}

textures/wdm6/blx_spacer_teleport
{
	qer_editorimage textures/blx/blx_spacer_2.png	
	nopicmip
	surfaceparm nomarks
	surfaceparm noimpact
	noshadow

	{
		map textures/blx/blx_spacer_2.png
		tcmod scroll .6 0
	}
	{
		map textures/blx/blx_spacer_alpha.png
		blendfunc add
		tcmod scroll 3 .25
	}
}
