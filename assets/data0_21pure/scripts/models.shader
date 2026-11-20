//ARMORS
models/items/armor/armor_template
{
	nopicmip
	cull front
	
	{
		celshade $1 env/cell
		alphagen entity
	}
}

models/items/armor/shard/shard
{
	template models/items/armor/armor_template models/items/armor/shard/shard.png
}

models/items/armor/ga/ga
{
	template models/items/armor/armor_template models/items/armor/ga/ga.png
}

models/items/armor/ya/ya
{
	template models/items/armor/armor_template models/items/armor/ya/ya.png
}

models/items/armor/ra/ra
{
	template models/items/armor/armor_template models/items/armor/ra/ra.png
}

//----------------------
//AMMO BOXES
//----------------------

//AMMO BOXES
models/items/ammo/ammobox/ammobox
{
	nopicmip
	cull front

	{
		celshade models/items/ammo/ammobox/ammobox.png env/cell - - models/items/ammo/ammobox/ammobox_color.png
		alphagen entity
	}
}

models/items/ammo/pack/ammopack
{
	nopicmip
	cull front

	{
		celshade models/items/ammo/pack/ammopack.png env/cell
	}
}

//-----------------------------------------------------------


//------------
//WEAPONS
//------------

models/weapons/electrobolt/electrobolt
{
	nopicmip
	cull front

	{
		material models/weapons/electrobolt/electrobolt.png
		rgbgen entity
		alphagen entity
	}
}

models/weapons/electrobolt/electrobolt_fx
{
	nopicmip
	cull disable
	{
		map models/weapons/electrobolt/electrobolt_fx.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.5
	}
}
models/weapons/electrobolt/electrobolt_fx_1
{
	nopicmip
	cull disable
	//deformVertexes autosprite2
	{
		animmap 15 models/weapons/electrobolt/electrobolt_fx_1.png models/weapons/electrobolt/electrobolt_fx_2.png models/weapons/electrobolt/electrobolt_fx_3.png models/weapons/electrobolt/electrobolt_fx_4.png models/weapons/electrobolt/electrobolt_fx_5.png models/weapons/electrobolt/electrobolt_fx_6.png models/weapons/electrobolt/electrobolt_fx_7.png models/weapons/electrobolt/electrobolt_fx_8.png
		blendfunc add
	}
}

models/weapons/glauncher/glauncher
{
	nopicmip
	cull front

	{
		material models/weapons/glauncher/glauncher.png models/weapons/glauncher/glauncher_norm.png models/weapons/glauncher/glauncher_gloss.png
		rgbgen entity
	}
}

models/weapons/glauncher/glauncher_fx
{
	nopicmip
	cull disable
	{
		map models/weapons/glauncher/glauncher_fx.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.5
	}
}

models/weapons/plasmagun/plasmagun
{
	nopicmip
	cull front

	{
		material models/weapons/plasmagun/plasmagun.png
		rgbgen entity
	}
}

models/weapons/plasmagun/plasmagun_fx
{
	nopicmip
	cull disable
	{
		map models/weapons/plasmagun/plasmagun_fx.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.5
	}
}

models/weapons/riotgun/riotgun
{
	nopicmip
	cull front

	{
		material models/weapons/riotgun/riotgun.png
		rgbgen entity
	}
}

models/weapons/gunblade/gunblade
{
	nopicmip
	cull front
	
	{
		material models/weapons/gunblade/gunblade.png
		rgbgen entity
	}
}


models/weapons/gunblade/barrel
{
	nopicmip
	cull front
	
	{
		material models/weapons/gunblade/barrel.png
		rgbgen entity
	}
}

models/weapons/machinegun/machinegun
{
	nopicmip
	cull front

	{
		material models/weapons/machinegun/machinegun.png
		rgbgen entity
	}
}

models/weapons/rlauncher/rlauncher
{
	nopicmip
	cull front
	glossExponent 50
	glossIntensity 2.0

	{
		material models/weapons/rlauncher/rlauncher
		rgbgen entity
	}
}

models/weapons/rlauncher/rlauncher_fx
{
	nopicmip
	cull disable
	{
		map models/weapons/rlauncher/rlauncher_fx.png
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 0.5 0.5 0 0.5
	}
}
models/weapons/lasergun/lasergun
{
	nopicmip
	cull front

	{
		material models/weapons/lasergun/lasergun.png
		rgbgen entity
	}
}

models/weapons/lasergun/lasergun_fx
{
	nopicmip
	cull disable
	{
		map models/weapons/lasergun/lasergun_fx.png
		blendFunc GL_ONE GL_ONE
		tcmod scroll -2 0
	}
	{
		map models/weapons/lasergun/lasergun_fx.png
		blendFunc GL_ONE GL_ONE
		tcmod scroll 2 0
	}
}

models/weapons/instagun/instagun
{
	nopicmip
	cull front

	{
		material models/weapons/instagun/instagun.png models/weapons/instagun/instagun_norm.png
		rgbgen entity
	}
}

models/weapons/instagun/instabarrel
{
	nopicmip
	cull front

	{
		material models/weapons/instagun/instabarrel.png models/weapons/instagun/instabarrel_norm.png
		rgbgen entity
	}
}

models/v_weapons/instagun/instalaser
{
	nopicmip
	cull none
	
	{
		map models/weapons/instagun/instalaser.png
		blendFunc blend
		alphaGen const 0.3
	}

}


//---------------------
//WEAPON PROJECTILES
//---------------------

models/objects/projectile/plasmagun/plnew
{
	nopicmip
 	//sort additive
 	cull disable
 	{
 		map models/objects/projectile/plasmagun/plnew.png
 		blendFunc add
		//blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcmod rotate -150
 	}
}

models/objects/projectile/gunblade/proj_gunblade
{
	nopicmip
	//deformVertexes autosprite
	cull none
	{
		map models/objects/projectile/gunblade/proj_gunblade.png
		blendFunc add
		tcmod rotate 90
	}
}

models/objects/projectile/gunblade/proj_gunblade_ring
{
	nopicmip
	//deformVertexes autosprite
	cull none
	{
		map models/objects/projectile/gunblade/proj_gunblade_ring.png
		blendFunc add
	}
}

models/objects/projectile/glauncher/grenade
{
	nopicmip

	{
		celshade models/objects/projectile/glauncher/grenade.png env/cell
	}

	// pulse
	{
		map models/objects/projectile/glauncher/grenade_pulse.png
		rgbgen wave Square 0 1 0 2.5
		blendfunc add
	}
}

models/objects/projectile/glauncher/grenadegradstrong
{
	nopicmip
	cull disable
	{
		map models/objects/projectile/glauncher/grenadegradstrong.png
		blendFunc add
		rgbGen wave triangle .07 .1 0 5
		tcmod scroll 0.2 0
	}
}

models/objects/projectile/glauncher/grenadegradweak
{
	nopicmip
	cull disable
	{
		map models/objects/projectile/glauncher/grenadegradweak.png
		blendFunc add
		rgbGen wave triangle .07 .1 0 20
		tcmod scroll 0.2 0
	}
}

models/objects/projectile/glauncher/grenadeweak
{
	nopicmip
	{
		map models/objects/projectile/glauncher/grenadeweak.png
	}
	// just fullbright
	//{
	//	map gfx/colors/celshade.png
	//	blendfunc filter
	//	rgbGen identity
	//	tcGen environment
	//}
}

models/objects/projectile/glauncher/grenadeweak_flare
{
	nopicmip
	//deformVertexes autosprite
	cull none
	{
		map models/objects/projectile/glauncher/grenadeweak_flare.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

models/objects/projectile/glauncher/grenadestrong
{
	nopicmip
	{
		map models/objects/projectile/glauncher/grenadestrong.png
	}
	// just fullbright
	//{
	//	map gfx/colors/celshade.png
	//	blendfunc filter
	//	rgbGen identity
	//	tcGen environment
	//}
}

models/objects/projectile/glauncher/grenadestrong_flare
{
	nopicmip
	//deformVertexes autosprite
	cull none
	{
		map models/objects/projectile/glauncher/grenadestrong_flare.png
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

models/objects/projectile/rlauncher/rocket_strong
{
	nopicmip
	sort additive
	cull disable
	{
		map models/objects/projectile/rlauncher/rocket_strong.png
		blendFunc add
		rgbGen wave triangle .2 .25 0 5
		tcmod rotate -250
		//blendFunc GL_SRC_ALPHA GL_ONE
	}
}

models/objects/projectile/rlauncher/rocket_weak
{
	nopicmip
	sort additive
	cull disable
	{
		map models/objects/projectile/rlauncher/rocket_weak.png
		blendFunc add
		rgbGen wave triangle .2 .25 0 5
		tcmod rotate -250
		//blendFunc GL_SRC_ALPHA GL_ONE
	}
}

models/objects/projectile/rlauncher/rocket_flare_2
{
	nopicmip
	sort additive
	cull disable
	{
		map models/objects/projectile/rlauncher/rocket_flare_2.png
		blendFunc add
		rgbGen wave triangle .1 .1 0 1
		tcmod scroll 3.2 0
	}
}

models/objects/projectile/rlauncher/rocketgradstrong
{
	nopicmip
	cull disable
	{
		map models/objects/projectile/rlauncher/rocketgradstrong.png
		blendFunc add
		rgbGen wave triangle .01 .15 0 20
		tcmod scroll 0.2 0
	}
}

models/objects/projectile/rlauncher/rocketgradweak
{
	nopicmip
	cull disable
	{
		map models/objects/projectile/rlauncher/rocketgradweak.png
		blendFunc add
		rgbGen wave triangle .01 .15 0 20
		tcmod scroll 0.2 0
	}
}

models/objects/projectile/lasergun/laserbeam
{
	nopicmip
	nomipmaps
	cull none
	deformVertexes autosprite2
	{
		map models/objects/projectile/lasergun/laserbeam.png
		blendFunc add
		tcMod scroll 6 0
	}
}

//-----------------------------------------

//-----------------
//FLASH WEAPONS
//-----------------

//models/v_weapons/electrobolt/f_electro
//models/v_weapons/plasmagun/f_plasma
//models/v_weapons/lasergun/f_laser
//models/v_weapons/rlauncher/f_rlaunch
//models/v_weapons/glauncher/f_glaunch
//models/v_weapons/riotgun/f_riot

models/v_weapons/generic/f_generic
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/generic/f_generic.png
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/weapons/plasmagun/f_plasma
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/plasmagun/f_plasma.png
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}
models/weapons/plasmagun/f_plasma_2
{
	nopicmip
	sort nearest
	cull disable
	//deformVertexes autosprite2
	softParticle
	{
		map models/weapons/plasmagun/f_plasma_2.png
		rgbgen entity
		blendFunc add
	}
}

models/weapons/glauncher/f_glaunch
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/glauncher/f_glaunch.png
		rgbgen entity
		blendFunc add
	}
	{
		map models/weapons/glauncher/f_glaunch_spark.png
		rgbgen entity
		tcMod stretch sawtooth .65 .3 0 8 
		blendFunc add
	}
}
models/weapons/glauncher/f_glaunch_2
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/glauncher/f_glaunch_2.png
		rgbgen entity
		blendfunc add
	}
}
models/weapons/glauncher/f_glaunch_3
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/glauncher/f_glaunch_2.png
		rgbgen entity
		blendfunc add
	}
}
models/weapons/riotgun/f_riot
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/riotgun/f_riot.png
		rgbgen entity
		blendfunc add
	}
	{
		map models/weapons/riotgun/f_riot_spark.png
		rgbgen entity
		tcMod stretch sawtooth .65 .3 0 8 
		blendFunc add
	}
}
models/weapons/riotgun/f_riot_2
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/riotgun/f_riot_2.png
		rgbgen entity
		blendfunc add
	}
}
models/weapons/riotgun/f_riot_3
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/riotgun/f_riot_3.png
		rgbgen entity
		blendfunc add
	}
}
models/weapons/gunblade/f_gunblade
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/gunblade/f_gunblade.png
	rgbgen entity
	tcmod rotate 200
	blendFunc add
	}
	{
	map models/weapons/gunblade/f_gunblade_1.png
	rgbgen entity
	tcmod rotate -175
	blendFunc add
	}
}
models/weapons/gunblade/f_gunblade_2
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		map models/weapons/gunblade/f_gunblade_2.png
		rgbgen entity
		blendfunc add
	}
}

models/weapons/rlauncher/f_rlaunch
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/rlauncher/f_rlaunch.png
	rgbgen entity
	//tcmod rotate 90
	blendFunc add
	}
}

models/weapons/rlauncher/f_rlaunch_2
{
	nopicmip
	sort nearest
	cull disable
	//deformVertexes autosprite2
	softParticle
	{
		map models/weapons/rlauncher/f_rlaunch_2.png
		rgbgen entity
		blendFunc add
	}
}

models/weapons/electrobolt/f_electrobolt
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/electrobolt/f_electro.png
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/weapons/electrobolt/f_electrobolt_2
{
	nopicmip
	sort nearest
	cull disable
	//deformVertexes autosprite2
	softParticle
	{
		map models/weapons/electrobolt/f_electro_2.png
		rgbgen entity
		blendFunc add
	}
}
models/weapons/electrobolt/f_electrobolt_3
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
		animMap 6 models/weapons/electrobolt/f_electro_3.png models/weapons/electrobolt/f_electro_4.png models/weapons/electrobolt/f_electro_5.png models/weapons/electrobolt/f_electro_6.png
		rgbgen entity
		blendfunc add
	}
}

models/weapons/lasergun/f_laser
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/lasergun/f_laser.png
	rgbgen entity
	tcmod rotate 180
	blendFunc add
	}
}
models/weapons/lasergun/f_laser_2
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/lasergun/f_laser_2.png
	rgbgen entity
	tcmod scroll 0 3
	blendFunc add
	}
}

models/weapons/instagun/f_instag
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/instagun/f_instag.png
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}
models/weapons/instagun/f_instag_2
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/instagun/f_instag_2.png
	rgbgen entity
	blendFunc add
	}
}
models/weapons/instagun/f_instag_3
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	{
	map models/weapons/instagun/f_instag.png
	rgbgen entity
	tcmod rotate 90
	blendFunc add
	}
}

models/weapons/machinegun/machinegun_flash
{
	nopicmip
	sort nearest
	cull disable
	softParticle
	//deformVertexes wave 20 noise 5 5 0 15
	{
		animMap 6 models/weapons/machinegun/machinegun_flash_1.png models/weapons/machinegun/machinegun_flash_2.png models/weapons/machinegun/machinegun_flash_3.png models/weapons/machinegun/machinegun_flash_4.png
		rgbgen entity
		blendfunc add
	}
}


//HEALTH


models/items/health/small/torus
{
	nopicmip
	cull disable
	deformVertexes wave 100 sin 0.5 2 0 1.5
	{
		map models/items/health/small/torus
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0.5 0
	}
}

models/items/health/small/ball
{
	nopicmip
	{
		map models/items/health/small/ball2
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin 0.7 0.3 0 0.25
	}
}

models/items/health/medium/torus
{
	nopicmip
	cull disable
	deformVertexes wave 100 sin 0.5 2 0 1.5
	{
		map models/items/health/medium/torus
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0.5 0
	}
}

models/items/health/medium/ball
{
	nopicmip
	{
		map models/items/health/medium/ball2
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin 0.7 0.3 0 0.25
	}
}

models/items/health/large/torus
{
	nopicmip
	cull disable
	deformVertexes wave 100 sin 0.5 2 0 1.5
	{
		map models/items/health/large/torus
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0.5 0
	}
}

models/items/health/large/ball
{
	nopicmip
	{
		map models/items/health/large/ball2
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin 0.7 0.3 0 0.25
	}
}


models/items/health/mega/torus
{
	nopicmip
	deformVertexes wave 100 sin 0.5 2 0 1.5
	{
		map models/items/health/mega/torus
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0.5 0
	}
}

models/items/health/mega/ball
{
	nopicmip
	{
		map models/items/health/mega/ball3
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0 0.5
	}
	{
		map models/items/health/mega/ball2
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin 0.5 1 0 0.25
	}
}

models/items/health/ultra/torus
{
	nopicmip
	deformVertexes wave 100 sin 0.5 2 0 1.5
	{
		map models/items/health/ultra/torus
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0.5 0
	}
}

models/items/health/ultra/ball
{
	nopicmip
	{
		map models/items/health/ultra/ball3
		blendfunc GL_ONE GL_ONE
		tcmod scroll 0 0.5
	}
	{
		map models/items/health/ultra/ball2
		blendfunc GL_ONE GL_ONE
		rgbgen wave sin 0.5 1 0 0.25
	}
}

//BONUS

models/powerups/instant/quad_light
{
	nopicmip
	cull disable
	{
		map models/powerups/instant/quad_light.png
		rgbgen wave sin 0.5 1 0 3
		blendfunc GL_ONE GL_ONE_MINUS_SRC_COLOR
	}
	{
		map models/powerups/instant/quad_halo.png
		blendfunc add
		tcMod rotate 90
		//tcMod stretch sin 1.25 0.05 0 .4
		tcMod stretch sin 1.2 0.05 0 .7
		
	}
		
}


models/powerups/instant/regen_torus
{
	nopicmip
	deformVertexes wave 100 sin 0.5 2 0 1.5

	{
		map models/powerups/instant/regen_torus
		blendfunc add
		tcmod scroll 0.5 0
	}
}

models/powerups/instant/cross
{
	nopicmip

	{
		map models/powerups/instant/cross2
		blendfunc blend
		tcmod scroll 0 0.5
	}
	{
		map models/powerups/instant/cross3
		blendfunc add
		tcmod scroll 0 0.5
	}
}
models/powerups/instant/quad
{
	nopicmip
	cull front
	qer_editorimage models/powerups/instant/quad.png

	{
		celshade models/powerups/instant/quad.png env/cell
	}
}


models/powerups/instant/warshell_belt
{
	nopicmip
	cull front

	{
		celshade models/powerups/instant/warshell.png env/cell
	}
}

models/powerups/instant/warshell_sphere
{
	nopicmip
	cull none
	{
		map models/powerups/instant/warshell_sphere.png
		//rgbgen wave sin 0.5 1 0 3
		blendfunc blend	
		tcMod rotate 90
	}
}
models/powerups/instant/warshell_sprite
{
	nopicmip
	cull none
	deformvertexes autosprite
	{
		animmap 15 models/powerups/instant/warshell_sprite_0035.png models/powerups/instant/warshell_sprite_0036.png models/powerups/instant/warshell_sprite_0037.png models/powerups/instant/warshell_sprite_0038.png models/powerups/instant/warshell_sprite_0039.png models/powerups/instant/warshell_sprite_0038.png models/powerups/instant/warshell_sprite_0037.png models/powerups/instant/warshell_sprite_0036.png
		blendfunc blend
		tcMod rotate 90
	}
}



// Flag

models/objects/flag/flag_base
{
	nopicmip
	cull front

	{
		celshade models/objects/flag/flag_base.png env/cell - - models/objects/flag/flag_base_colorpass.png
	}
}

models/objects/flag/flag_pole
{
	nopicmip
	cull front

	{
		celshade models/objects/flag/flag_pole.png env/cell - - models/objects/flag/flag_pole_colorpass.png
	}
}

models/objects/flag/flag_light
{
	nopicmip
	cull none
	
	{
		map models/objects/flag/flag_light.png
		blendFunc blend
		alphaGen const 0.8
	}
}

models/objects/flag/Flag_holo
{
	nopicmip
	cull none
	deformVertexes wave 20 noise 0.5 0.6 0 1.6
	
	{
		map models/objects/flag/flag_holo.png
		blendFunc blend
		alphaGen const 0.6
		tcMod scroll 0.8 0.8
		depthWrite
	}

	{
		map models/objects/flag/flag_holo.png
		blendFunc blend
		alphaGen const 0.4
		tcMod scroll 0.4 0.4
	}
}

//------------------------------
// TDO Capture Area Markers
//------------------------------

models/objects/capture_point_template
{
	nopicmip
	cull front
	surfaceparm trans

	{
		map models/objects/capture_$1/$2.png
		alphaGen const 0.4
		rgbGen entity
		blendFunc blend
	}

	{
		map models/objects/capture_$1/$2_alpha.png
		blendFunc blend
	}

	if textureCubeMap	//for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		blendfunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.png
		blendfunc filter
		tcGen environment
	}
	endif

	{
		map models/objects/capture_$1/$2_color.png
		rgbGen entity
		blendFunc blend
	}
}

models/objects/capture_area/indicator
{
	template models/objects/capture_point_template area indicator
}

models/objects/capture_a/capture_a
{
	template models/objects/capture_indicator_template a capture_a
}

models/objects/capture_b/capture_b
{
	template models/objects/capture_indicator_template b capture_b
}

models/objects/capture_c/capture_c
{
	template models/objects/capture_indicator_template c capture_c
}

models/objects/capture_d/capture_d
{
	template models/objects/capture_indicator_template d capture_d
}

//----------------------
//ILLUMINATI GIBS
//----------------------
models/objects/gibs/illuminati/illuminati
{
	nopicmip
	cull front
	
	{
		celshade *black env/cell - - models/objects/gibs/illuminati/illuminati1.png
	}
}

// DEBRIS
models/objects/debris/debris_template
{
	nopicmip
	cull front

	{
		map models/objects/debris/debris$1.png
	}

	if textureCubeMap	//for 3d cards supporting cubemaps
	{
		shadecubemap env/cell
		blendFunc filter
	}
	endif

	if ! textureCubeMap //for 3d cards not supporting cubemaps
	{
		map gfx/colors/celshade.png
		blendfunc filter
		tcGen environment
	}
	endif
}

models/objects/debris/debris1
{
	template models/objects/debris/debris_template 1
}

models/objects/debris/debris2
{
	template models/objects/debris/debris_template 2
}

models/objects/debris/debris3
{
	template models/objects/debris/debris_template 3
}

models/objects/debris/debris4
{
	template models/objects/debris/debris_template 4
}

models/objects/debris/debris5
{
	template models/objects/debris/debris_template 5
}

models/objects/debris/debris6
{
	template models/objects/debris/debris_template 6
}

