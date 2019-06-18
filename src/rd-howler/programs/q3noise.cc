#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_time = 0;
static constexpr GLint location_mvp = 1;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vertex_pos;
	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;
	
	void main() {
		gl_Position = vertex_matrix * vec4(vertex_pos, 1);
	}
	)GLSL";

	return ss.str();
}

static std::string generate_fragment_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << location_time << R"GLSL() uniform float time;
		
	out vec4 color;
	
	float rand(vec2 co){
		return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
	}
		
	void main() {
		float freq = sin(pow(mod(time, 10.0)+10.0, 1.9));
		float v = rand(vec2(gl_FragCoord.x, gl_FragCoord.y) + mod(time, freq));
		color = vec4(v, v, v, 1);
	}
	)GLSL";

	return ss.str();
}

programs::q3noise::q3noise() {
	
	std::string log;
	std::string vsrc = generate_vertex_shader();
	std::string fsrc = generate_fragment_shader();
	
	shader vert {shader::type::vert};
	vert.source(vsrc.c_str());
	if (!vert.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3noise vertex shader failed to compile:\n%s", log.c_str()));
	}
	
	shader frag {shader::type::frag};
	frag.source(fsrc.c_str());
	if (!frag.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3noise fragment shader failed to compile:\n%s", log.c_str()));
	}
	
	attach(vert);
	attach(frag);
	
	if (!link(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3noise failed to link:\n%s", log.c_str()));
	}
}

void programs::q3noise::on_bind() {
	m_time.push(location_time);
	m_mvp.push(location_mvp);
}

void programs::q3noise::time(float const & v) {
	m_time = v;
	if (is_bound()) m_time.push(location_time);
}

void programs::q3noise::mvp(qm::mat4_t const & v) {
	m_mvp = v;
	if (is_bound()) m_mvp.push(location_mvp);
}
