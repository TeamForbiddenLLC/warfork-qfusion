





textures/water/calmwater_vcolor
{
	qer_editorimage textures/world/sh2026/water5_norm
	q3map_globaltexture
	qer_trans .75
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nodlight
	q3map_vertexcolor #699e00
	q3map_lightmapsamplesize 32
	sort portal

	if GLSL && portalMaps

	portal
	portalmip 2
	portalnoshadows
	
	{
		material textures/plastic/grey50 - $whiteimage
		rgbgen vertex
		blendfunc filter
	}

	{
		distortion textures/world/sh2026/water5_DUDV textures/world/sh2026/water5_norm
		tcmod scale 0.75 0.75
		tcmod scroll -0.05 -0.05
		blendfunc add
	}

	endif

	if ! GLSL || ! portalMaps

	{
		map textures/world/sh2026/wazzz/water_gloss.jpg
		blendFunc GL_dst_color GL_one
		rgbgen identity
		tcmod scale .75 .75
		tcmod scroll .02 .01
	}

	{
		map $whiteImage
		rgbgen const .0 .3 .4
		alphagen const 0.25
		blendfunc blend
	}

	endif
}


textures/water/watergoo_vcolor
{
	qer_editorimage textures/world/sh2026/water5_norm
	qer_trans .75
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm nodlight
	//q3map_lightmapsamplesize 32
	q3map_vertexcolor #699e00
	sort portal

	if GLSL && portalMaps

	portal // distortion already enforces this.
	portalmip 3
	portalnoshadows

	{
		distortion textures/world/sh2026/water5_DUDV textures/world/sh2026/water5_norm
		
		tcmod scale 0.75 0.75
		tcmod scroll -0.01 -0.01
		alphaGen dot 0.5 1.0
		blendfunc blend
	}

	endif

	if ! GLSL || ! portalMaps
	// use textures/world/bluewater1 as fallback.

	{
		map textures/world/sh/water2.png
		blendFunc GL_dst_color GL_one
		tcmod scale .25 .25
		tcmod scroll .02 .01
	}
	{
		map textures/world/sh/water1.png
		blendFunc GL_dst_color GL_one
		tcmod scale -.25 -.25
		tcmod scroll .02 .02
	}
	{
		map textures/world/sh/pooltint.png
		blendFunc filter
		tcmod scale .5 .5
		tcmod scroll .02 .01
	}

	endif
}

