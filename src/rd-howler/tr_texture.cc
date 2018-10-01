#include "rd-common/tr_common.hh"

#include "tr_local.hh"

void rend::initialize_texture() {
	R_ImageLoader_Init();
	glCreateTextures(GL_TEXTURE_2D, 1, &whiteimage);
	glTextureStorage2D(whiteimage, 1, GL_RGBA8, 1, 1);
	glTextureSubImage2D(whiteimage, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, "\xFF\xFF\xFF\xFF");
}

void rend::destruct_texture() noexcept {
	if (whiteimage) glDeleteTextures(1, &whiteimage);
	for (auto & i : texture_lookup) {
		glDeleteTextures(1, &i.second);
	}
}

GLuint rend::register_texture(char const * name, bool mipmaps) {
	if (!strcmp(name, "*white")) {
		return whiteimage;
	}
	if (!strcmp(name, "$whiteimage")) {
		return whiteimage;
	}
	
	char sname [MAX_QPATH];
	COM_StripExtension(name, sname, MAX_QPATH);
	
	auto find = texture_lookup.find(sname);
	if (find != texture_lookup.end()) return find->second;
	
	byte * idat;
	int width, height;
	
	R_LoadImage( sname, &idat, &width, &height );
	if (!idat) {
		Com_Printf(S_COLOR_RED "ERROR: Failed to load texture '%s'!\n", name);
		texture_lookup[sname] = 0;
		return 0;
	}
	
	GLuint tex;
	glCreateTextures(GL_TEXTURE_2D, 1, &tex);
	glTextureStorage2D(tex, mipmaps ? floor(log2(width > height ? width : height)) : 1, GL_RGBA8, width, height);
	glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idat);
	if (mipmaps) glGenerateTextureMipmap(tex);
	
	texture_lookup[sname] = tex;
	return tex;
};
