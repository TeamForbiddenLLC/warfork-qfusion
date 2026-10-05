textures/blx_houses/blx_build1
{
	qer_editorimage textures/blx_houses/blx_build1.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_build1.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_build2
{
	qer_editorimage textures/blx_houses/blx_build2.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_build2.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_house3
{
	qer_editorimage textures/blx_houses/blx_house3.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_house3.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_house4
{
	qer_editorimage textures/blx_houses/blx_house4.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_house4.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_house5
{
	qer_editorimage textures/blx_houses/blx_house5.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_house5.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_house1
{
	qer_editorimage textures/blx_houses/blx_house1.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_house1.png
		rgbgen vertex
	}
}

textures/blx_houses/blx_cars1
{
	qer_editorimage textures/blx_houses/blx_cars1.png
	surfaceparm nolightmap
	{
		map textures/blx_houses/blx_cars1.png
		blendfunc blend
		tcmod scroll 0 0.5
	}
	{
		map textures/blx_houses/blx_cars2.png
		blendfunc blend
		tcmod scroll 0 1
	}

}
textures/blx_houses/blx_grid2
{
	alphashadows
	surfaceparm nolightmap
	qer_editorimage textures/blx_houses/blx_grid2.png
	{
		map textures/blx_houses/blx_grid2.png
		blendfunc blend
	}
}

textures/blx_houses/ctf_sky
{
	qer_editorimage textures/blx_houses/ctf_sky.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	q3map_surfacelight 200
	q3map_sun 0.27 0.4 0.51 155 170 33
	skyParms - - -
	{
		map textures/blx_houses/ctf_sky.png
		tcMod scale 8 8
		tcMod scroll 0.0 0.0
		depthWrite
	}
	{
		map textures/cha0s_ws/ch_sky_night_front.png
		blendfunc add
		tcMod scale 2 2
		tcMod scroll 0.01 0.01
	}
}

textures/blx_houses/portal
{
   portal   //flags surface to behave as a portal
   qer_editorImage textures/blx/blx_dark.png
   surfaceparm nolightmap
   {
      map textures/blx/blx_dark.png
   }
   {
      map textures/blx_houses/blx_streets1.png
      blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
      alphaGen portal 512   //fades to opaque after a
               //distance of 512 game units
   }
} 

textures/blx_houses/blx_ctf_alpha
{
	surfcaeparm nolightmap
	surfaceparm nomarks
	nopicmip
	{
		map textures/blx_houses/blx_ctf_teamback1.png
		rgbGen teamcolor 2
	}
	{
		map textures/blx_houses/blx_ctf_alpha.png
		blendfunc blend
	}
}

textures/blx_houses/blx_ctf_beta
{
	surfcaeparm nolightmap
	surfaceparm nomarks
	nopicmip
	{
		map textures/blx_houses/blx_ctf_teamback1.png
		rgbGen teamcolor 3
	}
	{
		map textures/blx_houses/blx_ctf_beta.png
		blendfunc blend
	}
}

textures/blx_houses/blx_flare_team1
{
	qer_editorImage   gfx/misc/ctf_flare_b.png
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nonsolid
	deformVertexes autosprite
	nopicmip
	{
		clampmap gfx/misc/ctf_flare_a.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen teamcolor 2
		//rgbGen	vertex
		alphaGen	vertex
		tcMod rotate 3
	}
	{
		clampmap gfx/misc/ctf_flare_b.png
		blendFunc add
		rgbGen teamcolor 2
		alphaGen	vertex
		tcMod rotate 10
	}
}

textures/blx_houses/blx_flare_team2
{
	qer_editorImage   gfx/misc/ctf_flare_b.png
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nonsolid
	deformVertexes autosprite
	nopicmip
	{
		clampmap gfx/misc/ctf_flare_a.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		//rgbGen teamcolor 3
		//rgbGen	vertex
		alphaGen	vertex
		tcMod rotate 3
	}
	{
		clampmap gfx/misc/ctf_flare_b.png
		blendFunc add
		rgbGen teamcolor 3
		alphaGen	vertex
		tcMod rotate 10
	}
}
