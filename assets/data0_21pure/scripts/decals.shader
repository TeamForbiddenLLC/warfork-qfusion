
textures/decals/graffityPERON32
{	
	qer_editorimage textures/decals/graffityPERON32.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/graffityPERON32.png
		//blendFunc filter
		rgbgen const 0.12 0.12 0.12
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/graffityPERON32_darken
{	
	qer_editorimage textures/decals/graffityPERON32.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/graffityPERON32.png
		blendFunc filter
	}
}

textures/decals/graffityTEAMZISSOU32
{	
	qer_editorimage textures/decals/graffityTEAMZISSOU32.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		clampmap textures/decals/graffityTEAMZISSOU32.png
		rgbgen const 0.15 0.15 0.15
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/graffityTEAMZISSOU32_darken
{	
	qer_editorimage textures/decals/graffityTEAMZISSOU32.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		clampmap map textures/decals/graffityTEAMZISSOU32.png
		blendFunc filter
	}
}
textures/decals/graffiti_01
{	
	qer_editorimage textures/decals/graffiti_01.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/graffiti_01.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/graffiti_02
{	
	qer_editorimage textures/decals/graffiti_02.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/graffiti_02.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/graffiti_03
{	
	qer_editorimage textures/decals/graffiti_03.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/graffiti_03.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/kiki
{	
	qer_editorimage textures/decals/kiki.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/kiki.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}

textures/decals/orangearrows
{	
	qer_editorimage textures/decals/orangearrows.png
	qer_trans 0.6
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		map textures/decals/orangearrows.png
		blendFunc blend
	}
}

textures/decals/orangearrows_outlined
{	
	qer_editorimage textures/decals/orangearrows_outlined.png
	qer_trans 0.6
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		map textures/decals/orangearrows_outlined.png
		blendFunc blend
	}
}

textures/decals/sign_circuit01
{
	qer_editorimage textures/decals/sign_circuit01.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/sign_circuit01.png
		rgbgen const 0.85 0.85 0.85
		blendFunc add
	}
}

textures/decals/sign_circuit02
{
	qer_editorimage textures/decals/sign_circuit02.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/sign_circuit02.png
		rgbgen const 0.85 0.85 0.85
		blendFunc add
	}
}

template_Dirt
{
	{
		detail
		material textures/decals/dirt_$1.png
		blendfunc blend
	}
}

textures/decals/dirt_01
{	
	qer_editorimage textures/decals/dirt_01.png
	qer_trans 0.3
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	template template_Dirt 01
}

textures/decals/dirt_02
{	
	qer_editorimage textures/decals/dirt_02.png
	qer_trans 0.3
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	template template_Dirt 02
}

textures/decals/dirt_scratch_01
{	
	qer_editorimage textures/decals/dirt_scratch_01.png
	qer_trans 0.3
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	template template_Dirt scratch_01
}

textures/decals/dirt_scratch_02
{	
	qer_editorimage textures/decals/dirt_scratch_02.png
	qer_trans 0.3
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	template template_Dirt scratch_02
}

textures/decals/sign_warning01
{	
	qer_editorimage textures/decals/sign_warning01.png
	qer_trans 0.6
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/sign_warning01.png
		blendFunc blend
	}
}

textures/decals/sign_up01
{	
	qer_editorimage textures/decals/sign_up01.png
	qer_trans 0.6
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/sign_up01.png
		blendFunc blend
	}
}


textures/decals/poster01
{	
	qer_editorimage textures/decals/poster01.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster01.png
		blendfunc blend
	}
}

textures/decals/poster_bobot
{	
	qer_editorimage textures/decals/poster_bobot.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster_bobot.png
		blendfunc blend
	}
}

textures/decals/poster_monada
{	
	qer_editorimage textures/decals/poster_monada.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster_monada.png
		blendfunc blend
	}
}

textures/decals/poster_padpork
{	
	qer_editorimage textures/decals/poster_padpork.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster_padpork.png
		blendfunc blend
	}
}

textures/decals/poster_smallbunch
{	
	qer_editorimage textures/decals/poster_smallbunch.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster_smallbunch.png
		blendfunc blend
	}
}

textures/decals/poster_weapons
{	
	qer_editorimage textures/decals/poster_weapons.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/poster_weapons.png
		blendfunc blend
	}
}


textures/decals/posterbigbobot
{	
	qer_editorimage textures/decals/posterbigbobot.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/posterbigbobot.png
		blendfunc blend
	}
}

textures/decals/posterbigpadpork
{	
	qer_editorimage textures/decals/posterbigpadpork.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/posterbigpadpork.png
		blendfunc blend
	}
}

textures/decals/posterbigmonada
{	
	qer_editorimage textures/decals/posterbigmonada.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/posterbigmonada.png
		blendfunc blend
	}
}

textures/decals/posterbigwarsow
{	
	qer_editorimage textures/decals/posterbigwarsow.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/posterbigwarsow.png
		blendfunc blend
	}
}


textures/decals/plasterbroken01
{	
	qer_editorimage textures/decals/plasterbroken01.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip
	offsetMappingScale 4

	{
		detail
		material textures/decals/plasterbroken01.png
		blendfunc blend
	}
}

textures/decals/plasterworn0030
{	
	qer_editorimage textures/decals/plasterworn0030.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip
	q3map_bounceScale 0

	{
		detail
		map textures/decals/plasterworn0030.png
		blendFunc filter
	}
}

textures/decals/leavedead01
{	
	qer_editorimage textures/decals/leavedead01.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/leavedead01.png
		blendfunc blend
	}
}

textures/decals/leavesdead01
{	
	qer_editorimage textures/decals/leavesdead01.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/leavesdead01.png
		blendfunc blend
	}
}

textures/decals/trafficline04
{
	qer_editorimage textures/decals/trafficline04.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04.png
		blendfunc blend
	}
}

textures/decals/trafficline04b
{
	qer_editorimage textures/decals/trafficline04b.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04b.png
		blendfunc blend
	}
}

textures/decals/trafficline04stripes
{
	qer_editorimage textures/decals/trafficline04stripes.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04stripes.png
		blendfunc blend
	}
}

textures/decals/trafficline04stripes2
{
	qer_editorimage textures/decals/trafficline04stripes2.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04stripes2.png
		blendfunc blend
	}
}

textures/decals/trafficline04yellow
{
	qer_editorimage textures/decals/trafficline04yellow.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04yellow.png
		blendfunc blend
	}
}

textures/decals/trafficline04byellow
{
	qer_editorimage textures/decals/trafficline04byellow.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04byellow.png
		blendfunc blend
	}
}

textures/decals/trafficline04yellowstripes
{
	qer_editorimage textures/decals/trafficline04yellowstripes.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04yellowstripes.png
		blendfunc blend
	}
}

textures/decals/trafficline04yellowstripes2
{
	qer_editorimage textures/decals/trafficline04yellowstripes2.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04yellowstripes2.png
		blendfunc blend
	}
}

textures/decals/trafficline04red
{
	qer_editorimage textures/decals/trafficline04red.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04red.png
		blendfunc blend
	}
}

textures/decals/trafficline04redstripes
{
	qer_editorimage textures/decals/trafficline04redstripes.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04redstripes.png
		blendfunc blend
	}
}

textures/decals/trafficline04redstripes2
{
	qer_editorimage textures/decals/trafficline04redstripes2.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04redstripes2.png
		blendfunc blend
	}
}

textures/decals/trafficline04redandwhitestripes2
{
	qer_editorimage textures/decals/trafficline04redandwhitestripes2.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04redandwhitestripes2.png
		blendfunc blend
	}
}


textures/decals/trafficline04orange
{
	qer_editorimage textures/decals/trafficline04orange.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04orange.png
		blendfunc blend
	}
}

textures/decals/trafficline04orangestripes
{
	qer_editorimage textures/decals/trafficline04orangestripes.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04orangestripes.png
		blendfunc blend
	}
}

textures/decals/trafficline04orangestripes2
{
	qer_editorimage textures/decals/trafficline04orangestripes2.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/trafficline04orangestripes2.png
		blendfunc blend
	}
}

textures/decals/evilface_stamp
{
	qer_editorimage textures/decals/evilface_stamp.png
	nopicmip
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	polygonOffset
	qer_trans 1

	{
		detail
		map textures/decals/evilface_stamp.png
		rgbGen identity
		blendfunc add
		rgbgen teamcolor 2
	}
}


textures/decals/b
{
	qer_editorimage textures/decals/b.png
	qer_trans 0.8
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/b.png
		blendfunc blend
	}
}


textures/decals/a
{
	qer_editorimage textures/decals/a.png
	qer_trans 0.8
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/a.png
		blendfunc blend
	}
}

textures/decals/thewall
{	
	qer_editorimage textures/decals/thewall.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/thewall.png
		blendFunc filter
	}
}

textures/decals/0080
{	
	qer_editorimage textures/decals/0080.png
	qer_trans 0.3
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/0080.png
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}




textures/decals/alpha
{	
	qer_editorimage textures/decals/alpha.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/alpha.png
		blendFunc add
		rgbgen teamcolor 2
	}
}


textures/decals/beta
{	
	qer_editorimage textures/decals/beta.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/beta.png
		blendfunc add
		rgbgen teamcolor 3
	}
}


textures/decals/stain001
{	
	qer_editorimage textures/decals/stain001.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/stain001.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only

	}
}

textures/decals/stain002
{	
	qer_editorimage textures/decals/stain002.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/stain002.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only

	}
}

textures/decals/stain003
{	
	qer_editorimage textures/decals/stain003.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/stain003.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only

	}
}

textures/decals/stain004
{	
	qer_editorimage textures/decals/stain004.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/stain004.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only

	}
}

textures/decals/stain005
{	
	qer_editorimage textures/decals/stain005.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/stain005.png
		blendFunc filter
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only

	}
}

//ALPHA//

textures/decals/team_alpha_stain001
{	
	qer_editorimage textures/decals/team_alpha_stain001.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain001.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}

textures/decals/team_alpha_stain002
{	
	qer_editorimage textures/decals/team_alpha_stain002.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain002.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}

textures/decals/team_alpha_stain003
{	
	qer_editorimage textures/decals/team_alpha_stain003.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain003.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}

textures/decals/team_alpha_stain004
{	
	qer_editorimage textures/decals/team_alpha_stain004.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain004.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}

textures/decals/team_alpha_stain005
{	
	qer_editorimage textures/decals/team_alpha_stain005.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain005.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}

textures/decals/team_alpha_stain006
{	
	qer_editorimage textures/decals/team_alpha_stain006.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_alpha_stain006.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 2
		rgbgen teamColorWave 2 sin 0.25 0 0 0

	}
}


//BETA//


textures/decals/team_beta_stain001
{	
	qer_editorimage textures/decals/team_beta_stain001.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain001.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}


textures/decals/team_beta_stain002
{	
	qer_editorimage textures/decals/team_beta_stain002.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain002.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}


textures/decals/team_beta_stain003
{	
	qer_editorimage textures/decals/team_beta_stain003.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain003.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}


textures/decals/team_beta_stain004
{	
	qer_editorimage textures/decals/team_beta_stain004.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain004.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}


textures/decals/team_beta_stain005
{	
	qer_editorimage textures/decals/team_beta_stain005.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain005.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}


textures/decals/team_beta_stain006
{	
	qer_editorimage textures/decals/team_beta_stain006.png
	qer_trans 0.8
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/team_beta_stain006.png
		blendFunc add
		//rgbgen const 0.1 0.1 0.1
		//blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
		rgbgen teamcolor 3
		rgbgen teamColorWave 3 sin 0.25 0 0 0

	}
}

textures/decals/decal_armor_ra
{	
	qer_editorimage textures/decals/decal_armor_ra.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_armor_ra.png
		blendfunc blend
	}
}

textures/decals/decal_armor_ya
{	
	qer_editorimage textures/decals/decal_armor_ya.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_armor_ya.png
		blendfunc blend
	}
}

textures/decals/decal_armor_ga
{	
	qer_editorimage textures/decals/decal_armor_ga.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_armor_ga.png
		blendfunc blend
	}
}

textures/decals/decal_danger
{	
	qer_editorimage textures/decals/decal_danger.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_danger.png
		blendfunc blend
	}
}

textures/decals/decal_dirt1
{	
	qer_editorimage textures/decals/decal_dirt1.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_dirt1.png
		blendfunc blend
	}
}

textures/decals/decal_dirt2
{	
	qer_editorimage textures/decals/decal_dirt2.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_dirt2.png
		blendfunc blend
	}
}


textures/decals/decal_health
{	
	qer_editorimage textures/decals/decal_health.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_health.png
		blendfunc blend
	}
}


textures/decals/decal_telesign_left
{	
	qer_editorimage textures/decals/decal_telesign_left.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_telesign_left.png
	}
}

textures/decals/decal_telesign_right
{	
	qer_editorimage textures/decals/decal_telesign_right.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_telesign_right.png
	}
}

textures/decals/decal_drop
{	
	qer_editorimage textures/decals/decal_drop.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/decal_drop.png
		blendfunc blend
	}
}

textures/decals/wdm17easteregg
{	
	qer_editorimage textures/decals/wdm17easteregg.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/wdm17easteregg.png
		blendfunc blend
	}
}

textures/decals/xmarksthespot_decal
{	
	qer_editorimage textures/decals/xmarksthespot_decal.png
	qer_trans 0.1
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		map textures/decals/xmarksthespot_decal.png
		//blendFunc filter
		rgbgen const 0.4 0.4 0.4
		blendFunc GL_SRC_ALPHA GL_ONE // blendfunc add the alphamasked part only
	}
}





//--------------------------------------------------
//
// INFOCIRCLE: For marking spots. Taken from baxandall jumpads
//
//

textures/decals/infocircle_vc
{
	qer_editorimage textures/decals/infocircle_parts/infocircle.png
	qer_trans 0.5
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_vertexcolor #4e696b
	nopicmip
	polygonOffset
	
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle.png
		rgbgen vertex
		blendfunc add
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_3.png
		blendfunc add
		tcmod rotate 150
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_4.png
		blendfunc add
		tcmod rotate -150
	}
}

textures/decals/infocirclespin_vc
{
	qer_editorimage textures/decals/infocircle_parts/infocircle_2.png
	qer_trans 0.5
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_vertexcolor #4e696b
	nopicmip
	polygonOffset
	
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_2.png
		rgbgen vertex
		blendfunc add
		tcmod rotate 250
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_3.png
		blendfunc add
		tcmod rotate 150
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_4.png
		blendfunc add
		tcmod rotate -150
	}
}

textures/decals/infocircle_blend_vc
{
	qer_editorimage textures/decals/infocircle_parts/infocircle.png
	qer_trans 0.5
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_vertexcolor #4e696b
	nopicmip
	polygonOffset
	
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle.png
		rgbgen vertex
		blendFunc blend
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_3.png
		blendFunc GL_SRC_ALPHA GL_ONE
		tcmod rotate 150
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_4.png
		blendFunc GL_SRC_ALPHA GL_ONE
		tcmod rotate -150
	}
}

textures/decals/infocirclespin_blend_vc
{
	qer_editorimage textures/decals/infocircle_parts/infocircle_2.png
	qer_trans 0.5
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	surfaceparm nomarks
	surfaceparm nodlight
	q3map_vertexcolor #4e696b
	nopicmip
	polygonOffset
	
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_2.png
		rgbgen vertex
		blendFunc blend
		tcmod rotate 250
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_3.png
		blendFunc GL_SRC_ALPHA GL_ONE
		tcmod rotate 150
	}
	{
		detail
		clampmap textures/decals/infocircle_parts/infocircle_4.png
		blendFunc GL_SRC_ALPHA GL_ONE
		tcmod rotate -150
	}
}




//--------------------------------------------------
//
// OLD ONES MOVED INTO DECALS
//
//

textures/decals/01
{	
	qer_editorimage textures/scifi_interior/01.png
	qer_trans 0.6
	q3map vertexcolor 1.0 1.0 1.0
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/01.png textures/flatnorm.png
		alphagen const 0.5
		rgbgen vertex
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}

textures/decals/02
{	
	qer_editorimage textures/scifi_interior/02.png
	qer_trans 0.6
	q3map vertexcolor 1.0 1.0 1.0
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/02.png textures/flatnorm.png
		alphagen const 0.5
		rgbgen vertex
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}

textures/decals/03
{	
	qer_editorimage textures/scifi_interior/03.png
	qer_trans 0.6
	q3map vertexcolor 1.0 1.0 1.0
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/03.png textures/flatnorm.png
		rgbgen vertex
		alphagen const 0.5
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}


textures/decals/covenant1
{	
	qer_editorimage textures/scifi_interior/covenant1.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/covenant1.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/covenant2
{	
	qer_editorimage textures/scifi_interior/covenant2.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/covenant2.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_arrow_orange
{	
	qer_editorimage textures/scifi_interior/decal_arrow_orange.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_arrow_orange.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_comptext1
{	
	qer_editorimage textures/scifi_interior/decal_comptext1.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext1.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_comptext2
{	
	qer_editorimage textures/scifi_interior/decal_comptext2.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext2.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_comptext3
{	
	qer_editorimage textures/scifi_interior/decal_comptext3.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_comptext3.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_up_grey
{	
	qer_editorimage textures/scifi_interior/decal_up_grey.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_up_grey.png textures/flatnorm.png
		blendfunc blend
	}
}

textures/decals/decal_up_orange
{	
	qer_editorimage textures/scifi_interior/decal_up_orange.png
	qer_trans 0.6
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/scifi_interior/decal_up_orange.png textures/flatnorm.png
		blendfunc blend
	}
}


textures/decals/out_of_service
{	
	qer_editorimage textures/factory/out_of_service.png
	qer_trans 0.8
	surfaceparm nomarks
	//surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/factory/out_of_service.png textures/flatnorm.png
		blendfunc blend
	}
}



//--------------------------------------------------
//
//    FONTS
//	Notice: These fonts are for makebsp to genate
//		decals directly from text.
//

textures/decals/fonts/PixelTwist
{
	qer_editorimage textures/decals/fonts/PixelTwist
	qer_trans 0.5
	q3map_vertexcolor 255 255 255
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/fonts/PixelTwist.png textures/flatnorm.tga
		rgbgen vertex
		alphagen vertex
		blendfunc blend
	}
}

textures/decals/fonts/Symtext
{
	qer_editorimage textures/decals/fonts/Symtext
	qer_trans 0.5
	q3map_vertexcolor 255 255 255
	surfaceparm nomarks
	surfaceparm nodlight
	surfaceparm nonsolid
	surfaceparm trans
	polygonOffset
	nopicmip

	{
		detail
		material textures/decals/fonts/Symtext.png textures/flatnorm.tga
		rgbgen vertex
		alphagen vertex
		blendfunc blend
	}
}
