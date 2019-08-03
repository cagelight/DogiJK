#pragma once

#include "tr_local.hh"
#include "glad.hh"

namespace howler {
	
//================================================================
// COMMON TYPES
//================================================================
	
	#define FUNDAMENTAL(name)\
	struct name;\
	using name##_ptr = std::shared_ptr<name>;\
	using name##_cptr = std::shared_ptr<name const>;\
	template <typename ... T> name##_ptr make_##name(T ... args) { return std::make_shared<name>(args ...); }
	
	FUNDAMENTAL(q3basemodel);
	FUNDAMENTAL(q3mesh);
	FUNDAMENTAL(q3model);
	FUNDAMENTAL(q3frame);
	FUNDAMENTAL(q3shader);
	FUNDAMENTAL(q3skin);
	FUNDAMENTAL(q3texture);
	FUNDAMENTAL(q3sampler);
	
	#undef FUNDAMENTAL
	
	//================================
	static constexpr GLint LAYOUT_VERTEX = 0; // VEC3
	static constexpr GLint LAYOUT_UV = 1; // VEC2
	static constexpr GLint LAYOUT_NORMAL = 2; // VEC3
	static constexpr GLint LAYOUT_TEXBIND = 3; // UINT -- NEEDS TO BE OPTIMIZED OUT!!! 
	static constexpr GLint LAYOUT_BONE_GROUPS = 4; // 4 GROUPS IDX AS IVEC4
	static constexpr GLint LAYOUT_BONE_WEIGHT = 5; // VEC4
	static constexpr GLint LAYOUT_COLOR0 = 6; // VEC4
	static constexpr GLint LAYOUT_COLOR1 = 7; // VEC4
	static constexpr GLint LAYOUT_LMUV01_COLOR2 = 8; // UV VEC4 (2 VEC2) OR COLOR VEC4
	static constexpr GLint LAYOUT_LMUV23_COLOR3 = 9; // VUV VEC4 (2 VEC2) OR COLOR VEC4
	static constexpr GLint LAYOUT_LMSTYLES = 10; // 4 STYLE IDX AS IVEC4
	//================================
	
	static constexpr GLint BINDING_DIFFUSE = 0;
	static constexpr GLint BINDING_LIGHTMAP = 1;
	static constexpr GLint BINDING_SKYBOX = 2; // TO 7
	static constexpr GLint BINDING_BONE_MATRICIES = 8;
	static constexpr GLint BINDING_LIGHTSTYLES = 9;
	static constexpr GLint BINDING_GRIDLIGHTING = 10;
	
	static constexpr uint_fast16_t LIGHTMAP_NUM = 4;
	static constexpr uint_fast16_t LIGHTMAP_DIM = 128;
	static constexpr uint_fast32_t LIGHTMAP_PIXELS = LIGHTMAP_DIM * LIGHTMAP_DIM;
	static constexpr uint_fast32_t LIGHTMAP_BYTES = LIGHTMAP_PIXELS * 3;
	
	static constexpr uint8_t LIGHTMAP_MODE_NONE = 0;
	static constexpr uint8_t LIGHTMAP_MODE_MAP = 1;
	static constexpr uint8_t LIGHTMAP_MODE_VERTEX = 2;
	static constexpr uint8_t LIGHTMAP_MODE_WHITEIMAGE = 3;
	
	using vertex_colors_t = qm::vec3_t;
	
	using lightmap_uv_t = std::array<qm::vec2_t, LIGHTMAP_NUM>;
	using lightmap_color_t = std::array<qm::vec4_t, LIGHTMAP_NUM>;
	
	using lightstyles_t = std::array<qm::vec4_t, 64>;
	using lightstylesidx_t = std::array<uint8_t, LIGHTMAP_NUM>;
	
	static_assert(sizeof(lightstyles_t) == sizeof(GLfloat) * 256);
	static_assert(sizeof(lightstylesidx_t) == LIGHTMAP_NUM);
	
	struct gridlighting_t {
		qm::vec3_t ambient {0, 0, 0};
		uint32_t pad_1;
		qm::vec3_t directed {0, 0, 0};
		uint32_t pad_2;
		qm::vec3_t direction {0, 0, 0};
		uint32_t pad_3;
	};
	
	static_assert(sizeof(gridlighting_t) == 12 * sizeof(float));
	
	enum struct default_shader_mode {
		basic,
		lightmap,
		diffuse
	};
	
//================================================================
// MODELS & MESHES
//================================================================
	
	// BASEMODEL
	struct q3basemodel {
		std::vector<char> buffer;
		model_t base {};
		q3model_ptr model = nullptr;
		
		void load();
		void setup_render();
	private:
		void load_mdxa();
		void load_mdxm(bool server = false);
		void load_md3(int32_t lod = 0);
		
		void setup_render_md3();
		void setup_render_mdxm();
	};
	
	// MESH
	struct q3mesh {
		struct uniform_info_t {
			GLuint lm_mode;
		};
		
		virtual ~q3mesh() = default;
		virtual void bind() = 0;
		virtual void draw() = 0;
		virtual bool is_bound() const = 0;
		virtual uniform_info_t const * uniform_info() const { return nullptr; }
		
		#ifdef _DEBUG
		static size_t m_debug_draw_count;
		#endif
	protected:
		static GLuint bound_handle;
	};
	
	struct q3mesh_basic : public q3mesh {
		enum struct mode : GLenum {
			triangles = GL_TRIANGLES,
			triangle_strip = GL_TRIANGLE_STRIP,
		};
		
		virtual ~q3mesh_basic();
		virtual void bind() override;
		virtual void draw() override;
		
		inline bool is_bound() const override { return m_handle == bound_handle; }
		inline uniform_info_t const * uniform_info() const override { return m_uniform_info.get(); }
		
		static q3mesh_ptr generate_fullquad();
		static q3mesh_ptr generate_unitquad();
		static q3mesh_ptr generate_skybox_mesh();

	protected:
		GLuint m_handle = 0;
		GLsizei m_size = 0; // NOTICE -- inheritors must set this!
		std::unique_ptr<uniform_info_t> m_uniform_info;
		
		q3mesh_basic(mode m);
		inline uniform_info_t & create_uniform_info() { m_uniform_info = std::make_unique<uniform_info_t>(); return *m_uniform_info; }
	private:
		mode m_mode;
	};
	
	// MODEL
	struct q3model {
		q3model() = default;
		q3model(q3model const &) = delete;
		q3model(q3model &&) = delete;
		~q3model() = default;
		
		std::vector<std::pair<q3shader_ptr, q3mesh_ptr>> meshes;
	};
	
//================================================================
// PROGRAM
//================================================================
	
	struct q3program {
		q3program(q3program const &) = delete;
		q3program(q3program &&) = delete;
		virtual ~q3program();
		
		void bind();
		inline bool is_bound() const { return m_handle == bound_id; }
	protected:
		
		struct shader final {
			friend q3program;
			
			enum struct type : GLenum {
				vert = GL_VERTEX_SHADER,
				frag = GL_FRAGMENT_SHADER,
			};
			inline shader(type t) : shader(static_cast<GLenum>(t)) {}
			~shader();
			
			void source(char const *);
			bool compile(std::string * log = nullptr);
		private:
			shader(GLenum type);
			GLuint m_handle;
		};
		
		q3program();
		
		void attach(shader const &);
		bool link(std::string * log = nullptr);
		
		virtual void on_bind() = 0;
		GLint get_location(char const * var) const { return glGetUniformLocation(m_handle, var); }
		GLuint const & get_handle() const { return m_handle; }
	private:
		GLuint m_handle = 0;
		static GLuint bound_id;
	};
	
//================================================================
// SKIN
//================================================================
	
	struct q3skinsurf {
		//!!! REQUIRED !!!
		char name[MAX_QPATH];
		//!!! REQUIRED !!!
		q3shader_ptr shader;
	};
	
	struct q3skin {
		qhandle_t index;
		skin_t skin;
		std::unordered_map<istring, q3skinsurf> lookup;
	};
	
//================================================================
// TEXTURE (& SAMPLER)
//================================================================
	
	struct q3texture {
		q3texture(GLsizei width, GLsizei height, bool mipmaps = true, GLenum type = GL_RGBA8);
		q3texture(q3texture const &) = delete;
		q3texture(q3texture &&) = delete;
		~q3texture();
		
		void upload(GLsizei width, GLsizei height, void const * data, GLenum format = GL_RGBA, GLenum data_type = GL_UNSIGNED_BYTE, GLint xoffs = 0, GLint yoffs = 0);
		void clear();
		void generate_mipmaps();
		
		void bind(GLuint binding) const;
		static void unbind(GLuint binding);
		
		void save(char const * path);
	
		constexpr void set_transparent(bool v = true) { m_transparent = v; }
		constexpr bool is_transparent() const { return m_transparent; }
	private:
		GLuint m_handle = 0;
		GLsizei m_width, m_height;
		bool m_mips;
		bool m_transparent = false;
	};
	
	struct q3sampler {
		q3sampler(float aniso = -1);
		q3sampler(q3sampler const &) = delete;
		q3sampler(q3sampler &&) = delete;
		~q3sampler();
		
		void bind(GLuint);
		void wrap(GLenum);
		
	private:
		GLuint m_handle = 0;
		GLenum m_current_wrap = GL_REPEAT;
	};
	
//================================================================
// SHADER
//================================================================
	
	struct q3stage {
		q3stage() = default;
		q3stage(q3stage const &) = delete;
		q3stage(q3stage &&) = default;
		~q3stage() = default;
		
		q3stage & operator = (q3stage const &) = delete;
		q3stage & operator = (q3stage &&) = default;
		
		//================================
		// DIFFUSE
		//================================
		
		struct diffuse_anim_t {
			std::vector<q3texture_ptr> maps;
			float speed;
			bool is_transparent = false;
		};
		using diffuse_anim_ptr = std::shared_ptr<diffuse_anim_t>;
		using diffuse_variant = std::variant<q3texture_ptr, diffuse_anim_ptr>;
		diffuse_variant diffuse;
		
		inline bool has_diffuse() const { return std::visit(lambda_visit{
			[&](q3texture_ptr const & i)->bool{return i.operator bool();},
			[&](diffuse_anim_ptr const & i)->bool{return i.operator bool();}}, diffuse);}
		inline bool diffuse_has_transparency() const {
			if (!has_diffuse()) return false;
			return std::visit( lambda_visit {
				[&](q3texture_ptr const & image) -> bool { return image->is_transparent(); },
				[&](diffuse_anim_ptr const & image) -> bool { return image->is_transparent; }
			}, diffuse);
		}
		
		//================================
		
		bool clamp = false;
		bool blend = false;
		bool depthwrite = false;
		bool gridlit = false;
		qm::vec4_t const_color {1, 1, 1, 1};
		
		// DO NOT SET DIRECTLY
		bool opaque = false;
		// ----
		
		enum struct alpha_func : uint8_t {
			none,
			gt0,
			lt128,
			ge128,
			ge192
		} alpha_test = alpha_func::none;
		
		GLenum blend_src = GL_ONE, blend_dst = GL_ZERO;
		GLenum depth_func = GL_LEQUAL;
		
		void validate();
		
		enum struct map_gen {
			diffuse,
			lightmap,
			normals,
			mnoise,
			anoise,
		} gen_map = map_gen::diffuse;
		
		enum struct gen_func {
			sine,
			square,
			triangle,
			sawtooth,
			inverse_sawtooth,
			noise,
			random,
		};
		
		enum struct gen_type {
			none,
			identity,
			constant,
			vertex,
			vertex_exact,
			wave,
			diffuse_lighting,
			diffuse_lighting_entity,
			specular_lighting,
			entity,
		} gen_rgb = gen_type::none, gen_alpha = gen_type::none;
		
		enum struct tcgen {
			none,
			environment,
			lightmap
		} gen_tc = tcgen::none;
		
		struct wave_func_t {
			gen_func func;
			float base;
			float amplitude;
			float phase;
			float frequency;
		} wave_rgb, wave_alpha;
		
		struct tx_turb {
			float base;
			float amplitude;
			float phase;
			float frequency;
		};
		
		struct tx_scale {
			qm::vec2_t value;
		};
		
		struct tx_scroll {
			qm::vec2_t value;
		};
		
		struct tx_stretch {
			gen_func gfunc;
			float base;
			float amplitude;
			float phase;
			float frequency;
		};
		
		struct tx_transform {
			qm::mat3_t matrix = qm::mat3_t::identity();
		};
		
		struct tx_rotate {
			float angle;
		};
		
		using texmod = std::variant<
			tx_turb,
			tx_scale,
			tx_scroll,
			tx_stretch,
			tx_transform,
			tx_rotate
		>;
		
		std::vector<texmod> texmods {};
		
		struct setup_draw_parameters_t {
			float time;
			bool is_2d = false;
			qm::mat4_t mvp = qm::mat4_t::identity();
			qm::mat3_t itm = qm::mat3_t::identity();
			qm::mat4_t m = qm::mat4_t::identity();
			qm::mat3_t uvm = qm::mat3_t::identity();
			q3mesh::uniform_info_t const * mesh_uniforms = nullptr;
			qm::vec4_t shader_color {1, 1, 1, 1};
			std::vector<qm::mat4_t> const * bone_weights = nullptr;
			qm::vec3_t view_origin;
			gridlighting_t const * gridlight;
			bool vertex_color_override = false; // used by sprite assemblies
		};
		void setup_draw(setup_draw_parameters_t const &) const;
	};
	
	struct q3shader {
		qhandle_t index;
		istring name;
		bool mips = false;
		bool valid = false;
		
		// used only for loading
		default_shader_mode dmode = default_shader_mode::basic;
		
		static constexpr float q3sort_opaque = 3.0f;
		static constexpr float q3sort_decal = 4.0f;
		static constexpr float q3sort_seethrough = 5.0f;
		static constexpr float q3sort_basetrans = 14.0f;
		
		// forwarded from stages, do not set directly
		bool opaque = false;
		bool blended = false;
		bool depthwrite = false;
		bool gridlit = false;
		
		// properties
		bool nodraw = false;
		bool polygon_offset = false;
		float sort = 0;
		std::vector<q3stage> stages;
		
		enum struct cull_type {
			front,
			back,
			both
		} cull = cull_type::front;
		
		struct sky_parms_t {
			std::array<q3texture_ptr, 6> sides;
			float cloud_height;
		};
		
		std::unique_ptr<sky_parms_t> sky_parms = nullptr;
		
		void setup_draw() const;
			
		bool parse_shader(istring const & src, bool mips);
		void validate(); // looks at stage values to determine what self's values should be
	private:
		bool parse_stage(q3stage & stg, char const * & sptr, bool mips);
		void parse_texmod(q3stage & stg, char const * ptr);
	};
	
//================================================================
// CUSTOM SHADER PROGRAMS
//================================================================
	
	template <typename T, void(*UPLOAD)(GLint, T const &)>
	struct uniform {
		constexpr uniform(T const & value_in) : value(value_in), value_default(value_in), modified(true) {}
		constexpr bool operator == (T const & other) const { return other == value; }
		constexpr T const & operator = (T const & other) { if (value != other) modified = true; value = other; return value; }
		inline void push() { if (!modified) return; push_direct(); }
		inline void push_direct() { modified = false; UPLOAD(location, value); }
		inline void reset() { operator = (value_default); }
		inline GLint set_location(GLint in) { location = in; return in; }
 	private:
		T value;
		T const value_default;
		GLint location;
		bool modified;
	};
	
	inline void upload_uint(GLint location, GLuint const & value) { glUniform1ui(location, value); }
	using uniform_uint = uniform<GLuint, upload_uint>;
	inline void upload_bool(GLint location, bool const & value) { glUniform1i(location, value); }
	using uniform_bool = uniform<bool, upload_bool>;
	inline void upload_float(GLint location, float const & value) { glUniform1f(location, value); }
	using uniform_float = uniform<float, upload_float>;
	inline void upload_vec3(GLint location, qm::vec3_t const & value) { glUniform3fv(location, 1, value); }
	using uniform_vec3 = uniform<qm::vec3_t, upload_vec3>;
	inline void upload_vec4(GLint location, qm::vec4_t const & value) { glUniform4fv(location, 1, value); }
	using uniform_vec4 = uniform<qm::vec4_t, upload_vec4>;
	inline void upload_mat3(GLint location, qm::mat3_t const & value) { glUniformMatrix3fv(location, 1, GL_FALSE, value); }
	using uniform_mat3 = uniform<qm::mat3_t, upload_mat3>;
	inline void upload_mat4(GLint location, qm::mat4_t const & value) { glUniformMatrix4fv(location, 1, GL_FALSE, value); }
	using uniform_mat4 = uniform<qm::mat4_t, upload_mat4>;
	
	inline void upload_lmmode(GLint location, GLuint const & value) { glUniform1ui(location, value); }
	using uniform_lmmode = uniform<GLuint, upload_lmmode>;
	
	namespace programs {
		struct q3main : public q3program {
			q3main();
			~q3main();
			
			void bind_and_reset();
			
			void time(float const &);
			void mvp(qm::mat4_t const &);
			void itm(qm::mat3_t const &);
			void m(qm::mat4_t const &);
			void uvm(qm::mat3_t const &);
			void color(qm::vec4_t const &);
			void use_vertex_colors(bool const &);
			void turb(bool const &);
			void turb_data(q3stage::tx_turb const &);
			void lm_mode(GLuint const &);
			void mapgen(q3stage::map_gen);
			void viewpos(qm::vec3_t const &);
			void tcgen(q3stage::tcgen);
			
			void rgbgen(q3stage::gen_type);
			void alphagen(q3stage::gen_type);
			
			void bone_matricies(qm::mat4_t const *, size_t num);
			void lightstyles(lightstyles_t const &);
			void gridlighting(gridlighting_t const *); // nullptr for off
		protected:
			virtual void on_bind() override;
		private:
			struct private_data;
			std::unique_ptr<private_data> m_data;
		};
		
		struct q3line : public q3program {
			q3line();
			~q3line();
			
			void mvp(qm::mat4_t const &);
			
			void bone_matricies(qm::mat4_t const *, size_t num);
		protected:
			virtual void on_bind() override;
		private:
			struct private_data;
			std::unique_ptr<private_data> m_data;
		};
		
		struct q3skybox : public q3program {
			q3skybox();
			~q3skybox() = default;
			
			void mvp(qm::mat4_t const &);
		protected:
			virtual void on_bind() override;
		private:
			uniform_mat4 m_mvp = qm::mat4_t::identity();
		};
		
		struct q3skyboxstencil : public q3program {
			q3skyboxstencil();
			~q3skyboxstencil() = default;
			
			void mvp(qm::mat4_t const &);
		protected:
			virtual void on_bind() override;
		private:
			uniform_mat4 m_mvp = qm::mat4_t::identity();
		};
	}
	
//================================================================
// FRAME
//================================================================
	
	namespace cmd2d {
		
		struct stretchpic {
			float x, y, w, h, s1, t1, s2, t2;
			q3shader_ptr shader;
			qm::vec4_t color;
		};
	};
	
	namespace cmd3d {
		
		struct basic_object {
			refEntity_t ref;
			q3basemodel_ptr basemodel;
			qm::mat4_t model_matrix;
			qm::vec4_t shader_color;
		};
		
		struct ghoul2_object {
			refEntity_t ref;
		};
		
		struct sprite {
			refEntity_t ref;
		};
	};
	
	struct q3scene {
		refdef_t ref {};
		bool finalized = false;
		
		std::vector<cmd3d::basic_object> basic_objects;
		std::vector<cmd3d::ghoul2_object> ghoul2_objects;
		
		std::vector<cmd3d::sprite> sprites;
		std::vector<cmd3d::sprite> beams;
	};
	
	struct q3frame {
		std::vector<cmd2d::stretchpic> ui_stretchpics;
		std::vector <q3scene> scenes;
		
		inline q3scene & scene() { return scenes.back(); }
		inline void new_scene() { scene().finalized = true; scenes.emplace_back(); }
	};
	
//================================================================
// STATE TRACKING
//================================================================
	
	namespace gl {
		void initialize();
		
		void alpha_test(bool);
		void alpha_func(GLenum func, float value);
		
		void blend(bool);
		void blend(GLenum src, GLenum dst);
		
		void cull(bool);
		void cull_face(GLenum face);
		
		void depth_func(GLenum);
		void depth_test(bool);
		void depth_write(bool);
		
		void stencil_func(GLenum func, GLint ref = 0, GLuint mask = 0xFFFFFFFF);
		void stencil_test(bool);
		void stencil_mask(GLuint);
		void stencil_op(GLenum fail, GLenum passdfail, GLenum pass);
		
		void line_width(float);
		
		void polygon_mode(GLenum, GLenum);
		void polygon_offset_fill(bool);
		void polygon_offset(float factor, float units);
	};
	
//================================================================
// WORLD
//================================================================
	
	struct q3world {
		friend instance;
		
		q3world();
		q3world(q3world const &) = delete;
		q3world(q3world &&) = delete;
		~q3world();
		
		void load(char const * name);
		qboolean get_entity_token(char * buffer, int size);
		q3model_ptr get_vis_model(refdef_t const & ref);
		gridlighting_t calculate_gridlight(qm::vec3_t const & pos);
		
	private:		
		istring m_name;
		istring m_basename;
		istring m_entity_string;
		char const * m_entity_parse_point = nullptr;
		
		bool m_base_allocated = false;
		union {
			byte * m_base = nullptr;
			dheader_t const * m_header;
		};
		template <typename T> T const * base(size_t offs) const { return reinterpret_cast<T const *>(m_base + offs); }
		
		//================================
		// MESH SPECIALIZATIONS
		//================================
		
		// LIT BY LIGHTMAP
		
		struct q3worldmesh_maplit : public q3mesh_basic {
			struct vertex_t {
				qm::vec3_t vert;
				qm::vec2_t uv;
				qm::vec3_t normal;
				qm::vec4_t color;
				lightmap_uv_t lm_uv;
				lightstylesidx_t styles;
			};
			
			q3worldmesh_maplit(vertex_t const * , size_t num, mode m = mode::triangles);
			~q3worldmesh_maplit();
		private:
			GLuint m_vbo;
		};
		
		struct q3worldmesh_maplit_proto {
			std::vector<q3worldmesh_maplit::vertex_t> verticies;
			std::vector<GLuint> indicies;
			
			inline q3mesh_ptr generate() const { return std::make_shared<q3worldmesh_maplit>(verticies.data(), verticies.size()); }
			inline void append_verticies(q3worldmesh_maplit_proto const & other) { verticies.insert(verticies.end(), other.verticies.begin(), other.verticies.end()); }
			inline void append_indicies(q3worldmesh_maplit_proto const & other) { indicies.insert(indicies.end(), other.indicies.begin(), other.indicies.end()); }
		};
		
		// LIT BY VERTEX COLORS
		struct q3worldmesh_vertexlit : public q3mesh_basic {
			struct vertex_t {
				qm::vec3_t vert;
				qm::vec2_t uv;
				qm::vec3_t normal;
				lightmap_color_t lm_color;
				lightstylesidx_t styles;
			};
			
			q3worldmesh_vertexlit(vertex_t const * , size_t num, mode m = mode::triangles);
			~q3worldmesh_vertexlit();
		private:
			GLuint m_vbo;
		};
		
		struct q3worldmesh_vertexlit_proto {
			std::vector<q3worldmesh_vertexlit::vertex_t> verticies;
			std::vector<GLuint> indicies;
			
			inline q3mesh_ptr generate() const { return std::make_shared<q3worldmesh_vertexlit>(verticies.data(), verticies.size()); }
			inline void append_verticies(q3worldmesh_vertexlit_proto const & other) { verticies.insert(verticies.end(), other.verticies.begin(), other.verticies.end()); }
			inline void append_indicies(q3worldmesh_vertexlit_proto const & other) { indicies.insert(indicies.end(), other.indicies.begin(), other.indicies.end()); }
		};
		
		using q3worldmesh_proto_variant = std::variant<q3worldmesh_maplit_proto, q3worldmesh_vertexlit_proto>;
		
		//================================
		// PATCH
		//================================
		
		using patch_int_t = int_fast16_t;
		static constexpr patch_int_t MAX_GRID_SIZE = 65;
		
		struct q3patchvert {
			qm::vec3_t xyz;
			qm::vec2_t uv;
			qm::vec3_t normal;
			qm::vec2_t lm_uvs[4];
			qm::vec4_t lm_colors[4];
			lightstylesidx_t styles;
		};
		
		struct q3patchsurface {
			uint_fast16_t width, height;
			std::vector<q3patchvert> verts;
			
			bool vertex_lit;
			lightstylesidx_t styles;
			GLuint mode;
			
			q3worldmesh_proto_variant process();
		};
		
		struct q3patchsubdivider;
		
		//================================
		
		struct q3worldsurface {
			dsurface_t const * info;
			q3shader_ptr shader;
			q3worldmesh_proto_variant proto;
			uint32_t index_start = 0;
			uint32_t index_num = 0;
		};
		
		//================================
		
		struct q3worldnode {
			
			struct node_data {
				q3worldnode * children [2] {};
				cplane_t const * plane;
			};
			
			struct leaf_data {
				int32_t cluster;
				int32_t area;
				std::vector<q3worldsurface *> surfaces;
			};
			
			q3worldnode * parent = nullptr;
			vec3_t mins, maxs;
			
			std::variant<std::monostate, node_data, leaf_data> data;
		};
		
		//================================
		
		struct q3vis {
			int32_t m_max_cluster = 0;
			int32_t m_cluster_bytes = 0;
			std::vector<byte> m_cluster_data;
		};
		
		//================================
		
		struct lightgrid_t {
			byte		ambientLight[MAXLIGHTMAPS][3];
			byte		directLight[MAXLIGHTMAPS][3];
			byte		styles[MAXLIGHTMAPS];
			byte		latLong[2];
		};

		//================================
		
		// MAP DATA POINTERS
		dshader_t const * m_shaders;
		int32_t m_shaders_count;
		
		dmodel_t const * dmodels;
		
		lightgrid_t const * m_lightgrid;
		int32_t m_lightgrid_num;
		uint16_t const * m_lightgrid_array;
		int32_t m_lightgrid_array_num;
		qm::vec3_t m_lightgrid_size;
		qm::vec3_t m_lightgrid_origin;
		int32_t m_lightgrid_bounds [3];
		
		// PARSED MAP DATA
		std::vector<cplane_t> planes;
		std::vector<q3worldsurface> m_surfaces;
		std::vector<q3worldnode> m_nodes;
		size_t m_nodes_leafs_offset;
		
		int32_t m_lightmap_span = -1;
		q3texture_ptr m_lightmap = nullptr;
		
		int32_t m_max_cluster = 0;
		std::unique_ptr<q3vis> m_vis = nullptr;
		//----------------
		
		//std::unordered_map<int32_t, q3model_ptr> m_vis_cache;
		using vis_cache_t = std::tuple<int32_t, q3model_ptr, std::array<byte, 32>>;
		std::vector<vis_cache_t> m_vis_cache;
		
		int32_t m_lockpvs_cluster = -1;
		q3worldnode const * find_leaf(qm::vec3_t const & coords);
		
		qm::vec2_t uv_for_lightmap(int32_t idx, qm::vec2_t uv_in);
		
		void load_shaders();
		void load_lightmaps();
		void load_planes();
		void load_fogs();
		void load_surfaces(int32_t idx);
		void load_nodesleafs();
		void load_submodels();
		void load_visibility();
		void load_entities();
		void load_lightgrid();
		void load_lightgridarray();
		
		//================================
		struct q3worldrendermesh;
		struct world_mesh_pair {
			q3mesh_ptr maplit;
			q3mesh_ptr vertexlit;
		};
		std::unordered_map<q3shader_ptr, world_mesh_pair> m_world_meshes;
		void build_world_meshes();
	};
	
//================================================================
// INSTANCE
//================================================================
	
	struct instance {
		friend q3stage;
		
		instance();
		~instance();
		void initialize_renderer();
		
		// MODELS
		struct model_registry {
			model_registry();
			model_registry(model_registry const &) = delete;
			model_registry(model_registry &&) = delete;
			~model_registry() = default;
			
			q3basemodel_ptr reg(char const * name, bool server = false);
			q3basemodel_ptr get(qhandle_t);
			
			void reg(q3basemodel_ptr); // direct register for q3world
			void process_waiting();
		private:
			std::unordered_map<istring, q3basemodel_ptr> lookup;
			std::vector<q3basemodel_ptr> models;
			std::vector<q3basemodel_ptr> waiting_models;
		} models;
		
		// SHADERS
		struct shader_registry {
			shader_registry();
			shader_registry(shader_registry const &) = delete;
			shader_registry(shader_registry &&) = delete;
			~shader_registry() = default;
			
			q3shader_ptr reg(istring const &, bool mipmaps = true, default_shader_mode dmode = default_shader_mode::basic);
			q3shader_ptr get(qhandle_t);
			void process_waiting();
		private:
			std::unordered_map<istring, std::string> source_lookup;
			std::unordered_map<istring, q3shader_ptr> lookup;
			std::vector<q3shader_ptr> shaders;
			std::vector<q3shader_ptr> waiting_shaders;
			
			void load_shader(q3shader_ptr shad);
		} shaders;
		
		// SKINS
		struct skin_registry {
			skin_registry();
			skin_registry(skin_registry const &) = delete;
			skin_registry(skin_registry &&) = delete;
			~skin_registry() = default;
			
			q3skin_ptr reg(char const * name, bool server = false);
			q3skin_ptr get(qhandle_t);
		private:
			qhandle_t hcounter = 0;
			std::unordered_map<istring, q3skin_ptr> lookup;
			std::vector<q3skin_ptr> skins;
		} skins;
		
		// TEXTURES
		struct texture_registry {
			texture_registry();
			texture_registry(texture_registry const &) = delete;
			texture_registry(texture_registry &&) = delete;
			~texture_registry() = default;
			
			void generate_named_defaults();
			
			q3texture_ptr reg(istring const & name, bool mips);
			q3texture_cptr whiteimage;
		private:
			std::unordered_map<istring, q3texture_ptr> lookup;
		} textures;
		
		float m_cull = 6000;
		
		lightstyles_t lightstyles;
		
		void begin_frame();
		void end_frame(float time);
		void load_world(char const * name);
		
		// CMD funcs
		void save_lightmap_atlas();
		void screenshot(char const * name);
		
		inline q3frame & frame() { return *m_frame; }
		inline qboolean world_get_entity_token(char * buffer, int size) { return m_world->get_entity_token(buffer, size); }
		
	private:
		uint32_t width, height;
		bool renderer_initialized = false;
		
		std::unique_ptr<programs::q3main> q3mainprog;
		std::unique_ptr<programs::q3line> q3lineprog;
		std::unique_ptr<programs::q3skyboxstencil> q3skyboxstencilprog;
		std::unique_ptr<programs::q3skybox> q3skyboxprog;
		
		q3sampler_ptr main_sampler = nullptr;
		
		bool m_ui_draw = false;
		qm::vec4_t m_shader_color {1, 1, 1, 1};
		
		q3mesh_ptr fullquad, unitquad, skybox;
		q3frame_ptr m_frame;
		std::unique_ptr<q3world> m_world = nullptr;
	};
}
