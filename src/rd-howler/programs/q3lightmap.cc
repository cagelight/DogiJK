#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_mvp = 0;
static constexpr GLint location_uv = 1;
static constexpr GLint location_color = 2;
static constexpr GLint location_styles = 3;
static constexpr GLint location_mode = 4;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	
	layout(location = )GLSL" << LAYOUT_LMUV0 << R"GLSL() in vec2 lm_uv0;
	layout(location = )GLSL" << LAYOUT_LMUV1 << R"GLSL() in vec2 lm_uv1;
	layout(location = )GLSL" << LAYOUT_LMUV2 << R"GLSL() in vec2 lm_uv2;
	layout(location = )GLSL" << LAYOUT_LMUV3 << R"GLSL() in vec2 lm_uv3;
	
	layout(location = )GLSL" << LAYOUT_LMCOLOR0 << R"GLSL() in vec4 lm_color0;
	layout(location = )GLSL" << LAYOUT_LMCOLOR1 << R"GLSL() in vec4 lm_color1;
	layout(location = )GLSL" << LAYOUT_LMCOLOR2 << R"GLSL() in vec4 lm_color2;
	layout(location = )GLSL" << LAYOUT_LMCOLOR3 << R"GLSL() in vec4 lm_color3;

	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;
	layout(location = )GLSL" << location_mode << R"GLSL() uniform uint lm_mode;

	out vec4 vertex_color;
	out vec2 vertex_uv[4];
	out float factor;
	
	void main() {
		gl_Position = vertex_matrix * vec4(vert, 1);
		
		switch (lm_mode) {
			case 1:
				vertex_color = vec4(1, 1, 1, 1);
				vertex_uv[0] = lm_uv0;
				vertex_uv[1] = lm_uv1;
				vertex_uv[2] = lm_uv2;
				vertex_uv[3] = lm_uv3;
				factor = 1;
				break;
			case 2:
				vertex_color = lm_color0;
				factor = 0;
				break;
			default:
			case 3:
				vertex_color = vec4(1, 1, 1, 1);
				factor = 0;
				break;
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
	layout(binding = )GLSL" << BINDING_LIGHTMAP << R"GLSL() uniform sampler2D lm;
	
	in vec4 vertex_color;
	in vec2 vertex_uv[4];
	in float factor;
	
	layout(location = 0) out vec4 color;
	
	void main() {
		
		vec4 vcolor = vertex_color;
		vec4 ucolor = texture(lm, vertex_uv[0]);
		color = mix(vcolor, ucolor, factor);
		color *= q3color;
		
	}
	)GLSL";
	
	return ss.str();
}

programs::q3lightmap::q3lightmap() {
	
	std::string log;
	std::string vsrc = generate_vertex_shader();
	std::string fsrc = generate_fragment_shader();
	
	shader vert {shader::type::vert};
	vert.source(vsrc.c_str());
	if (!vert.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3lightmap vertex shader failed to compile:\n%s", log.c_str()));
	}
	
	shader frag {shader::type::frag};
	frag.source(fsrc.c_str());
	if (!frag.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3lightmap fragment shader failed to compile:\n%s", log.c_str()));
	}
	
	attach(vert);
	attach(frag);
	
	if (!link(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3lightmap failed to link:\n%s", log.c_str()));
	}
}

void programs::q3lightmap::on_bind() {
	m_mvp.push(location_mvp);
	m_color.push(location_color);
	m_styles.push(location_styles);
	m_mode.push(location_mode);
}

void programs::q3lightmap::mvp(qm::mat4_t const & value) {
	m_mvp = value;
	if (is_bound()) m_mvp.push(location_mvp);
}

void programs::q3lightmap::color(qm::vec4_t const & value) {
	m_color = value;
	if (is_bound()) m_color.push(location_color);
}

void programs::q3lightmap::styles(lightmap_styles_t const & value) {
	m_styles = value;
	if (is_bound()) m_styles.push(location_styles);
}

void programs::q3lightmap::mode(GLuint const & value) {
	m_mode = value;
	if (is_bound()) m_mode.push_direct(location_mode);
}
