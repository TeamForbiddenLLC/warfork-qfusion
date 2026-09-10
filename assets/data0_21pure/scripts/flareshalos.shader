
halo
{
	qer_editorimage textures/flareshalos/qer_halo.tga
	qer_trans 0.25
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite2
	nopicmip
	cull none
	softParticle
	
	{
		detail
		clampmap textures/flareshalos/halo.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen wave distanceramp 0 1.0 80 400
	}
}

halo_large
{
	qer_editorimage textures/flareshalos/qer_halo_large.tga
	qer_trans 0.25
	cull none
	surfaceparm nomarks
	surfaceparm trans
	surfaceparm nonsolid
	surfaceparm nolightmap
	deformVertexes autosprite2
	nopicmip
	softParticle

	{
		detail
		clampmap textures/flareshalos/halo_large.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen wave distanceramp 0 1.0 80 400
	}
}

