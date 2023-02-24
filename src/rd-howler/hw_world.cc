#include "hw_local.hh"
using namespace howler;

#include <stack>
#include <unordered_set>

//================================================================
// WORLD MESH TYPE
//================================================================

struct q3world::q3worldrendermesh : public q3mesh {
	
	q3mesh_ptr world_mesh;
	GLuint idx_ary;
	GLsizei idx_num;
	
	q3worldrendermesh() : q3mesh() {
		glCreateBuffers(1, &idx_ary);
	}
	
	~q3worldrendermesh() {
		glDeleteBuffers(1, &idx_ary);
	}
	
	void upload_indicies(GLuint const * ptr, size_t num) {
		assert(num % 3 == 0);
		if (!ptr || !num) return;
		idx_num = num;
		glNamedBufferData(idx_ary, num * sizeof(GLuint), ptr, GL_STATIC_DRAW);
	}
	
	void bind() override {
		if (!world_mesh) return;
		world_mesh->bind();
	}
	
	void draw() override {
		if (!world_mesh) return;
		world_mesh->bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_ary);
		glDrawElements(GL_TRIANGLES, idx_num, GL_UNSIGNED_INT, nullptr);
	}
	
	bool is_bound() const override {
		return world_mesh ? world_mesh->is_bound() : false;
	}
	
	uniform_info_t const * uniform_info() const override { 
		return world_mesh ? world_mesh->uniform_info() : nullptr;
	}
	
};

//================================================================

q3world::q3world() {
	
}

q3world::~q3world() {
	
}

void q3world::load(char const * name) {
	
	m_name = name;
	
	char basename_temp[MAX_QPATH];
	Q_strncpyz(basename_temp, COM_SkipPath(name), sizeof(basename_temp));
	COM_StripExtension(basename_temp, basename_temp, sizeof(basename_temp));
	m_basename = basename_temp;
	
	void * base_ptr = ri.CM_GetCachedMapDiskImage();
	if (!base_ptr) {
		auto len = ri.FS_ReadFile(name, &base_ptr);
		if (!base_ptr) Com_Error (ERR_DROP, "q3world::load: %s not found", name);
		m_bspr_alloc.resize(len);
		memcpy(m_bspr_alloc.data(), base_ptr, len);
		ri.FS_FreeFile(base_ptr);
		base_ptr = m_bspr_alloc.data();
	}
	
	m_bspr.rebase(reinterpret_cast<uint8_t const *>(base_ptr));
	
	load_shaders();
	load_lightmaps();
	load_planes();
	load_fogs();
	load_surfaces(0);
	load_nodesleafs();
	load_submodels();
	load_visibility();
	load_entities();
	load_lightgrid();
	load_lightgridarray();
	
	build_world_meshes();
}

//================================================================
// RENDERABLE MODEL CALCULATION
//================================================================

q3world::q3world_draw const & q3world::get_vis_model(refdef_t const & ref) {
	
	std::array<byte, 32> areamask;
	memcpy(areamask.data(), ref.areamask, 32);
	int32_t cluster = -1;
	
	if (m_vis && r_vis->integer > 0) {
		q3worldnode const * leaf = find_leaf(ref.vieworg);
		cluster = std::get<q3worldnode::leaf_data>(leaf->data).cluster;
	} else if (m_vis && r_vis->integer < 0) {
		cluster = -r_vis->integer;
	}
	
	if (cluster < 0) cluster = -1;
	else if (m_vis && cluster > m_vis->header.clusters)
		cluster = m_vis->header.clusters - 1;
	
	if (r_lockpvs->integer)
		cluster = m_lockpvs_cluster;
	else
		m_lockpvs_cluster = cluster;
	
	auto iter = std::find_if(m_vis_cache.begin(), m_vis_cache.end(), [&](vis_cache_t const & i){return std::get<0>(i) == cluster && std::get<2>(i) == areamask;});
	if (iter != m_vis_cache.end()) return std::get<1>(*iter);
	
	if (r_viscachesize->integer > 0 && m_vis_cache.size() > static_cast<unsigned>(r_viscachesize->integer)) m_vis_cache.resize(r_viscachesize->integer);
	
	std::unordered_set<q3worldsurface const *> marked_surfaces;
	
	if (cluster >= 0) {
		auto cluster_vis = m_vis->cluster(cluster);
		for (size_t i = m_nodes_leafs_offset; i < m_nodes.size(); i++) {
			auto const & node = m_nodes[i];
			if (!node.parent) continue; // non-parented leaf means bmodel
			std::visit( lambda_visit{
				[&](std::monostate) {assert(0);},
				[&](q3worldnode::node_data const &) {},
				[&](q3worldnode::leaf_data const & data) {
					if (data.cluster < 0 || data.cluster >= m_vis->header.clusters) return;
					if (!cluster_vis.can_see(data.cluster)) return;
					if ((ref.areamask[data.area>>3] & (1<<(data.area&7)))) return;
					for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
				}
			}, node.data);
		}
	} else {
		for (size_t i = m_nodes_leafs_offset; i < m_nodes.size(); i++) {
			auto const & node = m_nodes[i];
			if (!node.parent) continue;
			std::visit( lambda_visit{
				[&](std::monostate) {assert(0);},
				[&](q3worldnode::node_data const &) {},
				[&](q3worldnode::leaf_data const & data) {
					if (data.cluster < 0) return;
					for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
				}
			}, node.data);
		}
	}
	
	q3world_draw & world_draw = std::get<1>(*m_vis_cache.emplace(m_vis_cache.begin(), cluster, q3world_draw {make_q3model(), {}}, areamask));
	std::unordered_map<q3shader_ptr, q3worldmesh_maplit_proto> buckets_maplit;
	std::unordered_map<q3shader_ptr, q3worldmesh_vertexlit_proto> buckets_vertlit;
	
	for (q3worldsurface const * surf : marked_surfaces) {
		std::visit( lambda_visit {
			[&](std::monostate const &) {},
			[&](q3worldmesh_maplit_proto const & proto) {
				q3worldmesh_maplit_proto & sub = buckets_maplit[surf->shader];
				sub.append_indicies(proto);
			},
			[&](q3worldmesh_vertexlit_proto const & proto) {
				q3worldmesh_vertexlit_proto & sub = buckets_vertlit[surf->shader];
				sub.append_indicies(proto);
			},
			[&](q3worldmesh_flare const & flare) {
				world_draw.flares[surf->shader].push_back(flare);
			}
		}, surf->proto);
	}
	
	q3model_ptr & model = world_draw.model;
	
	for (auto & [shader, proto] : buckets_maplit) {
		auto rend = std::make_shared<q3worldrendermesh>();
		rend->world_mesh = m_world_meshes[shader].maplit;
		rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
		model->meshes.emplace_back(shader, rend);
	}
	
	for (auto & [shader, proto] : buckets_vertlit) {
		auto rend = std::make_shared<q3worldrendermesh>();
		rend->world_mesh = m_world_meshes[shader].vertexlit;
		rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
		model->meshes.emplace_back(shader, rend);
	}
	
	return world_draw;
}

q3world::q3worldnode const * q3world::find_leaf(qm::vec3_t const & coords) {
	
	q3worldnode const * node = &m_nodes[0];
	while (true) {
		if (!std::holds_alternative<q3worldnode::node_data>(node->data)) break;
		auto const & data = std::get<q3worldnode::node_data>(node->data);
		
		if (DotProduct(coords, data.plane->normal) - data.plane->dist > 0)
			node = data.children[0];
		else
			node = data.children[1];
	}
	return node;
}

//================================================================
// WORLD MESHES
//================================================================

void q3world::build_world_meshes() {
	std::unordered_set<q3worldsurface *> marked_surfaces;
	std::unordered_map<q3shader_ptr, std::vector<q3worldsurface *>> buckets_maplit;
	std::unordered_map<q3shader_ptr, std::vector<q3worldsurface *>> buckets_vertlit;
	
	// determine which leafs are part of worldspawn
	for (size_t i = m_nodes_leafs_offset; i < m_nodes.size(); i++) {
		auto const & node = m_nodes[i];
		if (!node.parent) continue;
		std::visit( lambda_visit{
			[&](std::monostate) {assert(0);},
			[&](q3worldnode::node_data const &) {},
			[&](q3worldnode::leaf_data const & data) {
				if (data.cluster < 0) return;
				for (auto const & surf : data.surfaces) {
					marked_surfaces.emplace(surf);
				}
			}
		}, node.data);
	}
	
	// bucket them by shader and lighting type
	for (q3worldsurface * surf : marked_surfaces) {
		if (std::holds_alternative<q3worldmesh_flare>(surf->proto)) continue;
		assert(surf->info->index_count % 3 == 0);
		std::visit( lambda_visit {
			[&](std::monostate const &) {},
			[&](q3worldmesh_maplit_proto const & proto) {
				assert(proto.indicies.size() % 3 == 0);
				buckets_maplit[surf->shader].push_back(surf);
			},
			[&](q3worldmesh_vertexlit_proto const & proto) {
				assert(proto.indicies.size() % 3 == 0);
				buckets_vertlit[surf->shader].push_back(surf);
			},
			[&](q3worldmesh_flare const & flare) { assert(0); }
		}, surf->proto);
	}
	
	// set index offsets and finalize
	GLuint start_idx;
	for (auto & [shad, vec] : buckets_maplit) {
		start_idx = 0;
		
		q3worldmesh_maplit_proto master;
		
		for (auto & v : vec) {
			auto & proto = std::get<q3worldmesh_maplit_proto>(v->proto);
			v->index_start = start_idx;
			v->index_num = 0;
			for (GLuint & idx : proto.indicies) {
				if (idx > v->index_num) v->index_num = idx;
				idx += start_idx;
			}
			master.append_verticies(proto);
			start_idx += ++v->index_num;
		}
		
		m_world_meshes[shad].maplit = master.generate();
	}
	
	
	for (auto & [shad, vec] : buckets_vertlit) {
		start_idx = 0;
		
		q3worldmesh_vertexlit_proto master;
		
		for (auto & v : vec) {
			auto & proto = std::get<q3worldmesh_vertexlit_proto>(v->proto);
			v->index_start = start_idx;
			v->index_num = 0;
			for (GLuint & idx : proto.indicies) {
				if (idx > v->index_num) v->index_num = idx;
				idx += start_idx;
			}
			master.append_verticies(proto);
			start_idx += ++v->index_num;
		}
		
		m_world_meshes[shad].vertexlit = master.generate();
	}
}

//================================================================
// SHADERS
//================================================================

void q3world::load_shaders() {
	m_shaders = m_bspr.shaders();
}

//================================================================
// LIGHTMAPS
//================================================================

void q3world::load_lightmaps() {
	
	auto lightmaps = m_bspr.lightmaps();
	
	if (lightmaps.size() <= 0) return;
	
	m_lightmap = make_q3texture(LIGHTMAP_DIM, LIGHTMAP_DIM, lightmaps.size(), true);
	m_lightmap->clear();
	m_lightmap->set_transparent(false);

	for (size_t lightmap_i = 0; lightmap_i < lightmaps.size(); lightmap_i++)
			m_lightmap->upload3D(LIGHTMAP_DIM, LIGHTMAP_DIM, 1, lightmaps[lightmap_i].pixels, GL_RGB, GL_UNSIGNED_BYTE, 0, 0, lightmap_i);

	m_lightmap->generate_mipmaps();
}

//================================================================
// PLANES
//================================================================

void q3world::load_planes() {

	auto bplanes = m_bspr.planes();
	planes.resize(bplanes.size());

	for (size_t i = 0; i < bplanes.size(); i++) {
		BSP::Plane const & bplane = bplanes[i];
		cplane_t & cplane = planes[i];
		
		int32_t bits = 0;
		for (int32_t j = 0; j < 3; j++) {
			cplane.normal[j] = bplane.normal[j];
			if (cplane.normal[j] < 0) bits |= 1 << j;
		}
		
		cplane.dist = bplane.dist;
		cplane.type = PlaneTypeForNormal(cplane.normal);
		cplane.signbits = bits;
	}
}

//================================================================
// FOGS
//================================================================

void q3world::load_fogs() {
	
}

//================================================================
// SURFACES
//================================================================

static constexpr qm::vec4_t conv_color(byte const * arr) {
	return qm::vec4_t {
		static_cast<float>(arr[0]) / 255.0f,
		static_cast<float>(arr[1]) / 255.0f,
		static_cast<float>(arr[2]) / 255.0f,
		static_cast<float>(arr[3]) / 255.0f,
	};
}

// IMPORTED FROM RD-VANILLA
#define LIGHTMAP_2D			-4		// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX	-3		// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE	-2
#define	LIGHTMAP_NONE		-1
// ----

static constexpr uint8_t conv_lm_mode(int32_t lm_idx) {
	if (lm_idx >= 0) return LIGHTMAP_MODE_MAP;
	if (lm_idx == LIGHTMAP_BY_VERTEX) return LIGHTMAP_MODE_VERTEX;
	if (lm_idx == LIGHTMAP_WHITEIMAGE) return LIGHTMAP_MODE_WHITEIMAGE;
	else return LIGHTMAP_MODE_NONE;
}

qm::vec2_t q3world::uv_for_lightmap(int32_t idx, qm::vec2_t uv_in) {
	return uv_in;
	/*
	if (!m_lightmap_span) return {0, 0};
	int32_t x = idx % m_lightmap_span;
	int32_t y = idx / m_lightmap_span;
	qm::vec2_t uv_base {static_cast<float>(x), static_cast<float>(y)};
	return (uv_base + uv_in) / m_lightmap_span;
	*/
}

void q3world::load_surfaces(int32_t idx) {

	auto bsurfs = m_bspr.surfaces();
	auto bdvert = m_bspr.drawverts();
	auto bdindx = m_bspr.drawindices();
	
	m_surfaces.resize(bsurfs.size());
	
	for (size_t s = 0; s < bsurfs.size(); s++) {
		
		BSP::Surface const & surfi = bsurfs[s];
		q3worldsurface & surfo = m_surfaces[s];
		
		surfo.info = &surfi;
		
		surfo.shader = hw_inst->shaders.reg(m_shaders[surfi.shader].shader, true, default_shader_mode::lightmap);
		if (surfo.shader->nodraw || (m_shaders[surfi.shader].surface_flags & SURF_NODRAW)) continue;
		
		lightstylesidx_t lm_styles;
		memcpy(lm_styles.data(), surfi.lightmap_styles, 4);
		
		enum struct lmode_e {
			none,
			map,
			vertex
		} lmode = lmode_e::none;
		
		switch (surfi.type) {
			default: break;
			//================================
			// BRUSH AND TRIANGLE SOUP
			//================================
		case BSP::SurfaceType::TRISOUP:
			lmode = lmode_e::vertex;
			[[fallthrough]];
		case BSP::SurfaceType::PLANAR: {
				
			BSP::DrawVert const * surf_verts = &bdvert[surfi.vert_idx];
			int32_t const * surf_indicies = &bdindx[surfi.index_idx];

			if (lmode == lmode_e::none) {
				switch(conv_lm_mode(surfi.lightmap[0])) {
					case LIGHTMAP_MODE_NONE: break;
					case LIGHTMAP_MODE_VERTEX: lmode = lmode_e::vertex; break;
					default:
					case LIGHTMAP_MODE_MAP: lmode = lmode_e::map; break;
				}
			}

			lightstylesidx_t styles {
				lmode == lmode_e::vertex ? surfi.vertex_styles[0] : surfi.lightmap_styles[0],
				lmode == lmode_e::vertex ? surfi.vertex_styles[1] : surfi.lightmap_styles[1],
				lmode == lmode_e::vertex ? surfi.vertex_styles[2] : surfi.lightmap_styles[2],
				lmode == lmode_e::vertex ? surfi.vertex_styles[3] : surfi.lightmap_styles[3],
			};

			if (lmode == lmode_e::vertex) {
				q3worldmesh_vertexlit_proto & proto = surfo.proto.emplace<q3worldmesh_vertexlit_proto>();
				for (int32_t i = 0; i < surfi.vert_count; i ++) {
					proto.verticies.emplace_back( q3worldmesh_vertexlit::vertex_t {
						qm::vec3_t {
							surf_verts[i].pos[1],
							surf_verts[i].pos[2],
							surf_verts[i].pos[0]
						},
						qm::vec2_t {
							surf_verts[i].uv[0],
							surf_verts[i].uv[1]
						},
						qm::vec3_t {
							surf_verts[i].normal[1],
							surf_verts[i].normal[2],
							surf_verts[i].normal[0]
						},
						lightmap_color_t {
							conv_color(surf_verts[i].color[0]),
							conv_color(surf_verts[i].color[1]),
							conv_color(surf_verts[i].color[2]),
							conv_color(surf_verts[i].color[3]),
						},
						styles
					});
				}

				for (int32_t i = 0; i < surfi.index_count; i ++) {
					proto.indicies.push_back(surf_indicies[i]);
				}

			} else if (lmode == lmode_e::map) {
				q3worldmesh_maplit_proto & proto = surfo.proto.emplace<q3worldmesh_maplit_proto>();
				for (int32_t i = 0; i < surfi.vert_count; i ++) {
					proto.verticies.emplace_back( q3worldmesh_maplit::vertex_t {
						qm::vec3_t {
							surf_verts[i].pos[1],
							surf_verts[i].pos[2],
							surf_verts[i].pos[0]
						},
						qm::vec2_t {
							surf_verts[i].uv[0],
							surf_verts[i].uv[1]
						},
						qm::vec3_t {
							surf_verts[i].normal[1],
							surf_verts[i].normal[2],
							surf_verts[i].normal[0]
						},
						conv_color(surf_verts[i].color[0]),
						lightmap_idx_t {
							static_cast<uint16_t>(surfi.lightmap[0] > 0 ? surfi.lightmap[0] : 0),
							static_cast<uint16_t>(surfi.lightmap[1] > 0 ? surfi.lightmap[1] : 0),
							static_cast<uint16_t>(surfi.lightmap[2] > 0 ? surfi.lightmap[2] : 0),
							static_cast<uint16_t>(surfi.lightmap[3] > 0 ? surfi.lightmap[3] : 0),
						},
						lightmap_uv_t {
							uv_for_lightmap( surfi.lightmap[0], qm::vec2_t {
								surf_verts[i].lightmap[0][0],
								surf_verts[i].lightmap[0][1]
							}),
							uv_for_lightmap( surfi.lightmap[1], qm::vec2_t {
								surf_verts[i].lightmap[1][0],
								surf_verts[i].lightmap[1][1]
							}),
							uv_for_lightmap( surfi.lightmap[2], qm::vec2_t {
								surf_verts[i].lightmap[2][0],
								surf_verts[i].lightmap[2][1]
							}),
							uv_for_lightmap( surfi.lightmap[3], qm::vec2_t {
								surf_verts[i].lightmap[3][0],
								surf_verts[i].lightmap[3][1]
							})
						},
						styles
					});
				}

				for (int32_t i = 0; i < surfi.index_count; i ++) {
					proto.indicies.push_back(surf_indicies[i]);
				}
			}
		} break;
		//================================
		// PATCH MESH SOUP
		//================================
		case BSP::SurfaceType::PATCH: {

			BSP::DrawVert const * surf_verts = &bdvert[surfi.vert_idx];
			int32_t num_points = surfi.patch_width * surfi.patch_height;

			q3patchsurface surf;
			surf.width = surfi.patch_width;
			surf.height = surfi.patch_height;

			if (lmode == lmode_e::none) {
				switch(conv_lm_mode(surfi.lightmap[0])) {
					case LIGHTMAP_MODE_NONE: break;
					case LIGHTMAP_MODE_VERTEX: lmode = lmode_e::vertex; break;
					default:
					case LIGHTMAP_MODE_MAP: lmode = lmode_e::map; break;
				}
			}

			surf.vertex_lit = lmode == lmode_e::vertex;

			surf.styles = lightstylesidx_t {
				lmode == lmode_e::vertex ? surfi.vertex_styles[0] : surfi.lightmap_styles[0],
				lmode == lmode_e::vertex ? surfi.vertex_styles[1] : surfi.lightmap_styles[1],
				lmode == lmode_e::vertex ? surfi.vertex_styles[2] : surfi.lightmap_styles[2],
				lmode == lmode_e::vertex ? surfi.vertex_styles[3] : surfi.lightmap_styles[3],
			};

			for (int32_t i = 0; i < num_points; i ++) {
				surf.verts.emplace_back( q3patchvert {
					qm::vec3_t {surf_verts[i].pos[1], surf_verts[i].pos[2], surf_verts[i].pos[0]},
					qm::vec2_t {surf_verts[i].uv[0], surf_verts[i].uv[1]},
					qm::vec3_t {surf_verts[i].normal[1], surf_verts[i].normal[2], surf_verts[i].normal[0]},
					lightmap_idx_t {
						static_cast<uint16_t>(surfi.lightmap[0] > 0 ? surfi.lightmap[0] : 0),
						static_cast<uint16_t>(surfi.lightmap[1] > 0 ? surfi.lightmap[1] : 0),
						static_cast<uint16_t>(surfi.lightmap[2] > 0 ? surfi.lightmap[2] : 0),
						static_cast<uint16_t>(surfi.lightmap[3] > 0 ? surfi.lightmap[3] : 0),
					},
					{
						uv_for_lightmap(surfi.lightmap[0], surf_verts[i].lightmap[0]),
						uv_for_lightmap(surfi.lightmap[1], surf_verts[i].lightmap[1]),
						uv_for_lightmap(surfi.lightmap[2], surf_verts[i].lightmap[2]),
						uv_for_lightmap(surfi.lightmap[3], surf_verts[i].lightmap[3]),
					}, {
						conv_color(surf_verts[i].color[0]),
						conv_color(surf_verts[i].color[1]),
						conv_color(surf_verts[i].color[2]),
						conv_color(surf_verts[i].color[3]),
					},
					lm_styles
				});
			}

			surfo.proto = surf.process();
		} break;
		//================================
		// PATCH MESH SOUP
		//================================
		case BSP::SurfaceType::FLARE: {
			surfo.proto = q3worldmesh_flare {
				qm::vec3_t { surfi.lightmap_origin[1], -surfi.lightmap_origin[2], surfi.lightmap_origin[0] },
				qm::vec3_t { surfi.lightmap_vectors[2][1], -surfi.lightmap_vectors[2][2], surfi.lightmap_vectors[2][0] },
				surfi.lightmap_vectors[0],
			};
		} break;
		//================================
		}
	}
}
//================================================================
// NODES & LEAFS
//================================================================

void q3world::load_nodesleafs() {

	auto dnodes = m_bspr.nodes();
	auto dleafs = m_bspr.leafs();
	auto dmarks = m_bspr.leafsurfaces();
	
	m_nodes.resize(dnodes.size() + dleafs.size());
	
	for (size_t i = 0; i < dnodes.size(); i++) {
		
		q3worldnode & node = m_nodes[i];
		auto const & dnode = dnodes[i];
		
		q3worldnode::node_data & data = node.data.emplace<q3worldnode::node_data>();
		
		for (int32_t j = 0; j < 3; j++) {
			node.mins[j] = dnode.mins[j];
			node.maxs[j] = dnode.maxs[j];
		}
		
		data.plane = &planes[dnode.plane];
		
		for (int32_t j = 0; j < 2; j++) {
			int32_t offs = dnode.children[j];
			if (offs >= 0) data.children[j] = &m_nodes[offs];
			else data.children[j] = &m_nodes[( -offs - 1) + dnodes.size()];
		}
	}
	
	m_nodes_leafs_offset = dnodes.size();
	
	for (size_t i = 0; i < dleafs.size(); i++) {
		
		q3worldnode & leaf = m_nodes[dnodes.size() + i];
		auto const & dleaf = dleafs[i];
		
		q3worldnode::leaf_data & data = leaf.data.emplace<q3worldnode::leaf_data>();
		
		for (int32_t j = 0; j < 3; j++) {
			leaf.mins[j] = dleaf.mins[j];
			leaf.maxs[j] = dleaf.maxs[j];
		}
		
		data.cluster = dleaf.cluster;
		data.area = dleaf.area;
		
		data.surfaces.resize(dleaf.num_surfaces);
		for (int32_t j = 0; j < dleaf.num_surfaces; j++) {
			int32_t idx = dmarks[dleaf.first_surface + j];
			data.surfaces[j] = &m_surfaces[idx];
		}
	}
	
	struct reparent_iterator {
		reparent_iterator(q3worldnode * node, q3worldnode * parent) : node(node), parent(parent) {}
		q3worldnode * node = nullptr;
		q3worldnode * parent = nullptr;
	};
	
	std::stack<reparent_iterator> reparent_stack;
	reparent_stack.emplace(&m_nodes[0], nullptr);
	
	while (reparent_stack.size()) {
		reparent_iterator cur = reparent_stack.top();
		reparent_stack.pop();
		
		cur.node->parent = cur.parent;
		
		if (std::holds_alternative<q3worldnode::leaf_data>(cur.node->data)) continue;
		q3worldnode::node_data & data = std::get<q3worldnode::node_data>(cur.node->data);
		
		reparent_stack.emplace(data.children[0], cur.node);
		reparent_stack.emplace(data.children[1], cur.node);
	}
}

//================================================================
// SUBMODELS
//================================================================

void q3world::load_submodels() {
	
	m_models = m_bspr.models();
	
	for (size_t i = 0; i < m_models.size(); i++) {
		
		auto const & in = m_models[i];
		
		q3basemodel_ptr mod = make_q3basemodel();
		
		mod->buffer.resize(sizeof(bmodel_t));
		bmodel_t & bmod = *(mod->base.bmodel = reinterpret_cast<bmodel_t *>(mod->buffer.data()));
		
		bmod = {};
		mod->base.type = MOD_BRUSH;
		
		VectorCopy(in.mins, bmod.bounds[0]);
		VectorCopy(in.maxs, bmod.bounds[1]);
		
		bmod.surf_idx = in.first_surface;
		bmod.surf_num = in.num_surfaces;
		
		Com_sprintf( mod->base.name, sizeof( mod->base.name ), "*%d", i);
		
		q3model & model = *(mod->model = make_q3model());
		for (int32_t m = 0; m < bmod.surf_num; m++) {
			q3worldsurface const & surf = m_surfaces[bmod.surf_idx + m];
			
			if (std::holds_alternative<q3worldmesh_flare>(surf.proto)) continue;
			auto rend = std::make_shared<q3worldrendermesh>();
			std::visit( lambda_visit {
				[&](std::monostate const &) {},
				[&](q3worldmesh_maplit_proto const & proto) { 
					rend->world_mesh = proto.generate(); 
					rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
				},
				[&](q3worldmesh_vertexlit_proto const & proto) { 
					rend->world_mesh = proto.generate(); 
					rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
				},
				[&](q3worldmesh_flare const &) { assert(0); }
			}, surf.proto);
			model.meshes.emplace_back(surf.shader, rend);
		}
		
		hw_inst->models.reg(mod);
	}
}

//================================================================
// VISIBILITY
//================================================================

void q3world::load_visibility() {
	
	if (m_bspr.has_visibility())
		m_vis = std::make_unique<BSP::Reader::Visibility>( m_bspr.visibility() );
	/*
	lump_t const & l = m_header->lumps[LUMP_VISIBILITY];
	if (!l.filelen) return;
	
	m_vis = std::make_unique<q3vis>();
	
	int32_t const * bufi = base<int32_t>(l.fileofs);
	
	m_vis->m_max_cluster = bufi[0];
	m_vis->m_cluster_bytes = bufi[1];
	
	m_vis->m_cluster_data.resize(l.filelen - 8);
	memcpy(m_vis->m_cluster_data.data(), bufi + 2, m_vis->m_cluster_data.size());
	*/
}

//================================================================
// ENTITIES
//================================================================

void q3world::load_entities() {

	char const *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	float ambient = 1;

	m_lightgrid_size[0] = 64;
	m_lightgrid_size[1] = 64;
	m_lightgrid_size[2] = 128;

	p = m_bspr.entities().data();

	// store for reference by the cgame
	m_entity_string = p;
	m_entity_parse_point = m_entity_string.data();

	COM_BeginParseSession ("world_t::load_entities");

	token = COM_ParseExt( &p, qtrue );
	if (!*token || *token != '{') {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {
		// parse key
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(keyname, token, sizeof(keyname));

		// parse value
		token = COM_ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz(value, token, sizeof(value));

		// check for remapping of shaders for vertex lighting
		s = va("vertexremapshader");
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			/*
			if (r_vertexLight->integer) {
				R_RemapShader(value, s, "0");
			}
			*/
			continue;
		}
		// check for remapping of shaders
		s = va("remapshader");
		if (!Q_strncmp(keyname, s, strlen(s)) ) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			//R_RemapShader(value, s, "0");
			continue;
		}
 		if (!Q_stricmp(keyname, "distanceCull")) {
			float cull;
			sscanf(value, "%f", &cull );
			hw_inst->m_cull = cull;
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &m_lightgrid_size[0], &m_lightgrid_size[1], &m_lightgrid_size[2] );
			continue;
		}
	// find the optional world ambient for arioche
	/*
		if (!Q_stricmp(keyname, "_color")) {
			sscanf(value, "%f %f %f", &tr.sunAmbient[0], &tr.sunAmbient[1], &tr.sunAmbient[2] );
			continue;
		}
		*/
		if (!Q_stricmp(keyname, "ambient")) {
			sscanf(value, "%f", &ambient);
			continue;
		}
	}
	//both default to 1 so no harm if not present.
	//VectorScale( tr.sunAmbient, ambient, tr.sunAmbient);
}

qboolean q3world::get_entity_token(char * buffer, int size) {
	const char	*s;

	if (size <= 0)
	{ //force reset
		m_entity_parse_point = m_entity_string.c_str();
		return qtrue;
	}

	s = COM_Parse( (const char **) &m_entity_parse_point );
	Q_strncpyz( buffer, s, size );
	if ( !m_entity_parse_point || !s[0] ) {
		return qfalse;
	} else {
		return qtrue;
	}
}

//================================================================
// LIGHTGRID
//================================================================

gridlighting_t q3world::calculate_gridlight(qm::vec3_t const & pos) {
	gridlighting_t ret {};
	if (!m_lightmap) return ret;
	
	qm::vec3_t direction {0, 0, 0};
	qm::vec3_t frac;
	qm::vec3_t npos = pos - m_lightgrid_origin;
	std::array<int32_t, 3> ipos;
	
	for (auto i = 0; i < 3; i++) {
		float v = npos[i] / m_lightgrid_size[i];
		ipos[i] = std::floor(v);
		frac[i] = v - ipos[i];
		if (ipos[i] < 0) ipos[i] = 0;
		else if (ipos[i] >= m_lightgrid_bounds[i] - 1)
			ipos[i] = m_lightgrid_bounds[i] - 1;
	}
	
	std::array<int32_t, 3> step {1, m_lightgrid_bounds[0], m_lightgrid_bounds[0] * m_lightgrid_bounds[1]};
	uint16_t const * start = m_lightarray.data() + (ipos[0] * step[0] + ipos[1] * step[1] + ipos[2] * step[2]);
	
	float factor_total = 0;
	for (auto i = 0; i < 8; i++) {
		
		float factor = 1.0f;
		uint16_t const * gpos = start;
		
		for (auto j = 0; j < 3; j++) {
			if (i & (1 << j)) {
				factor *= frac[j];
				gpos += step[j];
			} else factor *= (1.0 - frac[j]);
		}
		
		if (gpos >= m_lightarray.data() + m_lightarray.size()) continue;
		
		BSP::Lightgrid const * data = m_lightgrid.data() + *gpos;
		if (data->styles[0] == LS_LSNONE) continue;
		
		factor_total += factor;
		
		for (uint_fast16_t j = 0; j < LIGHTMAP_NUM; j++) {
			if (data->styles[j] == LS_LSNONE) break;
			ret.ambient[0] += factor * (data->ambient[j].r / 255.0f) * hw_inst->lightstyles[data->styles[j]][0];
			ret.ambient[1] += factor * (data->ambient[j].g / 255.0f) * hw_inst->lightstyles[data->styles[j]][1];
			ret.ambient[2] += factor * (data->ambient[j].b / 255.0f) * hw_inst->lightstyles[data->styles[j]][2];
			ret.directed[0] += factor * (data->direct[j].r / 255.0f) * hw_inst->lightstyles[data->styles[j]][0];
			ret.directed[1] += factor * (data->direct[j].g / 255.0f) * hw_inst->lightstyles[data->styles[j]][1];
			ret.directed[2] += factor * (data->direct[j].b / 255.0f) * hw_inst->lightstyles[data->styles[j]][2];
		}
		
		qm::vec3_t normal;
		float lat = data->longitude * (2 * qm::pi) / 255.0f;
		float lng = data->latitude * (2 * qm::pi) / 255.0f;
		normal[0] = std::cos(lat) * std::sin(lng);
		normal[1] = std::sin(lat) * std::sin(lng);
		normal[2] = std::cos(lng);
		VectorMA(&direction[0], factor, &normal[0], &direction[0]);
	}
	
	if (factor_total > 0 && factor_total < 0.99) {
		factor_total = 1.0f / factor_total;
		ret.ambient *= factor_total;
		ret.directed *= factor_total;
	}
	
	ret.direction = {direction[1], direction[2], direction[0]};
	ret.direction.normalize();
	
	if (r_vanilla_gridlighting->integer)
		ret.ambient += 0.125f; // vanilla ambient minimum
	else
		ret.ambient += 0.125f * ret.directed; // new ambient minimum calc
	
	return ret;
}

void q3world::load_lightgrid() {
	
	for (auto i = 0; i < 3; i++) {
		m_lightgrid_origin[i] = m_lightgrid_size[i] * std::ceil( m_models[0].mins[i] / m_lightgrid_size[i] );
		m_lightgrid_bounds[i] = ((m_lightgrid_size[i] * std::floor( m_models[0].maxs[i] / m_lightgrid_size[i])) - m_lightgrid_origin[i] ) / m_lightgrid_size[i] + 1;
	}
	
	m_lightgrid = m_bspr.lightgrids();
}

void q3world::load_lightgridarray() {
	
	m_lightarray = m_bspr.lightarray();

	size_t expect = m_lightgrid_bounds[0] * m_lightgrid_bounds[1] * m_lightgrid_bounds[2];
	if (m_lightarray.size() != expect) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: light grid array mismatch: expected %lu, found %zu\n", expect, m_lightarray.size());
		m_lightarray = {};
	}
}

void q3world::load_debug() {

}
