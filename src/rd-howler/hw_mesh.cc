#include "hw_local.hh"
using namespace howler;

#ifdef _DEBUG
size_t q3mesh::m_debug_draw_count = 0;
#endif

GLuint q3mesh::bound_handle = 0;

q3mesh_basic::q3mesh_basic(mode m) : m_mode(m) {
	glCreateVertexArrays(1, &m_handle);
}

q3mesh_basic::~q3mesh_basic() {
	if (!m_handle) return;
	glDeleteVertexArrays(1, &m_handle);
	if (is_bound()) {
		bound_handle = 0;
	}
}

void q3mesh_basic::bind() {
	if (is_bound()) return;
	glBindVertexArray(m_handle);
	bound_handle = m_handle;
}

void q3mesh_basic::draw() {
	bind();
	glDrawArrays(static_cast<GLenum>(m_mode), 0, m_size);
	
	#ifdef _DEBUG
	m_debug_draw_count++;
	#endif
}

struct basic_quad_mesh : public q3mesh_basic {
	struct vertex_t {
		qm::vec3_t vert;
		qm::vec2_t uv;
	};
	
	basic_quad_mesh(vertex_t const * data, size_t num) : q3mesh_basic(mode::triangle_strip) {
		
		static constexpr uint_fast16_t offsetof_verts = 0;
		static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
		static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
		static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
		static constexpr uint_fast16_t sizeof_all = offsetof_uv + sizeof_uv;
		static_assert(sizeof_all == sizeof(vertex_t));
		
		m_size = num;
		glCreateBuffers(1, &m_vbo);
		glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
		
		glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
		
		glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
		glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
		
		glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, sizeof_verts / 4, GL_FLOAT, GL_FALSE, offsetof_verts);
		glVertexArrayAttribFormat(m_handle, LAYOUT_UV, sizeof_uv / 4, GL_FLOAT, GL_FALSE, offsetof_uv);
		
	}
	
	~basic_quad_mesh() {
		glDeleteBuffers(1, &m_vbo);
	}
private:
	GLuint m_vbo;
};

static constexpr std::array<basic_quad_mesh::vertex_t, 4> fullquad_combo_verts = {
	basic_quad_mesh::vertex_t {{-1, -1,  0}, {0, 0}},
	basic_quad_mesh::vertex_t {{-1,  1,  0}, {0, 1}},
	basic_quad_mesh::vertex_t {{ 1, -1,  0}, {1, 0}},
	basic_quad_mesh::vertex_t {{ 1,  1,  0}, {1, 1}}
};

static constexpr std::array<basic_quad_mesh::vertex_t, 4> unitquad_combo_verts = {
	basic_quad_mesh::vertex_t {{0, 0, 0}, {0, 0}},
	basic_quad_mesh::vertex_t {{0, 1, 0}, {0, 1}},
	basic_quad_mesh::vertex_t {{1, 0, 0}, {1, 0}},
	basic_quad_mesh::vertex_t {{1, 1, 0}, {1, 1}}
};

q3mesh_ptr q3mesh_basic::generate_fullquad() {
	return std::make_shared<basic_quad_mesh>(fullquad_combo_verts.data(), fullquad_combo_verts.size());
}

q3mesh_ptr q3mesh_basic::generate_unitquad() {
	return std::make_shared<basic_quad_mesh>(unitquad_combo_verts.data(), unitquad_combo_verts.size());
}

struct skybox_mesh : public q3mesh_basic {
	struct vertex_t {
		qm::vec3_t vert;
		qm::vec2_t uv;
		uint32_t side;
	};
	
	skybox_mesh(vertex_t const * data, size_t num) : q3mesh_basic(mode::triangles) {
		
		static constexpr uint_fast16_t offsetof_verts = 0;
		static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
		static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
		static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
		static constexpr uint_fast16_t offsetof_side = offsetof_uv + sizeof_uv;
		static constexpr uint_fast16_t sizeof_side = sizeof(vertex_t::side);
		static constexpr uint_fast16_t sizeof_all = offsetof_side + sizeof_side;
		static_assert(sizeof_all == sizeof(vertex_t));
		
		m_size = num;
		glCreateBuffers(1, &m_vbo);
		glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
		
		glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
		
		glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_TEXBIND);
		glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_TEXBIND, 0);
		
		glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, 3, GL_FLOAT, GL_FALSE, offsetof_verts);
		glVertexArrayAttribFormat(m_handle, LAYOUT_UV, 2, GL_FLOAT, GL_FALSE, offsetof_uv);
		glVertexArrayAttribIFormat(m_handle, LAYOUT_TEXBIND, 1, GL_UNSIGNED_INT, offsetof_side);
	}
	
	~skybox_mesh() {
		glDeleteBuffers(1, &m_vbo);
	}
private:
	GLuint m_vbo;
};

static constexpr std::array<skybox_mesh::vertex_t, 36> skybox_verts {
	skybox_mesh::vertex_t {{-1, -1,  1 }, {1, 0}, 0},
	skybox_mesh::vertex_t {{-1,  1,  1 }, {1, 1}, 0},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {0, 1}, 0},
	skybox_mesh::vertex_t {{-1, -1,  1 }, {1, 0}, 0},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {0, 1}, 0},
	skybox_mesh::vertex_t {{ 1, -1,  1 }, {0, 0}, 0},
	
	skybox_mesh::vertex_t {{-1, -1, -1 }, {0, 0}, 1},
	skybox_mesh::vertex_t {{ 1, -1, -1 }, {1, 0}, 1},
	skybox_mesh::vertex_t {{ 1,  1, -1 }, {1, 1}, 1},
	skybox_mesh::vertex_t {{-1, -1, -1 }, {0, 0}, 1},
	skybox_mesh::vertex_t {{ 1,  1, -1 }, {1, 1}, 1},
	skybox_mesh::vertex_t {{-1,  1, -1 }, {0, 1}, 1},
	
	skybox_mesh::vertex_t {{ 1, -1, -1 }, {0, 0}, 2},
	skybox_mesh::vertex_t {{ 1,  1, -1 }, {0, 1}, 2},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {1, 1}, 2},
	skybox_mesh::vertex_t {{ 1, -1, -1 }, {0, 0}, 2},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {1, 1}, 2},
	skybox_mesh::vertex_t {{ 1, -1,  1 }, {1, 0}, 2},
	
	skybox_mesh::vertex_t {{-1, -1, -1 }, {1, 0}, 3},
	skybox_mesh::vertex_t {{-1,  1,  1 }, {0, 1}, 3},
	skybox_mesh::vertex_t {{-1,  1, -1 }, {1, 1}, 3},
	skybox_mesh::vertex_t {{-1, -1, -1 }, {1, 0}, 3},
	skybox_mesh::vertex_t {{-1, -1,  1 }, {0, 0}, 3},
	skybox_mesh::vertex_t {{-1,  1,  1 }, {0, 1}, 3},
	
	skybox_mesh::vertex_t {{-1, -1, -1 }, {1, 0}, 4},
	skybox_mesh::vertex_t {{ 1, -1, -1 }, {0, 0}, 4},
	skybox_mesh::vertex_t {{ 1, -1,  1 }, {0, 1}, 4},
	skybox_mesh::vertex_t {{-1, -1, -1 }, {1, 0}, 4},
	skybox_mesh::vertex_t {{ 1, -1,  1 }, {0, 1}, 4},
	skybox_mesh::vertex_t {{-1, -1,  1 }, {1, 1}, 4},
	
	skybox_mesh::vertex_t {{-1,  1, -1 }, {1, 1}, 5},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {0, 0}, 5},
	skybox_mesh::vertex_t {{ 1,  1, -1 }, {0, 1}, 5},
	skybox_mesh::vertex_t {{-1,  1, -1 }, {1, 1}, 5},
	skybox_mesh::vertex_t {{-1,  1,  1 }, {1, 0}, 5},
	skybox_mesh::vertex_t {{ 1,  1,  1 }, {0, 0}, 5},
};

q3mesh_ptr q3mesh_basic::generate_skybox_mesh() {
	return std::make_shared<skybox_mesh>(skybox_verts.data(), skybox_verts.size());
}
