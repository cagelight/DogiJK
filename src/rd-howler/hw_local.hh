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
	
	static constexpr GLint LAYOUT_VERTEX = 0;
	static constexpr GLint LAYOUT_UV = 1;
	static constexpr GLint LAYOUT_VERTEX_COLOR = 2;
	static constexpr GLint LAYOUT_LIGHTMAP_DATA = 3;
	static constexpr GLint LAYOUT_LIGHTMAP_STYLE = 4;
	static constexpr GLint LAYOUT_LIGHTMAP_MODE = 5;
	
	static constexpr GLint BINDING_DIFFUSE = 0;
	static constexpr GLint BINDING_LIGHTMAP = 1;
	
	static constexpr uint_fast16_t LIGHTMAP_NUM = 4;
	static constexpr uint_fast16_t LIGHTMAP_DIM = 128;
	static constexpr uint_fast32_t LIGHTMAP_PIXELS = LIGHTMAP_DIM * LIGHTMAP_DIM;
	static constexpr uint_fast32_t LIGHTMAP_BYTES = LIGHTMAP_PIXELS * 3;
	
	static constexpr uint8_t LIGHTMAP_MODE_NONE = 0;
	static constexpr uint8_t LIGHTMAP_MODE_MAP = 1;
	static constexpr uint8_t LIGHTMAP_MODE_VERTEX = 2;
	static constexpr uint8_t LIGHTMAP_MODE_WHITEIMAGE = 3;
	
	using vertex_colors_t = qm::vec3_t;
	
	using lightmap_data_t = std::array<qm::vec3_t, LIGHTMAP_NUM>; /* UVs or color */
	using lightmap_styles_t = std::array<uint8_t, LIGHTMAP_NUM>;
	using lightmap_modes_t = std::array<uint8_t, LIGHTMAP_NUM>;
	
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
	};
	
	// MESH
	struct q3mesh {
		enum struct mode : GLenum {
			triangles = GL_TRIANGLES,
			triangle_strip = GL_TRIANGLE_STRIP,
		};
		
		q3mesh(mode = mode::triangles);
		q3mesh(q3mesh const &) = delete;
		q3mesh(q3mesh &&) = delete;
		~q3mesh();
		
		void upload_verts(float const *, size_t);
		inline void upload_verts(qm::vec3_t const * ptr, size_t num) { upload_verts(reinterpret_cast<float const *>(ptr), num * 3); }
		template <typename T> inline void upload_verts(T const & v) { upload_verts(v.data(), v.size()); }
		
		void upload_uvs(float const *, size_t);
		inline void upload_uvs(qm::vec2_t const * ptr, size_t num) { upload_uvs(reinterpret_cast<float const *>(ptr), num * 2); }
		template <typename T> inline void upload_uvs(T const & v) { upload_uvs(v.data(), v.size()); }
		
		void upload_vertex_colors(float const *, size_t);
		inline void upload_vertex_colors(vertex_colors_t const * ptr, size_t num) { upload_vertex_colors(reinterpret_cast<float const *>(ptr), num * 3); }
		template <typename T> inline void upload_vertex_colors(T const & v) { upload_vertex_colors(v.data(), v.size()); }
		
		void upload_lightmap_data(float const *, size_t);
		inline void upload_lightmap_data(lightmap_data_t const * ptr, size_t num) { upload_lightmap_data(reinterpret_cast<float const *>(ptr), num * 12); }
		template <typename T> inline void upload_lightmap_data(T const & v) { upload_lightmap_data(v.data(), v.size()); }
		
		void upload_lightmap_styles(float const *, size_t);
		inline void upload_lightmap_styles(lightmap_styles_t const * ptr, size_t num) { upload_lightmap_styles(reinterpret_cast<float const *>(ptr), num * 4); }
		template <typename T> inline void upload_lightmap_styles(T const & v) { upload_lightmap_styles(v.data(), v.size()); }
		
		void upload_lightmap_modes(float const *, size_t);
		inline void upload_lightmap_modes(lightmap_modes_t const * ptr, size_t num) { upload_lightmap_modes(reinterpret_cast<float const *>(ptr), num * 4); }
		template <typename T> inline void upload_lightmap_modes(T const & v) { upload_lightmap_modes(v.data(), v.size()); }
		
		void bind();
		void draw();
		inline bool is_bound() const { return m_handle == bound_handle; }
		
		static q3mesh_ptr generate_fullquad();
		static q3mesh_ptr generate_unitquad();
	private:
		static GLuint bound_handle;
		static constexpr size_t num_vbos = 6;
		
		GLuint m_handle = 0;
		GLuint m_vbos[num_vbos] {};
		
		size_t m_size = 0;
		mode m_mode = mode::triangles;
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
		std::vector<q3skinsurf> surfs;
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
		
		void bind(GLuint binding);
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
		
		q3texture_ptr diffuse = nullptr;
		bool clamp = false;
		bool blend = false;
		bool depthwrite = false;
		qm::vec4_t const_color {1, 1, 1, 1};
		
		// DO NOT SET DIRECTLY
		bool opaque = false;
		// ----
		
		GLenum blend_src = GL_ONE, blend_dst = GL_ZERO;
		
		void validate();
		
		enum struct shading_mode {
			main,
			lightmap,
			noise,
		} mode = shading_mode::main;
		
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
		} gen_rgb = gen_type::none, gen_alpha = gen_type::none;
		
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
		
		void setup_draw(float time, qm::mat4_t const & mvp = qm::mat4_t::identity(), qm::mat3_t const & uvm = qm::mat3_t::identity()) const;
	};
	
	struct q3shader {
		qhandle_t index;
		istring name;
		bool mips = false;
		bool valid = false;
		
		// used only for loading -- if shader does not exist and thus is generated from a texture, generate lightmap stage
		bool lightmap_if_texture = false;
		
		static constexpr float q3sort_opaque = 3.0f;
		static constexpr float q3sort_seethrough = 5.0f;
		static constexpr float q3sort_basetrans = 14.0f;
		
		// forwarded from stages, do not set directly
		bool opaque = false;
		bool blended = false;
		bool depthwrite = false;
		
		// properties
		bool polygon_offset = false;
		float sort = q3sort_opaque;
		std::vector<q3stage> stages;
		
		enum struct cull_type {
			front,
			back,
			both
		} cull = cull_type::front;
		
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
		constexpr uniform(T const & value_in) : value(value_in), modified(true) {}
		constexpr bool operator == (T const & other) const { return other == value; }
		constexpr T const & operator = (T const & other) { if (value != other) modified = true; value = other; return value; }
		inline void push(GLint location) { if (!modified) return; push_direct(location); }
		inline void push_direct(GLint location) { modified = false; UPLOAD(location, value); }
 	private:
		T value;
		bool modified;
	};
	
	inline void upload_bool(GLint location, bool const & value) { glUniform1i(location, value); }
	using uniform_bool = uniform<bool, upload_bool>;
	inline void upload_float(GLint location, float const & value) { glUniform1f(location, value); }
	using uniform_float = uniform<float, upload_float>;
	inline void upload_vec4(GLint location, qm::vec4_t const & value) { glUniform4fv(location, 1, value); }
	using uniform_vec4 = uniform<qm::vec4_t, upload_vec4>;
	inline void upload_mat3(GLint location, qm::mat3_t const & value) { glUniformMatrix3fv(location, 1, GL_FALSE, value); }
	using uniform_mat3 = uniform<qm::mat3_t, upload_mat3>;
	inline void upload_mat4(GLint location, qm::mat4_t const & value) { glUniformMatrix4fv(location, 1, GL_FALSE, value); }
	using uniform_mat4 = uniform<qm::mat4_t, upload_mat4>;
	
	namespace programs {
		struct q3main : public q3program {
			q3main();
			~q3main() = default;
			
			void time(float const &);
			void mvp(qm::mat4_t const &);
			void uvm(qm::mat3_t const &);
			void color(qm::vec4_t const &);
			void use_vertex_colors(bool const &);
			void turb(bool const &);
			void turb_data(q3stage::tx_turb const &);
		protected:
			virtual void on_bind() override;
		private:
			uniform_float m_time = 0;
			uniform_mat4 m_mvp = qm::mat4_t::identity();
			uniform_mat3 m_uv = qm::mat3_t::identity();
			uniform_vec4 m_color = qm::vec4_t {1, 1, 1, 1};
			uniform_bool m_use_vertex_colors = false;
			
			uniform_bool m_turb = false;
			uniform_vec4 m_turb_data = qm::vec4_t {0, 0, 0, 0};
		};
		
		struct q3lightmap : public q3program {
			q3lightmap();
			~q3lightmap() = default;
			
			void mvp(qm::mat4_t const &);
			void color(qm::vec4_t const &);
		protected:
			virtual void on_bind() override;
		private:
			uniform_mat4 m_mvp = qm::mat4_t::identity();
			uniform_vec4 m_color = qm::vec4_t {1, 1, 1, 1};
		};
		
		struct q3vertexlit : public q3program {
			q3vertexlit();
			~q3vertexlit() = default;
			
			void mvp(qm::mat4_t const &);
			void color(qm::vec4_t const &);
		protected:
			virtual void on_bind() override;
		private:
			uniform_mat4 m_mvp = qm::mat4_t::identity();
			uniform_vec4 m_color = qm::vec4_t {1, 1, 1, 1};
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
			q3basemodel_ptr basemodel;
			qm::mat4_t model_matrix;
		};
	};
	
	using c2d = std::variant<
		cmd2d::stretchpic
	>;
	
	using c3d = std::variant<
		cmd3d::basic_object
	>;
	
	struct q3scene {
		refdef_t ref {};
		bool finalized = false;
		std::vector<c3d> c3ds;
		
		template <typename T, typename ... ARGS>
		T & emplace( ARGS ... args ) { return std::get<T>( c3ds.emplace_back(T { args ... }) ); }
	};
	
	struct q3frame {
		std::vector<c2d> c2ds;
		std::vector <q3scene> scenes;
		
		inline q3scene & scene() { return scenes.back(); }
		inline void new_scene() { scene().finalized = true; scenes.emplace_back(); }

		template <typename T, typename ... ARGS>
		T & emplace_2d( ARGS ... args ) { return std::get<T>( c2ds.emplace_back(T { args ... }) ); }
		template <typename T, typename ... ARGS>
		T & emplace_3d( ARGS ... args ) { return scene().emplace<T>(args ...); }
	};
	
//================================================================
// STATE TRACKING
//================================================================
	
	namespace gl {
		void initialize();
		
		void blend(bool);
		void blend(GLenum src, GLenum dst);
		
		void cull(bool);
		void cull_face(GLenum face);
		
		void depth_func(GLenum);
		void depth_test(bool);
		void depth_write(bool);
		
		void polygon_mode(GLenum, GLenum);
		void polygon_offset_fill(bool);
		void polygon_offset(float factor, float units);
	};
	
//================================================================
// WORLD
//================================================================
	
	struct q3world {
		friend instance;
		
		q3world() = default;
		q3world(q3world const &) = delete;
		q3world(q3world &&) = delete;
		~q3world();
		
		void load(char const * name);
		qboolean get_entity_token(char * buffer, int size);
		
		q3model_ptr get_vis_model(qm::vec3_t coords);
		
	private:		
		istring m_name;
		istring m_basename;
		istring m_entity_string;
		qm::vec3_t m_light_grid_size;
		char const * m_entity_parse_point = nullptr;
		
		bool m_base_allocated = false;
		union {
			byte * m_base = nullptr;
			dheader_t const * m_header;
		};
		template <typename T> T const * base(size_t offs) const { return reinterpret_cast<T const *>(m_base + offs); }
		
		//================================
		// PROTO
		//================================
		
		struct q3protosurface {
			std::vector<qm::vec3_t> verticies;
			std::vector<qm::vec2_t> uvs;
			std::vector<vertex_colors_t> vertex_colors;
			std::vector<lightmap_data_t> lightmap_data;
			std::vector<lightmap_styles_t> lightmap_styles;
			std::vector<lightmap_modes_t> lightmap_modes; 
			
			inline void append(q3protosurface const & other) {
				verticies.insert(verticies.end(), other.verticies.begin(), other.verticies.end());
				uvs.insert(uvs.end(), other.uvs.begin(), other.uvs.end());
				vertex_colors.insert(vertex_colors.end(), other.vertex_colors.begin(), other.vertex_colors.end());
				lightmap_data.insert(lightmap_data.end(), other.lightmap_data.begin(), other.lightmap_data.end());
				lightmap_styles.insert(lightmap_styles.end(), other.lightmap_styles.begin(), other.lightmap_styles.end());
				lightmap_modes.insert(lightmap_modes.end(), other.lightmap_modes.begin(), other.lightmap_modes.end());
			}
			
			inline void upload(q3mesh & mesh) const {
				mesh.upload_verts(verticies);
				mesh.upload_uvs(uvs);
				mesh.upload_vertex_colors(vertex_colors);
				mesh.upload_lightmap_data(lightmap_data);
				mesh.upload_lightmap_styles(lightmap_styles);
				mesh.upload_lightmap_modes(lightmap_modes);
			}
		};
		
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
		};
		
		struct q3patchsurface {
			uint_fast16_t width, height;
			std::vector<q3patchvert> verts;
			
			bool vertex_lit;
			lightmap_styles_t styles;
			lightmap_modes_t modes;
			
			q3protosurface process();
		};
		
		struct q3patchsubdivider;
		
		//================================
		
		struct q3worldsurface {
			dsurface_t const * info;
			q3shader_ptr shader;
			q3protosurface proto;
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
			
			std::variant<node_data, leaf_data> data;
		};
		
		//================================
		
		struct q3vis {
			int32_t m_clusters = 0;
			int32_t m_cluster_bytes = 0;
			std::vector<byte> m_cluster_data;
		};
		
		//================================
		
		// MAP DATA POINTERS
		dshader_t const * m_shaders;
		int32_t m_shaders_count;
		
		// PARSED MAP DATA
		std::vector<cplane_t> planes;
		std::vector<q3worldsurface> m_surfaces;
		std::vector<q3worldnode> m_nodes;
		
		int32_t m_lightmap_span = -1;
		q3texture_ptr m_lightmap = nullptr;
		
		int32_t m_clusters = 0;
		std::unique_ptr<q3vis> m_vis = nullptr;
		//----------------
		
		//std::unordered_map<int32_t, q3model_ptr> m_vis_cache;
		std::vector<std::pair<int32_t, q3model_ptr>> m_vis_cache;
		
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
		private:
			std::unordered_map<istring, q3basemodel_ptr> lookup;
			std::vector<q3basemodel_ptr> models;
		} models;
		
		// SHADERS
		struct shader_registry {
			shader_registry();
			shader_registry(shader_registry const &) = delete;
			shader_registry(shader_registry &&) = delete;
			~shader_registry() = default;
			
			q3shader_ptr reg(istring const &, bool mipmaps = true, bool lightmap_if_texture = false);
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
			skin_registry() = default;
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
		private:
			std::unordered_map<istring, q3texture_ptr> lookup;
		} textures;
		
		float m_cull = 6000;
		
		void begin_frame();
		void end_frame(float time);
		void load_world(char const * name);
		
		// CMD funcs
		void save_lightmap_atlas();
		void screenshot(char const * name);
		
		inline q3frame & frame() { return *m_frame; }
		inline qboolean world_get_entity_token(char * buffer, int size) { return m_world->get_entity_token(buffer, size); }
		
	private:
		window_t window;
		uint32_t width, height;
		bool renderer_initialized = false;
		
		std::unique_ptr<programs::q3main> q3mainprog = nullptr;
		std::unique_ptr<programs::q3lightmap> q3lmprog = nullptr;
		
		q3sampler_ptr main_sampler = nullptr;
		
		bool m_ui_draw = false;
		qm::vec4_t m_shader_color {1, 1, 1, 1};
		
		q3mesh_ptr fullquad, unitquad;
		q3frame_ptr m_frame;
		std::unique_ptr<q3world> m_world = nullptr;
	};
}
