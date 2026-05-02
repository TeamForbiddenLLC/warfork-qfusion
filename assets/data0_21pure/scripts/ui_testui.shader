ui/testui/gfx/background
{
	nopicmip
	nomipmaps
	nocompress
	nofiltering
	cull none

	{
		map ui/testui/gfx/back2.png
		blendFunc blend
		alphaGen const .1
	}

	{
		map textures/billboard/scanlinenoise.png
		blendFunc blend
		tcMod scale 2 2
		tcMod scroll 0 0.02
		alphaGen const 0.05
	}
	
	{
		map textures/billboard/scanlinenoise.png
		blendFunc blend
		tcMod scale 2 2
		tcMod transform 1 1 0 0 0.32 0.32
		tcMod scroll 0 0.05
		alphaGen const 0.05
	}
}

ui/testui/gfx/background2
{
	nopicmip
	nomipmaps
	nocompress
	cull none

	{
		map ui/testui/gfx/bandes2.png
		blendFunc blend
  		alphagen wave sin 0.05 0.1 0 0.05
  		tcmod scroll 0 -.08
	}
}

ui/testui/gfx/loader_simple
{
	noPicmip
	noMipmaps
	nocompress
	cull none

	{
		clampmap ui/testui/gfx/loader_simple.png
		blendfunc blend
		tcmod rotate 500
	}
}
