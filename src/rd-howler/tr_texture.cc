#include "rd-common/tr_common.hh"

#include "tr_local.hh"

void rend::initialize_texture() {
	R_ImageLoader_Init();
	std::shared_ptr<q3texture> whiteimage = std::make_shared<q3texture>();
	glCreateTextures(GL_TEXTURE_2D, 1, &whiteimage->id);
	glTextureStorage2D(whiteimage->id, 1, GL_RGBA8, 1, 1);
	glTextureSubImage2D(whiteimage->id, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, "\xFF\xFF\xFF\xFF");
	texture_lookup["*white"] = whiteimage;
	texture_lookup["$whiteimage"] = whiteimage;
	texture_lookup["$lightmap"] = whiteimage;
	
	texture_lookup["*invalid"] = std::make_shared<q3texture>();
}

std::shared_ptr<q3texture const> rend::texture_register(char const * name, bool mipmaps) {
	char sname [MAX_QPATH];
	//COM_StripExtension(name, sname, MAX_QPATH);
	
	auto find = texture_lookup.find(name);
	if (find != texture_lookup.end()) return find->second;
	
	byte * idat;
	int width, height;
	
	R_LoadImage( name, &idat, &width, &height );
	if (!idat) {
		Com_Printf(S_COLOR_RED "ERROR: Failed to load texture '%s'!\n", name);
		texture_lookup[name] = 0;
		return texture_lookup["*invalid"];
	}
	
	std::shared_ptr<q3texture> tex = std::make_shared<q3texture>();
	
	for (size_t x = 0; x < width; x++) for (size_t y = 0; y < height; y++) {
		byte * pixel = idat + (y * width + x) * 4;
		if (pixel[3] != 0xFF) tex->has_transparency = true;
	}
	
	glCreateTextures(GL_TEXTURE_2D, 1, &tex->id);
	glTextureStorage2D(tex->id, mipmaps ? floor(log2(width > height ? width : height)) : 1, GL_RGBA8, width, height);
	glTextureSubImage2D(tex->id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, idat);
	if (mipmaps) glGenerateTextureMipmap(tex->id);
	
	Z_Free(idat);
	texture_lookup[name] = tex;
	return tex;
};
