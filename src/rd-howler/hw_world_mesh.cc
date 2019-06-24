#include "hw_local.hh"
using namespace howler;

q3world::q3worldmesh_maplit::q3worldmesh_maplit(vertex_t const * data, size_t num, mode m) : q3mesh_basic(m) {
	
	static constexpr uint_fast8_t offsetof_verts = 0;
	static constexpr uint_fast8_t sizeof_verts = sizeof(vertex_t::vert);
	static constexpr uint_fast8_t offsetof_uv = offsetof_verts + sizeof_verts;
	static constexpr uint_fast8_t sizeof_uv = sizeof(vertex_t::uv);
	static constexpr uint_fast8_t offsetof_normal = offsetof_uv + sizeof_uv;
	static constexpr uint_fast8_t sizeof_normal = sizeof(vertex_t::normal);
	static constexpr uint_fast8_t offsetof_color = offsetof_normal + sizeof_normal;
	static constexpr uint_fast8_t sizeof_color = sizeof(vertex_t::color);
	static constexpr uint_fast8_t offsetof_lm_uv = offsetof_color + sizeof_color;
	static constexpr uint_fast8_t sizeof_lm_uv = sizeof(vertex_t::lm_uv);
	static constexpr uint_fast8_t offsetof_styles = offsetof_lm_uv + sizeof_lm_uv;
	static constexpr uint_fast8_t sizeof_styles = sizeof(vertex_t::styles);
	static constexpr uint_fast8_t sizeof_all = offsetof_styles + sizeof_styles;
	static_assert(sizeof_all == sizeof(vertex_t));
	
	m_size = num;
	uniform_info_t & info = create_uniform_info();
	
	info.lm_mode = 1;
	
	glCreateBuffers(1, &m_vbo);
	glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
	
	glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
	
	glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_NORMAL);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR0);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV01_COLOR2);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV23_COLOR3);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMSTYLES);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_NORMAL, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR0, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV01_COLOR2, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV23_COLOR3, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMSTYLES, 0);
	
	glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, 3, GL_FLOAT, GL_FALSE, offsetof_verts);
	glVertexArrayAttribFormat(m_handle, LAYOUT_UV, 2, GL_FLOAT, GL_FALSE, offsetof_uv);
	glVertexArrayAttribFormat(m_handle, LAYOUT_NORMAL, 3, GL_FLOAT, GL_FALSE, offsetof_normal);
	glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR0, 4, GL_FLOAT, GL_FALSE, offsetof_color);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV01_COLOR2, 4, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 0);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV23_COLOR3, 4, GL_FLOAT, GL_FALSE, offsetof_lm_uv + 16);
	glVertexArrayAttribIFormat(m_handle, LAYOUT_LMSTYLES, 4, GL_UNSIGNED_BYTE, offsetof_styles);
}

q3world::q3worldmesh_maplit::~q3worldmesh_maplit() {
	glDeleteBuffers(1, &m_vbo);
}

q3world::q3worldmesh_vertexlit::q3worldmesh_vertexlit(vertex_t const * data, size_t num, mode m) : q3mesh_basic(m) {
	
	static constexpr uint_fast8_t offsetof_verts = 0;
	static constexpr uint_fast8_t sizeof_verts = sizeof(vertex_t::vert);
	static constexpr uint_fast8_t offsetof_uv = offsetof_verts + sizeof_verts;
	static constexpr uint_fast8_t sizeof_uv = sizeof(vertex_t::uv);
	static constexpr uint_fast8_t offsetof_normal = offsetof_uv + sizeof_uv;
	static constexpr uint_fast8_t sizeof_normal = sizeof(vertex_t::normal);
	static constexpr uint_fast8_t offsetof_color = offsetof_normal + sizeof_normal;
	static constexpr uint_fast8_t sizeof_color = sizeof(vertex_t::lm_color);
	static constexpr uint_fast8_t offsetof_styles = offsetof_color + sizeof_color;
	static constexpr uint_fast8_t sizeof_styles = sizeof(vertex_t::styles);
	static constexpr uint_fast8_t sizeof_all = offsetof_styles + sizeof_styles;
	static_assert(sizeof_all == sizeof(vertex_t));
	
	m_size = num;
	uniform_info_t & info = create_uniform_info();
	
	info.lm_mode = 2;
	
	glCreateBuffers(1, &m_vbo);
	glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
	
	glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
	
	glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_NORMAL);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR0);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR1);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV01_COLOR2);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMUV23_COLOR3);
	glEnableVertexArrayAttrib(m_handle, LAYOUT_LMSTYLES);
	
	glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_NORMAL, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR0, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR1, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV01_COLOR2, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMUV23_COLOR3, 0);
	glVertexArrayAttribBinding(m_handle, LAYOUT_LMSTYLES, 0);
	
	glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, 3, GL_FLOAT, GL_FALSE, offsetof_verts);
	glVertexArrayAttribFormat(m_handle, LAYOUT_UV, 2, GL_FLOAT, GL_FALSE, offsetof_uv);
	glVertexArrayAttribFormat(m_handle, LAYOUT_NORMAL, 3, GL_FLOAT, GL_FALSE, offsetof_normal);
	glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR0, 4, GL_FLOAT, GL_FALSE, offsetof_color + 0);
	glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR1, 4, GL_FLOAT, GL_FALSE, offsetof_color + 16);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV01_COLOR2, 4, GL_FLOAT, GL_FALSE, offsetof_color + 32);
	glVertexArrayAttribFormat(m_handle, LAYOUT_LMUV23_COLOR3, 4, GL_FLOAT, GL_FALSE, offsetof_color + 48);
	glVertexArrayAttribIFormat(m_handle, LAYOUT_LMSTYLES, 4, GL_UNSIGNED_BYTE, offsetof_styles);
}

q3world::q3worldmesh_vertexlit::~q3worldmesh_vertexlit() {
	glDeleteBuffers(1, &m_vbo);
}
