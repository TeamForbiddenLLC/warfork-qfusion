textures/time/water
	
	{
	//qer_editorimage textures/time/water.png
	//qer_trans .5
	q3map_globaltexture
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm water
	surfaceparm fog
	fogparms ( .176 .278 .227 ) 512 // Old, dark aqua = ( .176 .278 .227 ) 512
	//cull disable // slash AFTER compile, to make water trans from below (important)	
	{ 
		map textures/hipshot_m9/water.tga
		blendFunc blend
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_dst_color GL_zero
		rgbgen identity
	}
}

textures/time/orangegreyrock
{
 	q3map_nonplanar
 	q3map_shadeangle 60
 	qer_editorimage textures/time/orangegreyrock.tga

 	{
 		map $lightmap
 		rgbGen identity
 	}

 	{
 		map textures/time/orangegreyrock.tga
 		blendFunc filter
 	}
}

textures/time/orangegreyrock_2
{
 	q3map_nonplanar
 	q3map_shadeangle 60
 	qer_editorimage textures/time/orangegreyrock_2.tga

 	{
 		map $lightmap
 		rgbGen identity
 	}

 	{
 		map textures/time/orangegreyrock_2.tga
 		blendFunc filter
 	}
}

textures/time/jump_beam
	{
	qer_editorimage textures/time/particles.jpg
	surfaceparm trans	
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm nolightmap
	qer_trans .7
	cull none
	{
		map textures/time/particles.jpg
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		tcMod Scroll .01 .1
	}
	{
		map textures/time/particles2.jpg
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		tcmod scale .5 .5
		tcMod Scroll -.02 .2
	}
	{
		map textures/time/particles3.jpg
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		tcmod scale .25 .25
		tcMod Scroll -.02 .1
	}
	{
		map textures/time/particles4.jpg
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		tcmod scale .25 .25
		tcMod Scroll .03 .2
	}
}
