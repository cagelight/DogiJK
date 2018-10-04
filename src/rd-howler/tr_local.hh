#pragma once

#include "glad.hh"
#include "rd-common/tr_public.hh"
#include "rd-common/tr_types.hh"

#include "hmath.hh"

typedef math::vec2_t<float> rv2_t;
typedef math::vec3_t<float> rv3_t;
typedef math::vec4_t<float> rv4_t;
typedef math::mat3_t<float> rm3_t;
typedef math::mat4_t<float> rm4_t;
typedef math::quaternion_t<float> rq_t;

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

struct q3model;
struct q3shader;
struct q3stage;
struct q3texture;

struct rcmd {
	enum struct mode_e {
		color_2d,
		stretch_pic,
		refent
	} mode;
	
	rcmd() = delete;
	rcmd(mode_e m) : mode(m) {}
	rcmd(rcmd const &) = default;
	rcmd(rcmd &&) = default;
	
	union {
		struct {
			float x, y, w, h, s1, t1, s2, t2;
			qhandle_t hShader;
		} stretch_pic;
		rv4_t color_2d;
		refEntity_t refent;
	};
};

struct frame_t {
	rm4_t vp;
	float shader_time = 0;
	std::vector<rcmd> cmds;
};

struct modelbank final {
	modelbank();
	~modelbank();
	modelbank(modelbank const &) = delete;
	modelbank(modelbank &&) = delete;
	
	qhandle_t register_model(char const * name, bool server = false);
	model_t * get_model(qhandle_t);
	
private:
	std::unordered_map<istring, qhandle_t> model_lookup;
	std::vector<q3model> models;
};

struct rend final {
	rend() = default;
	rend(rend const &) = delete;
	rend(rend &&) = delete;
	~rend();
	
	void initialize();
	
	// PUBLIC MODEL
	void setup_model(qhandle_t);
	void setup_world_model();
	
	// PUBLIC SHADER
	qhandle_t register_shader(char const * name, bool mipmaps = true);
	inline void set_color_2d(rv4_t const & v) { color_2d = v; }
	void shader_set_vp(rm4_t const & vp); // GLSL MVP is not updated until a call to shader_set_m
	void shader_set_m(rm4_t const & m, bool flip = false);
	void shader_setup_stage(q3stage const &, rm3_t const & uvm, float time);
	
	// PUBLIC TEXTURE
	GLuint register_texture(char const * name, bool mipmaps = true);
	
	void load_world(char const * name);
	qboolean get_entity_token(char *buffer, int size); // not entirely sure what this does
	
	void draw(std::shared_ptr<frame_t>, bool doing_3d);
	
	void swap();
	
private:
	bool initialized = false;
	window_t window;
	
	// MODEL
	void initialize_model();
	struct rendmesh final {
		rendmesh() = default;
		rendmesh(rendmesh const &) = delete;
		rendmesh(rendmesh &&);
		~rendmesh();
		inline void bind() const { glBindVertexArray(vao); }
		inline void draw() const { glDrawArrays(mode, 0, size); }
		GLuint vao = 0;
		GLuint vbo[3] {0};
		qhandle_t shader = 0;
		size_t size = 0;
		GLenum mode = GL_TRIANGLES;
	};
	struct rendmodel final {
		rendmodel() = default;
		std::vector<rendmesh> meshes;
	};
	rendmesh unitquad;
	rendmesh fullquad;
	std::unordered_map<qhandle_t, rendmodel> bankmodels;
	
	// SHADER
	void initialize_shader();
	void destruct_shader() noexcept;
	
	rm4_t vp;
	rv4_t color_2d;
	
	GLuint q3program = 0;
	GLuint q3sampler = 0;
	enum struct q3uniform : size_t {
		vertex_matrix,
		uv_matrix,
		q3color,
		max
	};
	GLuint q3uniforms[static_cast<size_t>(q3uniform::max)];
	inline GLuint & q3u(q3uniform const & u) { return q3uniforms[static_cast<size_t>(u)]; }
	
	std::unordered_map<istring, std::string> shader_source_lookup;
	std::unordered_map<istring, qhandle_t> shader_lookup;
	std::vector<q3shader> shaders;
	
// TEXTURE
	void initialize_texture();
	void destruct_texture() noexcept;
	std::unordered_map<istring, GLuint> texture_lookup;
	GLuint whiteimage;
	
// WORLD
	void initialize_world();
	void destruct_world() noexcept;
	void * world_data = nullptr;
};

struct q3model {
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

struct q3shader {
	bool in_use = false;
	istring name;
	qhandle_t index;	
	std::vector<q3stage> stages;
};

struct q3stage {
	
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

struct q3texture {
	bool in_use;
	GLuint id;
};

extern std::unique_ptr<modelbank> mbank;
extern std::unique_ptr<rend> r;

extern std::shared_ptr<frame_t> frame2d;
extern std::shared_ptr<frame_t> frame3d;
