
textures/sky/pinksky
{
	qer_editorimage textures/sky/pinksky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyParms - - -

	{
		map textures/sky/pinksky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/pinksky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/pinksky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}

textures/sky/violetsky
{
	qer_editorimage textures/sky/violetsky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyParms - - -

	{
		map textures/sky/violetsky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/violetsky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/violetsky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}

textures/sky/purplesky
{
	qer_editorimage textures/sky/purplesky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyParms - - -

	{
		map textures/sky/purplesky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/purplesky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/purplesky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}


textures/sky/indigosky
{
	qer_editorimage textures/sky/indigosky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyParms - - -

	{
		map textures/sky/indigosky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/indigosky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/indigosky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}

textures/sky/orangesky
{
	qer_editorimage textures/sky/orangesky1.blend.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyParms - - -

	{
		map textures/sky/orangesky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 -0.05
		rgbgen const 0.25 0.25 0.25
	}

	{
		map textures/sky/orangesky1.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.05
		rgbgen const 0.25 0.25 0.275
		blendFunc add
	}

	{
		map textures/sky/orangesky2.blend.png
		tcMod scale 4 4
		tcMod scroll 0 0.045
		rgbgen const 0.25 0.25 0.25
		blendFunc add
	}
}

// MIRAMAR
// high res 1024^2 environment map
// ships as png.
// 
// 
// By Jockum Skoglund aka hipshot
// hipshot@zfight.com
// www.zfight.com
// Stockholm, 2005 08 25
// 
// 
// Modify however you like, just cred me for my work, maybe link to my page.

textures/sky/miramar
{
	qer_editorimage env/miramar/miramar_ft.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight

	skyparms env/miramar/miramar - -
}

//STORMY DAYS
//high res 1024^2 environment map
//ships as png.
//
//
//By Jockum Skoglund aka hipshot
//hipshot@zfight.com
//www.zfight.com
//Stockholm, 2005 08 25
//
//
//Modify however you like, just cred me for my work, maybe link to my page.

textures/sky/stormydays
{
	qer_editorimage env/stormydays/stormydays_ft.png
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_sunExt 1 1 1 100 315 40 3 16
	q3map_lightmapFilterRadius 0 8
	q3map_skyLight 100 3
	
	skyparms env/stormydays/stormydays - -
}
