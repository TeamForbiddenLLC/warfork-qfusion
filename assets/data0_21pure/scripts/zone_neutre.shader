// =======================================
//  Zone_Neutre shaderfile by lichen
// =======================================

// =================================================
// fog
// =================================================
textures/zone_neutre/fog
{
	qer_editorimage textures/sfx/fog_grey.png
	surfaceparm	trans
	surfaceparm	nonsolid
	surfaceparm	fog
	surfaceparm	nolightmap
	surfaceparm 	nodrop
	fogparms ( .48609375 .44140625 .390625 ) 300
}


// =======================================
//  Zone_Neutre skybox
// =======================================
textures/zone_neutre/skygrey
{
	qer_editorimage textures/zone_neutre/skygrey.png
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 65
	//q3map_lightsubdivide 512
	q3map_sun 1 1 1 85 90 90
	{
		map	textures/zone_neutre/skygrey.png
	}
}



// =======================================
//  Zone_Neutre blue skybox
// =======================================
textures/zone_neutre/skyblue
{
	q3map_lightimage textures/zone_neutre/skyblue.png
	qer_editorimage textures/zone_neutre/skyblue.png
	surfaceparm noimpact
	surfaceparm nolightmap
	q3map_surfacelight 0
	q3map_lightsubdivide 512
	q3map_sun 1 1 1 20 90 90

	{
		map	textures/zone_neutre/skyblue.png
	}
}


//shader du portal orange avec la spirale qui tourne n'importe comment
textures/zone_neutre/portal
{
	qer_editorimage textures/zone_neutre/portal.png
	cull none
	qer_trans 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm noimpact
	tessSize 64
	{
		clampmap textures/zone_neutre/portal.png  
		blendFunc add 
		tcmod rotate 120 
		tcMod stretch sin 2 .5 0 .24		
		rgbGen wave sin .6 .5 0 .4 
	}
	{
		clampmap textures/zone_neutre/portal.png 
		blendFunc add 
		tcMod turb 0 .1 0 .06 
		tcmod rotate 75 
		tcMod stretch sin 2 0.4 0 .2
		rgbGen wave sin .7 .3 0 .5 
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc filter
		depthFunc equal
	}
}




//shader du portal ctf
//ne fonctione pas dutout, � corriger.
textures/zone_neutre/portalcft
{
	qer_editorimage textures/zone_neutre/portalcft.png
	cull none
	qer_trans 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm noimpact
	{
		clampmap textures/zone_neutre/portalctf.png  
		blendFunc add 
		tcmod rotate 120 
		tcMod stretch sin 2 .5 0 .24		
		rgbGen wave sin .6 .5 0 .4 
	}
	{
		clampmap textures/zone_neutre/portalctf.png 
		blendFunc add 
		tcMod turb 0 .1 0 .06 
		tcmod rotate 75 
		tcMod stretch sin 2 0.4 0 .2
		rgbGen wave sin .7 .3 0 .5 
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc filter
		depthFunc equal
	}
}


//shader du portal de test
//ne fonctionne pas dutout; � corriger.
textures/zone_neutre/test
{
	qer_editorimage textures/zone_neutre/test.png
	cull none
	qer_trans 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm noimpact
	tcmod rotate 120 
	tcMod stretch sin 2 .5 0 .24		
	rgbGen wave sin .6 .5 0 .4
	{
		map textures/zone_neutre/test.png
	} 

}

//shader pour l'outline du cell-shading q3map2
//on peut passer l'offset � -1 pour une outline tr�s fine et tr�s propre,
//mais de loin, l'outline devient presque invisible, � tweaker selon la map.
textures/zone_neutre/celshader
{
	qer_editorimage textures/zone_neutre/celshader.png
	
	q3map_notjunc
	q3map_nonplanar
	q3map_bounce 0.0
	q3map_shadeangle 120
	q3map_texturesize 1 1
	q3map_invert
	q3map_offset -2.0
	
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks

	deformVertexes wave 100 sin 2.0 0 0 0
	cull back

	
	sort 10
	
	{
		map	textures/zone_neutre/celshader.png
	}
}


//shader pour le scroll des fleches (bump)
//ajout par blem

textures/zone_neutre/beige_fleche01_scroll
{
	qer_editorimage textures/zone_neutre/beige_fleche01
	
	{
		map	textures/zone_neutre/beige_fleche01.png
	}
	{
		map	textures/zone_neutre/beige_fleche01_glow.png
		tcMod scroll 0 1
		blendfunc add
	}
	{
		map	textures/zone_neutre/beige_fleche01.png
		blendfunc blend
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
		rgbGen identity
	}
	
}
