#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_mvp = 1;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vertex_pos;
	layout(location = )GLSL" << LAYOUT_UV << R"GLSL() in vec2 vertex_uv;
	layout(location = )GLSL" << LAYOUT_TEXBIND << R"GLSL() in uint vertex_texbind;

	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;

	out vec2 uv;
	out flat uint texbind;
	
	void main() {
		uv = vertex_uv;
		texbind = vertex_texbind;
		gl_Position = vertex_matrix * vec4(vertex_pos, 1);
	}
	)GLSL";

	return ss.str();
}

static std::string generate_fragment_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	// TEXTURE BINDINGS
	layout(binding = )GLSL" << BINDING_SKYBOX + 0 << R"GLSL() uniform sampler2D sb0;
	layout(binding = )GLSL" << BINDING_SKYBOX + 1 << R"GLSL() uniform sampler2D sb1;
	layout(binding = )GLSL" << BINDING_SKYBOX + 2 << R"GLSL() uniform sampler2D sb2;
	layout(binding = )GLSL" << BINDING_SKYBOX + 3 << R"GLSL() uniform sampler2D sb3;
	layout(binding = )GLSL" << BINDING_SKYBOX + 4 << R"GLSL() uniform sampler2D sb4;
	layout(binding = )GLSL" << BINDING_SKYBOX + 5 << R"GLSL() uniform sampler2D sb5;
		
	in vec2 uv;
	in flat uint texbind;
	
	layout(location = 0) out vec4 color;
	
	void main() {
		color = vec4(0, 0, 0, 0);
		switch(texbind) {
			case 0: color = texture(sb0, uv); break;
			case 1: color = texture(sb1, uv); break;
			case 2: color = texture(sb2, uv); break;
			case 3: color = texture(sb3, uv); break;
			case 4: color = texture(sb4, uv); break;
			case 5: color = texture(sb5, uv); break;
		}
	}
	)GLSL";

	return ss.str();
}

programs::q3skybox::q3skybox() {
	
	std::string log;
	std::string vsrc = generate_vertex_shader();
	std::string fsrc = generate_fragment_shader();
	
	shader vert {shader::type::vert};
	vert.source(vsrc.c_str());
	if (!vert.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3skybox vertex shader failed to compile:\n%s", log.c_str()));
	}
	
	shader frag {shader::type::frag};
	frag.source(fsrc.c_str());
	if (!frag.compile(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3skybox fragment shader failed to compile:\n%s", log.c_str()));
	}
	
	attach(vert);
	attach(frag);
	
	if (!link(&log)) {
		Com_Error(ERR_FATAL, va("programs::q3skybox failed to link:\n%s", log.c_str()));
	}
}

void programs::q3skybox::on_bind() {
	m_mvp.push(location_mvp);
}

void programs::q3skybox::mvp(qm::mat4_t const & v) {
	m_mvp = v;
	if (is_bound()) m_mvp.push(location_mvp);
}
