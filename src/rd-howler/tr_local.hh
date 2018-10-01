#pragma once

#include "glad.hh"
#include "rd-common/tr_public.hh"
#include "rd-common/tr_types.hh"

#include <asterid/brassica.hh>

namespace b = asterid::brassica;
typedef b::vec2_t<float> rv2_t;
typedef b::vec3_t<float> rv3_t;
typedef b::vec4_t<float> rv4_t;
typedef b::mat3_t<float> rm3_t;

extern refimport_t ri;

extern glconfig_t glConfig;

extern cvar_t * r_aspectCorrectFonts;

// fore tr_font
void RE_SetColor ( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
qhandle_t RE_RegisterShaderNoMip ( const char *name );

typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
/*
Ghoul2 Insert Start
*/
	SF_MDX,
/*
Ghoul2 Insert End
*/
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0xffffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

struct rend final {
	rend() = default;
	rend(rend const &) = delete;
	rend(rend &&) = delete;
	~rend();
	
	void initialize();
	
	qhandle_t register_model(char const * name);
	model_t * get_model(qhandle_t);
	
	qhandle_t register_shader(char const * name, bool mipmaps = true);
	
	GLuint register_texture(char const * name, bool mipmaps = true);
	
	struct q3model;
	struct q3shader;
	struct q3texture;
	
private:
	bool initialized = false;
	window_t window;
	
// MODEL
	void initialize_model();
	void destruct_model() noexcept;
	std::unordered_map<istring, qhandle_t> model_lookup;
	std::vector<q3model> models;
	
// SHADER
	void initialize_shader();
	void destruct_shader() noexcept;
	std::unordered_map<istring, std::string> shader_source_lookup;
	std::unordered_map<istring, qhandle_t> shader_lookup;
	std::vector<q3shader> shaders;
	
// TEXTURE
	void initialize_texture();
	void destruct_texture() noexcept;
	std::unordered_map<istring, GLuint> texture_lookup;
	GLuint whiteimage;
};

struct rend::q3model {
	q3model() = default;
	q3model(q3model const &) = delete;
	q3model(q3model && other) noexcept {
		buffer = std::move(other.buffer);
		ptr = other.ptr;
		other.ptr = nullptr;
	}
	~q3model() {
		if (ptr) delete ptr;
	}
	std::vector<char> buffer;
	model_t * ptr = nullptr;
	
	q3model & operator == (q3model &&) {
		return *this;
	}
};

struct rend::q3shader {
	bool in_use = false;
	istring name;
	qhandle_t index;
	
	enum struct gen_func {
		sine,
		square,
		triangle,
		sawtooth,
		inverse_sawtooth,
		noise,
		random,
	};
	struct texmod {
		enum struct tctype {
			turb,
			scale,
			scroll,
			stretch,
			transform,
			rotate,
			//entityTranslate, ???
		} type;
		
		texmod() = delete;
		texmod(tctype t) : type(t) {}

		union {
			struct {
				float base;
				float amplitude;
				float phase;
				float frequency;
			} turb_data;
			struct {
				rv2_t scale;
			} scale_data;
			struct {
				rv2_t scroll;
			} scroll_data;
			struct {
				gen_func gfunc;
				float base;
				float amplitude;
				float phase;
				float frequency;
			} stretch_data;
			struct {
				rm3_t trans;
			} transform_data;
			struct {
				float rot;
			} rotate_data;
		};
	};
	struct stage {
		
		enum struct gen_type {
			none,
			constant,
			vertex,
			wave,
		} gen_rgb = gen_type::none, gen_alpha = gen_type::none;
		
		GLuint diffuse = 0; // GL Texture Handle
		bool clamp = false;
		GLenum blend_src = GL_ONE, blend_dst = GL_ZERO;
		
		rv4_t color {1, 1, 1, 1};
		
		struct {
			struct {
				gen_func func;
				float base;
				float amplitude;
				float phase;
				float frequency;
			} rgb;
			struct {
				gen_func func;
				float base;
				float amplitude;
				float phase;
				float frequency;
			} alpha;
		} wave;
		
		std::vector<texmod> texmods;
	};
	
	std::vector<stage> stages;
};

struct rend::q3texture {
	bool in_use;
	GLuint id;
};

extern std::unique_ptr<rend> r;
