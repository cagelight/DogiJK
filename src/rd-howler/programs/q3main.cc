#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_time = 0;
static constexpr GLint location_mvp = 1;
static constexpr GLint location_uv = 2;
static constexpr GLint location_color = 3;
static constexpr GLint location_use_vertex_color = 4;
static constexpr GLint location_turb = 5;
static constexpr GLint location_turb_data = 6;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	layout(location = )GLSL" << LAYOUT_UV << R"GLSL() in vec2 uv;
	layout(location = )GLSL" << LAYOUT_COLOR << R"GLSL() in vec4 vertex_color;

	layout(location = )GLSL" << location_time << R"GLSL() uniform float time;
	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;
	layout(location = )GLSL" << location_uv << R"GLSL() uniform mat3 uv_matrix;
	layout(location = )GLSL" << location_use_vertex_color << R"GLSL() uniform bool use_vertex_color;
	
	layout(location = )GLSL" << location_turb << R"GLSL() uniform bool turb;
	layout(location = )GLSL" << location_turb_data << R"GLSL() uniform vec4 turb_data;

	out vec2 f_uv;
	out vec4 vcolor;

	void main() {
	
		if (use_vertex_color)
			vcolor = vertex_color;
		else
			vcolor = vec4(1, 1, 1, 1);
		
		gl_Position = vertex_matrix * vec4(vert, 1);
		
		f_uv = (uv_matrix * vec3(uv, 1)).xy;
		if (turb) {
			f_uv.x += sin((vert[0] + vert[2]) * (turb_data.w + time * turb_data.z) / 1024.0f) * turb_data.x;
			f_uv.y += sin((vert[1]) * (turb_data.w + time * turb_data.z) / 1024.0f) * turb_data.x;
		}
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
	in vec4 vcolor;
	
	layout(location = 0) out vec4 color;
	
	void main() {
		color = texture(tex, f_uv) * q3color * vcolor;
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

void programs::q3main::bind_and_reset() {
	m_time.reset();
	m_mvp.reset();
	m_uv.reset();
	m_color.reset();
	m_use_vertex_colors.reset();
	m_turb.reset();
	m_turb_data.reset();
	
	bind();
}

void programs::q3main::on_bind() {
	m_time.push(location_time);
	m_mvp.push(location_mvp);
	m_uv.push(location_uv);
	m_color.push(location_color);
	m_use_vertex_colors.push(location_use_vertex_color);
	m_turb.push(location_turb);
	m_turb_data.push(location_turb_data);
}

void programs::q3main::time(float const & v) {
	m_time = v;
	if (is_bound()) m_time.push(location_time);
}

void programs::q3main::mvp(qm::mat4_t const & v) {
	m_mvp = v;
	if (is_bound()) m_mvp.push(location_mvp);
}

void programs::q3main::uvm(qm::mat3_t const & v) {
	m_uv = v;
	if (is_bound()) m_uv.push(location_uv);
}

void programs::q3main::color(qm::vec4_t const & v) {
	m_color = v;
	if (is_bound()) m_color.push(location_color);
}

void programs::q3main::use_vertex_colors(bool const & v) {
	m_use_vertex_colors = v;
	if (is_bound()) m_use_vertex_colors.push(location_use_vertex_color);
}

void programs::q3main::turb(bool const & v) {
	m_turb = v;
	if (is_bound()) m_turb.push(location_turb);
}

void programs::q3main::turb_data(q3stage::tx_turb const & v) {
	m_turb_data = { v.amplitude, v.base, v.frequency, v.phase };
	if (is_bound()) m_turb_data.push(location_turb_data);
}
