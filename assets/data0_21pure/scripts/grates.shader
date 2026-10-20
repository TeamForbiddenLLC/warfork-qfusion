textures/grates/fence01
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence01.png
	surfaceparm trans
	surfaceparm nomarks
	//surfaceparm	nonsolid
	surfaceparm alphashadow
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64
	
	{
		// notice: 01 doesn't use the gloss map
		material textures/grates/fence01.png textures/grates/fence01_norm.png
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/fence01_noshadow
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence01.png
	surfaceparm trans
	surfaceparm nomarks
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64
	
	{
		// notice: 01 doesn't use the gloss map
		material textures/grates/fence01.png textures/grates/fence01_norm.png
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/fence02
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence02.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material textures/grates/fence02.png textures/grates/fence01_norm.png textures/grates/fence01_gloss.png
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/fence02_noshadow
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence02.png
	surfaceparm trans
	surfaceparm nomarks
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64
	
	{
		material textures/grates/fence02.png textures/grates/fence01_norm.png textures/grates/fence01_gloss.png
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/fence03
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence03.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material textures/grates/fence03.png textures/grates/fence01_norm.png textures/grates/fence01_gloss.png
		alphaFunc GE128
		depthWrite
	}
}


textures/grates/fence03_noshadow
{	
	qer_alphafunc gequal 0.5
	qer_editorimage textures/grates/fence03.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64
	
	{
		material textures/grates/fence03.png 
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/simplegrid
{
	qer_editorimage textures/grates/simplegrid.png
	qer_alphafunc gequal 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	smallestmipmapsize 8
	glossExponent 100
	glossIntensity 0.25

	{
		material textures/grates/simplegrid.png $blankBumpImage $whiteimage
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/grid02
{
	qer_editorimage textures/grates/grid02.png
	qer_alphafunc gequal 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material textures/grates/grid02.png
		alphaFunc GE128
		depthWrite
	}
}



textures/grates/grid03
{
	qer_editorimage textures/grates/grid03
	qer_alphafunc gequal 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	
	{
		material textures/grates/grid03
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/grid03_blue
{
	qer_editorimage textures/grates/grid03_blue
	qer_alphafunc gequal 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	
	{
		material textures/grates/grid03_blue
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/grid03_orange
{
	qer_editorimage textures/grates/grid03_orange
	qer_alphafunc gequal 0.5
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	
	{
		material textures/grates/grid03_orange
		alphaFunc GE128
		depthWrite
	}
}

textures/grates/gratehzh
{
	qer_editorimage textures/grates/gratehzh
	qer_alphafunc gequal 0.5
	surfaceparm metalsteps
	surfaceparm alphashadow
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm trans
	nopicmip
	cull none

	{
		map textures/grates/gratehzh
		rgbgen vertex
		alphafunc GE128
		depthWrite
	}
}
