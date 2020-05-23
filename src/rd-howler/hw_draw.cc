#include "ghoul2/G2.hh"

#include "hw_local.hh"
using namespace howler;

void instance::begin_frame() {
	m_frame = make_q3frame();
	m_frame->scenes.emplace_back();
}

static constexpr qm::mat4_t ui_ortho = qm::mat4_t::ortho(0, 480, 0, 640, 0, 1);

template <typename T>
constexpr int8_t compare3(T const & A, T const & B) {
	if (A > B) return 1;
	if (A < B) return -1;
	return 0;
}

struct q3drawmesh {
	q3drawmesh() = delete;
	q3drawmesh(q3mesh_ptr const & mesh, qm::mat4_t const & mvp, qm::vec4_t shader_color = {1, 1, 1, 1}) : 
		mesh(mesh),
		mvp(mvp),
		shader_color(shader_color) {}
	q3drawmesh(q3drawmesh const &) = default;
	q3drawmesh(q3drawmesh &&) = default;
	
	q3drawmesh & operator = (q3drawmesh const &) = default;
	q3drawmesh & operator = (q3drawmesh &&) = default;
	
	q3mesh_ptr mesh;
	
	qm::mat4_t mvp = qm::mat4_t::identity();
	qm::mat3_t itm = qm::mat3_t::identity();;
	qm::mat4_t m = qm::mat4_t::identity();;
	qm::vec4_t shader_color = {1, 1, 1, 1};
	std::vector<qm::mat4_t> weights;
	gridlighting_t gridlight {};
};

struct q3drawset {
	q3shader_ptr shader;
	std::vector<q3drawmesh> meshes;
	float sort_offset = 0; // using this for particles
	float relative_depth = 0;
	
	q3drawset() = default;
	q3drawset(q3shader_ptr const & shader, std::vector<q3drawmesh> && meshes) : shader(shader), meshes(std::move(meshes)) {}
	q3drawset(q3drawset const &) = delete;
	q3drawset(q3drawset &&) = default;
	
	q3drawset & operator = (q3drawset const &) = delete;
	q3drawset & operator = (q3drawset &&) = default;
	
	static inline bool compare(q3drawset & A, q3drawset & B) {	
		
		switch (compare3(A.shader->sort + A.sort_offset, B.shader->sort + B.sort_offset)) {
			case -1: return true;
			case 1: return false;
			case 0: break;
		}
		
		switch (compare3(A.shader->depthwrite, B.shader->depthwrite)) {
			case -1: return false;
			case 1: return true;
			case 0: break;
		}
		
		switch (compare3(A.relative_depth, B.relative_depth)) {
			case -1: return false;
			case 1: return true;
			case 0: break;
		}
		
		return A.shader->index > B.shader->index;
	}
};

struct sprite_assembly : public q3mesh_basic {
	
	struct vertex_t {
		qm::vec3_t vert;
		qm::vec2_t uv;
		qm::vec3_t normal;
		qm::vec4_t color;
	};
	
	sprite_assembly(vertex_t const * data, size_t num) : q3mesh_basic(mode::triangles) {
		
		static constexpr uint_fast16_t offsetof_verts = 0;
		static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
		static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
		static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
		static constexpr uint_fast16_t offsetof_norm = offsetof_uv + sizeof_uv;
		static constexpr uint_fast16_t sizeof_norm = sizeof(vertex_t::normal);
		static constexpr uint_fast16_t offsetof_color = offsetof_norm + sizeof_norm;
		static constexpr uint_fast16_t sizeof_color = sizeof(vertex_t::color);
		static constexpr uint_fast16_t sizeof_all = offsetof_color + sizeof_color;
		static_assert(sizeof_all == sizeof(vertex_t));
		
		m_size = num;
		glCreateBuffers(1, &m_vbo);
		glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
		
		glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
		
		glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_NORMAL);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_COLOR0);
		
		glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_NORMAL, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_COLOR0, 0);
		
		glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, sizeof_verts / 4, GL_FLOAT, GL_FALSE, offsetof_verts);
		glVertexArrayAttribFormat(m_handle, LAYOUT_UV, sizeof_uv / 4, GL_FLOAT, GL_FALSE, offsetof_uv);
		glVertexArrayAttribFormat(m_handle, LAYOUT_NORMAL, sizeof_norm / 4, GL_FLOAT, GL_FALSE, offsetof_norm);
		glVertexArrayAttribFormat(m_handle, LAYOUT_COLOR0, sizeof_color / 4, GL_FLOAT, GL_FALSE, offsetof_color);
	}
	
	~sprite_assembly() {
		glDeleteBuffers(1, &m_vbo);
	}
private:
	GLuint m_vbo;
};

static constexpr qm::vec4_t convert_4u8(byte const * RGBA) {
	return qm::vec4_t {
		RGBA[0] / 255.0f,
		RGBA[1] / 255.0f,
		RGBA[2] / 255.0f,
		RGBA[3] / 255.0f,
	};
}

void instance::end_frame(float time) {
	
	q3frame_ptr drawframe = m_frame;
	m_frame.reset();
	
	gl::depth_write(true);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	m_ui_draw = false;
	m_shader_color = {1, 1, 1, 1};
	
	std::vector<q3drawmesh> debug_meshes;
	bool debug_enabled = r_showtris->integer || r_showedges->integer;
	
	bool skyportal_drawn = false;
	
	for (q3scene const & scene : drawframe->scenes) {
		
		if (!scene.finalized) continue;
		
		qm::vec3_t const axis_forward = {scene.ref.viewaxis[0][1], -scene.ref.viewaxis[0][2], scene.ref.viewaxis[0][0]};
		qm::vec3_t const axis_up = {scene.ref.viewaxis[2][1], -scene.ref.viewaxis[2][2], scene.ref.viewaxis[2][0]};
		qm::vec3_t const axis_left = {scene.ref.viewaxis[1][1], -scene.ref.viewaxis[1][2], scene.ref.viewaxis[1][0]};
		
		qm::vec3_t view_origin {scene.ref.vieworg[1], -scene.ref.vieworg[2], scene.ref.vieworg[0]};
		
		qm::mat4_t p = qm::mat4_t::perspective(qm::deg2rad(scene.ref.fov_y), scene.ref.width, scene.ref.height, 4, m_cull * 2);
		qm::mat4_t v = qm::mat4_t::translate(-view_origin);
		qm::quat_t rq = qm::quat_t::identity();
		rq *= qm::quat_t { {1, 0, 0}, qm::deg2rad(scene.ref.viewangles[PITCH]) };
		rq *= qm::quat_t { {0, 0, 1}, qm::deg2rad(scene.ref.viewangles[ROLL]) + qm::pi };
		rq *= qm::quat_t { {0, 1, 0}, qm::deg2rad(scene.ref.viewangles[YAW]) };
		qm::mat4_t r = qm::mat4_t { qm::mat3_t {rq} };
		qm::mat4_t vp = (v * r) * p;
		
		qm::mat4_t sbvp = r * qm::mat4_t::perspective(qm::deg2rad(scene.ref.fov_y), scene.ref.width, scene.ref.height, 0.125, 8);
		
		//================================================================
		// DRAW TRACKING DECLARATIONS
		//================================================================
		std::unordered_map<q3shader_ptr, std::vector<q3drawmesh>> draw_map;
		std::unordered_map<q3shader_ptr, std::vector<sprite_assembly::vertex_t>> sprites_verticies;
		std::vector<q3drawset> additonal_draws;
		
		//================================================================
		// UNIVERSAL SPRITE FUNCTION
		//================================================================
		auto do_sprite = [&](qm::vec3_t const & origin, float radius, float rotation, q3shader_ptr shader, qm::vec4_t const & shader_color){
			qm::vec3_t left = axis_left, up = axis_up;
			if (!rotation) {
				left *= radius;
				up *= radius;
			} else {
				float ang = qm::deg2rad(rotation);
				float s = std::sin(ang);
				float c = std::cos(ang);
				
				left *= radius * c;
				up *= radius * c;
				
				VectorMA(left.ptr(), -s * radius, axis_up.ptr(), left.ptr());
				VectorMA(up.ptr(), s * radius, axis_left.ptr(), up.ptr());
			}
			
			sprite_assembly::vertex_t v0 = {
				qm::vec3_t { 
					origin[0] + left[0] + up[0], 
					origin[1] + left[1] + up[1],
					origin[2] + left[2] + up[2],
				},
				qm::vec2_t { 0, 0 }, axis_forward, shader_color
			};
			
			sprite_assembly::vertex_t v1 = {
				qm::vec3_t { 
					origin[0] - left[0] + up[0], 
					origin[1] - left[1] + up[1],
					origin[2] - left[2] + up[2],
				},
				qm::vec2_t { 1, 0 }, axis_forward, shader_color
			};
			
			sprite_assembly::vertex_t v2 = {
				qm::vec3_t { 
					origin[0] + left[0] - up[0], 
					origin[1] + left[1] - up[1],
					origin[2] + left[2] - up[2],
				},
				qm::vec2_t { 0, 1 }, axis_forward, shader_color
			};
			
			sprite_assembly::vertex_t v3 = {
				qm::vec3_t { 
					origin[0] - left[0] - up[0], 
					origin[1] - left[1] - up[1],
					origin[2] - left[2] - up[2],
				},
				qm::vec2_t { 1, 1 }, axis_forward, shader_color
			};
			
			std::vector<sprite_assembly::vertex_t> & sasm = sprites_verticies[shader];
			sasm.emplace_back(v0);
			sasm.emplace_back(v1);
			sasm.emplace_back(v2);
			sasm.emplace_back(v2);
			sasm.emplace_back(v1);
			sasm.emplace_back(v3);
		};
		
		//================================================================
		// WORLD -- WORLD MESHES
		//================================================================
		
		if (!(scene.ref.rdflags & RDF_NOWORLDMODEL) && m_world) {
			q3world::q3world_draw const & wdraw = m_world->get_vis_model(scene.ref);
			
			qm::mat4_t m = qm::mat4_t::scale(1, -1, 1);
			for (auto const & dmesh : wdraw.model->meshes) {
				qm::mat4_t mvp = m * vp;
				q3drawmesh draw {dmesh.second, mvp};
				draw.m = qm::mat4_t::identity();
				if (dmesh.first && dmesh.first->gridlit) {
					draw.gridlight = m_world->calculate_gridlight({0, 0, 0});
				}
				if (!dmesh.second) continue;
				if (debug_enabled) debug_meshes.emplace_back(draw);
				draw_map[dmesh.first].emplace_back(draw);
			}
			for (auto const & [shad, flares] : wdraw.flares) {
				for (auto const & flare : flares) do_sprite(flare.origin, 30, 0, shad, qm::vec4_t { flare.color, 1 });
			}
		}
		
		//================================================================
		// BASIC -- SIMPLE MODELS LIKE MD3
		//================================================================
		
		for (auto const & obj : scene.basic_objects) {
			if (!obj.basemodel->model) continue;
			qm::mat4_t mvp = obj.model_matrix * vp;
			for (auto const & mesh : obj.basemodel->model->meshes) {
				
				q3drawmesh draw {mesh.second, mvp};
				
				if (mesh.first && mesh.first->gridlit) {
					
					qm::mat3_t m3 = {
						obj.model_matrix[0][0], obj.model_matrix[0][1], obj.model_matrix[0][2],
						obj.model_matrix[1][0], obj.model_matrix[1][1], obj.model_matrix[1][2],
						obj.model_matrix[2][0], obj.model_matrix[2][1], obj.model_matrix[2][2],
					};
					qm::mat3_t testlawl;
					MatrixInverse(reinterpret_cast<vec3_t *>(m3.ptr()), reinterpret_cast<vec3_t *>(testlawl.ptr()));
					qm::mat3_t trans3 = {
						testlawl[0][0], testlawl[1][0], testlawl[2][0],
						testlawl[0][1], testlawl[1][1], testlawl[2][1],
						testlawl[0][2], testlawl[1][2], testlawl[2][2],
					};
					draw.itm = trans3;
					draw.m = obj.model_matrix;
					draw.shader_color = convert_4u8(obj.ref.shaderRGBA);
					
					if (mesh.first->gridlit && m_world)
						draw.gridlight = m_world->calculate_gridlight((obj.ref.renderfx & RF_LIGHTING_ORIGIN) ? obj.ref.lightingOrigin : obj.ref.origin);
				}
				
				if (debug_enabled) debug_meshes.emplace_back(draw);
				draw_map[mesh.first].emplace_back(draw);
			}
		}
		
		//================================================================
		// GHOUL2 -- MDXM MODELS AND THEIR ATTACHMENTS + ANIMATIONS
		//================================================================
		
		for (auto const & obj : scene.ghoul2_objects) {
					
			CGhoul2Info_v & g2i = *reinterpret_cast<CGhoul2Info_v *>(obj.ref.ghoul2);
			
			qm::mat4_t axis_conv = {
				obj.ref.axis[1][1], -obj.ref.axis[1][2], obj.ref.axis[1][0], 0,
				obj.ref.axis[2][1], -obj.ref.axis[2][2], obj.ref.axis[2][0], 0,
				obj.ref.axis[0][1], -obj.ref.axis[0][2], obj.ref.axis[0][0], 0,
				0, 0, 0, 1
			};
			qm::mat4_t model_matrix = axis_conv * qm::mat4_t::translate({obj.ref.origin[1], -obj.ref.origin[2], obj.ref.origin[0]});
			
			if (!ri.G2_IsValid(g2i)) 
				continue;
			if (!ri.G2_SetupModelPointers(g2i)) 
				continue;
			
			if ((obj.ref.renderfx & RF_THIRD_PERSON)) continue; // FIXME -- these render in mirrors and portals
				
			int g2time = ri.G2API_GetTime(time);
			
			mdxaBone_t root;
			ri.G2_RootMatrix(g2i, time, obj.ref.modelScale, root);
			
			int32_t model_count;
			int32_t model_list[256]; model_list[255]=548;
			ri.G2_Sort_Models(g2i, model_list, &model_count);
			ri.G2_GenerateWorldMatrix(obj.ref.angles, obj.ref.origin);
			
			for (int32_t model_idx = 0; model_idx < model_count; model_idx++) {
				
				CGhoul2Info & g2 = ri.G2_At(g2i, model_idx);
				if (!g2.mValid || (g2.mFlags & (GHOUL2_NOMODEL | GHOUL2_NORENDER)))
					continue;
				
				if (model_idx && g2.mModelBoltLink != -1) {
					int	boltMod = (g2.mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
					int	boltNum = (g2.mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
					mdxaBone_t bolt;
					ri.G2_GetBoltMatrixLow(ri.G2_At(g2i, boltMod), boltNum, obj.ref.modelScale, bolt);
					ri.G2_TransformGhoulBones(g2.mBlist, bolt, g2, g2time, true);
				} else
					ri.G2_TransformGhoulBones(g2.mBlist, root, g2, g2time, true);
				
				q3basemodel_ptr basemod = models.get(g2.mModel);
				
				for (size_t s = 0; s < basemod->model->meshes.size(); s++) {
					
					auto const & mesh = basemod->model->meshes[s];
					if (!mesh.second) continue;
					q3shader_ptr shader = mesh.first;
					
					mdxmSurface_t 			* surf =  (mdxmSurface_t *)ri.G2_FindSurface(&basemod->base, s, 0);
					mdxmHierarchyOffsets_t	* surfI = (mdxmHierarchyOffsets_t *)((byte *)basemod->base.mdxm + sizeof(mdxmHeader_t));
					mdxmSurfHierarchy_t		* surfH = (mdxmSurfHierarchy_t *)((byte *)surfI + surfI->offsets[surf->thisSurfaceIndex]);
					
					if (g2.mCustomSkin && !g2.mCustomShader) {
						q3skin_ptr skin = skins.get(g2.mCustomSkin);
						auto const & iter = skin->lookup.find(surfH->name);
						if (iter == skin->lookup.end())
							continue;
						shader = iter->second.shader;
					} else if (g2.mCustomShader) {
						shader = shaders.get(g2.mCustomShader);
					}
					
					if (!shader->valid) continue;
					
					std::vector<qm::mat4_t> bone_array;
					
					int const * refs = reinterpret_cast<int const *>(reinterpret_cast<byte const *>(surf) + surf->ofsBoneReferences);
					for (int32_t i = 0; i < surf->numBoneReferences; i++) {
						
						mdxaBone_t const & bone = g2.mBoneCache->EvalRender(refs[i]);
						
						qm::mat4_t bone_mat {
							bone.matrix[1][1], -bone.matrix[1][2], bone.matrix[1][0], 0,
							bone.matrix[2][1], -bone.matrix[2][2], bone.matrix[2][0], 0,
							bone.matrix[0][1], -bone.matrix[0][2], bone.matrix[0][0], 0,
							bone.matrix[1][3], bone.matrix[2][3], bone.matrix[0][3], 0,
						};
						
						bone_array.emplace_back(bone_mat);
					}
					
					q3drawmesh draw {mesh.second, model_matrix * vp};
					draw.weights = std::move(bone_array);
					draw.m = model_matrix * v;
					draw.shader_color = convert_4u8(obj.ref.shaderRGBA);
					
					if (shader->gridlit && m_world)
						draw.gridlight = m_world->calculate_gridlight((obj.ref.renderfx & RF_LIGHTING_ORIGIN) ? obj.ref.lightingOrigin : obj.ref.origin);
						
					if (debug_enabled) debug_meshes.emplace_back(draw);
					draw_map[shader].emplace_back(draw);
				}
			}
		}
		
		//================================================================
		// PRIMITIVE -- SPRITES AND OTHER PRIMITIVE RENDERABLES
		//================================================================
		
		//================
		// SPRITES & ORIENTED QUADS
		//================
		
		for (auto const & obj : scene.sprites) {
			do_sprite(
				{obj.ref.origin[1], -obj.ref.origin[2], obj.ref.origin[0]}, 
				obj.ref.radius, 
				obj.ref.rotation, 
				hw_inst->shaders.get(obj.ref.customShader), 
				convert_4u8(obj.ref.shaderRGBA)
			);
		}
		
		//================
		// BEAMS
		//================
		
		for (auto const & obj : scene.beams) {
			// what even is a beam?
			/*
			qm::vec3_t origin = obj.ref.origin, old_origin = obj.ref.oldorigin;
			qm::vec3_t dir = old_origin - origin;
			qm::vec3_t dirn = dir;
			if (!dirn.normalize()) continue;
			
			qm::vec3_t perpvec;
			PerpendicularVector(perpvec.ptr(), dirn.ptr());
			perpvec *= 4;
			
			*/
		}
		
		//================
		// LINES
		//================
		
		for (auto const & obj : scene.lines) {
			qm::vec3_t start = {obj.ref.origin[1], -obj.ref.origin[2], obj.ref.origin[0]};
			qm::vec3_t end = {obj.ref.oldorigin[1], -obj.ref.oldorigin[2], obj.ref.oldorigin[0]};
			
			qm::vec3_t right = qm::vec3_t::cross(start - view_origin, end - view_origin).normalized();
			qm::vec3_t xyz;
			
			VectorMA(start.ptr(), obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v0 = {
				xyz,
				qm::vec2_t { 0, 0 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(start.ptr(), -obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v1 = {
				xyz,
				qm::vec2_t { 1, 0 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(end.ptr(), obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v2 = {
				xyz,
				qm::vec2_t { 0, 1 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(end.ptr(), -obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v3 = {
				xyz,
				qm::vec2_t { 1, 1 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			std::vector<sprite_assembly::vertex_t> & sasm = sprites_verticies[hw_inst->shaders.get(obj.ref.customShader)];
			sasm.emplace_back(v0);
			sasm.emplace_back(v1);
			sasm.emplace_back(v2);
			sasm.emplace_back(v2);
			sasm.emplace_back(v1);
			sasm.emplace_back(v3);
		}
		
		for (auto const & obj : scene.oriented_lines) {
			qm::vec3_t start = {obj.ref.origin[1], -obj.ref.origin[2], obj.ref.origin[0]};
			qm::vec3_t end = {obj.ref.oldorigin[1], -obj.ref.oldorigin[2], obj.ref.oldorigin[0]};
			
			qm::vec3_t right = {obj.ref.axis[2][1], -obj.ref.axis[2][2], obj.ref.axis[2][0]};
			right = -right;
			right.normalize();
			qm::vec3_t xyz;
			
			VectorMA(start.ptr(), obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v0 = {
				xyz,
				qm::vec2_t { 0, 0 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(start.ptr(), -obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v1 = {
				xyz,
				qm::vec2_t { 1, 0 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(end.ptr(), obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v2 = {
				xyz,
				qm::vec2_t { 0, 1 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			VectorMA(end.ptr(), -obj.ref.radius, right.ptr(), xyz.ptr());
			sprite_assembly::vertex_t v3 = {
				xyz,
				qm::vec2_t { 1, 1 }, qm::vec3_t { 0, 1, 0 }, convert_4u8(obj.ref.shaderRGBA)
			};
			
			std::vector<sprite_assembly::vertex_t> & sasm = sprites_verticies[hw_inst->shaders.get(obj.ref.customShader)];
			sasm.emplace_back(v0);
			sasm.emplace_back(v1);
			sasm.emplace_back(v2);
			sasm.emplace_back(v2);
			sasm.emplace_back(v1);
			sasm.emplace_back(v3);
		}
		
		//================
		// SABER GLOW
		//================
		
		for (auto const & obj : scene.saber_glow) {
			
			qm::vec3_t origin = {obj.ref.origin[1], -obj.ref.origin[2], obj.ref.origin[0]};
			qm::vec3_t axis = {obj.ref.axis[0][1], -obj.ref.axis[0][2], obj.ref.axis[0][0]};
			auto shader = hw_inst->shaders.get(obj.ref.customShader);
			auto color = convert_4u8(obj.ref.shaderRGBA);
			float radius = obj.ref.radius;
			
			for (float i = obj.ref.saberLength; i > 0; i -= radius * 0.65f) {
				qm::vec3_t end;
				VectorMA(origin.ptr(), i, axis.ptr(), end.ptr());
				do_sprite(end, radius, 0, shader, color);
				radius += 0.017f;
			}
			do_sprite(origin, Q_flrand(5.5f, 5.75f), 0, shader, color);
		}
		
		//================
		
		for (auto & [shad, verts] : sprites_verticies) {
			q3drawmesh sprite_draw {std::make_shared<sprite_assembly>(verts.data(), verts.size()), vp};
			if (debug_enabled) debug_meshes.emplace_back(sprite_draw);
			
			q3drawset & set = additonal_draws.emplace_back();
			set.sort_offset = 0.1f;
			set.shader = shad;
			set.meshes.emplace_back(std::move(sprite_draw));
		}
		
		//================================================================
		
		std::vector<std::pair<q3shader_ptr, std::vector<q3drawmesh>>> skyboxes;
		
		std::vector<q3drawset> draw_set;
		
		for (auto & [shader, meshes] : draw_map) {
			if (shader && shader->nodraw) continue;
			if (shader && shader->sky_parms) {
				skyboxes.emplace_back(shader, std::move(meshes));
				continue;
			}
			draw_set.emplace_back(
				(shader && shader->valid) ? shader : shaders.get(0),
				std::move(meshes)
			);
		}
		
		for (auto & draw : additonal_draws) {
			if (!draw.shader || !draw.shader->valid) draw.shader = shaders.get(0);
			if (draw.shader->nodraw) continue;
			draw_set.emplace_back(std::move(draw));
		}
		
		std::sort(draw_set.begin(), draw_set.end(), q3drawset::compare);
		
		// ================================
		// SKYBOXES
		// ================================
		
		if (!skyportal_drawn) {
		
			gl::initialize();
			gl::polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
			gl::stencil_test(true);
			gl::depth_test(true);
			gl::depth_func(GL_LESS);
			gl::depth_write(true);
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			
			q3skyboxstencilprog->bind();
			
			// stencil skybox zones
			GLuint stencil_id = 1;
			for (auto const & sbp : skyboxes) {
				gl::stencil_func(GL_ALWAYS, stencil_id);
				gl::stencil_op(GL_KEEP, GL_KEEP, GL_REPLACE);
				for (auto const & dmesh : sbp.second) {
					q3skyboxstencilprog->mvp(dmesh.mvp);
					dmesh.mesh->draw();
				}
				stencil_id ++;
			}
			
			gl::depth_test(false);
			main_sampler->wrap(GL_CLAMP_TO_EDGE);
			q3skyboxprog->bind();
			q3skyboxprog->mvp(sbvp);
			
			for (auto i = 0; i < 6; i++)
				main_sampler->bind(BINDING_SKYBOX + i);
			
			// draw skyboxes on correct stencil
			stencil_id = 1;
			for (auto const & sbp : skyboxes) {
				if (!sbp.first->sky_parms->sides[0]) continue; // skybox has no textures
				gl::stencil_func(GL_EQUAL, stencil_id);
				gl::stencil_op(GL_KEEP, GL_KEEP, GL_KEEP);
				for (auto i = 0; i < 6; i++)
					sbp.first->sky_parms->sides[i]->bind(BINDING_SKYBOX + i);
				skybox->draw();
				stencil_id ++;
			}
		
		}
		
		if (scene.ref.rdflags & RDF_SKYBOXPORTAL && scene.ref.rdflags & RDF_DRAWSKYBOX)
			skyportal_drawn = true;
		
		// ================================
		// REGULAR GEOMETRY
		// ================================
		
		gl::initialize();
		gl::polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
		gl::depth_test(true);
		gl::stencil_test(true);
		gl::depth_write(true);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		q3mainprog->bind();
		q3mainprog->lightstyles(lightstyles);
		
		if (m_world && m_world->m_lightmap) m_world->m_lightmap->bind(BINDING_LIGHTMAP);
		
		q3stage::setup_draw_parameters_t params {};
		
		for (q3drawset const & draw : draw_set) {
			if (draw.shader->nodraw) continue;
			draw.shader->setup_draw();
			for (auto const & stg : draw.shader->stages)
				for (auto const & mesh : draw.meshes) {
					if (!mesh.mesh) continue;
					params.time = time;
					params.mvp = mesh.mvp;
					params.itm = mesh.itm;
					params.m = mesh.m;
					params.mesh_uniforms = mesh.mesh->uniform_info();
					params.shader_color = mesh.shader_color;
					params.bone_weights = mesh.weights.size() ? &mesh.weights : nullptr;
					params.view_origin = view_origin;
					params.gridlight = &mesh.gridlight;
					stg.setup_draw(params);
					mesh.mesh->draw();
			}
		}
		
		// ================================
	}
	
	gl::initialize();
	gl::polygon_mode(GL_FRONT_AND_BACK, GL_FILL);
	gl::depth_test(false);
	gl::blend(true);
	gl::cull(false);
	
	main_sampler->bind(BINDING_DIFFUSE);
	
	m_ui_draw = true;
	m_shader_color = {1, 1, 1, 1};
	
	for (auto & cmd : drawframe->ui_stretchpics) {
		if (!cmd.shader || !cmd.shader->valid) cmd.shader = shaders.get(0);
		
		qm::mat4_t m = qm::mat4_t::scale(cmd.w ? cmd.w : cmd.h * (640.0f / 480.0f), cmd.h, 1);
		m *= qm::mat4_t::translate(cmd.x, cmd.y, 0);
		
		qm::mat3_t uv = qm::mat3_t::scale(cmd.s2 - cmd.s1, cmd.t2 - cmd.t1);
		uv *= qm::mat3_t::translate(cmd.s1, cmd.t1);
		
		m_shader_color = cmd.color;
		qm::mat4_t mvp = m * ui_ortho;
		if (debug_enabled) debug_meshes.emplace_back(unitquad, mvp);
		for (auto const & stg : cmd.shader->stages) {
			q3stage::setup_draw_parameters_t params {};
			params.time = time;
			params.mvp = mvp;
			params.uvm = uv;
			params.is_2d = true;
			stg.setup_draw(params);
			unitquad->draw();
		}
	}
	
	//================================================================
	// CUSTOM SHADER PROGRAMS
	//================================================================
	if (debug_enabled) {
		std::sort(debug_meshes.begin(), debug_meshes.end(), [&](q3drawmesh const & A, q3drawmesh const & B) -> bool{ return A.mesh < B.mesh; });
		if (r_showtris->integer || r_showedges->integer) {
			gl::initialize();
			gl::polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
			gl::line_width(r_showtris->value);
			gl::blend(true);
			gl::blend(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
			
			if (r_showtris->integer) {
				gl::stencil_test(true);
				gl::stencil_mask(1);
				gl::stencil_func(GL_EQUAL, 0);
				gl::stencil_op(GL_KEEP, GL_KEEP, GL_INCR);
			}
			
			glClear(GL_STENCIL_BUFFER_BIT);
			
			q3lineprog->bind();
			
			for (q3drawmesh const & d : debug_meshes) {
				q3lineprog->mvp(d.mvp);
				if (d.weights.size())
					q3lineprog->bone_matricies(d.weights.data(), d.weights.size());
				else
					q3lineprog->bone_matricies(nullptr, 0);
				d.mesh->draw();
			}
		}
	}
	//================================================================
	
	ri.WIN_Present(&window);
	
	#ifdef _DEBUG
	if (r_drawcalls->integer)
		Com_Printf("Draw Calls: %zu\n", q3mesh::m_debug_draw_count);
	q3mesh::m_debug_draw_count = 0;
	#endif
}
