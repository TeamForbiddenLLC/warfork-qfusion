textures/hazelh/floorpanels_mirror
{
   qer_editorimage textures/hazelh/floorpanels.png
   q3map_globaltexture

      {
         material textures/hazelh/floorpanels.png textures/hazelh/floorpanels_norm.png textures/hazelh/floorpanels_gloss.png
      }
}

textures/hazelh/wallpanels_dark
{
   qer_editorimage textures/hazelh/wallpanels_dark.png
   q3map_normalimage textures/hazelh/wallpanels_norm.png

      {
         material textures/hazelh/wallpanels_dark.png textures/hazelh/wallpanels_norm.png textures/hazelh/wallpanels_gloss.png
      }
}

textures/hazelh/verticalborder2
{
   qer_editorimage textures/hazelh/verticalborder2.png
   q3map_normali	mage textures/hazelh/verticalborder_norm.png

      {
         material textures/hazelh/verticalborder2.png textures/hazelh/verticalborder_norm.png textures/hazelh/verticalborder_gloss.png
      }
}


textures/hazelh/verticalborder2_whitelight2
{
   qer_editorimage textures/hazelh/verticalborder2_whitelight2.png
   q3map_normalimage textures/hazelh/verticalborder_lights_norm.png

   if deluxe
      {
         material textures/hazelh/verticalborder2_whitelight2.png textures/hazelh/verticalborder_lights_norm.png textures/hazelh/verticalborder_gloss.png textures/hazelh/verticalborder2_whitelight_bright.png 
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/verticalborder2_whitelight2.png
	 blendfunc filter
      }
      {
         map textures/hazelh/verticalborder2_whitelight_bright.png
	 blendfunc add
      }
   endif
}


textures/hazelh/verticalborder2_whitelight
{
   qer_editorimage textures/hazelh/verticalborder2_whitelight.png
   q3map_normalimage textures/hazelh/verticalborder_lights_norm.png

   if deluxe
      {
         material textures/hazelh/verticalborder2_whitelight.png textures/hazelh/verticalborder_lights_norm.png textures/hazelh/verticalborder_gloss.png textures/hazelh/verticalborder2_whitelight_bright.png
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/verticalborder2_whitelight.png
	 blendfunc filter
      }
      {
         map textures/hazelh/verticalborder2_whitelight_bright.png
	 blendfunc add
      }
   endif
}

textures/hazelh/verticalborder_whitelight
{
   qer_editorimage textures/hazelh/verticalborder_whitelight.png
   q3map_normalimage textures/hazelh/verticalborder_lights_norm.png

   if deluxe
      {
         material textures/hazelh/verticalborder_whitelight.png textures/hazelh/verticalborder_lights_norm.png textures/hazelh/verticalborder_gloss.png textures/hazelh/verticalborder_whitelight_bright.png
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/verticalborder_whitelight.png
	 blendfunc filter
      }
      {
         map textures/hazelh/verticalborder_whitelight_bright.png
	 blendfunc add
      }
   endif
}


textures/hazelh/wallborder_orangelight
{
   qer_editorimage textures/hazelh/wallborder_orangelight.png
   q3map_normalimage textures/hazelh/wallborder_light_norm.png

   if deluxe
      {
         material textures/hazelh/wallborder_orangelight.png textures/hazelh/wallborder_light_norm.png textures/hazelh/wallborder_gloss.png textures/hazelh/wallborder_orangelight_bright.png
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/wallborder_orangelight.png
	 blendfunc filter
      }
      {
         map textures/hazelh/wallborder_orangelight_bright.png
	 blendfunc add
      }
   endif
}

textures/hazelh/wallborder_whitelight
{
   qer_editorimage textures/hazelh/wallborder_whitelight.png
   q3map_normalimage textures/hazelh/wallborder_light_norm.png

      {
         material textures/hazelh/wallborder_whitelight.png textures/hazelh/wallborder_light_norm.png textures/hazelh/wallborder_gloss.png
      }
}

textures/hazelh/verticalborder_orangelight
{
   qer_editorimage textures/hazelh/verticalborder_orangelight.png
   q3map_normalimage textures/hazelh/verticalborder_lights_norm.png

   if deluxe
      {
         material textures/hazelh/verticalborder_orangelight.png textures/hazelh/verticalborder_lights_norm.png textures/hazelh/verticalborder_gloss.png textures/hazelh/verticalborder_orangelight_bright.png
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/verticalborder_orangelight.png
	 blendfunc filter
      }
      {
         map textures/hazelh/verticalborder_orangelight_bright.png
	blendfunc add
      }
   endif
}


textures/hazelh/verticalborder2_orangelight
{
   qer_editorimage textures/hazelh/verticalborder2_orangelight.png
   q3map_normalimage textures/hazelh/verticalborder_lights_norm.png

   if deluxe
      {
         material textures/hazelh/verticalborder2_orangelight.png textures/hazelh/verticalborder_lights_norm.png textures/hazelh/verticalborder_gloss.png textures/hazelh/verticalborder2_orangelight_bright.png
      }
   endif

   if ! deluxe
      {
         map $lightmap
      }
      {
         map textures/hazelh/flat/verticalborder2_orangelight.png
	 blendfunc filter
      }
      {
         map textures/hazelh/verticalborder2_orangelight_bright.png
	 blendfunc add
      }
   endif
}


textures/hazelh/base_dblue2_pong
{	
	qer_editorimage textures/hazelh/base_dblue2.png

	q3map_nonplanar
	q3map_shadeangle 89 

	{
		material textures/hazelh/base_dblue2.png
	}
}

textures/hazelh/vertex_base_dblue2
{	
	qer_editorimage textures/hazelh/base_dblue2.png
	surfaceparm nolightmap

	{
		material textures/hazelh/base_dblue2.png
		rgbgen vertex
	}
}

textures/hazelh/base_lightgrey_nonsolid
{	
	qer_editorimage textures/hazelh/base_lightgrey.png
	surfaceparm nonsolid

	{
		material textures/hazelh/base_lightgrey.png
	}
}

textures/hazelh/base_lightgrey_pong
{	
	qer_editorimage textures/hazelh/base_lightgrey.png
	q3map_nonplanar 
	q3map_shadeangle 89 

	{
		material textures/hazelh/base_lightgrey.png
	}
}

textures/hazelh/base_midblue_pong
{	
	qer_editorimage textures/hazelh/base_midblue.png
	q3map_nonplanar
	q3map_shadeangle 89 

	{
		material textures/hazelh/base_midblue.png
	}
}

textures/hazelh/base_dblue_pong
{	
	qer_editorimage textures/hazelh/base_dblue.png
	q3map_nonplanar 
	q3map_shadeangle 89 

	{
		material textures/hazelh/base_dblue.png
	}
}


textures/hazelh/lightstrip
{	
	qer_editorimage textures/hazelh/lightstrip.png
  	//q3map_surfacelight   600
	surfaceparm nomarks
	surfaceparm nolightmap

	{
		material textures/hazelh/lightstrip_transparent.png	
	}
}

textures/hazelh/glow_orange
{	
	nomipmaps
	nopicmip
	qer_editorimage textures/hazelh/glow_orange.png
	cull front
	surfaceparm trans
	surfaceparm nonsolid
//	qer_trans 0.5
	{
		map textures/hazelh/glow_orange.png
		blendfunc add
	}
}

textures/hazelh/glow_orange_circle
{	
	nomipmaps
	nopicmip
	qer_editorimage textures/hazelh/glow_orange_circle.png
	cull front
	surfaceparm trans
	surfaceparm nonsolid
//	qer_trans 0.5
	{
		map textures/hazelh/glow_orange_circle.png
		blendfunc add
	}
}
