#include "hw_local.hh"
using namespace howler;

q3world::q3worldmesh_maplit::q3worldmesh_maplit(vertex_t const * data, size_t num, mode m) : q3mesh(m) {
	
	static constexpr uint_fast16_t offsetof_verts = 0;
	static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
	static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
	static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
	static constexpr uint_fast16_t offsetof_color = offsetof_uv + sizeof_uv;
	static constexpr uint_fast16_t sizeof_color = sizeof(vertex_t::color);
	static constexpr uint_fast16_t offsetof_lm_uv = offsetof_color + sizeof_color;
	static constexpr uint_fast16_t sizeof_lm_uv = sizeof(vertex_t::lm_uv);
	static constexpr uint_fast16_t sizeof_all = offsetof_lm_uv + sizeof_lm_uv;
	static_assert(sizeof_all == sizeof(vertex_t));
	
	m_size = num;
	uniform_info_t & info = create_uniform_info();
	
	info.mode = 1;
	
	glCreateBuffers(1, &m_vbo);
	glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
	
	glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
	
	glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV0);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV1);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV2);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV3);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV0, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV1, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV2, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV3, 0);
	
	glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, 3, GL_FLOAT, GL_FALSE, offsetof_verts);
	glVertexArrayAttribFormat(m_handle, LAYOUT_UV, 2, GL_FLOAT, GL_FALSE, offsetof_uv);
	glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR, 4, GL_FLOAT, GL_FALSE, offsetof_color);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV0, 2, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 0);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV1, 2, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 8);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV2, 2, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 16);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV3, 2, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 24);
}

q3world::q3worldmesh_maplit::~q3worldmesh_maplit() {
	glDeleteBuffers(1, &m_vbo);
}

q3world::q3worldmesh_vertexlit::q3worldmesh_vertexlit(vertex_t const * data, size_t num, mode m) : q3mesh(m) {
	
	static constexpr uint_fast16_t offsetof_verts = 0;
	static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
	static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
	static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
	static constexpr uint_fast16_t offsetof_color = offsetof_uv + sizeof_uv;
	static constexpr uint_fast16_t sizeof_color = sizeof(vertex_t::lm_color);
	static constexpr uint_fast16_t sizeof_all = offsetof_color + sizeof_color;
	static_assert(sizeof_all == sizeof(vertex_t));
	
	m_size = num;
	uniform_info_t & info = create_uniform_info();
	
	info.mode = 2;
	
	glCreateBuffers(1, &m_vbo);
	glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
	
	glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
	
	glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMCOLOR0);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMCOLOR1);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMCOLOR2);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMCOLOR3);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMCOLOR0, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMCOLOR1, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMCOLOR2, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMCOLOR3, 0);
	
	glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, 3, GL_FLOAT, GL_FALSE, offsetof_verts);
	glVertexArrayAttribFormat(m_handle, LAYOUT_UV, 2, GL_FLOAT, GL_FALSE, offsetof_uv);
	glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR, 4, GL_FLOAT, GL_FALSE, offsetof_color);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMCOLOR0, 4, GL_FLOAT, GL_FALSE, offsetof_color + 0);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMCOLOR1, 4, GL_FLOAT, GL_FALSE, offsetof_color + 16);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMCOLOR2, 4, GL_FLOAT, GL_FALSE, offsetof_color + 32);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMCOLOR3, 4, GL_FLOAT, GL_FALSE, offsetof_color + 48);
}

q3world::q3worldmesh_vertexlit::~q3worldmesh_vertexlit() {
	glDeleteBuffers(1, &m_vbo);
}
