#include "hw_local.hh"
using namespace howler;

GLuint q3mesh::bound_handle = 0;

static constexpr std::array<float, 12> fullquad_verts = {
	-1, -1,  0,
	-1,  1,  0,
	1, -1,  0,
	1,  1,  0,
};

static constexpr std::array<float, 12> unitquad_verts = {
	0,  0,  0,
	0,  1,  0,
	1,  0,  0,
	1,  1,  0,
};

static constexpr std::array<float, 8> quad_uvs = {
	0,  0,
	0,  1,
	1,  0,
	1,  1,
};

q3mesh::q3mesh(mode m) : m_mode(m) {
	glCreateVertexArrays(1, &m_handle);
}

q3mesh::~q3mesh() {
	if (!m_handle) return;
	glDeleteBuffers(num_vbos, m_vbos);
	glDeleteVertexArrays(1, &m_handle);
	if (is_bound()) {
		bound_handle = 0;
	}
}

void q3mesh::upload_verts(float const * data, size_t size) {

	static constexpr uint_fast8_t bytes_per_element = 4;
	static constexpr uint_fast8_t elements_per_vertex = 3;
	static constexpr uint_fast8_t bytes_per_vertex = bytes_per_element * elements_per_vertex;
	
	if (m_vbos[LAYOUT_VERTEX])
		Com_Error(ERR_FATAL, "q3mesh::upload_verts: attempted to replace existing data");
	if (m_size && size != m_size * elements_per_vertex)
		Com_Error(ERR_FATAL, "q3mesh::upload_verts: data size mismatch");
	if (!m_size)
		m_size = size / elements_per_vertex;
	
	glCreateBuffers(1, m_vbos + LAYOUT_VERTEX);
	glNamedBufferData(m_vbos[LAYOUT_VERTEX], size * bytes_per_element, data, GL_STATIC_DRAW);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, LAYOUT_VERTEX);
	glVertexArrayVertexBuffer(m_handle, LAYOUT_VERTEX, m_vbos[LAYOUT_VERTEX], 0, bytes_per_vertex);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
	glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, elements_per_vertex, GL_FLOAT, GL_FALSE, 0);
}

void q3mesh::upload_uvs(float const * data, size_t size) {

	static constexpr uint_fast8_t bytes_per_element = 4;
	static constexpr uint_fast8_t elements_per_vertex = 2;
	static constexpr uint_fast8_t bytes_per_vertex = bytes_per_element * elements_per_vertex;
	
	if (m_vbos[LAYOUT_UV])
		Com_Error(ERR_FATAL, "q3mesh::upload_uvs: attempted to replace existing data");
	if (m_size && size != m_size * elements_per_vertex)
		Com_Error(ERR_FATAL, "q3mesh::upload_uvs: data size mismatch");
	if (!m_size)
		m_size = size / elements_per_vertex;
	
	glCreateBuffers(1, m_vbos + LAYOUT_UV);
	glNamedBufferData(m_vbos[LAYOUT_UV], size * bytes_per_element, data, GL_STATIC_DRAW);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_UV, LAYOUT_UV);
	glVertexArrayVertexBuffer(m_handle, LAYOUT_UV, m_vbos[LAYOUT_UV], 0, bytes_per_vertex);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
	glVertexArrayAttribFormat(m_handle, LAYOUT_UV, elements_per_vertex, GL_FLOAT, GL_FALSE, 0);
}

void q3mesh::upload_lightmap_uvs(float const * data, size_t size) {
	
	static constexpr uint_fast8_t bytes_per_element = 4;
	static constexpr uint_fast8_t elements_per_vertex = 8;
	static constexpr uint_fast8_t bytes_per_vertex = bytes_per_element * elements_per_vertex;
	
	if (m_vbos[LAYOUT_LIGHTMAP_UV])
		Com_Error(ERR_FATAL, "q3mesh::upload_lightmap_uvs: attempted to replace existing data");
	if (m_size && size != m_size * elements_per_vertex)
		Com_Error(ERR_FATAL, "q3mesh::upload_lightmap_uvs: data size mismatch");
	if (!m_size)
		m_size = size / elements_per_vertex;
	
	glCreateBuffers(1, m_vbos + LAYOUT_LIGHTMAP_UV);
	glNamedBufferData(m_vbos[LAYOUT_LIGHTMAP_UV], size * bytes_per_element, data, GL_STATIC_DRAW);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_LIGHTMAP_UV, LAYOUT_LIGHTMAP_UV);
	glVertexArrayVertexBuffer(m_handle, LAYOUT_LIGHTMAP_UV, m_vbos[LAYOUT_LIGHTMAP_UV], 0, bytes_per_vertex);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LIGHTMAP_UV);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LIGHTMAP_UV, elements_per_vertex, GL_FLOAT, GL_FALSE, 0);
}

void q3mesh::upload_lightmap_styles(uint8_t const * data, size_t size) {

	static constexpr uint_fast8_t bytes_per_element = 1;
	static constexpr uint_fast8_t elements_per_vertex = 4;
	static constexpr uint_fast8_t bytes_per_vertex = bytes_per_element * elements_per_vertex;
	
	if (m_vbos[LAYOUT_LIGHTMAP_STYLE])
		Com_Error(ERR_FATAL, "q3mesh::upload_lightmap_styles: attempted to replace existing data");
	if (m_size && size != m_size * elements_per_vertex)
		Com_Error(ERR_FATAL, "q3mesh::upload_lightmap_styles: data size mismatch");
	if (!m_size)
		m_size = size / elements_per_vertex;
	
	glCreateBuffers(1, m_vbos + LAYOUT_LIGHTMAP_STYLE);
	glNamedBufferData(m_vbos[LAYOUT_LIGHTMAP_STYLE], size * bytes_per_element, data, GL_STATIC_DRAW);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_LIGHTMAP_STYLE, LAYOUT_LIGHTMAP_STYLE);
	glVertexArrayVertexBuffer(m_handle, LAYOUT_LIGHTMAP_STYLE, m_vbos[LAYOUT_LIGHTMAP_STYLE], 0, bytes_per_vertex);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LIGHTMAP_STYLE);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LIGHTMAP_STYLE, elements_per_vertex, GL_UNSIGNED_BYTE, GL_FALSE, 0);
}

void q3mesh::bind() {
	if (is_bound()) return;
	glBindVertexArray(m_handle);
	bound_handle = m_handle;
}

void q3mesh::draw() {
	bind();
	glDrawArrays(static_cast<GLenum>(m_mode), 0, m_size);
}

q3mesh_ptr q3mesh::generate_fullquad() {
	q3mesh_ptr mesh = make_q3mesh(mode::triangle_strip);
	mesh->upload_verts(fullquad_verts);
	mesh->upload_uvs(quad_uvs);
	return mesh;
}

q3mesh_ptr q3mesh::generate_unitquad() {
	q3mesh_ptr mesh = make_q3mesh(mode::triangle_strip);
	mesh->upload_verts(unitquad_verts);
	mesh->upload_uvs(quad_uvs);
	return mesh;
}
