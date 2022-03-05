models/dogijk/daki
{
	q3map_nolightmap
	
	{
		EntityMap $models/dogijk/daki
	}
	{
		map $whiteimage
		rgbGen lightingDiffuse
		blendFunc GL_DST_COLOR GL_ZERO
	}
}

models/dogijk/eggshad
{
	qer_editorimage	textures/editor/m_white
	q3map_nolightmap
	
	{
		map $whiteimage
		rgbGen entity
	}
	{
		EntityMap $models/dogijk/eggmask
		rgbGen oneMinusEntity
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $whiteimage
		rgbGen lightingDiffuse
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map $whiteimage
		rgbGen lightingDiffuse
		alphaGen lightingSpecular
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}

models/dogijk/pbox_image
{
	{
		EntityMap $models/dogijk/pbox
		blendFunc GL_ONE GL_ZERO
	}
}

models/dogijk/pbox_rimlight
{	
	{
		map $whiteimage
		rgbGen entity
	}
	{
		map $whiteimage
		rgbGen wave sin 0.8 0.2 0 4
		blendFunc GL_DST_COLOR GL_ZERO
	}
}
