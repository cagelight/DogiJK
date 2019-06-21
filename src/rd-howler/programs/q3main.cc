#include "../hw_local.hh"
using namespace howler;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout (std140) uniform BoneMatricies {
		mat4 bone[72];
	};
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	layout(location = )GLSL" << LAYOUT_UV << R"GLSL() in vec2 uv;
	layout(location = )GLSL" << LAYOUT_COLOR << R"GLSL() in vec4 vertex_color;
	
	// GHOUl2
	layout(location = )GLSL" << LAYOUT_BONE_GROUP0 << R"GLSL() in uint vertex_bg0;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP1 << R"GLSL() in uint vertex_bg1;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP2 << R"GLSL() in uint vertex_bg2;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP3 << R"GLSL() in uint vertex_bg3;
	layout(location = )GLSL" << LAYOUT_BONE_WEIGHT << R"GLSL() in vec4 vertex_wgt;

	uniform float time;
	uniform mat4 mvp;
	uniform mat3 uvm;
	uniform bool use_vertex_color;
	uniform bool turb;
	uniform vec4 turb_data;
	
	uniform uint ghoul2;

	out vec2 f_uv;
	out vec4 vcolor;

	void main() {
	
		if (ghoul2 != 0) {
			
			uint num_groups = 1;
			if (vertex_bg1 != 255) num_groups++;
			if (vertex_bg2 != 255) num_groups++;
			if (vertex_bg3 != 255) num_groups++;
			
			switch(num_groups) {
				case 1:
					gl_Position = mvp * vec4((bone[vertex_bg0] * vec4(vert, 1)).xyz, 1);
					break;
					
				default:
				case 2: {
					vec3 v0 = (bone[vertex_bg0] * vec4(vert, 1)).xyz;
					vec3 v1 = (bone[vertex_bg1] * vec4(vert, 1)).xyz;
					
					gl_Position = mvp * vec4(vertex_wgt.x * (v0 - v1) + v1, 1);
					break;
				}
				
				case 3: {
					vec3 v0 = (bone[vertex_bg0] * vec4(vert, 1)).xyz;
					vec3 v1 = (bone[vertex_bg1] * vec4(vert, 1)).xyz;
					vec3 v2 = (bone[vertex_bg2] * vec4(vert, 1)).xyz;
					
					vec3 sum = vertex_wgt.x * v0;
					sum     += vertex_wgt.y * v1;
					sum     += vertex_wgt.z * v2;
					
					gl_Position = mvp * vec4(sum, 1);
					break;
				}
				
				case 4: {
					vec3 v0 = (bone[vertex_bg0] * vec4(vert, 1)).xyz;
					vec3 v1 = (bone[vertex_bg1] * vec4(vert, 1)).xyz;
					vec3 v2 = (bone[vertex_bg2] * vec4(vert, 1)).xyz;
					vec3 v3 = (bone[vertex_bg3] * vec4(vert, 1)).xyz;
					
					vec3 sum = vertex_wgt.x * v0;
					sum     += vertex_wgt.y * v1;
					sum     += vertex_wgt.z * v2;
					sum     += vertex_wgt.w * v3;
					
					gl_Position = mvp * vec4(sum, 1);
					break;
				}
			}
			
		} else {
			gl_Position = mvp * vec4(vert, 1);
		}
		
		if (use_vertex_color)
			vcolor = vertex_color;
		else
			vcolor = vec4(1, 1, 1, 1);
		
		f_uv = (uvm * vec3(uv, 1)).xy;
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
	uniform float time;
	uniform vec4 q3color;
	uniform uint mapgen;
	
	// TEXTURE BINDINGS
	layout(binding = )GLSL" << BINDING_DIFFUSE << R"GLSL() uniform sampler2D tex;
	
	in vec2 f_uv;
	in vec4 vcolor;
	
	layout(location = 0) out vec4 color;
	
	float rand(vec2 co){
		return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
	}
	
	float rand_value(float seed) {
		float freq = sin(pow(mod(seed, 10.0)+10.0, 1.9));
		return rand(vec2(gl_FragCoord.x, gl_FragCoord.y) + mod(seed, freq));
	}
	
	void main() {
	
		vec4 scolor;
		
		switch(mapgen) {
			case 0: // diffuse
				scolor = texture(tex, f_uv);
				break;
			default:
			case 1: { // mnoise
				float v = rand_value(time);
				scolor = vec4(v, v, v, 1);
			} break;
			case 2: { // cnoise
				scolor = vec4(rand_value(time), rand_value(time+1), rand_value(time+2), 1);
			} break;
			case 3: { // anoise
				float v = rand_value(time);
				scolor = vec4(1, 1, 1, v);
			} break;
			case 4: { // enoise
				scolor = vec4(rand_value(time), rand_value(time+1), rand_value(time+2), rand_value(time+3));
			} break;
		}
		color = scolor * q3color * vcolor;
	}
	)GLSL";

	return ss.str();
}

struct programs::q3main::private_data {
	uniform_float m_time = 0;
	uniform_mat4 m_mvp = qm::mat4_t::identity();
	uniform_mat3 m_uv = qm::mat3_t::identity();
	uniform_vec4 m_color = qm::vec4_t {1, 1, 1, 1};
	uniform_bool m_use_vertex_colors = false;
	uniform_bool m_turb = false;
	uniform_vec4 m_turb_data = qm::vec4_t {0, 0, 0, 0};
	uniform_uint m_bones = 0;
	uniform_uint m_mapgen = 0;
	
	GLint bone_matricies_binding;
	GLuint bone_matricies_buffer = 0;
	
	void reset() {
		m_time.reset();
		m_mvp.reset();
		m_uv.reset();
		m_color.reset();
		m_use_vertex_colors.reset();
		m_turb.reset();
		m_turb_data.reset();
		m_bones.reset();
		m_mapgen.reset();
	}
	
	void push() {
		m_time.push();
		m_mvp.push();
		m_uv.push();
		m_color.push();
		m_use_vertex_colors.push();
		m_turb.push();
		m_turb_data.push();
		m_bones.push();
		m_mapgen.push();
	}
};

programs::q3main::q3main() : m_data(new private_data) {
	
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
	
	if (m_data->m_time.set_location(get_location("time")) == -1)							
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"time\"");
	if (m_data->m_mvp.set_location(get_location("mvp")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"mvp\"");
	if (m_data->m_uv.set_location(get_location("uvm")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"uvm\"");
	if (m_data->m_color.set_location(get_location("q3color")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"q3color\"");
	if (m_data->m_use_vertex_colors.set_location(get_location("use_vertex_color")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"use_vertex_color\"");
	if (m_data->m_turb.set_location(get_location("turb")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"turb\"");
	if (m_data->m_turb_data.set_location(get_location("turb_data")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"turb_data\"");
	if (m_data->m_bones.set_location(get_location("ghoul2")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"ghoul2\"");
	if (m_data->m_mapgen.set_location(get_location("mapgen")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"mapgen\"");
		
	m_data->bone_matricies_binding = glGetUniformBlockIndex(get_handle(), "BoneMatricies");
	if (m_data->bone_matricies_binding == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform buffer binding for \"BoneMatricies\"");
	
	glCreateBuffers(1, &m_data->bone_matricies_buffer);
	glUniformBlockBinding(get_handle(), m_data->bone_matricies_binding, BINDING_BONE_MATRICIES);
}

programs::q3main::~q3main() {
	glDeleteBuffers(1, &m_data->bone_matricies_buffer);
}

void programs::q3main::bind_and_reset() {
	m_data->reset();
	bind();
}

void programs::q3main::on_bind() {
	m_data->push();
}

void programs::q3main::time(float const & v) {
	m_data->m_time = v;
	if (is_bound()) m_data->m_time.push();
}

void programs::q3main::mvp(qm::mat4_t const & v) {
	m_data->m_mvp = v;
	if (is_bound()) m_data->m_mvp.push();
}

void programs::q3main::uvm(qm::mat3_t const & v) {
	m_data->m_uv = v;
	if (is_bound()) m_data->m_uv.push();
}

void programs::q3main::color(qm::vec4_t const & v) {
	m_data->m_color = v;
	if (is_bound()) m_data->m_color.push();
}

void programs::q3main::use_vertex_colors(bool const & v) {
	m_data->m_use_vertex_colors = v;
	if (is_bound()) m_data->m_use_vertex_colors.push();
}

void programs::q3main::turb(bool const & v) {
	m_data->m_turb = v;
	if (is_bound()) m_data->m_turb.push();
}

void programs::q3main::turb_data(q3stage::tx_turb const & v) {
	m_data->m_turb_data = { v.amplitude, v.base, v.frequency, v.phase };
	if (is_bound()) m_data->m_turb_data.push();
}

void programs::q3main::mapgen(uint8_t v) {
	m_data->m_mapgen = v;
	if (is_bound()) m_data->m_mapgen.push();
}

void programs::q3main::bone_matricies(qm::mat4_t const * ptr, size_t num) {
	assert(is_bound());
	
	if (ptr && num) {
		m_data->m_bones = num;
		m_data->m_bones.push();
		
		glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_BONE_MATRICIES, m_data->bone_matricies_buffer);
		glNamedBufferData(m_data->bone_matricies_buffer, num * sizeof(qm::mat4_t), ptr, GL_STATIC_DRAW);
		
	} else {
		m_data->m_bones = 0;
		m_data->m_bones.push();
	}
}
