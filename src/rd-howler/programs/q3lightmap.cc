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
	layout(location = )GLSL" << LAYOUT_LIGHTMAP_DATA << R"GLSL() in vec3 lm_data[4];
	layout(location = )GLSL" << LAYOUT_LIGHTMAP_STYLE << R"GLSL() in uint lm_styles[4];
	layout(location = )GLSL" << LAYOUT_LIGHTMAP_MODE << R"GLSL() in uint lm_modes[4];

	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;

	out vec2 lm_uv[4];
	out vec3 lm_col[4];
	out float lm_uvfactor[4];
	out float lm_colfactor[4];

	void main() {
	
		for (uint i = 0; i < 4; i++) {
			switch (lm_modes[i]) {
				default:
					lm_uvfactor[i] = 0;
					lm_colfactor[i] = 0;
					break;
				case 1:
					lm_uv[i] = lm_data[i].xy;
					lm_uvfactor[i] = 1;
					lm_colfactor[i] = 0;
					break;
				case 2:
					lm_col[i] = lm_data[i];
					lm_uvfactor[i] = 0;
					lm_colfactor[i] = 1;
					break;
				case 3:
					lm_col[i] = vec3(1, 1, 1);
					lm_uvfactor[i] = 0;
					lm_colfactor[i] = 1;
					break;
			}
		}
		
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
	in vec3 lm_col[4];
	in float lm_uvfactor[4];
	in float lm_colfactor[4];
	
	layout(location = 0) out vec4 color;
	
	void main() {
	
		color = vec4(0, 0, 0, 1);
		
		for (uint i = 0; i < 4; i++) {
			color += vec4(texture(lm, lm_uv[i]).xyz * lm_uvfactor[i], 0);
			//color += vec4(lm_col[i] * lm_colfactor[i], 0);
		}
		
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
