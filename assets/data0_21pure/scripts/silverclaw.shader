models/players/silverclaw/silverclaw
{
	nopicmip
	cull front

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade models/players/silverclaw/silverclaw.png env/celldouble - - models/players/silverclaw/silverclaw_colorpass.png models/players/silverclaw/silverclaw_stripes.png env/celllight
	}
}

// fullbright


models/players/silverclaw/silverclaw_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/silverclaw/silverclaw_colorpass_fb.png - - - env/celllight
		rgbGen entity
	}
}
