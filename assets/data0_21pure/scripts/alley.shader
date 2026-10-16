

textures/alley/terrainblend
{
	qer_editorimage textures/terrain/soilsand0104
	q3map_lightmapaxis z
	//q3map_lightmapsamplesize 64
	//q3map_lightmapSampleOffset 64
	//surfaceparm dust
	q3map_noDirty

	{
		material textures/terrain/rocks001_512x512
		//material textures/terrain/rocks002_512x512 textures/terrain/rocks001_512x512_norm
		//material textures/terrain/sand
	}

	{
		//material textures/terrain/rocks002_512x512 textures/terrain/rocks001_512x512_norm
		material textures/terrain/soilsand0104
		alphagen vertex
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		
	}
}

textures/alley/terraintunel
{
	qer_editorimage textures/terrain/rocks001_512x512.png
	//q3map_lightmapaxis z
	//q3map_lightmapsamplesize 64
	//q3map_lightmapSampleOffset 64
	//surfaceparm dust
	q3map_lightmapSampleOffset 16
	q3map_noDirty

	{
		material textures/terrain/rocks001_512x512
		tcmod scale 1.25 1.25
	}

//	{
//		map textures/world/sh/water2.png
//		tcmod scale .35 .35
//		tcmod turb 0 0.5 0 0.1
//		rgbgen const 0.00 0.12 0.13
//		blendfunc add
//	}

	{
		material textures/terrain/rockwall01
		tcmod scale 1.25 1.25
		alphagen vertex
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA		
	}
}

