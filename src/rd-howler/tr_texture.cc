#include "rd-common/tr_common.hh"

#include "tr_local.hh"

void rend::initialize_texture() {
	R_ImageLoader_Init();
	
	q3texture_ptr whiteimage = std::make_shared<q3texture>(1, 1, false);
	whiteimage->upload(1, 1, "\xFF\xFF\xFF\xFF");
	
	texture_lookup["*white"] = whiteimage;
	texture_lookup["$whiteimage"] = whiteimage;
	texture_lookup["$lightmap"] = whiteimage;
	
	texture_lookup["*invalid"] = nullptr;
}

q3texture_ptr rend::texture_register(char const * name, bool mipmaps) {
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
	
	q3texture_ptr tex = std::make_shared<q3texture>(width, height, mipmaps);
	
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
		byte * pixel = idat + (y * width + x) * 4;
		if (pixel[3] != 0xFF) tex->has_transparency = true;
	}
	
	tex->upload(width, height, idat);
	tex->generate_mipmaps();
	
	Z_Free(idat);
	texture_lookup[name] = tex;
	return tex;
};
