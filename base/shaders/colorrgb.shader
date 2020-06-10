textures/colors/m_rgb
{
	qer_editorimage	textures/editor/m_white
	q3map_nolightmap
	
	{
		map $whiteimage
		rgbGen entity
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
