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
	layout(location = )GLSL" << LAYOUT_LIGHTMAP_UV << R"GLSL() in vec2 lm_uvs[4];
	layout(location = )GLSL" << LAYOUT_LIGHTMAP_STYLE << R"GLSL() in uint lm_styles[4];

	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;

	out vec2 lm_uv[4];
	out float lm_factor[4];
	
	flat out uint lm_style[4];

	void main() {
	
		lm_uv[0] = lm_uvs[0];
		/*lm_uv[1] = lm_uvs[1];
		lm_uv[2] = lm_uvs[2];
		lm_uv[3] = lm_uvs[3];*/
		
		lm_factor[0] = lm_styles[0] == 0 ? 1 : 0;
		/*lm_factor[1] = lm_styles[1] == 0 ? 1 : 0;
		lm_factor[2] = lm_styles[2] == 0 ? 1 : 0;
		lm_factor[3] = lm_styles[3] == 0 ? 1 : 0;*/
		
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
	layout(binding = )GLSL" << BINDING_LIGHTMAP << R"GLSL() uniform sampler2D lm;
	
	in vec2 lm_uv[4];
	in float lm_factor[4];
	
	layout(location = 0) out vec4 color;
	
	void main() {
	
		color = vec4(0, 0, 0, 0);
		
		color += texture(lm, lm_uv[0]) * lm_factor[0];
		/*color += texture(lm, lm_uv[1]) * lm_factor[1];
		color += texture(lm, lm_uv[2]) * lm_factor[2];
		color += texture(lm, lm_uv[3]) * lm_factor[3];*/
		
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
}

void programs::q3lightmap::mvp(qm::mat4_t const & value) {
	m_mvp = value;
	if (is_bound()) m_mvp.push_direct(location_mvp);
}

void programs::q3lightmap::color(qm::vec4_t const & value) {
	m_color = value;
	if (is_bound()) m_color.push_direct(location_color);
}
