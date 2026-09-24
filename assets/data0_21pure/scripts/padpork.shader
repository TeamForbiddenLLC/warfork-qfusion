//================
// SIMPLE
//================
models/players/padpork/simple
{
	nopicmip
	cull front

	{
		celshade $whiteImage env/celldouble models/players/padpork/padpork_diff.png - models/players/padpork/padpork_mask.png - env/celllight
	}
}

//================
// FULLBRIGHT
//================

models/players/padpork/fullbright
{
	nopicmip
	cull front
	nofog

	{
		celshade $whiteImage env/cellbright models/players/padpork/padpork_diff_fb.png - - - env/celllight
		rgbGen entity
	}
}


//================
// DEFAULT
//================

models/players/padpork/default
{
	nopicmip
	cull front
	glossIntensity 0.2
	glossExponent 200
	
	{
		material models/players/padpork/padpork_diff.png models/players/padpork/padpork_norm.png models/players/padpork/padpork_gloss.png - models/players/padpork/padpork_mask.png
	}
}
