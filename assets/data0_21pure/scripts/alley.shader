// GRIMM NIGHT
// high res 1024^2 environment map
// ships as png.
// 
// 
// By Jockum Skoglund aka hipshot
// hipshot@zfight.com
// www.zfight.com
// Stockholm, 2005 08 25
// 
// 
// Modify however you like, just cred me for my work, maybe link to my page.
// 
// (Grimm is not a typo of Grim, some people understand the meaning)


textures/alley/newsky
{
	qer_editorimage env/alleyskybox/grimmnight_ft.png
	surfaceparm noimpact
	surfaceparm nolightmap
	//q3map_globaltexture
	surfaceparm sky
	q3map_sun 0.52021 0.711518 1 75 350 45

	skyparms env/alleyskybox/grimmnight - -
}

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

