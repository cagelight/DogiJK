#include "../hw_local.hh"
using namespace howler;

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

	uniform mat4 mvp;
	uniform uint lm_mode;

	out vec4 vertex_color;
	out vec2 vertex_uv[4];
	out float factor;
	
	void main() {
		gl_Position = mvp * vec4(vert, 1);
		
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
	uniform vec4 q3color;
	
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

struct programs::q3lightmap::private_data {
	uniform_mat4 m_mvp = qm::mat4_t::identity();
	uniform_vec4 m_color = qm::vec4_t {1, 1, 1, 1};
	uniform_lmmode m_mode = 0;
	
	void reset() {
		m_mvp.reset();
		m_color.reset();
		m_mode.reset();
	}
	
	void push() {
		m_mvp.push_direct();
		m_color.push_direct();
		m_mode.push_direct();
	}
};


programs::q3lightmap::q3lightmap() : m_data(new private_data) {
	
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
	
	if (m_data->m_mvp.set_location(get_location("mvp")) == -1)
		Com_Error(ERR_FATAL, "programs::q3lightmap: could not find uniform location for \"mvp\"");
	if (m_data->m_color.set_location(get_location("q3color")) == -1)
		Com_Error(ERR_FATAL, "programs::q3lightmap: could not find uniform location for \"q3color\"");
	if (m_data->m_mode.set_location(get_location("lm_mode")) == -1)
		Com_Error(ERR_FATAL, "programs::q3lightmap: could not find uniform location for \"lm_mode\"");
}

programs::q3lightmap::~q3lightmap() {}

void programs::q3lightmap::on_bind() {
	m_data->push();
}

void programs::q3lightmap::mvp(qm::mat4_t const & value) {
	m_data->m_mvp = value;
	if (is_bound()) m_data->m_mvp.push();
}

void programs::q3lightmap::color(qm::vec4_t const & value) {
	m_data->m_color = value;
	if (is_bound()) m_data->m_color.push();
}

void programs::q3lightmap::mode(GLuint const & value) {
	m_data->m_mode = value;
	if (is_bound()) m_data->m_mode.push();
}
