textures/evillair_eu/eu_trimlights1
{	
	qer_editorimage textures/evillair_eu/eu_trimlights1.png
	q3map_lightimage textures/evillair_eu/eu_trimlights1.png
  	q3map_surfacelight 150
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	surfaceparm nolightmap

	{
		map textures/evillair_eu/eu_trimlights1.png
	}
}

textures/evillair_eu/eu_floortile5
{
	qer_trans 0.5
	qer_editorimage textures/evillair_eu/eu_floortile5.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	smallestmipmapsize 8
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material textures/evillair_eu/eu_floortile5.png
		alphaFunc GE128
		depthWrite
	}
}
