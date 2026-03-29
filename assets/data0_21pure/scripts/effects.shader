gfx/dash/dash_burst_1
{
	cull disable
	{
		map gfx/dash/dash_burst_1.png
		blendfunc blend
	}
}

gfx/dash/dash_burst_2
{
	cull disable
	{
		map gfx/dash/dash_burst_2.png
		blendfunc blend
	}

}
gfx/misc/smokepuff1
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff1.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}
gfx/misc/smokepuff3
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff3.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}
gfx/misc/smokepuff2
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff2.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}
gfx/misc/smokepuff2_dark
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff2_dark.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}
gfx/misc/smokepuff1_dark
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff1_dark.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}
gfx/misc/smokepuff3_dark
{
	cull none
	nopicmip
	entityMergable		// allow all the sprites to be merged together
	softParticle
	{
		clampmap gfx/misc/cartoon_smokepuff3_dark.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}

models/effects/head_stun
{
	cull none
	nopicmip
	{
		map models/effects/head_stun.png
		tcMod scroll 1 0
		blendfunc blend
		rgbGen entity
		alphagen entity
	}
}
