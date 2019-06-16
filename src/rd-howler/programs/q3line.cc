#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_mvp = 1;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	
	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;

	out vec4 vcolor;

	void main() {
		vcolor = vec4(1, 1, 1, 0.75);
		gl_Position = vertex_matrix * vec4(vert, 1);
	}
	)GLSL";

	return ss.str();
}

static std::string generate_fragment_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	in vec4 vcolor;
	layout(location = 0) out vec4 color;
	
	void main() {
		color = vcolor;
	}
	)GLSL";

	return ss.str();
}

programs::q3line::q3line() {
	
	std::string log;
	std::string vsrc = generate_vertex_shader();
	std::string fsrc = generate_fragment_shader();
	
	shader vert {shader::type::vert};
	vert.source(vsrc.c_str());
	if (!vert.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3line vertex shader failed to compile:\n%s", log.c_str()));
	}
	
	shader frag {shader::type::frag};
	frag.source(fsrc.c_str());
	if (!frag.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3line fragment shader failed to compile:\n%s", log.c_str()));
	}
	
	attach(vert);
	attach(frag);
	
	if (!link(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3line failed to link:\n%s", log.c_str()));
	}
}

void programs::q3line::on_bind() {
	m_mvp.push(location_mvp);
}

void programs::q3line::mvp(qm::mat4_t const & v) {
	m_mvp = v;
	if (is_bound()) m_mvp.push(location_mvp);
}
