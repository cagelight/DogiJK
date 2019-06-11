#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_mvp = 0;
static constexpr GLint location_uv = 1;
static constexpr GLint location_color = 2;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	layout(location = )GLSL" << LAYOUT_UV << R"GLSL() in vec2 uv;

	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;
	layout(location = )GLSL" << location_uv << R"GLSL() uniform mat3 uv_matrix;

	out vec2 f_uv;

	void main() {
		f_uv = (uv_matrix * vec3(uv, 1)).xy;
		gl_Position = vertex_matrix * vec4(vert, 1);
	}
	)GLSL";

	return ss.str();
}

static std::string generate_fragment_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
	
	// UNIFORMS
	layout(location = )GLSL" << location_color << R"GLSL() uniform vec4 q3color;
	
	// TEXTURE BINDINGS
	layout(binding = )GLSL" << BINDING_DIFFUSE << R"GLSL() uniform sampler2D tex;
	
	in vec2 f_uv;
	
	layout(location = 0) out vec4 color;
	
	void main() {
		color = texture(tex, f_uv) * q3color;
	}
	)GLSL";

	return ss.str();
}

programs::q3main::q3main() {
	
	std::string log;
	std::string vsrc = generate_vertex_shader();
	std::string fsrc = generate_fragment_shader();
	
	shader vert {shader::type::vert};
	vert.source(vsrc.c_str());
	if (!vert.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3main vertex shader failed to compile:\n%s", log.c_str()));
	}
	
	shader frag {shader::type::frag};
	frag.source(fsrc.c_str());
	if (!frag.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3main fragment shader failed to compile:\n%s", log.c_str()));
	}
	
	attach(vert);
	attach(frag);
	
	if (!link(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3main failed to link:\n%s", log.c_str()));
	}
}

void programs::q3main::on_bind() {
	m_mvp.push(location_mvp);
	m_uv.push(location_uv);
	m_color.push(location_color);
}

void programs::q3main::mvp(qm::mat4_t const & value) {
	m_mvp = value;
	if (is_bound()) m_mvp.push_direct(location_mvp);
}

void programs::q3main::uvm(qm::mat3_t const & value) {
	m_uv = value;
	if (is_bound()) m_uv.push_direct(location_uv);
}

void programs::q3main::color(qm::vec4_t const & value) {
	m_color = value;
	if (is_bound()) m_color.push_direct(location_color);
}
