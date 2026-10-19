


vertex/trees/barkdecidious048
{
	qer_editorimage textures/trees/barkdecidious048.png
	surfaceparm nolightmap

	{
		rgbgen vertex
		material textures/trees/barkdecidious048.png
	}
}

textures/trees/barkdecidious048
{
	qer_editorimage textures/trees/barkdecidious048.png
	q3map_forceMeta

	{
		material textures/trees/barkdecidious048.png
	}
}


textures/nature/leaf2
{
	surfaceparm lightfilter      // Use texture's RGB and alpha channels to generate colored alpha
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm noimpact
	cull disable
	{
		map textures/nature/leaf2.png
		rgbGen Vertex 
		depthWrite
		alphaFunc GE128
	}
	{
		map $lightmap 
		blendfunc filter
		rgbGen identity
	}
}

textures/nature/nateweb
{
	qer_editorimage textures/nature/nateweb.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	cull disable	
	{
		map textures/nature/nateweb.png
		blendfunc add
		rgbgen identity
	}
}

