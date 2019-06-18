#include "hw_local.hh"
using namespace howler;

static constexpr byte const FakeGLAFile[] = {
	0x32, 0x4C, 0x47, 0x41, 0x06, 0x00, 0x00, 0x00, 0x2A, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x00, 0x00, 0x00,
	0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
	0x26, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4D, 0x6F, 0x64, 0x56, 0x69, 0x65, 0x77, 0x20,
	0x69, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C, 0x20, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
	0x00, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
	0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
	0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xBF, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F,
	0x00, 0x80, 0x00, 0x80, 0x00, 0x80
};

instance::model_registry::model_registry() {
	model_t & mod =  models.emplace_back(make_q3basemodel())->base;
	strcpy(mod.name, "BAD MODEL");
	mod.type = MOD_BAD;
	mod.index = 0;
}

q3basemodel_ptr instance::model_registry::reg(char const * name, bool server) {
	auto m = lookup.find(name);
	if (m != lookup.end()) return m->second;
	
	q3basemodel_ptr mod = models.emplace_back(make_q3basemodel());
	mod->base.index = models.size() - 1;
	strcpy(mod->base.name, name);
	lookup[name] = mod;
	
	if (!Q_stricmp(name, "*default.gla")) {
		mod->buffer.resize(mod->base.dataSize = sizeof(FakeGLAFile));
		memcpy(mod->buffer.data(), FakeGLAFile, sizeof(FakeGLAFile));
		mod->load();
		assert(mod->base.type == MOD_MDXA);
		return mod;
	}
	
	FS_Reader fs {name};
	if (!fs.valid()) {
		return mod;
	}
	
	mod->buffer.resize(fs.size());
	memcpy(mod->buffer.data(), fs.data(), fs.size());
	mod->load();
	
	if (mod->base.type == MOD_BAD) return mod;
	if (mod->base.type == MOD_MDXA) return mod;
	
	if (!server) {
		if (!hw_inst->renderer_initialized)
			waiting_models.push_back(mod);
		else
			mod->setup_render();
	}
	
	return mod;
}

void instance::model_registry::reg(q3basemodel_ptr ptr) {
	ptr->base.index = models.size();
	models.push_back(ptr);
	lookup[ptr->base.name] = ptr;
}

q3basemodel_ptr instance::model_registry::get(qhandle_t h) {
	if (h < 0 || h >= static_cast<int32_t>(models.size())) return nullptr;
	return models[h];
}

void instance::model_registry::process_waiting() {
	for (q3basemodel_ptr & model : waiting_models)
		model->setup_render();
	waiting_models.clear();
}

void q3basemodel::load() {
	uint32_t ident = reinterpret_cast<uint32_t *>(buffer.data())[0];
	switch(ident) {
		case MDXA_IDENT: load_mdxa(); break;
		case MDXM_IDENT: load_mdxm(); break;
		case MD3_IDENT: load_md3(0); break;
		default:
			base.type = MOD_BAD;
	}
}

void q3basemodel::setup_render() {
	switch (base.type) {
		default: break;
		case MOD_MESH:
			setup_render_md3();
	}
}

//================================================================
// MD3
//================================================================

struct q3md3mesh : public q3mesh {
	struct vertex_t {
		qm::vec3_t vert;
		qm::vec2_t uv;
	};
	
	q3md3mesh(vertex_t const * data, size_t num) : q3mesh(mode::triangles) {
		
		static constexpr uint_fast16_t offsetof_verts = 0;
		static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
		static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
		static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
		static constexpr uint_fast16_t sizeof_all = offsetof_uv + sizeof_uv;
		static_assert(sizeof_all == sizeof(vertex_t));
		
		m_size = num;
		glCreateBuffers(1, &m_vbo);
		glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
		
		glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
		
		glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
		glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
		
		glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, sizeof_verts / 4, GL_FLOAT, GL_FALSE, offsetof_verts);
		glVertexArrayAttribFormat(m_handle, LAYOUT_UV, sizeof_uv / 4, GL_FLOAT, GL_FALSE, offsetof_uv);
		
	}
	
	~q3md3mesh() {
		glDeleteBuffers(1, &m_vbo);
	}
private:
	GLuint m_vbo;
};

void q3basemodel::setup_render_md3() {
	model = make_q3model();
	
	md3Header_t * header = base.md3[0];
	
	md3Surface_t * surf = (md3Surface_t *)( (byte *)header + header->ofsSurfaces );
	for (int32_t s = 0; s < header->numSurfaces; s++) {
		
		
		md3XyzNormal_t * verts = (md3XyzNormal_t *) ((byte *)surf + surf->ofsXyzNormals);
		md3St_t * uvs = (md3St_t *) ((byte *)surf + surf->ofsSt);
		md3Triangle_t * triangles = (md3Triangle_t *) ((byte *)surf + surf->ofsTriangles);
		
		std::vector<q3md3mesh::vertex_t> vert_data;
		
		for (int32_t i = 0; i < surf->numTriangles; i++) {
			for (size_t j = 0; j < 3; j++) {
				auto const & v = verts[triangles[i].indexes[j]];
				auto const & u = uvs[triangles[i].indexes[j]];
				vert_data.emplace_back( q3md3mesh::vertex_t {
					qm::vec3_t { v.xyz[1], v.xyz[2], v.xyz[0] } / 64.0,
					qm::vec2_t { u.st[0], u.st[1] }
				});
			}
		}
		
		std::shared_ptr<q3md3mesh> mesh = std::make_shared<q3md3mesh>(vert_data.data(), vert_data.size());
		
		assert(surf->numShaders == 1); // how can there ever be more than 1? that makes no sense...
		md3Shader_t * shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
		model->meshes.emplace_back( hw_inst->shaders.get(shader->shaderIndex), mesh );
		
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
}

struct q3MDXMmesh : public q3mesh {
	struct vertex_t {
		qm::vec3_t vert;
		qm::vec2_t uv;
	};
	
	q3MDXMmesh(vertex_t const * data, size_t num) : q3mesh(mode::triangles) {
		
		static constexpr uint_fast16_t offsetof_verts = 0;
		static constexpr uint_fast16_t sizeof_verts = sizeof(vertex_t::vert);
		static constexpr uint_fast16_t offsetof_uv = offsetof_verts + sizeof_verts;
		static constexpr uint_fast16_t sizeof_uv = sizeof(vertex_t::uv);
		static constexpr uint_fast16_t sizeof_all = offsetof_uv + sizeof_uv;
		static_assert(sizeof_all == sizeof(vertex_t));
		
		m_size = num;
		glCreateBuffers(1, &m_vbo);
		glNamedBufferData(m_vbo, num * sizeof_all, data, GL_STATIC_DRAW);
		
		glVertexArrayVertexBuffer(m_handle, 0, m_vbo, 0, sizeof_all);
		
		glEnableVertexArrayAttrib(m_handle, LAYOUT_VERTEX);
		glEnableVertexArrayAttrib(m_handle, LAYOUT_UV);
		glVertexArrayAttribBinding(m_handle, LAYOUT_VERTEX, 0);
		glVertexArrayAttribBinding(m_handle, LAYOUT_UV, 0);
		
		glVertexArrayAttribFormat(m_handle, LAYOUT_VERTEX, sizeof_verts / 4, GL_FLOAT, GL_FALSE, offsetof_verts);
		glVertexArrayAttribFormat(m_handle, LAYOUT_UV, sizeof_uv / 4, GL_FLOAT, GL_FALSE, offsetof_uv);
		
	}
	
	~q3MDXMmesh() {
		glDeleteBuffers(1, &m_vbo);
	}
private:
	GLuint m_vbo;
};

void q3basemodel::setup_render_mdxm() {
	model = make_q3model();
	
	mdxmHeader_t * header = base.mdxm;
	for (size_t i = 0; i < header->numSurfaces; i++) {
		mdxmSurface_t * surf = (mdxmSurface_t *)ri.G2_FindSurface(&base, i, 0);
		mdxmHierarchyOffsets_t * surfI = (mdxmHierarchyOffsets_t *)((byte *)base.mdxm + sizeof(mdxmHeader_t));
		mdxmSurfHierarchy_t * surfH = (mdxmSurfHierarchy_t *)((byte *)surfI + surfI->offsets[surf->thisSurfaceIndex]);
		if (surfH->name[0] == '*') continue;
		
		std::vector<q3MDXMmesh::vertex_t> verticies;
	}
}

//================================================================

void q3basemodel::load_mdxa() {
	mdxaHeader_t * header = (mdxaHeader_t *)buffer.data();
	if (header->version != MDXA_VERSION)
		Com_Error(ERR_FATAL, "q3basemodel::load_mdxa: \"%s\" is wrong MDXA version", base.name);
	
	base.type = MOD_MDXA;
	base.mdxa = header;
	
	if (base.mdxa->numFrames < 1)
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "q3basemodel::load_mdxa: WARNING: \"%s\" has no frames\n", base.name );
}

static constexpr int OldToNewRemapTable[72] = {
0,// Bone 0:   "model_root":           Parent: ""  (index -1)
1,// Bone 1:   "pelvis":               Parent: "model_root"  (index 0)
2,// Bone 2:   "Motion":               Parent: "pelvis"  (index 1)
3,// Bone 3:   "lfemurYZ":             Parent: "pelvis"  (index 1)
4,// Bone 4:   "lfemurX":              Parent: "pelvis"  (index 1)
5,// Bone 5:   "ltibia":               Parent: "pelvis"  (index 1)
6,// Bone 6:   "ltalus":               Parent: "pelvis"  (index 1)
6,// Bone 7:   "ltarsal":              Parent: "pelvis"  (index 1)
7,// Bone 8:   "rfemurYZ":             Parent: "pelvis"  (index 1)
8,// Bone 9:   "rfemurX":	            Parent: "pelvis"  (index 1)
9,// Bone10:   "rtibia":	            Parent: "pelvis"  (index 1)
10,// Bone11:   "rtalus":	            Parent: "pelvis"  (index 1)
10,// Bone12:   "rtarsal":              Parent: "pelvis"  (index 1)
11,// Bone13:   "lower_lumbar":         Parent: "pelvis"  (index 1)
12,// Bone14:   "upper_lumbar":	        Parent: "lower_lumbar"  (index 13)
13,// Bone15:   "thoracic":	            Parent: "upper_lumbar"  (index 14)
14,// Bone16:   "cervical":	            Parent: "thoracic"  (index 15)
15,// Bone17:   "cranium":              Parent: "cervical"  (index 16)
16,// Bone18:   "ceyebrow":	            Parent: "face_always_"  (index 71)
17,// Bone19:   "jaw":                  Parent: "face_always_"  (index 71)
18,// Bone20:   "lblip2":	            Parent: "face_always_"  (index 71)
19,// Bone21:   "leye":		            Parent: "face_always_"  (index 71)
20,// Bone22:   "rblip2":	            Parent: "face_always_"  (index 71)
21,// Bone23:   "ltlip2":               Parent: "face_always_"  (index 71)
22,// Bone24:   "rtlip2":	            Parent: "face_always_"  (index 71)
23,// Bone25:   "reye":		            Parent: "face_always_"  (index 71)
24,// Bone26:   "rclavical":            Parent: "thoracic"  (index 15)
25,// Bone27:   "rhumerus":             Parent: "thoracic"  (index 15)
26,// Bone28:   "rhumerusX":            Parent: "thoracic"  (index 15)
27,// Bone29:   "rradius":              Parent: "thoracic"  (index 15)
28,// Bone30:   "rradiusX":             Parent: "thoracic"  (index 15)
29,// Bone31:   "rhand":                Parent: "thoracic"  (index 15)
29,// Bone32:   "mc7":                  Parent: "thoracic"  (index 15)
34,// Bone33:   "r_d5_j1":              Parent: "thoracic"  (index 15)
35,// Bone34:   "r_d5_j2":              Parent: "thoracic"  (index 15)
35,// Bone35:   "r_d5_j3":              Parent: "thoracic"  (index 15)
30,// Bone36:   "r_d1_j1":              Parent: "thoracic"  (index 15)
31,// Bone37:   "r_d1_j2":              Parent: "thoracic"  (index 15)
31,// Bone38:   "r_d1_j3":              Parent: "thoracic"  (index 15)
32,// Bone39:   "r_d2_j1":              Parent: "thoracic"  (index 15)
33,// Bone40:   "r_d2_j2":              Parent: "thoracic"  (index 15)
33,// Bone41:   "r_d2_j3":              Parent: "thoracic"  (index 15)
32,// Bone42:   "r_d3_j1":			    Parent: "thoracic"  (index 15)
33,// Bone43:   "r_d3_j2":		        Parent: "thoracic"  (index 15)
33,// Bone44:   "r_d3_j3":              Parent: "thoracic"  (index 15)
34,// Bone45:   "r_d4_j1":              Parent: "thoracic"  (index 15)
35,// Bone46:   "r_d4_j2":	            Parent: "thoracic"  (index 15)
35,// Bone47:   "r_d4_j3":		        Parent: "thoracic"  (index 15)
36,// Bone48:   "rhang_tag_bone":	    Parent: "thoracic"  (index 15)
37,// Bone49:   "lclavical":            Parent: "thoracic"  (index 15)
38,// Bone50:   "lhumerus":	            Parent: "thoracic"  (index 15)
39,// Bone51:   "lhumerusX":	        Parent: "thoracic"  (index 15)
40,// Bone52:   "lradius":	            Parent: "thoracic"  (index 15)
41,// Bone53:   "lradiusX":	            Parent: "thoracic"  (index 15)
42,// Bone54:   "lhand":	            Parent: "thoracic"  (index 15)
42,// Bone55:   "mc5":		            Parent: "thoracic"  (index 15)
43,// Bone56:   "l_d5_j1":	            Parent: "thoracic"  (index 15)
44,// Bone57:   "l_d5_j2":	            Parent: "thoracic"  (index 15)
44,// Bone58:   "l_d5_j3":	            Parent: "thoracic"  (index 15)
43,// Bone59:   "l_d4_j1":	            Parent: "thoracic"  (index 15)
44,// Bone60:   "l_d4_j2":	            Parent: "thoracic"  (index 15)
44,// Bone61:   "l_d4_j3":	            Parent: "thoracic"  (index 15)
45,// Bone62:   "l_d3_j1":	            Parent: "thoracic"  (index 15)
46,// Bone63:   "l_d3_j2":	            Parent: "thoracic"  (index 15)
46,// Bone64:   "l_d3_j3":	            Parent: "thoracic"  (index 15)
45,// Bone65:   "l_d2_j1":	            Parent: "thoracic"  (index 15)
46,// Bone66:   "l_d2_j2":	            Parent: "thoracic"  (index 15)
46,// Bone67:   "l_d2_j3":	            Parent: "thoracic"  (index 15)
47,// Bone68:   "l_d1_j1":				Parent: "thoracic"  (index 15)
48,// Bone69:   "l_d1_j2":	            Parent: "thoracic"  (index 15)
48,// Bone70:   "l_d1_j3":				Parent: "thoracic"  (index 15)
52// Bone71:   "face_always_":			Parent: "cranium"  (index 17)
};

void q3basemodel::load_mdxm(bool server) {
	mdxmHeader_t * header = (mdxmHeader_t *)buffer.data();
	if (header->version != MDXM_VERSION)
		Com_Error(ERR_FATAL, "q3basemodel::load_mdxm: \"%s\" is wrong MDXM version", base.name);
	
	base.type = MOD_MDXM;
	base.mdxm = header;
	
	char const * anim_name = va("%s.gla",header->animName);
	header->animIndex = hw_inst->models.reg(anim_name)->base.index;
	if (!header->animIndex)
		Com_Error(ERR_FATAL, "q3basemodel::load_mdxm: could not find animation \"%s\" for model \"%s\"", anim_name, base.name);
	
	base.numLods = header->numLODs - 1;
	
	mdxmSurfHierarchy_t * surfi = (mdxmSurfHierarchy_t *)(buffer.data() + header->ofsSurfHierarchy);
	for (int32_t i = 0; i < header->numSurfaces; i++) {
		Q_strlwr(surfi->name);
		if ( !strcmp( &surfi->name[strlen(surfi->name) - 4], "_off") ) 
			surfi->name[strlen(surfi->name) - 4] = 0; //remove "_off" from name
		if (surfi->shader[0])
			surfi->shaderIndex = hw_inst->shaders.reg(surfi->shader)->index;
		surfi = (mdxmSurfHierarchy_t *)( (byte *)surfi + (size_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfi->numChildren ] ));
	}
	
	bool needs_bone_conversion = header->numBones == 72 && strstr(header->animName, "_humanoid");
	mdxmLOD_t * lod = (mdxmLOD_t *)(buffer.data() + header->ofsLODs);
	for (int32_t l = 0; l < header->numLODs; l++) {
		mdxmSurface_t * surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (header->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		
		for (int32_t i = 0; i < header->numSurfaces; i++) {
			surf->ident = SF_MDX;
			if (needs_bone_conversion) {
				int *boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
				for (int32_t j = 0 ; j < surf->numBoneReferences ; j++) {
					assert(boneRef[j] >= 0 && boneRef[j] < 72);
					if (boneRef[j] >= 0 && boneRef[j] < 72)
						boneRef[j]=OldToNewRemapTable[boneRef[j]];
					else
						boneRef[j]=0;
				}
			}
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}
}

void q3basemodel::load_md3(int32_t lod) {
	md3Header_t * header = (md3Header_t *)buffer.data();
	if (header->version != MD3_VERSION)
		Com_Error(ERR_FATAL, "q3basemodel::load_md3: \"%s\" is wrong MD3 version", base.name);
	
	base.type = MOD_MESH;
	base.md3[lod] = header;
	
	md3Surface_t * surf = (md3Surface_t *) ( buffer.data() + header->ofsSurfaces );
	for (int32_t i = 0 ; i < header->numSurfaces ; i++) {
		
		surf->ident = SF_MD3;
		
		Q_strlwr(surf->name);
		int32_t j = strlen(surf->name);
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}
		
		md3Shader_t * shader = (md3Shader_t *) ((byte *)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++) {
			shader->shaderIndex = hw_inst->shaders.reg(shader->name, true)->index;
		}
		
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
}
