#include "tr_local.hh"

struct world_data_t {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2
	vec3_t		lightGridSize;
	char		*entityString;
	char		*entityParsePoint;
	
	std::vector<dshader_t> shaders;
};

#define WORL reinterpret_cast<world_data_t *>(this->world_data)

void rend::initialize_world() {
	this->world_data = new world_data_t;
	memset(this->world_data, 0, sizeof(world_data_t));
}

void rend::destruct_world() noexcept {
	if (WORL) delete WORL;
}

static void R_LoadEntities( lump_t *l, world_data_t * worldData, byte const * base );
static void R_LoadShaders( lump_t * l, world_data_t * worl, byte * base );
static void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump, world_data_t * worl, int index, byte * base );

void rend::world_load(char const * name) {
	
	byte * buffer = nullptr;
	
	if (ri.CM_GetCachedMapDiskImage()) {
		buffer = (byte *)ri.CM_GetCachedMapDiskImage();
	} else {
		ri.FS_ReadFile( name, (void **)&buffer );
		if ( !buffer ) {
			Com_Error (ERR_DROP, "RE_LoadWorldMap: %s not found", name);
		}
	}
	
	memset( WORL, 0, sizeof( *WORL ) );
	Q_strncpyz( WORL->name, name, sizeof( WORL->name ) );
	Q_strncpyz( WORL->baseName, COM_SkipPath( WORL->name ), sizeof( WORL->name ) );

	COM_StripExtension( WORL->baseName, WORL->baseName, sizeof( WORL->baseName ) );

	dheader_t * header = (dheader_t *)buffer;
	byte * base = (byte *)header;
	
	R_LoadEntities(&header->lumps[LUMP_ENTITIES], WORL, base);
	R_LoadShaders( &header->lumps[LUMP_SHADERS], WORL, base );
	R_LoadSurfaces(&header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES], WORL, 0, base);
}

// copied directly from rd-vanilla R_GetEntityToken, not sure what it does
qboolean rend::get_entity_token(char * buffer, int size) {
	const char	*s;

	if (size <= 0)
	{ //force reset
		WORL->entityParsePoint = WORL->entityString;
		return qtrue;
	}

	s = COM_Parse( (const char **) &WORL->entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !WORL->entityParsePoint || !s[0] ) {
		return qfalse;
	} else {
		return qtrue;
	}
}

static void R_LoadEntities( lump_t *l, world_data_t * worl, byte const * base ) {
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];
	float ambient = 1;

	worl->lightGridSize[0] = 64;
	worl->lightGridSize[1] = 64;
	worl->lightGridSize[2] = 128;

	p = (char *)(base + l->fileofs);

	// store for reference by the cgame
	worl->entityString = (char *)Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( worl->entityString, p );
	worl->entityParsePoint = worl->entityString;

	COM_BeginParseSession ("R_LoadEntities");

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
			sscanf(value, "%f %f %f", &worl->lightGridSize[0], &worl->lightGridSize[1], &worl->lightGridSize[2] );
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

static void R_LoadShaders( lump_t * l, world_data_t * worl, byte * base ) {
	int		i, count;
	dshader_t	*in;

	in = (dshader_t *)(base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "LoadMap: funny lump size in %s",worl->name);
	count = l->filelen / sizeof(*in);
	
	for (size_t i = 0 ; i < count; i++) {
		worl->shaders.emplace_back(in[i]);
	}
}

struct protomodel {
	std::vector<float> vert_data;
	std::vector<float> uv_data;
	q3shader_ptr shader;
};

static void R_LoadSurfaces( lump_t *lsurf, lump_t *lvert, lump_t *lind, world_data_t * worl, int index, byte * base ) {
	dsurface_t * msurf;
	mapVert_t * mvert;
	int * mind;
	
	msurf = reinterpret_cast<dsurface_t *>(base + lsurf->fileofs);
	mvert = reinterpret_cast<mapVert_t *>(base + lvert->fileofs);
	mind = reinterpret_cast<int *>(base + lind->fileofs);
	
	std::unordered_map<qhandle_t, protomodel> buckets;
	
	size_t count = lsurf->filelen / sizeof(dsurface_t);
	for (size_t i = 0; i < count; i++) {
		
		dsurface_t & surf = msurf[i];
		
		q3shader_ptr shader = r->shader_register(worl->shaders[surf.shaderNum].shader);
		protomodel & mod = buckets[shader->index];
		mod.shader = shader;
		
		switch (surf.surfaceType) {
		case MST_PLANAR: {
			
			mapVert_t * surf_verts = mvert + surf.firstVert;
			int * surf_indicies = mind + surf.firstIndex;
			
			for (size_t i = 0; i < surf.numIndexes; i ++) {
				mod.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[1]);
				mod.vert_data.push_back(-surf_verts[surf_indicies[i]].xyz[2]);
				mod.vert_data.push_back(surf_verts[surf_indicies[i]].xyz[0]);
				mod.uv_data.push_back(surf_verts[surf_indicies[i]].st[0]);
				mod.uv_data.push_back(surf_verts[surf_indicies[i]].st[1]);
			}
			
		} break;
		default: break;
		}
	}
	
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
