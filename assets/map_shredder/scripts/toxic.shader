textures/toxic_sky/space
{
	qer_editorimage textures/toxic_sky/env/toxic_sky_up.jpg
	q3map_lightmapFilterRadius 0 8
	q3map_sunExt 245 245 200 45 90 35 2 32
	q3map_skyLight 30 5
	q3map_noFog
	q3map_globalTexture
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	skyparms textures/toxic_sky/env/toxic_sky 1500 - 
	nopicmip

}

textures/toxic/lamp_1_on
{
	qer_editorimage textures/toxic/lamp_1_on.tga
	q3map_lightimage textures/toxic/lamp_1_on.tga	
	surfaceparm nomarks
	q3map_surfacelight 5000
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/toxic/lamp_1_on.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/toxic/lamp_1_blend.tga
		blendfunc GL_ONE GL_ONE
		rgbGen wave sin .8 .2 0 1
	}	
}

textures/toxic/slimet
{
    qer_editorimage textures/toxic/slime1t.tga
    q3map_globaltexture
    surfaceparm trans
    surfaceparm water
    surfaceparm slime
    surfaceparm nolightmap
	surfaceparm nomarks
    q3map_surfacelight 25
    cull none
    nopicmip
	sort underwater
	 

    { 
            map textures/toxic/slime2t.tga
            blendFunc GL_dst_color GL_one
		tcmod scale -.5 -.5
		tcmod scroll .02 .02
    }

    { 
            map textures/toxic/slime1t.tga
            blendFunc GL_dst_color GL_one
            tcmod scale -.25 -.25
            tcmod scroll .02 .02
    }
}

textures/toxic/toxicrock_1
{
 	q3map_nonplanar
 	q3map_shadeangle 60
 	qer_editorimage textures/toxic/toxicrock_1.png

 	{
 		map $lightmap
 		rgbGen identity
 	}

 	{
 		map textures/toxic/toxicrock_1.png
 		blendFunc filter
 	}
}
