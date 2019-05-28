#include "tr_local.hh"

#include <stack>
#include <unordered_set>

void rend::load_world(char const * name) {
	
	world.reset( new world_t );
	world->load(name);
}

// copied directly from rd-vanilla R_GetEntityToken, not sure what it does
qboolean rend::get_entity_token(char * buffer, int size) {
	const char	*s;

	if (size <= 0)
	{ //force reset
		world->entityParsePoint = world->entityString.data();
		return qtrue;
	}

	s = COM_Parse( (const char **) &world->entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !world->entityParsePoint || !s[0] ) {
		return qfalse;
	} else {
		return qtrue;
	}
}

void rend::world_t::load(char const * name) {
	
	base = reinterpret_cast<byte const *>(ri.CM_GetCachedMapDiskImage());
	if (!base) {
		ri.FS_ReadFile(name, (void **)&base);
		if (!base) Com_Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
	}
	
	Q_strncpyz(this->name, name, sizeof( this->name ));
	Q_strncpyz(this->baseName, COM_SkipPath( this->name ), sizeof( this->baseName ));
	COM_StripExtension( this->baseName, this->baseName, sizeof( this->baseName ) );
	
	load_shaders();
	load_lightmaps();
	load_planes();
	// TODO -- fogs
	load_surfaces(0);
	load_nodesleafs();
	// TODO -- submodels
	load_visibility();
	load_entities();
	// TODO -- lightgrid
	// TODO -- lightgridarray
}

q3model_ptr rend::world_t::get_model(int32_t visnum) {
	
	if (visnum < 0) visnum = -1;
	else if (!vis) visnum = -1;
	else if (visnum >= vis->data.size()) visnum = vis->data.size() - 1;
	
	for (auto const & entry : vis_cache_queue) {
		if (entry.cluster == visnum) return entry.model;
	}
	
	if (vis_cache_queue.size() > 20) vis_cache_queue.erase(vis_cache_queue.end() - 1);
	vis_cache_queue.emplace(vis_cache_queue.begin());
	
	vis_cache_queue[0].cluster = visnum;
	auto & cache_model = vis_cache_queue[0].model = std::make_shared<q3model>();
	
	struct protomodel {
		std::vector<float> vert_data;
		std::vector<float> uv_data;
		std::vector<float> lightmap_data;
		std::vector<float> lightmap_active;
	};
	
	std::unordered_set<surface_t const *> marked_surfaces;
	
	if (visnum > 0) {
		
		/*
		for (int32_t cluster = 0; cluster < >render_clusters.size(); cluster++) {
			q3model_ptr const & ptr = world->render_clusters[cluster];
			if (!ptr) continue;
			
			if ( (scene.def.areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) ) {
				continue;		// not visible
			}
			
			for (auto const & wmesh : ptr->meshes) {
				rmap[wmesh.shader->sort].emplace_back(&wmesh);
			}
		}
		*/
	}
	
	if (visnum >= 0) {
		
		byte const * cluster_vis = &vis->data[visnum * vis->cluster_bytes];
		
		for (mapnode_t const & node : nodes) {
			
			std::visit(lambda_visit{
					[&](mapnode_t::node_data const &){},
					[&](mapnode_t::leaf_data const & data){
						if ( !(cluster_vis[data.cluster>>3] & (1<<(data.cluster&7))) ) {
							return;
						}
						for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
					}
				}, node.data);
			
		}
		
	} else {
		for (mapnode_t const & node : nodes) {
			std::visit(lambda_visit{
					[&](mapnode_t::node_data const &){},
					[&](mapnode_t::leaf_data const & data){
						for (auto const & surf : data.surfaces) marked_surfaces.emplace(surf);
					}
				}, node.data);
		}
	}
	
	std::unordered_map<q3shader_ptr, protomodel> buckets;
	
	for (auto const & surf : marked_surfaces) {
		auto & model = buckets[surf->shader];
		model.vert_data.insert(model.vert_data.end(), surf->vert_data.begin(), surf->vert_data.end());
		model.uv_data.insert(model.uv_data.end(), surf->uv_data.begin(), surf->uv_data.end());
		model.lightmap_data.insert(model.lightmap_data.end(), surf->lightmap_data.begin(), surf->lightmap_data.end());
		model.lightmap_active.insert(model.lightmap_active.end(), surf->lightmap_active.begin(), surf->lightmap_active.end());
	}
	
	for (auto const & [shader, proto] : buckets) {
		if (!proto.vert_data.size()) continue;
		
		q3mesh & mesh = cache_model->meshes.emplace_back();
		
		mesh.shader = shader;
		mesh.size = proto.vert_data.size() / 3;
		glCreateVertexArrays(1, &mesh.vao);
		glCreateBuffers(4, mesh.vbo);
		
		glNamedBufferData(mesh.vbo[LAYOUT_VERTCOORD], proto.vert_data.size() * 4, proto.vert_data.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, LAYOUT_VERTCOORD, LAYOUT_VERTCOORD);
		glVertexArrayVertexBuffer(mesh.vao, LAYOUT_VERTCOORD, mesh.vbo[LAYOUT_VERTCOORD], 0, 12);
		glEnableVertexArrayAttrib(mesh.vao, LAYOUT_VERTCOORD);
		glVertexArrayAttribFormat(mesh.vao, LAYOUT_VERTCOORD, 3, GL_FLOAT, GL_FALSE, 0);
		
		glNamedBufferData(mesh.vbo[LAYOUT_TEXCOORD], proto.uv_data.size() * 4, proto.uv_data.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, LAYOUT_TEXCOORD, LAYOUT_TEXCOORD);
		glVertexArrayVertexBuffer(mesh.vao, LAYOUT_TEXCOORD, mesh.vbo[LAYOUT_TEXCOORD], 0, 8);
		glEnableVertexArrayAttrib(mesh.vao, LAYOUT_TEXCOORD);
		glVertexArrayAttribFormat(mesh.vao, LAYOUT_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0);
		
		glNamedBufferData(mesh.vbo[LAYOUT_LIGHTMAPCOORD], proto.lightmap_data.size() * 4, proto.lightmap_data.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, LAYOUT_LIGHTMAPCOORD, LAYOUT_LIGHTMAPCOORD);
		glVertexArrayVertexBuffer(mesh.vao, LAYOUT_LIGHTMAPCOORD, mesh.vbo[LAYOUT_LIGHTMAPCOORD], 0, 32);
		glEnableVertexArrayAttrib(mesh.vao, LAYOUT_LIGHTMAPCOORD);
		glVertexArrayAttribFormat(mesh.vao, LAYOUT_LIGHTMAPCOORD, 8, GL_FLOAT, GL_FALSE, 0);
		
		glNamedBufferData(mesh.vbo[LAYOUT_LIGHTMAPACTIVE], proto.lightmap_active.size() * 4, proto.lightmap_active.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, LAYOUT_LIGHTMAPACTIVE, LAYOUT_LIGHTMAPACTIVE);
		glVertexArrayVertexBuffer(mesh.vao, LAYOUT_LIGHTMAPACTIVE, mesh.vbo[LAYOUT_LIGHTMAPACTIVE], 0, 4);
		glEnableVertexArrayAttrib(mesh.vao, LAYOUT_LIGHTMAPACTIVE);
		glVertexArrayAttribFormat(mesh.vao, LAYOUT_LIGHTMAPACTIVE, 1, GL_FLOAT, GL_FALSE, 0);
		
	}
	
	return cache_model;
}

rend::world_t::mapnode_t const * rend::world_t::point_in_leaf(rv3_t const & point) {
	
	mapnode_t const * node = &nodes[0];
	while(true) {
		if (!std::holds_alternative<mapnode_t::node_data>(node->data)) break;
		auto const & data = std::get<mapnode_t::node_data>(node->data);
		
		if (DotProduct(point, data.plane->normal) - data.plane->dist > 0)
			node = data.children[0];
		else
			node = data.children[1];
	}
	return node;
}
//================================================================
// LOAD SHADERS
//================================================================

void rend::world_t::load_shaders() {
	
	lump_t const & l = header->lumps[LUMP_SHADERS];
	
	if (l.filelen % sizeof(dshader_t))
		Com_Error (ERR_DROP, "world_t::load_shaders: funny lump size in %s", name);
	
	shaders.resize(l.filelen / sizeof(dshader_t));
	memcpy(shaders.data(), base + l.fileofs, l.filelen);
}

//================================================================
// LOAD LIGHTMAPS
//================================================================

void rend::world_t::load_lightmaps() {
	
	lump_t const & l = header->lumps[LUMP_LIGHTMAPS];
	byte const * buffer = base + l.fileofs;
	int32_t buffer_len = l.filelen;
	
	int32_t num_lightmaps = buffer_len / lightmap_bytes;
	
	lightmap_span = qm::next_pow2(static_cast<int32_t>(std::ceil(std::sqrt(static_cast<float>(num_lightmaps)))));
	size_t atlas_dim = lightmap_span * lightmap_dim;
	
	lightmap_atlas = make_q3texture(atlas_dim, atlas_dim, true);
	glClearTexImage(lightmap_atlas->get_id(), 0, GL_RGBA, GL_UNSIGNED_BYTE, "\0\0\0\0");
	lightmap_atlas->has_transparency = false;
	
	for (int32_t atlas_y = 0, lightmap_i = 0; lightmap_i < num_lightmaps; atlas_y++)
		for (int32_t atlas_x = 0; atlas_x < lightmap_span && lightmap_i < num_lightmaps; atlas_x++, lightmap_i++, buffer += lightmap_bytes) {
			
			std::array<std::array<uint8_t, 4>, lightmap_pixels> image;
			static_assert(sizeof(image) == lightmap_pixels * 4);
			
			for (size_t j = 0; j < lightmap_pixels; j++) {
				image[j][0] = buffer[j * 3 + 0];
				image[j][1] = buffer[j * 3 + 1];
				image[j][2] = buffer[j * 3 + 2];
				image[j][3] = 0xFF;
			}
			
			lightmap_atlas->upload(lightmap_dim, lightmap_dim, &image[0][0], GL_RGBA, GL_UNSIGNED_BYTE, atlas_x * lightmap_dim, atlas_y * lightmap_dim);
	}
	
	lightmap_atlas->generate_mipmaps();
}

//================================================================
// LOAD PLANES
//================================================================

void rend::world_t::load_planes() {
	
	lump_t const & l = header->lumps[LUMP_PLANES];
	
	if (l.filelen % sizeof(dplane_t))
		Com_Error (ERR_DROP, "world_t::load_planes: funny lump size in %s", name);
	
	dplane_t const * dplanes = reinterpret_cast<dplane_t const *>(base + l.fileofs);
	int32_t dplanes_num = l.filelen / sizeof(dplane_t);
	
	planes.resize(dplanes_num);
	for (int32_t i = 0; i < dplanes_num; i++) {
		dplane_t const & in = dplanes[i];
		cplane_t & out = planes[i];
		
		int32_t bits = 0;
		for (int32_t j = 0; j < 3; j++) {
			out.normal[j] = in.normal[j];
			if (out.normal[j] < 0) bits |= 1 << j;
		}
		
		out.dist = in.dist;
		out.type = PlaneTypeForNormal(out.normal);
		out.signbits = bits;
	}
}

//================================================================
// LOAD FOGS
//================================================================

// TODO

//================================================================
// LOAD SURFACES
//================================================================

#include <iostream>

void rend::world_t::load_surfaces(int index) {
	
	lump_t const & lsurf = header->lumps[LUMP_SURFACES];
	lump_t const & lvert = header->lumps[LUMP_DRAWVERTS];
	lump_t const & lindx = header->lumps[LUMP_DRAWINDEXES];
	
	dsurface_t const * msurf = reinterpret_cast<dsurface_t const *>(base + lsurf.fileofs);
	mapVert_t const * mvert = reinterpret_cast<mapVert_t const *>(base + lvert.fileofs);
	int32_t const * mindx = reinterpret_cast<int32_t const *>(base + lindx.fileofs);
	
	int num_surfs = lsurf.filelen / sizeof(dsurface_t);
	surfaces.resize(num_surfs);
	
	for (int i = 0; i < num_surfs; i++) {
		
		dsurface_t const & surfi = msurf[i];
		surface_t & surfo = surfaces[i];
		
		surfo.info = surfi;
		surfo.shader = r->shader_register(shaders[surfi.shaderNum].shader, true, true);
		
		switch (surfi.surfaceType) {
			
		case MST_TRIANGLE_SOUP: {
			
			mapVert_t const * surf_verts = mvert + surfi.firstVert;
			int const * surf_indicies = mindx + surfi.firstIndex;
			
			for (int32_t i = 0; i < surfi.numIndexes; i ++) {
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[1]);
				surfo.vert_data.push_back(-surf_verts[surf_indicies[i]].xyz[2]);
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[0]);
				
				uint8_t lightmap_active = 0;
				for (int32_t j = 0; j < 4; j++) {
					rv2_t conv_uv;
					if (surfi.lightmapStyles[j] >= LS_UNUSED)
						conv_uv = {0, 0};
					else {
						lightmap_active++;
						conv_uv = uv_for_lightmap( surfi.lightmapNum[j], rv2_t {
							surf_verts[surf_indicies[i]].lightmap[j][0],
							surf_verts[surf_indicies[i]].lightmap[j][1]
						});
					}
					surfo.lightmap_data.push_back(conv_uv[0]);
					surfo.lightmap_data.push_back(conv_uv[1]);
				}
				
				surfo.lightmap_active.push_back(4);
				
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[0]);
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[1]);
			}
		} break;
		
		case MST_PLANAR: {
			
			mapVert_t const * surf_verts = mvert + surfi.firstVert;
			int const * surf_indicies = mindx + surfi.firstIndex;
			
			for (int32_t i = 0; i < surfi.numIndexes; i ++) {
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[1]);
				surfo.vert_data.push_back(-surf_verts[surf_indicies[i]].xyz[2]);
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[0]);
				
				uint8_t lightmap_active = 0;
				for (int32_t j = 0; j < 4; j++) {
					rv2_t conv_uv;
					if (surfi.lightmapStyles[j] >= LS_UNUSED)
						conv_uv = {0, 0};
					else {
						lightmap_active++;
						conv_uv = uv_for_lightmap( surfi.lightmapNum[j], rv2_t {
							surf_verts[surf_indicies[i]].lightmap[j][0],
							surf_verts[surf_indicies[i]].lightmap[j][1]
						});
					}
					surfo.lightmap_data.push_back(conv_uv[0]);
					surfo.lightmap_data.push_back(conv_uv[1]);
				}
				
				surfo.lightmap_active.push_back(lightmap_active);
				
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[0]);
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[1]);
			}
			
		} break;
		// TODO -- MST_PATCH
		// TODO -- MST_FLARE
		default: break;
		}
	}
}

/*
	
	for (auto [handle, proto] : buckets) {
		
		q3mesh & mesh = (proto.shader->opaque ? r->opaque_world : r->trans_world).meshes.emplace_back();
		
		mesh.shader = std::move(proto.shader);
		mesh.size = proto.vert_data.size() / 3;
		glCreateVertexArrays(1, &mesh.vao);
		glCreateBuffers(2, mesh.vbo);
		glNamedBufferData(mesh.vbo[0], proto.vert_data.size() * 4, proto.vert_data.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, 0, 0);
		glVertexArrayVertexBuffer(mesh.vao, 0, mesh.vbo[0], 0, 12);
		glEnableVertexArrayAttrib(mesh.vao, 0);
		glVertexArrayAttribFormat(mesh.vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
		glNamedBufferData(mesh.vbo[1], proto.uv_data.size() * 4, proto.uv_data.data(), GL_STATIC_DRAW);
		glVertexArrayAttribBinding(mesh.vao, 1, 1);
		glVertexArrayVertexBuffer(mesh.vao, 1, mesh.vbo[1], 0, 8);
		glEnableVertexArrayAttrib(mesh.vao, 1);
		glVertexArrayAttribFormat(mesh.vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
	}
}
*/

//================================================================
// LOAD NODES & LEAFS
//================================================================

void rend::world_t::load_nodesleafs() {
	
	lump_t const & lmark = header->lumps[LUMP_LEAFSURFACES];
	lump_t const & lnode = header->lumps[LUMP_NODES];
	lump_t const & lleaf = header->lumps[LUMP_LEAFS];

	int32_t const * marks = reinterpret_cast<int32_t const *>(base + lmark.fileofs);
	dnode_t const * nodes = reinterpret_cast<dnode_t const *>(base + lnode.fileofs);
	dleaf_t const * leafs = reinterpret_cast<dleaf_t const *>(base + lleaf.fileofs);
	
	int32_t num_nodes = lnode.filelen / sizeof(dnode_t);
	int32_t num_leafs = lleaf.filelen / sizeof(dleaf_t);
	
	if (lmark.filelen % sizeof(*marks))
		Com_Error (ERR_DROP, "world_t::load_nodesleafs: funny leafsurfaces size in %s", name);
	if (lnode.filelen % sizeof(dnode_t))
		Com_Error (ERR_DROP, "world_t::load_nodesleafs: funny node lump size in %s", name);
	if (lleaf.filelen % sizeof(dleaf_t))
		Com_Error (ERR_DROP, "world_t::load_nodesleafs: funny leaf lump size in %s", name);
	
	this->nodes.resize(num_nodes + num_leafs);
	
	int32_t mapnode_i = 0;
	for (int32_t i = 0; i < num_nodes; i++, mapnode_i++) {
		
		mapnode_t & mn = this->nodes[mapnode_i];
		dnode_t const & node = nodes[i];
		
		for (int j = 0; j < 3; j++) {
			mn.mins[j] = node.mins[j];
			mn.maxs[j] = node.maxs[j];
		}
		
		auto & data = mn.data.emplace<mapnode_t::node_data>();
		
		data.plane = &planes[node.planeNum];
		
		for (int j = 0; j < 2; j++) {
			int offs = node.children[j];
			if (offs >= 0) {
				data.children[j] = &this->nodes[offs];
			} else {
				offs = -offs - 1;
				data.children[j] = &this->nodes[offs + num_nodes];
			}
		}
	}
	
	for (int32_t i = 0; i < num_leafs; i++, mapnode_i++) {
		
		mapnode_t & mn = this->nodes[mapnode_i];
		dleaf_t const & leaf = leafs[i];
		
		for (int j = 0; j < 3; j++) {
			mn.mins[j] = leaf.mins[j];
			mn.maxs[j] = leaf.maxs[j];
		}
		
		auto & data = mn.data.emplace<mapnode_t::leaf_data>();
		
		data.cluster = leaf.cluster;
		data.area = leaf.area;
		
		if (data.cluster > num_clusters) num_clusters = data.cluster;
		
		data.surfaces.resize(leaf.numLeafSurfaces);
		for (int j = 0; j < leaf.numLeafSurfaces; j++) {
			data.surfaces[j] = &surfaces[marks[leaf.firstLeafSurface + j]];
		}
	}
	
	struct reparent_iterator {
		reparent_iterator(mapnode_t * mn, mapnode_t * parent) : mn(mn), parent(parent) {}
		mapnode_t * mn = nullptr;
		mapnode_t * parent = nullptr;
	};
	
	std::stack<reparent_iterator> reparent_stack;
	reparent_stack.emplace(&this->nodes[0], nullptr);
	
	while (reparent_stack.size()) {
		reparent_iterator cur = reparent_stack.top();
		reparent_stack.pop();
		
		cur.mn->parent = cur.parent;
		
		if (std::holds_alternative<mapnode_t::leaf_data>(cur.mn->data)) continue;
		mapnode_t::node_data & data = std::get<mapnode_t::node_data>(cur.mn->data);
		
		reparent_stack.emplace(data.children[0], cur.mn);
		reparent_stack.emplace(data.children[1], cur.mn);
	}
}

//================================================================
// LOAD SUBMODELS
//================================================================

// TODO

//================================================================
// LOAD VISIBILITY
//================================================================

void rend::world_t::load_visibility() {
	
	lump_t const & l = header->lumps[LUMP_VISIBILITY];
	if (!l.filelen) return;
	
	vis = std::make_unique<vis_t>();
	
	byte const * buffer = base + l.fileofs;
	int32_t const * buffer_ints = reinterpret_cast<int32_t const *>(buffer);
	
	vis->num_clusters = buffer_ints[0];
	vis->cluster_bytes = buffer_ints[1];
	
	vis->data.resize(l.filelen - 8);
	memcpy(vis->data.data(), buffer + 8, vis->data.size());
}

//================================================================
// LOAD ENTITIES (WORLDSPAWN)
//================================================================

void rend::world_t::load_entities() {
	
	lump_t const & l = header->lumps[LUMP_ENTITIES];
	
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	float ambient = 1;

	lightGridSize[0] = 64;
	lightGridSize[1] = 64;
	lightGridSize[2] = 128;

	p = (char *)(base + l.fileofs);

	// store for reference by the cgame
	entityString = p;
	entityParsePoint = entityString.data();

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
			sscanf(value, "%f", &r->cull );
			continue;
		}
		// check for a different grid size
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &lightGridSize[0], &lightGridSize[1], &lightGridSize[2] );
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

//================================================================
// LOAD LIGHTGRID
//================================================================

// TODO

//================================================================
// LOAD LIGHTGRIDARRAY
//================================================================

// TODO
