#include "hw_local.hh"
using namespace howler;

#include <stack>
#include <unordered_set>

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
}

q3model_ptr q3world::get_vis_model(qm::vec3_t coords) {
	
	int32_t cluster = -1;
	
	if (m_vis && r_vis->integer > 0) {
		q3worldnode const * leaf = find_leaf(coords);
		cluster = std::get<q3worldnode::leaf_data>(leaf->data).cluster;
	} else if (m_vis && r_vis->integer < 0) {
		cluster = -r_vis->integer;
	}
	
	if (cluster < 0) cluster = -1;
	if (cluster >= m_clusters)
		cluster = m_clusters - 1;
	
	if (r_lockpvs->integer)
		cluster = m_lockpvs_cluster;
	else
		m_lockpvs_cluster = cluster;
	
	auto cached_model = m_vis_cache.find(cluster);
	if (cached_model != m_vis_cache.end())
		return cached_model->second;
	
	q3model_ptr & model = m_vis_cache[cluster] = make_q3model();
	
	std::unordered_map<q3shader_ptr, q3protosurface> buckets;
	
	std::unordered_set<q3worldsurface const *> marked_surfaces;
	
	if (cluster >= 0) {
		byte const * cluster_vis = &m_vis->m_cluster_data[cluster * m_vis->m_cluster_bytes];
		for (q3worldnode const & node : m_nodes) {
			std::visit( lambda_visit{
				[&](q3worldnode::node_data const &) {},
				[&](q3worldnode::leaf_data const & data) {
					if (data.cluster <= 0 || data.cluster >= m_clusters) return;
					if (!(cluster_vis[data.cluster>>3] & (1<<(data.cluster&7)))) return; // WTF???
					for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
				}
			}, node.data);
		}
	} else {
		for (q3worldnode const & node : m_nodes) {
			std::visit( lambda_visit{
				[&](q3worldnode::node_data const &) {},
				[&](q3worldnode::leaf_data const & data) {
					if (data.cluster <= 0 || data.cluster >= m_clusters) return;
					for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
				}
			}, node.data);
		}
	}
	
	for (q3worldsurface const * surf : marked_surfaces) {
		q3protosurface & proto = buckets[surf->shader];
		proto.append(surf->proto);
	}
	
	for (auto & [shader, proto] : buckets) {
		q3mesh_ptr mesh = model->meshes.emplace_back(shader, make_q3mesh()).second;
		mesh->upload_verts(proto.verticies);
		mesh->upload_uvs(proto.uvs);
		mesh->upload_lightmap_uvs(proto.lightmap_uvs);
		mesh->upload_lightmap_styles(proto.lightmap_styles);
	}
	
	return model;
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
	
	for (int32_t i = 0; i < num_surfs; i++) {
		
		dsurface_t const & surfi = dsurf[i];
		q3worldsurface & surfo = m_surfaces[i];
		
		surfo.info = &surfi;
		surfo.shader = hw_inst->shaders.reg(m_shaders[surfi.shaderNum].shader, true, true);
		switch (surfi.surfaceType) {
			default: break;
			//================================
			case MST_PLANAR: {
				mapVert_t const * surf_verts = dvert + surfi.firstVert;
				int const * surf_indicies = dindx + surfi.firstIndex;
				
				for (int32_t i = 0; i < surfi.numIndexes; i ++) {
					
					surfo.proto.verticies.emplace_back(
						surf_verts[surf_indicies[i]].xyz[1],
						-surf_verts[surf_indicies[i]].xyz[2],
						surf_verts[surf_indicies[i]].xyz[0]
					);
					
					surfo.proto.uvs.emplace_back(
						surf_verts[surf_indicies[i]].st[0],
						surf_verts[surf_indicies[i]].st[1]
					);
					
					surfo.proto.lightmap_uvs.emplace_back(
						lightmap_uvs_t {
							    uv_for_lightmap( surfi.lightmapNum[0], qm::vec2_t {
								surf_verts[surf_indicies[i]].lightmap[0][0],
								surf_verts[surf_indicies[i]].lightmap[0][1]
							}), uv_for_lightmap( surfi.lightmapNum[1], qm::vec2_t {
								surf_verts[surf_indicies[i]].lightmap[1][0],
								surf_verts[surf_indicies[i]].lightmap[1][1]
							}), uv_for_lightmap( surfi.lightmapNum[2], qm::vec2_t {
								surf_verts[surf_indicies[i]].lightmap[2][0],
								surf_verts[surf_indicies[i]].lightmap[2][1]
							}), uv_for_lightmap( surfi.lightmapNum[3], qm::vec2_t {
								surf_verts[surf_indicies[i]].lightmap[3][0],
								surf_verts[surf_indicies[i]].lightmap[3][1]
							})
						}
					);
					
					surfo.proto.lightmap_styles.emplace_back(
						lightmap_styles_t {
							surfi.lightmapStyles[0],
							surfi.lightmapStyles[1],
							surfi.lightmapStyles[2],
							surfi.lightmapStyles[3]
						}
					);
				} break;
			//================================
			}
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
		if (data.cluster > m_clusters) m_clusters = data.cluster;
		
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
	
}

//================================================================
// VISIBILITY
//================================================================

void q3world::load_visibility() {
	
	lump_t const & l = m_header->lumps[LUMP_VISIBILITY];
	if (!l.filelen) return;
	
	m_vis = std::make_unique<q3vis>();
	
	int32_t const * bufi = base<int32_t>(l.fileofs);
	
	m_vis->m_clusters = bufi[0];
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

	m_light_grid_size[0] = 64;
	m_light_grid_size[1] = 64;
	m_light_grid_size[2] = 128;

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
			sscanf(value, "%f %f %f", &m_light_grid_size[0], &m_light_grid_size[1], &m_light_grid_size[2] );
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

void q3world::load_lightgrid() {
	
}

//================================================================
// LIGHTGRID ARRAY
//================================================================

void q3world::load_lightgridarray() {
	
}
