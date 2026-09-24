models/players/monada/monada_haut
{
	nopicmip
	cull front

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade models/players/monada/monada_haut.png env/celldouble - - models/players/monada/monada_haut_colorpass.png models/players/monada/monada_haut_stripes.png env/celllight
	}
}

models/players/monada/monada_bas
{
	nopicmip
	cull front

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade models/players/monada/monada_bas.png env/celldouble - - models/players/monada/monada_bas_colorpass.png models/players/monada/monada_bas_stripes.png env/celllight
	}
}

// fullbright

models/players/monada/monada_haut_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/monada/monada_haut_colorpass_fb.png - - - env/celllight
		rgbGen entity
	}
}

models/players/monada/monada_bas_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/monada/monada_bas_colorpass_fb.png - - - env/celllight
		rgbGen entity
	}
}
