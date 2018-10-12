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

struct basemodel {
	inline basemodel() { memset(&mod, 0, sizeof(mod)); }
	basemodel(basemodel const &) = delete;
	basemodel(basemodel && other) = delete;
	~basemodel() = default;
	
	std::vector<char> buffer;
	model_t mod;
};

using basemodel_ptr = std::shared_ptr<basemodel>;

struct modelbank final {
	modelbank();
	~modelbank();
	modelbank(modelbank const &) = delete;
	modelbank(modelbank &&) = delete;
	
	qhandle_t register_model(char const * name, bool server = false);
	basemodel_ptr get_model(qhandle_t);
	
private:
	std::unordered_map<istring, qhandle_t> model_lookup;
	std::vector<basemodel_ptr> models;
};

struct q3mesh final {
	q3mesh() = default;
	q3mesh(q3mesh const &) = delete;
	q3mesh(q3mesh &&);
	~q3mesh();
	inline void bind() const { glBindVertexArray(vao); }
	inline void draw() const { glDrawArrays(mode, 0, size); }
	GLuint vao = 0;
	GLuint vbo[3] {0};
	q3shader_ptr shader;
	size_t size = 0;
	GLenum mode = GL_TRIANGLES;
};

struct q3model final {
	q3model() = default;
	istring name;
	basemodel_ptr base;
	std::vector<q3mesh> meshes;
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

static constexpr float q3sort_opaque = 3.0f;
static constexpr float q3sort_basetrans = 14.0f;

struct q3shader {
	q3shader() = default;
	q3shader(q3shader const &) = delete;
	q3shader(q3shader &&) = delete;
	~q3shader() = default;
	
	enum struct cull_type {
		front,
		back,
		both
	} cull = cull_type::front;
	
	istring name;
	qhandle_t index = 0;
	bool mipmaps = false;
	
	bool opaque = true;
	float sort = q3sort_opaque;
	
	std::vector<q3stage> stages;
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

struct basic_mesh {
	
};

using cmd2d =
std::variant<
	rv4_t,
	stretch_pic
>;

struct frame_t {
	rm4_t vp;
	float shader_time = 0;
	std::vector<cmd2d> cmds2d;
};

extern std::shared_ptr<frame_t> r_frame;

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
		
	q3mesh unitquad;
	q3mesh fullquad;
	std::vector<q3model_ptr> models;
	
	void model_load(qhandle_t);
	q3model_ptr model_get(qhandle_t);
	
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
	std::unordered_map<istring, q3shader_ptr> shader_lookup;
	std::vector<q3shader_ptr> shaders;
	
	q3shader_ptr & shader_register(char const * name, bool mipmaps = true);
	inline q3shader_ptr & shader_get(qhandle_t h) { return (h < 0 || h >= shaders.size()) ? shaders[0] : shaders[h]; }
	void shader_set_mvp(rm4_t const & m);
	void shader_presetup(q3shader const &);
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
	
	void world_load(char const * name);
	qboolean get_entity_token(char *buffer, int size); // not entirely sure what this does
	
	q3model opaque_world;
	q3model trans_world;
	
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
