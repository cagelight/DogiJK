#include "../hw_local.hh"
using namespace howler;

static constexpr GLint location_mvp = 1;
static constexpr GLint location_g2 = 2;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout (std140) uniform BoneMatricies {
		mat4 bone[72];
	};
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	
	layout(location = )GLSL" << LAYOUT_BONE_GROUP0 << R"GLSL() in uint vertex_bg0;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP1 << R"GLSL() in uint vertex_bg1;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP2 << R"GLSL() in uint vertex_bg2;
	layout(location = )GLSL" << LAYOUT_BONE_GROUP3 << R"GLSL() in uint vertex_bg3;
	layout(location = )GLSL" << LAYOUT_BONE_WEIGHT << R"GLSL() in vec4 vertex_wgt;
	
	layout(location = )GLSL" << location_mvp << R"GLSL() uniform mat4 vertex_matrix;
	layout(location = )GLSL" << location_g2 << R"GLSL() uniform uint ghoul2;

	out vec4 vcolor;

	void main() {
	
		if (ghoul2 != 0) {
			
			uint num_groups = 1;
			if (vertex_bg1 != 255) num_groups++;
			if (vertex_bg2 != 255) num_groups++;
			if (vertex_bg3 != 255) num_groups++;
			
			switch(num_groups) {
				case 1:
					gl_Position = vertex_matrix * vec4((bone[vertex_bg0] * vec4(vert, 1)).xyz, 1);
					break;
					
				default:
				case 2: {
					vec3 v0 = (bone[vertex_bg0] * vec4(vert, 1)).xyz;
					vec3 v1 = (bone[vertex_bg1] * vec4(vert, 1)).xyz;
					
					gl_Position = vertex_matrix * vec4(vertex_wgt.x * (v0 - v1) + v1, 1);
					break;
				}
				
				case 3: {
					vec3 v0 = (bone[vertex_bg0] * vec4(vert, 1)).xyz;
					vec3 v1 = (bone[vertex_bg1] * vec4(vert, 1)).xyz;
					vec3 v2 = (bone[vertex_bg2] * vec4(vert, 1)).xyz;
					
					vec3 sum = vertex_wgt.x * v0;
					sum     += vertex_wgt.y * v1;
					sum     += vertex_wgt.z * v2;
					
					gl_Position = vertex_matrix * vec4(sum, 1);
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
					
					gl_Position = vertex_matrix * vec4(sum, 1);
					break;
				}
			}
			
		} else {
			gl_Position = vertex_matrix * vec4(vert, 1);
		}
	
		vcolor = vec4(1, 1, 1, 0.75);
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

struct programs::q3line::private_data {
	uniform_mat4 m_mvp = qm::mat4_t::identity();
	uniform_uint m_bones = 0;
	
	GLint bone_matricies_binding;
	GLuint bone_matricies_buffer = 0;
	
	void reset() {
		m_mvp.reset();
		m_bones.reset();
	}
	
	void push() {
		m_mvp.push();
		m_bones.push();
	}
};

programs::q3line::q3line() : m_data(new private_data) {
	
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
	
	m_data->m_mvp.set_location(location_mvp);
	m_data->m_bones.set_location(location_g2);
	
	m_data->bone_matricies_binding = glGetUniformBlockIndex(get_handle(), "BoneMatricies");
	if (m_data->bone_matricies_binding == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform buffer binding for \"BoneMatricies\"");
	
	glCreateBuffers(1, &m_data->bone_matricies_buffer);
	glUniformBlockBinding(get_handle(), m_data->bone_matricies_binding, BINDING_BONE_MATRICIES);
}

programs::q3line::~q3line() {
	glDeleteBuffers(1, &m_data->bone_matricies_buffer);
}

void programs::q3line::on_bind() {
	m_data->m_mvp.push();
}

void programs::q3line::mvp(qm::mat4_t const & v) {
	m_data->m_mvp = v;
	if (is_bound()) m_data->m_mvp.push();
}

void programs::q3line::bone_matricies(qm::mat4_t const * ptr, size_t num) {
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
