
textures/sky/stars
{
	qer_editorimage env/36sky_up
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nomarks
	skyparmssides env/36sky rt rt rt rt rt 1500 -
}

textures/sky/autumtrees
{
	qer_editorimage env/autumtrees/autumtrees_up
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nomarks
	skyparms env/autumtrees/autumtrees - -
}

textures/sky/starsky
{
	qer_editorimage env/starsky.png
	surfaceparm sky
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nomarks
	skyparmssides env/starsky rt rt rt env/starsky_up rt - -
}

textures/sky/grimmnight
{
	qer_editorimage env/grimmnight/grimmnight_ft.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight
	skyparms env/grimmnight/grimmnight - -
	{
		map textures/sky/sky_night_front
		blendfunc add
		tcMod scale 2 2 
		tcMod scroll 0.01 0.01
	}
}

textures/sky/grimmnight_static
{
	qer_editorimage env/grimmnight/grimmnight_ft.png
	surfaceparm noimpact
	surfaceparm nomarks
	surfaceparm nolightmap
	surfaceparm sky
	surfaceparm nodlight
	skyparms env/grimmnight/grimmnight 1500 -
}

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
	
	skyparms env/stormydays/stormydays - -
}
