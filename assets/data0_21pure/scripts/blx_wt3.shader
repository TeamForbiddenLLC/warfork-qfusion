
textures/blx_wtest3/blx_wt3_pillar3d
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3d.png

	{
		material textures/blx_wtest3/blx_wt3_pillar3d.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3d_gloss.png
	}
}

textures/blx_wtest3/blx_wt3_lines
{
	qer_editorimage textures/blx_wtest3/blx_wt3_lines.png

	{
		material textures/blx_wtest3/blx_wt3_lines.png textures/blx_wtest3/blx_wt3_lines_norm.png textures/blx_wtest3/blx_wt3_lines_gloss.png
	}
}

textures/blx_wtest3/blx_wt3_surfmetal1
{
	qer_editorimage textures/blx_wtest3/blx_wt3_surfmetal1.png

	{
		material textures/blx_wtest3/blx_wt3_surfmetal1.png $blankBumpImage
	}
}

textures/blx_wtest3/blx_wt3_surfmetal2
{
	qer_editorimage textures/blx_wtest3/blx_wt3_surfmetal2.png

	{
		material textures/blx_wtest3/blx_wt3_surfmetal2.png $blankBumpImage textures/blx_wtest3/blx_wt3_surfmetal_gloss.png
	}
}

textures/blx_wtest3/blx_wt3_surfmetal2_nosolid
{
	qer_editorimage textures/blx_wtest3/blx_wt3_surfmetal2.png
	surfaceparm nonsolid

	{
		material textures/blx_wtest3/blx_wt3_surfmetal2.png $blankBumpImage textures/blx_wtest3/blx_wt3_surfmetal_gloss.png
	}
}

textures/blx_wtest3/blx_wt3_surfmetal3
{
	qer_editorimage textures/blx_wtest3/blx_wt3_surfmetal3.png

	{
		material textures/blx_wtest3/blx_wt3_surfmetal3.png $blankBumpImage textures/blx_wtest3/blx_wt3_surfmetal_gloss.png
	}
}


textures/blx_wtest3/glow2
{	
	nopicmip
	qer_editorimage textures/blx/blx_glow2.png
	cull back
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	qer_trans 0.5
	{	
		map textures/blx/blx_glow2.png
		blendfunc add
		tcmod scale 1 0.98
	}
}

textures/blx_wtest3/blx_wt3_glow
{	
	nopicmip
	qer_editorimage textures/blx_wtest3/blx_wt3_glow.png
	polygonOffset 
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nomarks
	qer_trans 0.5
	{	
		map textures/blx_wtest3/blx_wt3_glow.png
		blendfunc add
	}
}


textures/blx_wtest3/blx_trans_noclip
{
	qer_editorimage textures/blx_wtest3/blx_trans_noclip.png
	surfaceparm trans
	surfaceparm nonsolid
	{
		map textures/blx_wtest3/blx_trans_noclip.png
		blendfunc filter
	}
}

textures/blx_wtest3/blx_grid2
{
	qer_editorimage textures/grates/fence01.png
	surfaceparm nomarks
	//surfaceparm	nonsolid
	surfaceparm alphashadow
	cull none
	nopicmip
	//nomipmaps
	q3map_forceMeta

	{
		material textures/grates/fence01.png textures/grates/fence01_norm.png
		alphaFunc GE128
		//blendFunc blend
		depthWrite
		tcmod scale 0.75 0.75
	}
}

textures/blx_wtest3/blx_grid2_noalphashadow
{
	qer_editorimage textures/grates/fence01.png
	surfaceparm nomarks
	//surfaceparm	nonsolid
	cull none
	nopicmip
	//nomipmaps

	{
		material textures/grates/fence01.png textures/grates/fence01_norm.png
		alphaFunc GE128
		//blendFunc blend
		depthWrite
		tcmod scale 0.75 0.75
	}
}

textures/blx_wtest3/blx_wt3_grid
{
	qer_editorimage textures/grates/simplegrid.png
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	nopicmip
	smallestmipmapsize 8

	{
		material textures/grates/simplegrid.png
		//blendFunc blend
		alphaFunc GE128
		depthWrite
	}
}

textures/blx_wtest3/lava
{
	q3map_globaltexture
	//surfaceparm trans
	surfaceparm nonsolid
	surfaceparm noimpact
	surfaceparm lava
	surfaceparm nolightmap
	q3map_surfacelight 250
	qer_editorimage textures/world/sh/lava2.png
	tesssize 64

	deformVertexes wave 100 sin 3 2 .1 0.1
	
        {
		map textures/world/sh/lava1_glow.png
                //tcmod scale .2 .2
                //tcmod scroll .04 .03
		tcmod scroll .005 .005
                tcMod turb 0 .1 0 .05
                blendFunc GL_ONE GL_ZERO
	}
	{
		map textures/world/sh/lava2.png
            	blendfunc add
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod turb 0 .2 0 .1
		tcmod scroll .0025 .0025
	}
}

textures/blx_wtest3/blx_wt3_rock3-switch
{
	qer_editorimage textures/blx_wtest3/blx_wt3_rock3-switch.png

	offsetMappingScale 3

	if deluxe
		{
			material textures/blx_wtest3/blx_wt3_rock3-switch.png textures/blx_wtest3/blx_wt3_rock3_norm.png - textures/blx_wtest3/blx_wt3_rock3-switch-red.png
			rgbGen wave sin 0 1 0 0.2
		}
	endif


	if ! deluxe
		{
			map $lightmap
			rgbGen identity
		}
		{
			map textures/blx_wtest3/blx_wt3_rock3-switch.png
			blendfunc filter
		}
		{
			map textures/blx_wtest3/blx_wt3_rock3-switch-red.png
			rgbGen wave sin 0 1 0 0.2
			blendfunc add
		}
	endif
}


textures/blx_wtest3/blx_wt3_rock3
{
	qer_editorimage textures/blx_wtest3/blx_wt3_rock3.png

	offsetMappingScale 3

	{
		material textures/blx_wtest3/blx_wt3_rock3
	}
}

textures/blx_wtest3/blx_wt3_rock3-grey
{
	qer_editorimage textures/blx_wtest3/blx_wt3_rock3-grey.png

	offsetMappingScale 3

	if deluxe
		{
			material textures/blx_wtest3/blx_wt3_rock3-grey.png textures/blx_wtest3/blx_wt3_rock3_norm.png - textures/blx_wtest3/blx_wt3_rock3-grey-red.png
			rgbGen wave sin 0 1 0 0.2
		}
	endif

	if ! deluxe
		{
			map $lightmap
			rgbGen identity
		}
		{
			map textures/blx_wtest3/blx_wt3_rock3-grey.png
			blendfunc filter
		}
		{
			map textures/blx_wtest3/blx_wt3_rock3-grey-red.png
			rgbGen wave sin 0 1 0 0.2
			blendfunc add
		}

	endif
}

textures/blx_wtest3/blx_wt3_grey
{
	qer_editorimage textures/blx_wtest3/blx_wt3_grey.png

	{
		material textures/blx_wtest3/blx_wt3_grey.png $blankbumpimage
	}
}

textures/blx_wtest3/blx_wt3_grey_nonsolid
{
	qer_editorimage textures/blx_wtest3/blx_wt3_grey.png
	surfaceparm nonsolid
	{
		material textures/blx_wtest3/blx_wt3_grey.png $blankbumpimage
	}
}

textures/blx_wtest3/blx_wt3_floor4_nonsolid
{
	qer_editorimage textures/blx_wtest3/blx_wt3_floor4.png
	surfaceparm nonsolid

	{
		material textures/blx_wtest3/blx_wt3_floor4.png textures/blx_wtest3/blx_wt3_floor4_norm.png
	}
}

// TEAM COLORED SHADERS


textures/blx_wtest3/blx_wt3_pillar2_alpha
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar2.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/blx_wtest3/blx_wt3_pillar2.png
		blendfunc filter
		rgbgen teamColorWave 2 sin 2.5 0 0 0
	}
endif

if deluxe
	{
		material textures/blx_wtest3/blx_wt3_pillar2.png textures/blx_wtest3/blx_wt3_pillar2_norm.png textures/blx_wtest3/blx_wt3_pillar2_gloss.png
		rgbgen teamColorWave 2 sin 2.5 0 0 0
	}

endif
}

textures/blx_wtest3/blx_wt3_pillar2_beta
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar2.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/blx_wtest3/blx_wt3_pillar2.png
		blendfunc filter
		rgbgen teamColorWave 3 sin 2.5 0 0 0
	}
endif

if deluxe
	{
		material textures/blx_wtest3/blx_wt3_pillar2.png textures/blx_wtest3/blx_wt3_pillar2_norm.png textures/blx_wtest3/blx_wt3_pillar2_gloss.png
		rgbgen teamColorWave 3 sin 2.5 0 0 0
	}
endif
}

textures/blx_wtest3/blx_wt3_pillar3_alpha
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/blx_wtest3/blx_wt3_pillar3.png
		blendfunc filter
		rgbgen teamColorWave 2 sin 2.5 0 0 0
	}
endif

if deluxe
	{
		material textures/blx_wtest3/blx_wt3_pillar3.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
		rgbgen teamColorWave 2 sin 2.5 0 0 0
	}
endif
}

textures/blx_wtest3/blx_wt3_pillar3_beta
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/blx_wtest3/blx_wt3_pillar3.png
		blendfunc filter
		rgbgen teamColorWave 3 sin 2.5 0 0 0
	}
endif

if deluxe
	{
		material textures/blx_wtest3/blx_wt3_pillar3.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
		rgbgen teamColorWave 3 sin 2.5 0 0 0
	}
endif
}

textures/blx_wtest3/blx_wt3_pillar3_alpha2
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png

	{
		map $whiteImage
	}

	{
		map $whiteImage
		alphagen const 0.4
		rgbgen teamcolor 2
		tcMod scale 0.5 4
		blendFunc blend
	}

if ! deluxe

	{
		map $lightmap
		blendFunc filter
	} 
	{
		map textures/blx_wtest3/blx_wt3_pillar3.png
		blendFunc filter
	}

endif

if deluxe

	{
		material textures/blx_wtest3/blx_wt3_pillar3.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
		rgbgen identity
		blendFunc filter
	}

endif
}

textures/blx_wtest3/blx_wt3_pillar3_beta2
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar3.png

	{
		map $whiteImage
	}

	{
		map $whiteImage
		alphagen const 0.4
		rgbgen teamcolor 3
		tcMod scale 0.5 4
		blendFunc blend
	}

if ! deluxe

	{
		map $lightmap
		blendFunc filter
	} 

	{
		map textures/blx_wtest3/blx_wt3_pillar3.png
		blendFunc filter
	}

endif

if deluxe

	{
		material textures/blx_wtest3/blx_wt3_pillar3.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
		rgbgen identity
		blendFunc filter
	}

endif
}

textures/blx_wtest3/blx_wt3_pillar_alpha
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar.png

	{
		map $whiteImage
	}

	{
		map $whiteImage
		alphagen const 0.4
		rgbgen teamcolor 2
		tcMod scale 0.5 4
		blendFunc blend
	}

if ! deluxe

	{
		map $lightmap
		blendFunc filter
	} 

	{
		map textures/blx_wtest3/blx_wt3_pillar.png
		blendFunc filter
	}

endif

if deluxe

	{
		material textures/blx_wtest3/blx_wt3_pillar.png textures/blx_wtest3/blx_wt3_pillar_norm.png textures/blx_wtest3/blx_wt3_pillar_gloss.png
		rgbgen identity
		blendFunc filter
	}

endif
}


textures/blx_wtest3/blx_wt3_pillar_beta
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar.png

	{
		map $whiteImage
	}

	{
		map $whiteImage
		alphagen const 0.4
		rgbgen teamcolor 3
		tcMod scale 0.5 4
		blendFunc blend
	}

if ! deluxe

	{
		map $lightmap
		blendFunc filter
	} 

	{
		map textures/blx_wtest3/blx_wt3_pillar.png
		blendFunc filter
	}

endif

if deluxe

	{
		material textures/blx_wtest3/blx_wt3_pillar.png textures/blx_wtest3/blx_wt3_pillar_norm.png textures/blx_wtest3/blx_wt3_pillar_gloss.png
		rgbgen identity
		blendFunc filter
	}

endif
}

textures/blx_wtest3/blx_wt3_pillar_nonsolid
{
	qer_editorimage textures/blx_wtest3/blx_wt3_pillar.png
	surfaceparm nonsolid

	{
		material textures/blx_wtest3/blx_wt3_pillar.png textures/blx_wtest3/blx_wt3_pillar3_norm.png textures/blx_wtest3/blx_wt3_pillar3_gloss.png
	}
}

textures/blx_wtest3/blx_wt3_mud1_tessSize
{
	qer_editorimage textures/blx_wtest3/blx_wt3_mud1.png
	tessSize 256

	{
		material textures/blx_wtest3/blx_wt3_mud1.png
	}
}
