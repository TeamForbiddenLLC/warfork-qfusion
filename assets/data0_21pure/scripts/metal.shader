
textures/metal/aluminiumwrap
{	// from Boqu
	qer_editorimage textures/boqu/bright_metal
	
	{
		material textures/boqu/bright_metal
		tcmod scale 3 3
	}
}


textures/metal/treadplate0015
{	
	qer_editorimage textures/metal/treadplate0015.png

	{
		material textures/metal/treadplate0015.png
	}
}

textures/metal/treadplate0015b
{	
	qer_editorimage textures/metal/treadplate0015b.png

	{
		material textures/metal/treadplate0015b.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015_gloss.png
	}
}

textures/metal/treadplate0015c
{	
	qer_editorimage textures/metal/treadplate0015c.png

	{
		material textures/metal/treadplate0015c.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015_gloss.png
	}
}

textures/metal/treadplate0015d
{	
	qer_editorimage textures/metal/treadplate0015d.png

	{
		material textures/metal/treadplate0015d.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015_gloss.png
	}
}

textures/metal/treadplate0015e
{	
	qer_editorimage textures/metal/treadplate0015e.png

	{
		material textures/metal/treadplate0015e.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015_gloss.png
	}
}

textures/metal/treadplate0015f
{	
	qer_editorimage textures/metal/treadplate0015f.png

	{
		material textures/metal/treadplate0015f.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015f_gloss.png
	}
}

textures/metal/treadplate0015g
{	
	qer_editorimage textures/metal/treadplate0015g.png

	{
		material textures/metal/treadplate0015g.png textures/metal/treadplate0015_norm.png textures/metal/treadplate0015f_gloss.png
	}
}

textures/metal/metalbaseyellow0033
{	
	qer_editorimage textures/metal/metalbaseyellow0033.png
	glossExponent 256

	{
		material textures/metal/metalbaseyellow0033.png $blankbumpimage textures/metal/metalbaseyellow0033_gloss.png
	}
}

textures/metal/corrugated0023green
{	
	qer_editorimage textures/metal/corrugated0023green.png
	glossExponent 256

	{
		material textures/metal/corrugated0023green.png textures/metal/corrugated0023_norm.png textures/metal/corrugated0023green_gloss.png
	}
}

textures/metal/corrugated0023oxid
{	
	qer_editorimage textures/metal/corrugated0023oxid.png
	glossExponent 256

	{
		material textures/metal/corrugated0023oxid.png textures/metal/corrugated0023_norm.png textures/metal/corrugated0023_gloss.png
	}
}

textures/metal/aluminium
{	
	qer_editorimage textures/metal/aluminium.png

	glossExponent 24
	glossIntensity 0.9

	{
		material textures/metal/aluminium.png $blankbumpimage textures/metal/aluminium_gloss.png
	}
}

textures/metal/chrome
{	
	qer_editorimage textures/metal/chrome
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
	}

	{
		map $whiteImage
		rgbgen vertex
		alphagen const 0.25
		blendFunc blend
	}

	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
		alphagen const 0.6
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/metal/chrome2
{	
	qer_editorimage textures/metal/chrome
	surfaceparm nolightmap
	surfaceparm nomarks

	{
		map $whiteImage
		rgbgen vertex
	}

	{
		map env/2d/map1.png 
		tcGen environment
		alphagen const 0.35
		blendFunc blend
	}
}

textures/metal/chrome_rusty
{	
	qer_editorimage textures/metal/chrome
	surfaceparm nolightmap
	surfaceparm nomarks
	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
	}

	{
		map $whiteImage
		rgbgen vertex
		alphagen const 0.25
		blendFunc blend
	}

	{
		map env/2d/chrome1.png 
		tcGen environment
		rgbgen vertex
		alphagen const 0.6
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}

	{
		map textures/metal/chrome.blend.png
		rgbgen vertex
		blendFunc blend
	}
}

