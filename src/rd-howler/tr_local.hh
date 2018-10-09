#pragma once

#include <variant>

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
extern cvar_t * r_showtris;

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
struct q3skin;
struct q3stage;
struct q3texture;

using q3model_ptr = std::shared_ptr<q3model>;
using q3shader_ptr = std::shared_ptr<q3shader>;
using q3skin_ptr = std::shared_ptr<q3skin>;
using q3stage_ptr = std::shared_ptr<q3stage>;
using q3texture_ptr = std::shared_ptr<q3texture>;

// ================================================================
// MODEL
// ================================================================

struct modelbank final {
	modelbank();
	~modelbank();
	modelbank(modelbank const &) = delete;
	modelbank(modelbank &&) = delete;
	
	qhandle_t register_model(char const * name, bool server = false);
	model_t * get_model(qhandle_t);
	
private:
	std::unordered_map<istring, qhandle_t> model_lookup;
	std::vector<q3model_ptr> models;
};

struct q3model {
	inline q3model() { memset(&mod, 0, sizeof(mod)); }
	q3model(q3model const &) = delete;
	q3model(q3model && other) = delete;
	~q3model() = default;
	
	std::vector<char> buffer;
	model_t mod;
};

extern std::unique_ptr<modelbank> mbank;

// ================================================================
// SHADER
// ================================================================

#define LAYOUT_VERTCOORD 0
#define LAYOUT_TEXCOORD 1
#define LAYOUT_NORMAL 2

#define UNIFORM_VERTEX_MATRIX 0
#define UNIFORM_TEXCOORD_MATRIX 1
#define UNIFORM_TIME 3
#define UNIFORM_COLOR 4

struct q3stage {
	
	q3stage() = default;
	~q3stage() = default;
	q3stage(q3stage const &) = delete;
	q3stage(q3stage &&) = default;
	
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
	
	std::shared_ptr<q3texture const> diffuse;
	bool clamp = false;
	GLenum blend_src = GL_ONE, blend_dst = GL_ZERO;
	
	bool opaque = true;
	bool blend = false;
	
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

struct q3shader {
	q3shader() = default;
	q3shader(q3shader const &) = delete;
	q3shader(q3shader &&) = delete;
	~q3shader() = default;
	
	istring name;
	qhandle_t index = 0;	
	std::vector<q3stage> stages;
	bool opaque = true;
	bool mipmaps = false;
};

// ================================================================
// SKIN
// ================================================================

typedef struct skinSurface_s {
	char		name[MAX_QPATH];
	int64_t		shader; // must fill sizeof(void *)
} skinSurface_t;
static_assert(sizeof(skinSurface_t) == sizeof(_skinSurface_t));

struct q3skin {
	inline q3skin() {
		memset(&skin, 0, sizeof(skin_t));
	}
	q3skin(q3skin const &) = delete;
	q3skin(q3skin &&) = delete;
	~q3skin() = default;
	skin_t skin;
	std::vector<skinSurface_t> surfs;
};

struct skinbank final {
	skinbank() = default;
	~skinbank() = default;
	skinbank(skinbank const &) = delete;
	skinbank(skinbank &&) = delete;
	
	qhandle_t register_skin(char const * name, bool server = false);
	q3skin_ptr get_skin(qhandle_t);
private:
	qhandle_t hcounter = 0;
	std::unordered_map<istring, qhandle_t> skin_lookup;
	std::vector<q3skin_ptr> skins;
};

extern std::unique_ptr<skinbank> sbank;

// ================================================================
// TEXTURE
// ================================================================

struct q3texture {
	q3texture() = default;
	q3texture(q3texture const &) = delete;
	inline q3texture(q3texture && other) = delete;
	inline ~q3texture() {
		if (id) glDeleteTextures(1, &id);
	}
	
	GLuint id = 0;
	bool has_transparency = false;
};

// ================================================================
// FRAME
// ================================================================

struct stretch_pic {
	float x, y, w, h, s1, t1, s2, t2;
	q3shader_ptr shader;
};

using rcmd =
std::variant<
	rv4_t,
	stretch_pic,
	refEntity_t
>;

struct frame_t {
	rm4_t vp;
	float shader_time = 0;
	std::vector<rcmd> cmds;
};

extern std::shared_ptr<frame_t> frame2d;
extern std::shared_ptr<frame_t> frame3d;

// ================================================================
// REND
// ================================================================

struct rend final {
	
	rend() = default;
	rend(rend const &) = delete;
	rend(rend &&) = delete;
	~rend();
	
	void initialize();
	
	void draw(std::shared_ptr<frame_t>);
	void swap();
	
// ================================
// MODEL
// ================================
	
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
		istring name;
	};
	
	rendmesh unitquad;
	rendmesh fullquad;
	std::unordered_map<qhandle_t, rendmodel> models;
	
	void model_load(qhandle_t);
	
// ================================
// SHADER
// ================================
	
	struct gl_shader final {
		GLuint id;
		gl_shader(GLenum type) : id(glCreateShader(type)) {}
		gl_shader(gl_shader const &) = delete;
		gl_shader(gl_shader &&) = delete;
		~gl_shader() { glDeleteShader(id); }
		void source(char const * src) { glShaderSource(id, 1, &src, 0); }
		bool compile(std::string & complog) {
			complog.clear();
			GLint success, maxlen;
			glCompileShader(id);
			glGetShaderiv(id, GL_COMPILE_STATUS, &success);
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxlen);
			if (!maxlen) return success == GL_TRUE;
			char * logtemp = new char [maxlen + 1];
			glGetShaderInfoLog(id, maxlen, &maxlen, logtemp);
			complog = logtemp;
			delete [] logtemp;
			return success == GL_TRUE;
		}
	};
	
	struct gl_program final {
		GLuint id;
		gl_program() : id(glCreateProgram()) {}
		gl_program(gl_program const &) = delete;
		gl_program(gl_program &&) = delete;
		~gl_program() { glDeleteProgram(id); }
		inline void attach(gl_shader const & st) { glAttachShader(id, st.id); }
		inline bool link(std::string & complog) {
			complog.clear();
			GLint success, maxlen;
			glLinkProgram(id);
			glGetProgramiv(id, GL_LINK_STATUS, &success);
			glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxlen);
			if (!maxlen) return success == GL_TRUE;
			char * logtemp = new char [maxlen + 1];
			glGetProgramInfoLog(id, maxlen, &maxlen, logtemp);
			complog = logtemp;
			delete [] logtemp;
			return success == GL_TRUE;
		}
		inline void use() {
			glUseProgram(id);
		}
	};
	
	std::unique_ptr<gl_program> q3program;
	std::unique_ptr<gl_program> basic_color_program;
	std::unique_ptr<gl_program> missingnoise_program;
	
	rv4_t color_2d;
	
	std::unordered_map<istring, std::string> shader_source_lookup;
	std::unordered_map<istring, qhandle_t> shader_lookup;
	std::vector<q3shader_ptr> shaders;
	
	qhandle_t shader_register(char const * name, bool mipmaps = true);
	q3shader_ptr shader_get(qhandle_t h);
	void shader_set_mvp(rm4_t const & m);
	void shader_setup_stage(q3stage const &, rm3_t const & uvm, float time);
	
// ================================
// TEXTURE
// ================================
	
	GLuint q3sampler = 0;
	
	std::unordered_map<istring, std::shared_ptr<q3texture>> texture_lookup;
	
	std::shared_ptr<q3texture const> texture_register(char const * name, bool mipmaps = true);
	
// ================================
// WORLD
// ================================
	
	void load_world(char const * name);
	qboolean get_entity_token(char *buffer, int size); // not entirely sure what this does
	
private:
		
	bool initialized = false;
	window_t window;
	void * world_data = nullptr;
	
	void initialize_model();
	void initialize_shader();
	void initialize_texture();
	void initialize_world();
	
	void destruct_world() noexcept;
};

extern std::unique_ptr<rend> r;
