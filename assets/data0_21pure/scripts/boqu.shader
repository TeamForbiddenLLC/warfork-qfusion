
textures/boqu/bright_metal
{
	qer_editorimage textures/boqu/bright_metal
	{
		material textures/boqu/bright_metal
		tcmod scale 3 3
	}
}


textures/boqu/hexx
{	
	qer_trans 0.5
	qer_editorimage textures/boqu/hexx.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 64

	{
		material
		alphaFunc GE128
		blendFunc blend
		depthWrite
	}
}

textures/boqu/hex_grate
{	
	qer_trans 0.5
	qer_editorimage textures/boqu/hex_grate.png
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	sort additive
	cull none
	nopicmip
	nomipmaps
	q3map_forceMeta
	q3map_lightmapsamplesize 32

	{
		material
		alphaFunc GE128
		blendFunc blend
		depthWrite
	}
}

textures/boqu/greytrim2
{	
	qer_editorimage textures/boqu/greytrim2x.png

	{
		material textures/boqu/greytrim2x
	}
}

textures/boqu/greytrim_hexyellow
{	
	qer_editorimage textures/boqu/greytrim_hexyellow.png

	{
		material
	}
}

textures/boqu/midtrim
{
	qer_editorimage textures/boqu/midtrim.png
	q3map_lightimage textures/boqu/midtrim_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_glow.png
		blendFunc blend	
	}	
endif

if deluxe
	{
		material
	}
endif
}

textures/boqu/midtrim_2
{
	qer_editorimage textures/boqu/midtrim_2.png
	q3map_lightimage textures/boqu/midtrim_2_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim_2.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_2_glow.png 
		blendFunc blend
		
	}	
endif

if deluxe
	{
		material textures/boqu/midtrim_2.png textures/boqu/midtrim_norm.png - textures/boqu/midtrim_2_glow.png 
	}
endif
}

textures/boqu/midtrim_2_w
{
	qer_editorimage textures/boqu/midtrim_2_w.png
	q3map_lightimage textures/boqu/midtrim_2_w_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim_2_w.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_2_w_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/midtrim_2_w.png textures/boqu/midtrim_norm.png - textures/boqu/midtrim_2_w_glow.png 
	}
endif
}

textures/boqu/tile_slim_light_yellow_200
{
	qer_editorimage textures/boqu/tile_slim_light_yellow.png
	q3map_lightimage textures/boqu/tile_slim_light_yellow_glow.png
	q3map_surfacelight 200
  	q3map_lightsubdivide 128
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light_yellow.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_yellow_glow.png 
	}
endif
}

textures/boqu/tile_slim_light_yellow_400
{
	qer_editorimage textures/boqu/tile_slim_light_yellow.png
	q3map_lightimage textures/boqu/tile_slim_light_yellow_glow.png
	q3map_surfacelight 400
  	q3map_lightsubdivide 128
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_yellow_glow.png 
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim_light_yellow.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_yellow_glow.png 
	}
endif
}
textures/boqu/tile_slim_light_yellow_800
{
	qer_editorimage textures/boqu/tile_slim_light_yellow.png
	q3map_lightimage textures/boqu/tile_slim_light_yellow_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light_yellow.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_yellow_glow.png 
	}
endif
}

textures/boqu/tile_slim_light_yellow_1200
{
	qer_editorimage textures/boqu/tile_slim_light_yellow.png
	q3map_lightimage textures/boqu/tile_slim_light_yellow_glow.png
	q3map_surfacelight 1200
  	q3map_lightsubdivide 128	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light_yellow.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_yellow_glow.png 
	}
endif
}
textures/boqu/tile_slim_light_yellow_1600
{
	qer_editorimage textures/boqu/tile_slim_light_yellow.png
	q3map_lightimage textures/boqu/tile_slim_light_yellow_glow.png
	q3map_surfacelight 1600
  	q3map_lightsubdivide 128
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light_yellow.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif
}

textures/boqu/tile_slim__yellow1
{
	qer_editorimage textures/boqu/tile_slim__yellow1.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow1.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow1.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}
textures/boqu/tile_slim__yellow2
{
	qer_editorimage textures/boqu/tile_slim__yellow2.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow2.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}
textures/boqu/tile_slim__yellow3
{
	qer_editorimage textures/boqu/tile_slim__yellow3.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow3.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow3.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}

textures/boqu/tile_slim__yellow4
{
	qer_editorimage textures/boqu/tile_slim__yellow4.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow4.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow4.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}

textures/boqu/tile_slim__yellow5
{
	qer_editorimage textures/boqu/tile_slim__yellow5.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow5.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow5.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}

textures/boqu/tile_slim__yellow6
{
	qer_editorimage textures/boqu/tile_slim__yellow6.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow6.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow6.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}

textures/boqu/tile_slim__yellow7
{
	qer_editorimage textures/boqu/tile_slim__yellow7.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow7.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow7.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}

textures/boqu/tile_slim__yellow8
{
	qer_editorimage textures/boqu/tile_slim__yellow8.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim__yellow8.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/tile_slim__yellow8.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png 
	}
endif

}


textures/boqu/tile_slim_light_200
{
	qer_editorimage textures/boqu/tile_slim_light.png
	q3map_lightimage textures/boqu/tile_slim_light_glow.png
	q3map_surfacelight 200
  	q3map_lightsubdivide 128
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	
	
	q3map_forcemeta

if ! deluxe
	{
		map textures/boqu/tile_slim_light.png
	}
	{
		map textures/boqu/tile_slim_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_glow.png 
	}
endif
}

textures/boqu/tile_slim_light_400
{
	qer_editorimage textures/boqu/tile_slim_light.png
	q3map_lightimage textures/boqu/tile_slim_light_glow.png
	q3map_surfacelight 400
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_glow.png 
	}
endif
}
textures/boqu/tile_slim_light_800
{
	qer_editorimage textures/boqu/tile_slim_light.png
	q3map_lightimage textures/boqu/tile_slim_light_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_glow.png 
	}
endif
}

textures/boqu/tile_slim_light_1000
{
	qer_editorimage textures/boqu/tile_slim_light.png
	q3map_lightimage textures/boqu/tile_slim_light_glow.png
	q3map_surfacelight 1000
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_slim_light.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_slim_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_slim_light.png textures/boqu/tile_slim_norm.png textures/boqu/tile_slim_light_gloss.png textures/boqu/tile_slim_light_glow.png 
	}
endif
}
textures/boqu/midtrim_blue
{
	qer_editorimage textures/boqu/midtrim_blue.png
	q3map_lightimage textures/boqu/midtrim_blue_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim_blue.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_blue_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/midtrim_blue.png textures/boqu/midtrim_norm.png - textures/boqu/midtrim_blue_glow.png 
	}
endif
}
textures/boqu/midtrim_pink
{
	qer_editorimage textures/boqu/midtrim_pink.png
	q3map_lightimage textures/boqu/midtrim_pink_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim_pink.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_pink_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/midtrim_pink.png textures/boqu/midtrim_norm.png - textures/boqu/midtrim_pink_glow.png 
	}
endif
}

textures/boqu/midtrim_yellow
{
	qer_editorimage textures/boqu/midtrim_yellow.png
	q3map_lightimage textures/boqu/midtrim_yellow_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/midtrim_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/midtrim_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/midtrim_yellow.png textures/boqu/midtrim_norm.png - textures/boqu/midtrim_yellow_glow.png 
	}
endif
}

textures/boqu/ceilinglamptrim
{
	qer_editorimage textures/boqu/ceilinglamptrim.png
	q3map_lightimage textures/boqu/ceilinglamptrim_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamptrim.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamptrim_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/ceilinglamptrim
	}
endif
}

textures/boqu/ceilinglamp_w
{
	qer_editorimage textures/boqu/ceilinglamp_w.png
	q3map_lightimage textures/boqu/ceilinglamp_w_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamp_w.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamp_w_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/ceilinglamp
	}
endif
}

textures/boqu/squarelamp
{
	qer_editorimage textures/boqu/squarelamp.png
	q3map_lightimage textures/boqu/squarelamp_glow.png
	q3map_surfacelight 800
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/squarelamp.png
		blendFunc filter
	}
	{
	   map textures/boqu/squarelamp_glow.png 
	   blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/squarelamp
	}
endif
}

textures/boqu/squarelamp_blue
{
	qer_editorimage textures/boqu/squarelamp_blue.png
	q3map_lightimage textures/boqu/squarelamp_blue_glow.png
	q3map_surfacelight 800
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/squarelamp_blue.png
		blendFunc filter
	}
	{
		map textures/boqu/squarelamp_blue_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/squarelamp_blue.png textures/boqu/squarelamp_norm.png textures/boqu/squarelamp_gloss.png textures/boqu/squarelamp_blue_glow.png 
	}
endif
}

textures/boqu/light2_800
{
	qer_editorimage textures/boqu/light2.png
	q3map_lightimage textures/boqu/light2_glow.png
	q3map_surfacelight 800
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/light2.png
		blendFunc filter
	}
	{
	   map textures/boqu/light2_glow.png 
	   blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/light2 textures/flatnorm - textures/boqu/light2_glow.png 
	}
endif
}
textures/boqu/light2_400
{
	qer_editorimage textures/boqu/light2.png
	q3map_lightimage textures/boqu/light2_glow.png
	q3map_surfacelight 400
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/light2.png
		blendFunc filter
	}
	{
	   map textures/boqu/light2_glow.png 
	   blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/light2
	}
endif
}

textures/boqu/light2_200
{
	qer_editorimage textures/boqu/light2.png
	q3map_lightimage textures/boqu/light2_glow.png
	q3map_surfacelight 200
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/light2.png
		blendFunc filter
	}
	{
		map textures/boqu/light2_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/light2
	}
endif
}


textures/boqu/stairfront_light_200
{
	qer_editorimage textures/boqu/stairfront_light.png
	q3map_lightimage textures/boqu/stairfront_light_glow.png
	q3map_surfacelight 200
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/stairfront_light.png
		blendFunc filter
	}
	{
		map textures/boqu/stairfront_light_glow.png 
		blendFunc blend
    }	
endif

if deluxe
	{
		material textures/boqu/stairfront_light.png textures/boqu/stairfront_norm.png textures/boqu/stairfront_gloss.png textures/boqu/stairfront_light_glow.png 
	}
endif
}
textures/boqu/stairfront_light_400
{
	qer_editorimage textures/boqu/stairfront_light.png
	q3map_lightimage textures/boqu/stairfront_light_glow.png
	q3map_surfacelight 400
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/stairfront_light.png
		blendFunc filter
	}
	{
		map textures/boqu/stairfront_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/stairfront_light.png textures/boqu/stairfront_norm.png textures/boqu/stairfront_gloss.png textures/boqu/stairfront_light_glow.png 	
	}
endif
}

textures/boqu/stairfront_light_600
{
	qer_editorimage textures/boqu/stairfront_light.png
	q3map_lightimage textures/boqu/stairfront_light_glow.png
	q3map_surfacelight 600
  	q3map_lightsubdivide 128
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/stairfront_light.png
		blendFunc filter
	}
	{
		map textures/boqu/stairfront_light_glow.png 
		blendFunc blend
    }	
endif

if deluxe
	{
		material textures/boqu/stairfront_light.png textures/boqu/stairfront_norm.png textures/boqu/stairfront_gloss.png textures/boqu/stairfront_light_glow.png 
	}
endif
}

textures/boqu/stairfront_light_800
{
	qer_editorimage textures/boqu/stairfront_light.png
	q3map_lightimage textures/boqu/stairfront_light_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	
	
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/stairfront_light.png
		blendFunc filter
	}
	{
		map textures/boqu/stairfront_light_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/stairfront_light.png textures/boqu/stairfront_norm.png textures/boqu/stairfront_gloss.png textures/boqu/stairfront_light_glow.png 
	}
endif
}

textures/boqu/biglightfloor_200
{
	qer_editorimage textures/boqu/biglightfloor.png
	q3map_lightimage textures/boqu/biglightfloor_glow.png
	q3map_surfacelight 200
  	q3map_lightsubdivide 128
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/biglightfloor.png
		blendFunc filter
	}
	{
		map textures/boqu/biglightfloor_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/biglightfloor
	}
endif
}

textures/boqu/ceilinglamptrim_orange
{
	qer_editorimage textures/boqu/ceilinglamptrim_orange.png
	q3map_lightimage textures/boqu/ceilinglamptrim_orange_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamptrim_orange.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamptrim_orange_glow.png 
		blendFunc blend
	}
endif

if deluxe
	{
		material textures/boqu/ceilinglamptrim_orange.png textures/boqu/ceilinglamptrim_w_norm.png - textures/boqu/ceilinglamptrim_orange_glow.png 
	}
endif
}

textures/boqu/ceilinglamptrim_blue
{
	qer_editorimage textures/boqu/ceilinglamptrim_blue.png
	q3map_lightimage textures/boqu/ceilinglamptrim_blue_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamptrim_blue.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamptrim_blue_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/ceilinglamptrim_blue.png textures/boqu/ceilinglamptrim_w_norm.png - textures/boqu/ceilinglamptrim_blue_glow.png 
	}
endif
}

textures/boqu/ceilinglamptrim_yellow
{
	qer_editorimage textures/boqu/ceilinglamptrim_yellow.png
	q3map_lightimage textures/boqu/ceilinglamptrim_yellow_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamptrim_yellow.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamptrim_yellow_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/ceilinglamptrim_yellow.png textures/boqu/ceilinglamptrim_w_norm.png - textures/boqu/ceilinglamptrim_yellow_glow.png 
	}
endif
}
textures/boqu/ceilinglamptrim_pink
{
	qer_editorimage textures/boqu/ceilinglamptrim_pink.png
	q3map_lightimage textures/boqu/ceilinglamptrim_pink_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/ceilinglamptrim_pink.png
		blendFunc filter
	}
	{
		map textures/boqu/ceilinglamptrim_pink_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/ceilinglamptrim_pink.png textures/boqu/ceilinglamptrim_w_norm.png - textures/boqu/ceilinglamptrim_pink_glow.png 
	}
endif
}

textures/boqu/tile_whiteglow
{
	qer_editorimage textures/boqu/tile_whiteglow.png
	q3map_lightimage textures/boqu/tile_whiteglow_glow.png
	q3map_surfacelight 500
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/tile_whiteglow.png
		blendFunc filter
	}
	{
		map textures/boqu/tile_whiteglow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/tile_whiteglow
	}
endif
}

textures/boqu/biglightfloor
{
	qer_editorimage textures/boqu/biglightfloor.png
	q3map_lightimage textures/boqu/biglightfloor_glow.png
	q3map_surfacelight 250
  	q3map_lightsubdivide 120
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/biglightfloor.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/biglightfloor
	}
endif
}

textures/boqu/glass1
{
	qer_editorimage textures/boqu/glass1.png
	qer_trans 0.5
	surfaceparm lightfilter
	surfaceparm	nolightmap
	surfaceparm	nomarks
	surfaceparm	trans
	cull disable
	nopicmip	
	{
		map textures/boqu/glass1.png
		blendFunc blend
		rgbGen identity
	}
}

textures/boqu/glass2
{
	qer_editorimage textures/boqu/glass1.png
	qer_trans 0.5
	surfaceparm lightfilter
	surfaceparm	nolightmap
	surfaceparm	nomarks
	surfaceparm	trans
	cull disable
	nopicmip
	{
		map textures/boqu/glass2.png
		blendFunc blend
		rgbGen identity
	}
}	
	
textures/boqu/whitelight
{
	qer_editorimage textures/boqu/whitelight.png
	q3map_lightimage textures/boqu/whitelight.png
	q3map_surfacelight 100
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta
	{
		map $lightmap
	}
	{
		map textures/boqu/whitelight.png
		blendFunc filter
	}
}

textures/boqu/hexagon_floor3
{	
	qer_editorimage textures/boqu/hexagon_floor3.png

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/hexagon_floor3.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/hexagon_floor3.png textures/boqu/hexagon_floor2_norm.png textures/boqu/hexagon_floor2_gloss.png
	}
endif
}

textures/boqu/teleporterback
{
	qer_editorimage textures/boqu/teleporterback.png
	q3map_lightimage textures/boqu/teleporterback_glow.png
	q3map_surfacelight 200
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/teleporterback.png
	
	}
	{
		map textures/boqu/teleporterback_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/teleporterback
	}
endif
}
	
	
textures/boqu/glow_halo_yellow
{
	qer_editorimage textures/boqu/glow_halo_yellow.png
	//qer_trans 0.25
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite
	nopicmip
	softParticle

	{
		detail
		clampmap textures/boqu/glow_halo_yellow.png
		blendfunc add
		rgbgen wave distanceramp 0 0.7 80 400
	}
}

textures/boqu/glow_halo_blue
{
	qer_editorimage textures/boqu/glow_halo_blue.png
	//qer_trans 0.25
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite
	nopicmip
	softParticle

	{
		detail
		clampmap textures/boqu/glow_halo_blue.png
		blendfunc add
		rgbgen wave distanceramp 0 0.7 80 400
	}
}

textures/boqu/glow_halo_yellow_soft
{
	qer_editorimage textures/boqu/glow_halo_yellow.png
	qer_trans 0.35
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite
	softParticle

	{
		detail
		clampmap textures/boqu/glow_halo_yellow.png
		blendfunc add
		rgbgen wave distanceramp 0 0.5 80 400
	}
}
textures/boqu/glow_halo_blue_soft
{
	qer_editorimage textures/boqu/glow_halo_blue.png
	qer_trans 0.35
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite
	softParticle

	{
		detail
		clampmap textures/boqu/glow_halo_blue.png
		blendfunc add
		rgbgen wave distanceramp 0 0.5 80 400
	}
}
textures/boqu/warsow_neon_gold
{
	q3map_lightimage textures/boqu/warsow_neon_gold.png
	q3map_surfacelight 50
  	q3map_lightsubdivide 128
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip
	qer_editorimage textures/boqu/warsow_neon_gold.png
	cull none
	{
		map textures/boqu/warsow_neon_gold.png
		blendFunc add
	}
}

textures/boqu/warsow_neon_gold2
{
	surfaceparm nonsolid
	surfaceparm nomarks
	surfaceparm nolightmap
	nopicmip
	qer_editorimage textures/boqu/warsow_neon_gold2.png
	cull none
	{
		map textures/boqu/warsow_neon_gold2.png
		blendFunc add
	}
}

textures/boqu/tele_decal1
{
    qer_editorimage textures/boqu/decals/tele_decal1.png
    surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip
	detail
	
	{
		map textures/boqu/decals/tele_decal1.png 
		rgbgen const 0.9 0.9 0.9
		blendFunc blend
		alphaFunc GE128
	}
}

textures/boqu/explosive_decal
{
    qer_editorimage textures/boqu/decals/explosive_decal.png
    surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip
	detail

	{
		map textures/boqu/decals/explosive_decal.png 
		rgbgen const 0.9 0.9 0.9
		blendFunc blend
		alphaFunc GE128
	}
}

textures/boqu/rect_lamp
{
	qer_editorimage textures/boqu/rect_lamp.png
	q3map_lightimage textures/boqu/rect_lamp_glow.png
	q3map_surfacelight 800
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/rect_lamp.png
		blendFunc filter
	}
	{
		map textures/boqu/rect_lamp_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material
	}
endif
}

textures/boqu/rect_lamp_yellow
{
	qer_editorimage textures/boqu/rect_lamp_yellow.png
	q3map_lightimage textures/boqu/rect_lamp_yellow_glow.png
	q3map_surfacelight 800
	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	
if ! deluxe
	{
		map $lightmap
	}

	{
		map textures/boqu/rect_lamp_yellow.png
		blendFunc filter
	}

	{
	   map textures/boqu/rect_lamp_yellow_glow.png 
	   blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/rect_lamp_yellow.png textures/boqu/rect_lamp_norm.png textures/boqu/rect_lamp_gloss.png textures/boqu/rect_lamp_yellow_glow.png 
	}
endif
}

textures/boqu/decals/yellowarrows_outlined
{	
	qer_editorimage textures/boqu/decals/yellowarrows_outlined.png
	qer_trans 0.6
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		map textures/boqu/decals/yellowarrows_outlined.png
		blendFunc blend
		alphaFunc GE128
	}
}

textures/boqu/decals/wswdecal
{	
	qer_editorimage textures/boqu/decals/wswdecal.png
	qer_trans 0.6
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		map textures/boqu/decals/wswdecal.png
		blendFunc blend
		alphaFunc GE128
	}
}

textures/boqu/decals/numbers
{	
	qer_editorimage textures/boqu/decals/numbers.png
	qer_trans 0.6
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip
	detail
	{
		map textures/boqu/decals/numbers.png
		blendFunc blend
		alphaFunc GE128
	}
}
textures/boqu/lightstripe_800
{
	qer_editorimage textures/boqu/lightstripe.png
	q3map_lightimage textures/boqu/lightstripe_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight

if ! deluxe
	{
		map textures/boqu/lightstripe.png
	}
	{
		map textures/boqu/lightstripe_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/lightstripe
	}
endif
}

textures/boqu/lightstripe_blue_800
{
	qer_editorimage textures/boqu/lightstripe_blue.png
	q3map_lightimage textures/boqu/lightstripe_blue_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight

if ! deluxe
	{
		map textures/boqu/lightstripe_blue.png
	}
	{
		map textures/boqu/lightstripe_blue_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/lightstripe_blue.png textures/boqu/lightstripe_norm.png textures/boqu/lightstripe_gloss.png textures/boqu/lightstripe_blue_glow.png 
	}
endif
}
textures/boqu/simplemid
{
	qer_editorimage textures/boqu/simplemid.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/simplemid.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/simplemid.png textures/boqu/simpledark_norm.png textures/boqu/simpledark_gloss.png 
	}
endif
}

textures/boqu/simplelight
{
	qer_editorimage textures/boqu/simplelight.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/simplelight.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/simplelight.png textures/boqu/simpledark_norm.png textures/boqu/simpledark_gloss.png 
	}
endif
}


textures/boqu/simplemid2
{
	qer_editorimage textures/boqu/simplemid2.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/simplemid2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/simplemid2.png textures/boqu/simpledark2_norm.png textures/boqu/simpledark2_gloss.png 
	}
endif
}
textures/boqu/simplelight2
{
	qer_editorimage textures/boqu/simplelight2.png
	
if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/simplelight2.png
		blendFunc filter
	}
endif

if deluxe
	{
		material textures/boqu/simplelight2.png textures/boqu/simpledark2_norm.png textures/boqu/simpledark2_gloss.png 
	}
endif
}

textures/boqu/simplelight_grate
{
	qer_editorimage textures/boqu/simplelight_grate.png

	{
		material
	}
}

textures/boqu/roundlighttile
{
	qer_editorimage textures/boqu/roundlighttile.png
	q3map_lightimage textures/boqu/roundlighttile_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/roundlighttile.png
		blendFunc filter
	}
	{
		map textures/boqu/roundlighttile_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/roundlighttile 
	}
endif
}


textures/boqu/lamppanel
{
	qer_editorimage textures/boqu/lamppanel.png
	q3map_lightimage textures/boqu/lamppanel_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/lamppanel.png
		blendFunc filter
	}
	{
		map textures/boqu/lamppanel_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/lamppanel
	}
endif
}

textures/boqu/lamppanel2
{
	qer_editorimage textures/boqu/lamppanel2.png
	q3map_lightimage textures/boqu/lamppanel2_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/lamppanel2.png
		blendFunc filter
	}
	{
		map textures/boqu/lamppanel2_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material
	}
endif
}

textures/boqu/lamppanel3
{
	qer_editorimage textures/boqu/lamppanel3.png
	q3map_lightimage textures/boqu/lamppanel3_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/lamppanel3.png
		blendFunc filter
	}
	{
		map textures/boqu/lamppanel3_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material textures/boqu/lamppanel3.png textures/boqu/lamppanel3_norm.png textures/boqu/lamppanel2_gloss.png textures/boqu/lamppanel3_glow.png 
	}
endif
}

textures/boqu/hexa_lamp
{
	qer_editorimage textures/boqu/hexa_lamp.png
	q3map_lightimage textures/boqu/hexa_lamp_glow.png
	q3map_surfacelight 800
  	q3map_lightsubdivide 128
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_forcemeta

if ! deluxe
	{
		map $lightmap
	}
	{
		map textures/boqu/hexa_lamp.png
		blendFunc filter
	}
	{
		map textures/boqu/hexa_lamp_glow.png 
		blendFunc blend
	}	
endif

if deluxe
	{
		material
	}
endif
}
