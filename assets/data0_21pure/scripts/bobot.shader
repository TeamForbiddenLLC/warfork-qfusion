//================
// SIMPLE
//================

models/players/bobot/bobot_head_simple
{
	nopicmip
	cull front

	{
		celshade $whiteImage env/celldouble models/players/bobot/u_head_def.png models/players/bobot/u_head_eye.png models/players/bobot/u_head_color.png - env/celllight
	}
}

models/players/bobot/bobot_torso_simple
{
	nopicmip
	cull front

	{
		celshade $whiteImage env/celldouble models/players/bobot/u_torso_def.png models/players/bobot/u_torso_backlights.png models/players/bobot/u_torso_color.png - env/celllight
	}
}

models/players/bobot/bobot_legs_simple
{
	nopicmip
	cull front

	{
		celshade $whiteImage env/celldouble models/players/bobot/u_legs_def.png - models/players/bobot/u_legs_color.png - env/celllight
	}
}

//================
// FULLBRIGHT
//================

models/players/bobot/bobot_head_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/bobot/u_head_def.png - - - env/celllight
		rgbGen entity
	}
}

models/players/bobot/bobot_torso_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/bobot/u_torso_def.png - - - env/celllight
		rgbGen entity
	}
}

models/players/bobot/bobot_legs_fb
{
	nopicmip
	cull front
	nofog

	{
		// <base> <celshade> [diffuse] [decal] [entitydecal] [stripes] [celllight]
		celshade $whiteImage env/cellbright models/players/bobot/u_legs_def.png - - - env/celllight
		rgbGen entity
	}
}

//================
// DEFAULT
//================

models/players/bobot/bobot_head
{
	nopicmip
	cull front
	glossIntensity 0.5
	glossExponent 10
	
	{
		material models/players/bobot/u_head_def.png models/players/bobot/u_head_norm.png models/players/bobot/u_head_gloss.png models/players/bobot/u_head_eye.png models/players/bobot/u_head_color.png
	}
}

models/players/bobot/bobot_torso
{
	nopicmip
	cull front
	glossIntensity 1
	glossExponent 100

	{
		material models/players/bobot/u_torso_def.png models/players/bobot/u_torso_norm.png models/players/bobot/u_torso_gloss.png models/players/bobot/u_torso_backlights.png models/players/bobot/u_torso_color.png
	}
}

models/players/bobot/bobot_legs
{
	nopicmip
	cull front
	glossIntensity 1
	glossExponent 100

	{
		material models/players/bobot/u_legs_def.png models/players/bobot/u_legs_norm.png models/players/bobot/u_legs_gloss.png - models/players/bobot/u_legs_color.png
	}
}
