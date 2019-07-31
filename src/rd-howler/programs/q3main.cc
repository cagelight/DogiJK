#include "../hw_local.hh"
using namespace howler;

static std::string generate_vertex_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout (std140) uniform BoneMatricies {
		mat4 bone[72];
	};
	
	layout (std140) uniform LightStyles {
		vec4 lightstyles[64];
	};
	
	layout (std140) uniform GridLighting {
		// treat as vec3, vec4s used for consistent padding
		vec4 grid_ambient;
		vec4 grid_directed;
		vec4 grid_direction;
	};
		
	layout(location = )GLSL" << LAYOUT_VERTEX << R"GLSL() in vec3 vert;
	layout(location = )GLSL" << LAYOUT_UV << R"GLSL() in vec2 uv;
	layout(location = )GLSL" << LAYOUT_NORMAL << R"GLSL() in vec3 normal;
	layout(location = )GLSL" << LAYOUT_BONE_GROUPS << R"GLSL() in ivec4 vertex_bg;
	layout(location = )GLSL" << LAYOUT_BONE_WEIGHT << R"GLSL() in vec4 vertex_wgt;
	layout(location = )GLSL" << LAYOUT_COLOR0 << R"GLSL() in vec4 color0;
	layout(location = )GLSL" << LAYOUT_COLOR1 << R"GLSL() in vec4 color1;
	layout(location = )GLSL" << LAYOUT_LMUV01_COLOR2 << R"GLSL() in vec4 lm_color2_uv01;
	layout(location = )GLSL" << LAYOUT_LMUV23_COLOR3 << R"GLSL() in vec4 lm_color3_uv23;
	layout(location = )GLSL" << LAYOUT_LMSTYLES << R"GLSL() in ivec4 lm_styles;

	uniform float time;
	
	uniform mat4 mvp;
	uniform mat4 m;
	
	uniform mat3 uvm;
	
	uniform vec3 view_origin;
	uniform bool use_vertex_color;
	uniform bool turb;
	uniform vec4 turb_data;
	uniform uint mapgen;
	uniform uint lm_mode;
	uniform uint tcgen;
	uniform uint ghoul2;

	// 0 = none, 1 = diffuse, 2 = specular
	uniform uint rgbgen;
	uniform uint alphagen;
	// --------
	
	out vec2 f_uv;
	out vec4 vcolor;
	out vec2 lm_uv[4];
	flat out ivec4 lm_styles_frag;

	void main() {
	
		vec3 calcnorm = normal;
	
		if (ghoul2 != 0) {
			
			uint num_groups = 1;
			if (vertex_bg.y != 255) num_groups++;
			if (vertex_bg.z != 255) num_groups++;
			if (vertex_bg.w != 255) num_groups++;
			
			calcnorm.x = dot(bone[vertex_bg.x][0].xyz, normal);
			calcnorm.y = dot(bone[vertex_bg.x][1].xyz, normal);
			calcnorm.z = dot(bone[vertex_bg.x][2].xyz, normal);
			
			switch(num_groups) {
				default:
				case 1:
					
					vec3 bvert;
					bvert.x = dot(bone[vertex_bg.x][0].xyz, vert) + bone[vertex_bg.x][3].x;
					bvert.y = dot(bone[vertex_bg.x][1].xyz, vert) + bone[vertex_bg.x][3].y;
					bvert.z = dot(bone[vertex_bg.x][2].xyz, vert) + bone[vertex_bg.x][3].z;
					
					gl_Position = mvp * vec4(bvert, 1);
					break;
				
				case 2: {
				
					vec3 bvert0;
					bvert0.x = dot(bone[vertex_bg.x][0].xyz, vert) + bone[vertex_bg.x][3].x;
					bvert0.y = dot(bone[vertex_bg.x][1].xyz, vert) + bone[vertex_bg.x][3].y;
					bvert0.z = dot(bone[vertex_bg.x][2].xyz, vert) + bone[vertex_bg.x][3].z;
					
					vec3 bvert1;
					bvert1.x = dot(bone[vertex_bg.y][0].xyz, vert) + bone[vertex_bg.y][3].x;
					bvert1.y = dot(bone[vertex_bg.y][1].xyz, vert) + bone[vertex_bg.y][3].y;
					bvert1.z = dot(bone[vertex_bg.y][2].xyz, vert) + bone[vertex_bg.y][3].z;
					
					gl_Position = mvp * vec4(vertex_wgt.x * (bvert0 - bvert1) + bvert1, 1);
					break;
				}
				
				case 3: {
					
					vec3 bvert0;
					bvert0.x = dot(bone[vertex_bg.x][0].xyz, vert) + bone[vertex_bg.x][3].x;
					bvert0.y = dot(bone[vertex_bg.x][1].xyz, vert) + bone[vertex_bg.x][3].y;
					bvert0.z = dot(bone[vertex_bg.x][2].xyz, vert) + bone[vertex_bg.x][3].z;
					
					vec3 bvert1;
					bvert1.x = dot(bone[vertex_bg.y][0].xyz, vert) + bone[vertex_bg.y][3].x;
					bvert1.y = dot(bone[vertex_bg.y][1].xyz, vert) + bone[vertex_bg.y][3].y;
					bvert1.z = dot(bone[vertex_bg.y][2].xyz, vert) + bone[vertex_bg.y][3].z;
					
					vec3 bvert2;
					bvert2.x = dot(bone[vertex_bg.z][0].xyz, vert) + bone[vertex_bg.z][3].x;
					bvert2.y = dot(bone[vertex_bg.z][1].xyz, vert) + bone[vertex_bg.z][3].y;
					bvert2.z = dot(bone[vertex_bg.z][2].xyz, vert) + bone[vertex_bg.z][3].z;
					
					vec3 sum = vertex_wgt.x * bvert0;
					sum     += vertex_wgt.y * bvert1;
					sum     += vertex_wgt.z * bvert2;
					
					gl_Position = mvp * vec4(sum, 1);
					break;
				}
				
				case 4: {
					
					vec3 bvert0;
					bvert0.x = dot(bone[vertex_bg.x][0].xyz, vert) + bone[vertex_bg.x][3].x;
					bvert0.y = dot(bone[vertex_bg.x][1].xyz, vert) + bone[vertex_bg.x][3].y;
					bvert0.z = dot(bone[vertex_bg.x][2].xyz, vert) + bone[vertex_bg.x][3].z;
					
					vec3 bvert1;
					bvert1.x = dot(bone[vertex_bg.y][0].xyz, vert) + bone[vertex_bg.y][3].x;
					bvert1.y = dot(bone[vertex_bg.y][1].xyz, vert) + bone[vertex_bg.y][3].y;
					bvert1.z = dot(bone[vertex_bg.y][2].xyz, vert) + bone[vertex_bg.y][3].z;
					
					vec3 bvert2;
					bvert2.x = dot(bone[vertex_bg.z][0].xyz, vert) + bone[vertex_bg.z][3].x;
					bvert2.y = dot(bone[vertex_bg.z][1].xyz, vert) + bone[vertex_bg.z][3].y;
					bvert2.z = dot(bone[vertex_bg.z][2].xyz, vert) + bone[vertex_bg.z][3].z;
					
					vec3 bvert3;
					bvert3.x = dot(bone[vertex_bg.w][0].xyz, vert) + bone[vertex_bg.w][3].x;
					bvert3.y = dot(bone[vertex_bg.w][1].xyz, vert) + bone[vertex_bg.w][3].y;
					bvert3.z = dot(bone[vertex_bg.w][2].xyz, vert) + bone[vertex_bg.w][3].z;
					
					vec3 sum = vertex_wgt.x * bvert0;
					sum     += vertex_wgt.y * bvert1;
					sum     += vertex_wgt.z * bvert2;
					sum     += vertex_wgt.w * bvert3;
					
					gl_Position = mvp * vec4(sum, 1);
					break;
				}
			}
			
		} else {
			gl_Position = mvp * vec4(vert, 1);
		}
		
		calcnorm = mat3(m) * calcnorm;
		
		if (ghoul2 != 0) calcnorm.y = -calcnorm.y; // FIXME -- I don't know why I have to do this
		
		if (mapgen == 2) { // normals
			//vcolor = vec4(calcnorm / 2 + 0.5, 1);
			vcolor.xyz = calcnorm;
		} else if (lm_mode != 0 && mapgen == 1) {
			lm_styles_frag = lm_styles;
			switch (lm_mode) {
				case 1: {
					vcolor = vec4(1, 1, 1, 1);
					lm_uv[0] = lm_color2_uv01.xy;
					lm_uv[1] = lm_color2_uv01.zw;
					lm_uv[2] = lm_color3_uv23.xy;
					lm_uv[3] = lm_color3_uv23.zw;
				} break;
				default:
				case 2: { // VERTEX LIGHTING
					
					if (lm_styles.x == 0) {
						vcolor = color0;
					} else if (lm_styles.x < 0xFE) {
						vcolor = color0 * lightstyles[lm_styles.x];
					} else break;
					
					if (lm_styles.y == 0) {
						vcolor += color1;
					} else if (lm_styles.y < 0xFE) {
						vcolor += color1 * lightstyles[lm_styles.y];
					} else break;
					
					if (lm_styles.z == 0) {
						vcolor += lm_color2_uv01;
					} else if (lm_styles.z < 0xFE) {
						vcolor += lm_color2_uv01 * lightstyles[lm_styles.z];
					} else break;
					
					if (lm_styles.w == 0) {
						vcolor += lm_color3_uv23;
					} else if (lm_styles.w < 0xFE) {
						vcolor += lm_color3_uv23 * lightstyles[lm_styles.w];
					} else break;
					
				} break;
			}
		} else if (use_vertex_color) {
			vcolor = color0;
		} else {
			vcolor = vec4(1, 1, 1, 1);
		}
		
		if (tcgen == 0)
			f_uv = uv;
		else if (tcgen == 1) { // environment
			vec3 dir = normalize(view_origin - (m * vec4(vert, 1)).xyz);
			float d = dot(normal, dir);
			f_uv.x = 0.5 + (normal.y * 2 * d - dir.y) * 0.5;
			f_uv.y = 0.5 - (normal.z * 2 * d - dir.z) * 0.5;
		}
		else if (tcgen == 2 && lm_mode != 2) { // lightmap
			f_uv = lm_color2_uv01.xy;
		}
		
		f_uv = (uvm * vec3(f_uv, 1)).xy;
		
		if (turb) {
			f_uv.x += sin((vert[0] + vert[2]) * (turb_data.w + time * turb_data.z) / 1024.0f) * turb_data.x;
			f_uv.y += sin((vert[1]) * (turb_data.w + time * turb_data.z) / 1024.0f) * turb_data.x;
		}
		
		if (rgbgen == 1 && mapgen != 2) {
			vcolor.xyz = grid_ambient.xyz + 0.125;
			float dir_frac = dot(calcnorm, grid_direction.xyz);
			if (dir_frac < 0) dir_frac = 0;
			vcolor.xyz += dir_frac * grid_directed.xyz;
		}
		
		if (alphagen == 2 && mapgen != 2) {
			/*
			mat3 itm2 = transpose(inverse(mat3(m)));
			vec3 calcnorm = normalize(itm2 * normal);
			
			float dir_frac = 2 * dot(calcnorm, grid_direction.xyz);
			vec3 reflected = calcnorm * dir_frac - grid_direction.xyz;
			vec3 viewer = view_origin - (m * vec4(vert, 1)).xyz;
			float l = dot(reflected, viewer) * length(viewer);
			if (l < 0) vcolor.w = 0;
			vcolor.w = clamp(pow(l, 3), 0, 1);
			*/
			vcolor.w = 0;
		}
	}
	)GLSL";

	return ss.str();
}

static std::string generate_fragment_shader() {
	std::stringstream ss;
	ss << R"GLSL(
	#version 450
		
	layout (std140) uniform LightStyles {
		vec4 lightstyles[64];
	};
	
	// UNIFORMS
	uniform float time;
	uniform vec4 q3color;
	uniform uint mapgen;
	uniform uint lm_mode;
	
	// TEXTURE BINDINGS
	layout(binding = )GLSL" << BINDING_DIFFUSE << R"GLSL() uniform sampler2D tex;
	layout(binding = )GLSL" << BINDING_LIGHTMAP << R"GLSL() uniform sampler2D lm;
	
	in vec2 f_uv;
	in vec4 vcolor;
	in vec2 lm_uv[4];
	flat in ivec4 lm_styles_frag;
	
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
		
		if (mapgen != 0) {
			switch(mapgen) {
				case 1: { // lightmap
					switch (lm_mode) {
						case 1: {
							
							if (lm_styles_frag.x == 0) {
								scolor = texture(lm, lm_uv[0]);
							} else if (lm_styles_frag.x < 0xFE) {
								scolor = texture(lm, lm_uv[0]) * lightstyles[lm_styles_frag.x];
							} else break;
							
							if (lm_styles_frag.y == 0) {
								scolor += texture(lm, lm_uv[1]);
							} else if (lm_styles_frag.y < 0xFE) {
								scolor += texture(lm, lm_uv[1]) * lightstyles[lm_styles_frag.y];
							} else break;
							
							if (lm_styles_frag.z == 0) {
								scolor += texture(lm, lm_uv[2]);
							} else if (lm_styles_frag.z < 0xFE) {
								scolor += texture(lm, lm_uv[2]) * lightstyles[lm_styles_frag.z];
							} else break;
							
							if (lm_styles_frag.w == 0) {
								scolor += texture(lm, lm_uv[3]);
							} else if (lm_styles_frag.w < 0xFE) {
								scolor += texture(lm, lm_uv[3]) * lightstyles[lm_styles_frag.w];
							} else break;
							
						} break;
						case 2:
							scolor = vec4(1, 1, 1, 1);
							break;
					}
				} break;
				case 2: // normals
					scolor = vec4(1, 1, 1, 1);
					break; // do nothing because vertex color is used
				default:
				case 3: { // mnoise
					float v = rand_value(time);
					scolor = vec4(v, v, v, 1);
				} break;
				case 4: { // anoise
					float v = rand_value(time);
					scolor = vec4(1, 1, 1, v);
				} break;
			}
		} else
			scolor = texture(tex, f_uv);
		
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
	uniform_uint m_lmmode = 0;
	uniform_uint m_bones = 0;
	uniform_uint m_mapgen = 0;
	uniform_vec3 m_viewpos = qm::vec3_t {0, 0, 0};
	uniform_uint m_tcgen = 0;
	//uniform_mat3 m_itm = qm::mat3_t::identity(); // inverse transpose model
	uniform_mat4 m_m = qm::mat4_t::identity();
	
	uniform_uint m_genrgb = 0;
	uniform_uint m_genalpha = 0;
	
	GLint bone_matricies_binding;
	GLuint bone_matricies_buffer = 0;
	
	GLint lightstyles_binding;
	GLuint lightstyles_buffer = 0;
	
	GLint gridlighting_binding;
	GLuint gridlighting_buffer = 0;
	
	void reset() {
		m_time.reset();
		m_mvp.reset();
		m_uv.reset();
		m_color.reset();
		m_use_vertex_colors.reset();
		m_turb.reset();
		m_turb_data.reset();
		m_lmmode.reset();
		m_bones.reset();
		m_mapgen.reset();
		m_viewpos.reset();
		m_tcgen.reset();
		m_genrgb.reset();
		m_genalpha.reset();
		//m_itm.reset();
		m_m.reset();
	}
	
	void push() {
		m_time.push();
		m_mvp.push();
		m_uv.push();
		m_color.push();
		m_use_vertex_colors.push();
		m_turb.push();
		m_turb_data.push();
		m_lmmode.push();
		m_bones.push();
		m_mapgen.push();
		m_viewpos.push();
		m_tcgen.push();
		m_genrgb.push();
		m_genalpha.push();
		//m_itm.push();
		m_m.push();
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
	//if (m_data->m_itm.set_location(get_location("itm")) == -1)
	//	Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"itm\"");
	if (m_data->m_m.set_location(get_location("m")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"m\"");
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
	if (m_data->m_lmmode.set_location(get_location("lm_mode")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"lm_mode\"");
	if (m_data->m_bones.set_location(get_location("ghoul2")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"ghoul2\"");
	if (m_data->m_mapgen.set_location(get_location("mapgen")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"mapgen\"");
	if (m_data->m_viewpos.set_location(get_location("view_origin")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"view_origin\"");
	if (m_data->m_tcgen.set_location(get_location("tcgen")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"tcgen\"");
	if (m_data->m_genrgb.set_location(get_location("rgbgen")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"rgbgen\"");
	if (m_data->m_genalpha.set_location(get_location("alphagen")) == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform location for \"alphagen\"");
		
	m_data->bone_matricies_binding = glGetUniformBlockIndex(get_handle(), "BoneMatricies");
	if (m_data->bone_matricies_binding == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform buffer binding for \"BoneMatricies\"");
	
	m_data->lightstyles_binding = glGetUniformBlockIndex(get_handle(), "LightStyles");
	if (m_data->lightstyles_binding == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform buffer binding for \"LightStyles\"");
	
	m_data->gridlighting_binding = glGetUniformBlockIndex(get_handle(), "GridLighting");
	if (m_data->gridlighting_binding == -1)
		Com_Error(ERR_FATAL, "programs::q3main: could not find uniform buffer binding for \"GridLighting\"");
	
	glCreateBuffers(1, &m_data->bone_matricies_buffer);
	glUniformBlockBinding(get_handle(), m_data->bone_matricies_binding, BINDING_BONE_MATRICIES);
	
	glCreateBuffers(1, &m_data->lightstyles_buffer);
	glUniformBlockBinding(get_handle(), m_data->lightstyles_binding, BINDING_LIGHTSTYLES);
	
	glCreateBuffers(1, &m_data->gridlighting_buffer);
	glUniformBlockBinding(get_handle(), m_data->gridlighting_binding, BINDING_GRIDLIGHTING);
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

void programs::q3main::itm(qm::mat3_t const & v) {
	//m_data->m_itm = v;
	//if (is_bound()) m_data->m_itm.push();
}

void programs::q3main::m(qm::mat4_t const & v) {
	m_data->m_m = v;
	if (is_bound()) m_data->m_m.push();
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

void programs::q3main::lm_mode(GLuint const & v) {
	m_data->m_lmmode = v;
	if (is_bound()) m_data->m_lmmode.push();
}

void programs::q3main::mapgen(q3stage::map_gen v) {
	m_data->m_mapgen = static_cast<uint8_t>(v);
	if (is_bound()) m_data->m_mapgen.push();
}

void programs::q3main::viewpos(qm::vec3_t const & v) {
	m_data->m_viewpos = v;
	if (is_bound()) m_data->m_viewpos.push();
}

void programs::q3main::tcgen(q3stage::tcgen v) {
	m_data->m_tcgen = static_cast<uint8_t>(v);
	if (is_bound()) m_data->m_tcgen.push();
}

void programs::q3main::rgbgen(q3stage::gen_type gen) {
	switch(gen) {
		default:
			m_data->m_genrgb = 0;
			break;
		case q3stage::gen_type::diffuse_lighting:
		case q3stage::gen_type::diffuse_lighting_entity:
			m_data->m_genrgb = 1;
			break;
		case q3stage::gen_type::specular_lighting:
			m_data->m_genrgb = 2;
			break;
	}
	if (is_bound()) m_data->m_genrgb.push();
}

void programs::q3main::alphagen(q3stage::gen_type gen) {
	switch(gen) {
		default:
			m_data->m_genalpha = 0;
			break;
		case q3stage::gen_type::diffuse_lighting:
		case q3stage::gen_type::diffuse_lighting_entity:
			m_data->m_genalpha = 1;
			break;
		case q3stage::gen_type::specular_lighting:
			m_data->m_genalpha = 2;
			break;
	}
	if (is_bound()) m_data->m_genalpha.push();
}

void programs::q3main::bone_matricies(qm::mat4_t const * ptr, size_t num) {
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

void programs::q3main::lightstyles(lightstyles_t const & ls) {
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_LIGHTSTYLES, m_data->lightstyles_buffer);
	glNamedBufferData(m_data->lightstyles_buffer, sizeof(lightstyles_t), &ls, GL_STATIC_DRAW);
}

void programs::q3main::gridlighting(gridlighting_t const * ptr) {
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_GRIDLIGHTING, m_data->gridlighting_buffer);
	if (ptr) {
		glNamedBufferData(m_data->gridlighting_buffer, sizeof(gridlighting_t), ptr, GL_STATIC_DRAW);
	}
}
