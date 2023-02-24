#include "hw_local.hh"
using namespace howler;

// ================================
// Q3TEXTURE
// ================================

q3texture::q3texture(GLsizei width, GLsizei height, GLsizei depth, bool mipmaps, GLenum type) : m_width(width), m_height(height), m_depth(depth), m_mips(mipmaps) {
	if (depth == 1) {
		glCreateTextures(GL_TEXTURE_2D, 1, &m_handle);
		glTextureStorage2D(m_handle, m_mips ? std::floor(log2(m_width > m_height ? m_height : m_width)) : 1, type, m_width, m_height);
	} else {
		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_handle);
		glTextureStorage3D(m_handle, m_mips ? std::floor(log2(m_width > m_height ? m_height : m_width)) : 1, type, m_width, m_height, m_depth);
	}
}

q3texture::~q3texture() {
	if (m_handle) glDeleteTextures(1, &m_handle);
}

void q3texture::upload2D(GLsizei width, GLsizei height, void const * data, GLenum format, GLenum data_type, GLint xoffs, GLint yoffs) {
	glTextureSubImage2D(m_handle, 0, xoffs, yoffs, width, height, format, data_type, data);
}

void q3texture::upload3D(GLsizei width, GLsizei height, GLsizei depth, void const * data, GLenum format, GLenum data_type, GLint xoffs, GLint yoffs, GLint zoffs) {
	glTextureSubImage3D(m_handle, 0, xoffs, yoffs, zoffs, width, height, depth, format, data_type, data);
}

void q3texture::clear() {
	glClearTexImage(m_handle, 0, GL_RGBA, GL_UNSIGNED_BYTE, "\0\0\0\0");
}

void q3texture::generate_mipmaps() {
	if (m_mips) glGenerateTextureMipmap(m_handle);
}

void q3texture::bind(GLuint binding) const {
	glBindTextureUnit(binding, m_handle);
}

void q3texture::unbind(GLuint binding) {
	glBindTextureUnit(binding, 0);
}

void q3texture::save(char const * path) {
	std::vector<uint8_t> image_data;
	image_data.resize(m_width * m_height * 3);
	glGetTextureImage(m_handle, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.size(), image_data.data());
	RE_SavePNG(path, image_data.data(), m_width, m_height, 3);
}

// ================================
// Q3SAMPLER
// ================================

q3sampler::q3sampler(float aniso) {
	glCreateSamplers(1, &m_handle);
	glSamplerParameteri(m_handle, GL_TEXTURE_WRAP_S, m_current_wrap);
	glSamplerParameteri(m_handle, GL_TEXTURE_WRAP_T, m_current_wrap);
	
	glSamplerParameteri(m_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(m_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	
	if (aniso < 0 || aniso > glConfig.maxTextureFilterAnisotropy)
		aniso = glConfig.maxTextureFilterAnisotropy;
	if (aniso)
		glSamplerParameteri(m_handle, GL_TEXTURE_MAX_ANISOTROPY, aniso);
}

q3sampler::~q3sampler() {
	if (m_handle) glDeleteSamplers(1, &m_handle);
}

void q3sampler::bind(GLuint binding) {
	glBindSampler(binding, m_handle);
}

void q3sampler::wrap(GLenum wrap) {
	if (wrap == m_current_wrap) return;
	m_current_wrap = wrap;
	glSamplerParameteri(m_handle, GL_TEXTURE_WRAP_S, m_current_wrap);
	glSamplerParameteri(m_handle, GL_TEXTURE_WRAP_T, m_current_wrap);
}

// ================================
// REGISTRY
// ================================

instance::texture_registry::texture_registry() {
	R_ImageLoader_Init();
}

void instance::texture_registry::generate_named_defaults() {
	q3texture_ptr whiteimage = make_q3texture(1, 1, 1, false);
	whiteimage->upload2D(1, 1, "\xFF\xFF\xFF\xFF");
	
	this->whiteimage = lookup["$whiteimage"] = lookup["*white"] = whiteimage;
	lookup["*invalid"] = nullptr;
}

q3texture_ptr instance::texture_registry::reg(istring const & name, bool mips) {
	assert(hw_inst->renderer_initialized);
	
	auto iter = lookup.find(name);
	if (iter != lookup.end()) return iter->second;
	
	byte * data;
	int32_t width, height;
	
	R_LoadImage(name.c_str(), &data, &width, &height);
	if (!data) {
		Com_Printf(S_COLOR_RED "ERROR: Failed to load texture '%s'!\n", name.c_str());
		return lookup[name] = nullptr;
	}
	
	q3texture_ptr & tex = lookup[name] = make_q3texture(width, height, 1, mips);
	
	for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
		byte * pixel = data + (y * width + x) * 4;
		if (pixel[3] != 0xFF) tex->set_transparent();
	}
	
	tex->upload2D(width, height, data);
	tex->generate_mipmaps();
	
	Z_Free(data);
	return tex;
}
