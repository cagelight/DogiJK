#include "tr_local.hh"

#include <stack>

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
	// TODO -- lightmaps
	// TODO -- planes
	// TODO -- fogs
	load_surfaces(0);
	load_nodesleafs();
	// TODO -- submodels
	// TODO -- visibility
	load_entities();
	// TODO -- lightgrid
	// TODO -- lightgridarray
	
	generate_render_clusters();
}

void rend::world_t::generate_render_clusters() {
	
	std::stack<mapnode_t *> node_stack;
	node_stack.emplace(&nodes[0]);
	
	int32_t max_cluster = 0;
	
	struct protomodel_t {
		std::vector<float> vert_data;
		std::vector<float> uv_data;
	};
	
	using cluster_bucket_t = std::unordered_map<int32_t, protomodel_t>;
	std::unordered_map<int32_t, cluster_bucket_t> cluster_buckets;
	
	while(node_stack.size()) {
		
		mapnode_t * node = node_stack.top(); node_stack.pop();
		
		std::visit(lambda_visit{
			[&](mapnode_t::node_data const & data){
				node_stack.emplace(data.children[0]);
				node_stack.emplace(data.children[1]);
			},[&](mapnode_t::leaf_data const & data){
				if (data.cluster > max_cluster) max_cluster = data.cluster;
				auto & bucket = cluster_buckets[data.cluster];
				for (surface_t const * surf : data.surfaces) {
					auto & model = bucket[surf->shader->index];
					model.vert_data.insert(model.vert_data.end(), surf->vert_data.begin(), surf->vert_data.end());
					model.uv_data.insert(model.uv_data.end(), surf->uv_data.begin(), surf->uv_data.end());
				}
			},
		}, node->data);
	}
	
	render_clusters.resize(max_cluster);
	for (int32_t i = 0; i < max_cluster; i++) {
		
		auto const & cluster = cluster_buckets[i];
		auto & model = render_clusters[i];
		
		model.opque = std::make_unique<q3model>();
		
		for (auto const & [shader_id, proto] : cluster) {
			q3shader_ptr shader = r->shader_get(shader_id);
			if (!shader->opaque) continue; // TODO -- trans
			
			q3mesh & mesh = model.opque->meshes.emplace_back();
			
			mesh.shader = std::move(shader);
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

// TODO

//================================================================
// LOAD PLANES
//================================================================

// TODO

//================================================================
// LOAD FOGS
//================================================================

// TODO

//================================================================
// LOAD SURFACES
//================================================================

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
		
		surfo.shader = r->shader_register(shaders[surfi.shaderNum].shader);
		
		switch (surfi.surfaceType) {
		case MST_PLANAR: {
			
			mapVert_t const * surf_verts = mvert + surfi.firstVert;
			int const * surf_indicies = mindx + surfi.firstIndex;
			
			for (int32_t i = 0; i < surfi.numIndexes; i ++) {
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[1]);
				surfo.vert_data.push_back(-surf_verts[surf_indicies[i]].xyz[2]);
				surfo.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[0]);
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[0]);
				surfo.uv_data.push_back(surf_verts[surf_indicies[i]].st[1]);
			}
			
		} break;
		// TODO -- MST_PATCH
		// TODO -- MST_TRIANGLE_SOUP
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
		
		// TODO -- planes
		
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

// TODO

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
		/*
 		if (!Q_stricmp(keyname, "distanceCull")) {
			sscanf(value, "%f", &tr.distanceCull );
			continue;
		}
		*/
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
