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
		world_mesh->bind();
	}
	
	void draw() override {
		world_mesh->bind();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_ary);
		glDrawElements(GL_TRIANGLES, idx_num, GL_UNSIGNED_INT, nullptr);
	}
	
	bool is_bound() const override {
		return world_mesh->is_bound();
	}
	
	uniform_info_t const * uniform_info() const override { 
		return world_mesh->uniform_info();
	}
	
};

//================================================================

q3world::q3world() {
	
}

q3world::~q3world() {
	if (m_base_allocated && m_base) ri.FS_FreeFile(m_base);
}

void q3world::load(char const * name) {
	
	m_name = name;
	
	char basename_temp[MAX_QPATH];
	Q_strncpyz(basename_temp, COM_SkipPath(name), sizeof(basename_temp));
	COM_StripExtension(basename_temp, basename_temp, sizeof(basename_temp));
	m_basename = basename_temp;
	
	m_base = reinterpret_cast<byte *>(ri.CM_GetCachedMapDiskImage());
	if (!m_base) {
		ri.FS_ReadFile(name, (void **)&m_base);
		if (!m_base) Com_Error (ERR_DROP, "q3world::load: %s not found", name);
		m_base_allocated = true;
	}
	
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
	if (cluster > m_max_cluster)
		cluster = m_max_cluster;
	
	if (r_lockpvs->integer)
		cluster = m_lockpvs_cluster;
	else
		m_lockpvs_cluster = cluster;
	
	auto iter = std::find_if(m_vis_cache.begin(), m_vis_cache.end(), [&](vis_cache_t const & i){return std::get<0>(i) == cluster && std::get<2>(i) == areamask;});
	if (iter != m_vis_cache.end()) return std::get<1>(*iter);
	
	if (r_viscachesize->integer > 0 && m_vis_cache.size() > static_cast<unsigned>(r_viscachesize->integer)) m_vis_cache.resize(r_viscachesize->integer);
	
	std::unordered_set<q3worldsurface const *> marked_surfaces;
	
	if (cluster >= 0) {
		byte const * cluster_vis = &m_vis->m_cluster_data[cluster * m_vis->m_cluster_bytes];
		for (size_t i = m_nodes_leafs_offset; i < m_nodes.size(); i++) {
			auto const & node = m_nodes[i];
			if (!node.parent) continue; // non-parented leaf means bmodel
			std::visit( lambda_visit{
				[&](std::monostate) {assert(0);},
				[&](q3worldnode::node_data const &) {},
				[&](q3worldnode::leaf_data const & data) {
					if (data.cluster < 0 || data.cluster > m_max_cluster) return;
					if (!(cluster_vis[data.cluster>>3] & (1<<(data.cluster&7)))) return; // WTF???
					if ( (ref.areamask[data.area>>3] & (1<<(data.area&7)) ) ) return;
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
					if (data.cluster < 0 || data.cluster > m_max_cluster) return;
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
				if (data.cluster < 0 || data.cluster > m_max_cluster) return;
				for (auto const & surf : data.surfaces) {
					marked_surfaces.emplace(surf);
				}
			}
		}, node.data);
	}
	
	// bucket them by shader and lighting type
	for (q3worldsurface * surf : marked_surfaces) {
		if (std::holds_alternative<q3worldmesh_flare>(surf->proto)) continue;
		assert(surf->info->numIndexes % 3 == 0);
		std::visit( lambda_visit {
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
	
	lump_t const & l = m_header->lumps[LUMP_SHADERS];
	
	m_shaders = base<dshader_t>(l.fileofs);
	m_shaders_count = l.filelen / sizeof(dshader_t);
}

//================================================================
// LIGHTMAPS
//================================================================

void q3world::load_lightmaps() {
	
	lump_t const & l = m_header->lumps[LUMP_LIGHTMAPS];
	
	byte const * buffer = base<byte>(l.fileofs);
	int32_t buffer_len = l.filelen;
	
	int32_t num_lightmaps = buffer_len / LIGHTMAP_BYTES;
	m_lightmap_span = qm::next_pow2(static_cast<int32_t>(std::ceil(std::sqrt(static_cast<float>(num_lightmaps)))));
	size_t atlas_dim = m_lightmap_span * LIGHTMAP_DIM;
	
	if (buffer_len <= 0 || num_lightmaps <= 0) return;
	
	m_lightmap = make_q3texture(atlas_dim, atlas_dim, true);
	m_lightmap->clear();
	m_lightmap->set_transparent(false);
	
	for (int32_t atlas_y = 0, lightmap_i = 0; lightmap_i < num_lightmaps; atlas_y++)
		for (int32_t atlas_x = 0; atlas_x < m_lightmap_span && lightmap_i < num_lightmaps; atlas_x++, lightmap_i++, buffer += LIGHTMAP_BYTES) {
			
			std::array<std::array<uint8_t, 4>, LIGHTMAP_PIXELS> image;
			static_assert(sizeof(image) == LIGHTMAP_PIXELS * 4);
			
			for (size_t j = 0; j < LIGHTMAP_PIXELS; j++) {
				image[j][0] = buffer[j * 3 + 0];
				image[j][1] = buffer[j * 3 + 1];
				image[j][2] = buffer[j * 3 + 2];
				image[j][3] = 0xFF;
			}
			
			m_lightmap->upload(LIGHTMAP_DIM, LIGHTMAP_DIM, &image[0][0], GL_RGBA, GL_UNSIGNED_BYTE, atlas_x * LIGHTMAP_DIM, atlas_y * LIGHTMAP_DIM);
	}
	
	m_lightmap->generate_mipmaps();
}

//================================================================
// PLANES
//================================================================

void q3world::load_planes() {

	lump_t const & l = m_header->lumps[LUMP_PLANES];
	
	dplane_t const * dplane = base<dplane_t>(l.fileofs);
	int32_t dplanes_num = l.filelen / sizeof(dplane_t);
	
	planes.resize(dplanes_num);
	for (int32_t i = 0; i < dplanes_num; i++, dplane++) {
		cplane_t & plane = planes[i];
		
		int32_t bits = 0;
		for (int32_t j = 0; j < 3; j++) {
			plane.normal[j] = dplane->normal[j];
			if (plane.normal[j] < 0) bits |= 1 << j;
		}
		
		plane.dist = dplane->dist;
		plane.type = PlaneTypeForNormal(plane.normal);
		plane.signbits = bits;
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
	int32_t x = idx % m_lightmap_span;
	int32_t y = idx / m_lightmap_span;
	qm::vec2_t uv_base {static_cast<float>(x), static_cast<float>(y)};
	return (uv_base + uv_in) / m_lightmap_span;
}

void q3world::load_surfaces(int32_t idx) {
	
	lump_t const & lsurf = m_header->lumps[LUMP_SURFACES];
	lump_t const & lvert = m_header->lumps[LUMP_DRAWVERTS];
	lump_t const & lindx = m_header->lumps[LUMP_DRAWINDEXES];
	
	dsurface_t const * dsurf = base<dsurface_t>(lsurf.fileofs);
	mapVert_t const * dvert = base<mapVert_t>(lvert.fileofs);
	int32_t const * dindx = base<int32_t>(lindx.fileofs);
	
	int32_t num_surfs = lsurf.filelen / sizeof(dsurface_t);
	m_surfaces.resize(num_surfs);
	
	for (int32_t s = 0; s < num_surfs; s++) {
		
		dsurface_t const & surfi = dsurf[s];
		q3worldsurface & surfo = m_surfaces[s];
		
		surfo.info = &surfi;
		
		surfo.shader = hw_inst->shaders.reg(m_shaders[surfi.shaderNum].shader, true, default_shader_mode::lightmap);
		if (surfo.shader->nodraw) continue;
		
		lightstylesidx_t lm_styles;
		memcpy(lm_styles.data(), surfi.lightmapStyles, 4);
		
		enum struct lmode_e {
			none,
			map,
			vertex
		} lmode = lmode_e::none;
		
		switch (surfi.surfaceType) {
			default: break;
			//================================
			// BRUSH AND TRIANGLE SOUP
			//================================
			case MST_TRIANGLE_SOUP:
				lmode = lmode_e::vertex;
				[[fallthrough]];
			case MST_PLANAR: {
				
				mapVert_t const * surf_verts = dvert + surfi.firstVert;
				int32_t const * surf_indicies = dindx + surfi.firstIndex;
				
				if (lmode == lmode_e::none) {
					switch(conv_lm_mode(surfi.lightmapNum[0])) {
						case LIGHTMAP_MODE_NONE: break;
						case LIGHTMAP_MODE_VERTEX: lmode = lmode_e::vertex; break;
						default:
						case LIGHTMAP_MODE_MAP: lmode = lmode_e::map; break;
					}
				}
				
				lightstylesidx_t styles {
					lmode == lmode_e::vertex ? surfi.vertexStyles[0] : surfi.lightmapStyles[0],
					lmode == lmode_e::vertex ? surfi.vertexStyles[1] : surfi.lightmapStyles[1],
					lmode == lmode_e::vertex ? surfi.vertexStyles[2] : surfi.lightmapStyles[2],
					lmode == lmode_e::vertex ? surfi.vertexStyles[3] : surfi.lightmapStyles[3],
				};
				
				if (lmode == lmode_e::vertex) {
					q3worldmesh_vertexlit_proto & proto = surfo.proto.emplace<q3worldmesh_vertexlit_proto>();
					for (int32_t i = 0; i < surfi.numVerts; i ++) {
						proto.verticies.emplace_back( q3worldmesh_vertexlit::vertex_t {
							qm::vec3_t {
								surf_verts[i].xyz[1],
								surf_verts[i].xyz[2],
								surf_verts[i].xyz[0]
							},
							qm::vec2_t {
								surf_verts[i].st[0],
								surf_verts[i].st[1]
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
					
					for (int32_t i = 0; i < surfi.numIndexes; i ++) {
						proto.indicies.push_back(surf_indicies[i]);
					}
					
				} else if (lmode == lmode_e::map) {
					q3worldmesh_maplit_proto & proto = surfo.proto.emplace<q3worldmesh_maplit_proto>();
					for (int32_t i = 0; i < surfi.numVerts; i ++) {
						proto.verticies.emplace_back( q3worldmesh_maplit::vertex_t {
							qm::vec3_t {
								surf_verts[i].xyz[1],
								surf_verts[i].xyz[2],
								surf_verts[i].xyz[0]
							},
							qm::vec2_t {
								surf_verts[i].st[0],
								surf_verts[i].st[1]
							},
							qm::vec3_t {
								surf_verts[i].normal[1],
								surf_verts[i].normal[2],
								surf_verts[i].normal[0]
							},
							conv_color(surf_verts[i].color[0]),
							lightmap_uv_t {
								uv_for_lightmap( surfi.lightmapNum[0], qm::vec2_t {
									surf_verts[i].lightmap[0][0],
									surf_verts[i].lightmap[0][1]
								}),
								uv_for_lightmap( surfi.lightmapNum[1], qm::vec2_t {
									surf_verts[i].lightmap[1][0],
									surf_verts[i].lightmap[1][1]
								}),
								uv_for_lightmap( surfi.lightmapNum[2], qm::vec2_t {
									surf_verts[i].lightmap[2][0],
									surf_verts[i].lightmap[2][1]
								}),
								uv_for_lightmap( surfi.lightmapNum[3], qm::vec2_t {
									surf_verts[i].lightmap[3][0],
									surf_verts[i].lightmap[3][1]
								})
							},
							styles
						});
					}
					
					for (int32_t i = 0; i < surfi.numIndexes; i ++) {
						proto.indicies.push_back(surf_indicies[i]);
					}
				}
			} break;
			//================================
			// PATCH MESH SOUP
			//================================
			case MST_PATCH: {
				
				mapVert_t const * surf_verts = dvert + surfi.firstVert;
				int32_t num_points = surfi.patchWidth * surfi.patchHeight;
				
				q3patchsurface surf;
				surf.width = surfi.patchWidth;
				surf.height = surfi.patchHeight;
				
				if (lmode == lmode_e::none) {
					switch(conv_lm_mode(surfi.lightmapNum[0])) {
						case LIGHTMAP_MODE_NONE: break;
						case LIGHTMAP_MODE_VERTEX: lmode = lmode_e::vertex; break;
						default:
						case LIGHTMAP_MODE_MAP: lmode = lmode_e::map; break;
					}
				}
				
				surf.vertex_lit = lmode == lmode_e::vertex;
				
				surf.styles = lightstylesidx_t {
					lmode == lmode_e::vertex ? surfi.vertexStyles[0] : surfi.lightmapStyles[0],
					lmode == lmode_e::vertex ? surfi.vertexStyles[1] : surfi.lightmapStyles[1],
					lmode == lmode_e::vertex ? surfi.vertexStyles[2] : surfi.lightmapStyles[2],
					lmode == lmode_e::vertex ? surfi.vertexStyles[3] : surfi.lightmapStyles[3],
				};
				
				for (int32_t i = 0; i < num_points; i ++) {
					surf.verts.emplace_back( q3patchvert {
						qm::vec3_t {surf_verts[i].xyz[1], surf_verts[i].xyz[2], surf_verts[i].xyz[0]},
						qm::vec2_t {surf_verts[i].st[0], surf_verts[i].st[1]},
						qm::vec3_t {surf_verts[i].normal[1], surf_verts[i].normal[2], surf_verts[i].normal[0]},
						{
							uv_for_lightmap(surfi.lightmapNum[0], surf_verts[i].lightmap[0]),
							uv_for_lightmap(surfi.lightmapNum[1], surf_verts[i].lightmap[1]),
							uv_for_lightmap(surfi.lightmapNum[2], surf_verts[i].lightmap[2]),
							uv_for_lightmap(surfi.lightmapNum[3], surf_verts[i].lightmap[3]),
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
			case MST_FLARE: {
				surfo.proto = q3worldmesh_flare {
					qm::vec3_t { surfi.lightmapOrigin[1], -surfi.lightmapOrigin[2], surfi.lightmapOrigin[0] },
					qm::vec3_t { surfi.lightmapVecs[2][1], -surfi.lightmapVecs[2][2], surfi.lightmapVecs[2][0] },
					surfi.lightmapVecs[0],
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

	lump_t const & lmark = m_header->lumps[LUMP_LEAFSURFACES];
	lump_t const & lnode = m_header->lumps[LUMP_NODES];
	lump_t const & lleaf = m_header->lumps[LUMP_LEAFS];
	
	int32_t const * dmarks = base<int32_t>(lmark.fileofs);
	dnode_t const * dnodes = base<dnode_t>(lnode.fileofs);
	dleaf_t const * dleafs = base<dleaf_t>(lleaf.fileofs);
	
	int32_t num_nodes = lnode.filelen / sizeof(dnode_t);
	int32_t num_leafs = lleaf.filelen / sizeof(dleaf_t);
	
	m_nodes.resize(num_nodes + num_leafs);
	
	for (int32_t i = 0; i < num_nodes; i++) {
		
		q3worldnode & node = m_nodes[i];
		dnode_t const & dnode = dnodes[i];
		
		q3worldnode::node_data & data = node.data.emplace<q3worldnode::node_data>();
		
		for (int32_t j = 0; j < 3; j++) {
			node.mins[j] = dnode.mins[j];
			node.maxs[j] = dnode.maxs[j];
		}
		
		data.plane = &planes[dnode.planeNum];
		
		for (int32_t j = 0; j < 2; j++) {
			int32_t offs = dnode.children[j];
			if (offs >= 0) data.children[j] = &m_nodes[offs];
			else data.children[j] = &m_nodes[( -offs - 1) + num_nodes];
		}
	}
	
	m_nodes_leafs_offset = num_nodes;
	
	for (int32_t i = 0; i < num_leafs; i++) {
		
		q3worldnode & leaf = m_nodes[num_nodes + i];
		dleaf_t const & dleaf = dleafs[i];
		
		q3worldnode::leaf_data & data = leaf.data.emplace<q3worldnode::leaf_data>();
		
		for (int32_t j = 0; j < 3; j++) {
			leaf.mins[j] = dleaf.mins[j];
			leaf.maxs[j] = dleaf.maxs[j];
		}
		
		data.cluster = dleaf.cluster;
		data.area = dleaf.area;
		if (data.cluster > m_max_cluster) m_max_cluster = data.cluster;
		
		data.surfaces.resize(dleaf.numLeafSurfaces);
		for (int32_t j = 0; j < dleaf.numLeafSurfaces; j++) {
			int32_t idx = dmarks[dleaf.firstLeafSurface + j];
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
	
	lump_t const & l = m_header->lumps[LUMP_MODELS];
	
	dmodels = base<dmodel_t>(l.fileofs);
	int32_t num_dmodels = l.filelen / sizeof(dmodel_t);
	
	for (int32_t i = 0; i < num_dmodels; i++) {
		
		dmodel_t const & in = dmodels[i];
		
		q3basemodel_ptr mod = make_q3basemodel();
		
		mod->buffer.resize(sizeof(bmodel_t));
		bmodel_t & bmod = *(mod->base.bmodel = reinterpret_cast<bmodel_t *>(mod->buffer.data()));
		
		bmod = {};
		mod->base.type = MOD_BRUSH;
		
		VectorCopy(in.mins, bmod.bounds[0]);
		VectorCopy(in.maxs, bmod.bounds[1]);
		
		bmod.surf_idx = in.firstSurface;
		bmod.surf_num = in.numSurfaces;
		
		Com_sprintf( mod->base.name, sizeof( mod->base.name ), "*%d", i);
		
		q3model & model = *(mod->model = make_q3model());
		for (int32_t m = 0; m < bmod.surf_num; m++) {
			q3worldsurface const & surf = m_surfaces[bmod.surf_idx + m];
			
			if (std::holds_alternative<q3worldmesh_flare>(surf.proto)) continue;
			auto rend = std::make_shared<q3worldrendermesh>();
			std::visit( lambda_visit {
				[&](q3worldmesh_maplit_proto const & proto) { 
					rend->world_mesh = proto.generate(); 
					rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
				},
				[&](q3worldmesh_vertexlit_proto const & proto) { 
					rend->world_mesh = proto.generate(); 
					rend->upload_indicies(proto.indicies.data(), proto.indicies.size());
				},
				[&](q3worldmesh_flare const & flare) { assert(0); }
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
	
	lump_t const & l = m_header->lumps[LUMP_VISIBILITY];
	if (!l.filelen) return;
	
	m_vis = std::make_unique<q3vis>();
	
	int32_t const * bufi = base<int32_t>(l.fileofs);
	
	m_vis->m_max_cluster = bufi[0];
	m_vis->m_cluster_bytes = bufi[1];
	
	m_vis->m_cluster_data.resize(l.filelen - 8);
	memcpy(m_vis->m_cluster_data.data(), bufi + 2, m_vis->m_cluster_data.size());
}

//================================================================
// ENTITIES
//================================================================

void q3world::load_entities() {
	
	lump_t const & l = m_header->lumps[LUMP_ENTITIES];
	
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	float ambient = 1;

	m_lightgrid_size[0] = 64;
	m_lightgrid_size[1] = 64;
	m_lightgrid_size[2] = 128;

	p = (char *)(m_base + l.fileofs);

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
	uint16_t const * start = m_lightgrid_array + (ipos[0] * step[0] + ipos[1] * step[1] + ipos[2] * step[2]);
	
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
		
		if (gpos >= m_lightgrid_array + m_lightgrid_array_num) continue;
		
		lightgrid_t const * data = m_lightgrid + *gpos;
		if (data->styles[0] == LS_LSNONE) continue;
		
		factor_total += factor;
		
		for (uint_fast16_t j = 0; j < LIGHTMAP_NUM; j++) {
			if (data->styles[j] == LS_LSNONE) break;
			ret.ambient[0] += factor * (data->ambientLight[j][0] / 255.0f) * hw_inst->lightstyles[data->styles[j]][0];
			ret.ambient[1] += factor * (data->ambientLight[j][1] / 255.0f) * hw_inst->lightstyles[data->styles[j]][1];
			ret.ambient[2] += factor * (data->ambientLight[j][2] / 255.0f) * hw_inst->lightstyles[data->styles[j]][2];
			ret.directed[0] += factor * (data->directLight[j][0] / 255.0f) * hw_inst->lightstyles[data->styles[j]][0];
			ret.directed[1] += factor * (data->directLight[j][1] / 255.0f) * hw_inst->lightstyles[data->styles[j]][1];
			ret.directed[2] += factor * (data->directLight[j][2] / 255.0f) * hw_inst->lightstyles[data->styles[j]][2];
		}
		
		qm::vec3_t normal;
		float lat = data->latLong[1] * (2 * qm::pi) / 255.0f;
		float lng = data->latLong[0] * (2 * qm::pi) / 255.0f;
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
	
	lump_t const & l = m_header->lumps[LUMP_LIGHTGRID];
	
	for (auto i = 0; i < 3; i++) {
		m_lightgrid_origin[i] = m_lightgrid_size[i] * std::ceil( dmodels[0].mins[i] / m_lightgrid_size[i] );
		m_lightgrid_bounds[i] = ((m_lightgrid_size[i] * std::floor( dmodels[0].maxs[i] / m_lightgrid_size[i])) - m_lightgrid_origin[i] ) / m_lightgrid_size[i] + 1;
	}
	
	m_lightgrid_num = l.filelen / sizeof(lightgrid_t);;
	m_lightgrid = base<lightgrid_t>(l.fileofs);
}

void q3world::load_lightgridarray() {
	
	lump_t const & l = m_header->lumps[LUMP_LIGHTARRAY];
	
	m_lightgrid_array_num = m_lightgrid_bounds[0] * m_lightgrid_bounds[1] * m_lightgrid_bounds[2];
	if (static_cast<size_t>(l.filelen) != m_lightgrid_array_num * sizeof(*m_lightgrid_array)) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "WARNING: light grid array mismatch: expected %lu, found %u\n", m_lightgrid_array_num * sizeof(*m_lightgrid_array), l.filelen);
		m_lightgrid_array = nullptr;
	}
	
	m_lightgrid_array = base<uint16_t>(l.fileofs);
}
