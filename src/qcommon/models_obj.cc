#include "models.hh"

#include <vector>

static constexpr float size_mult = 10.0f;

static std::vector<objModel_t *> loaded_models;
static std::vector<std::shared_ptr<GenericModel>> loaded_models2;
/*
int Model_ObjGetVerticies(char const * name, vec3_t * points, int points_num) {
	objModel_t * surf = Model_LoadObj(name);
	if (!surf) return -1;
	int p = 0;
	for (int v = 0; v < surf->numVerts && p < points_num; v+=3, p++) {
		points[p][0] = surf->verts[v+0];
		points[p][1] = surf->verts[v+1];
		points[p][2] = surf->verts[v+2];
	}
	return p;
}
*/

/* FIXME -- should be used when loaded_models is cleared!
static void Model_FreeObj(objModel_t * mod) {
	
	if (mod->numVerts) delete [] mod->verts;
	if (mod->numUVs) delete [] mod->UVs;
	if (mod->numNormals) delete [] mod->normals;
	
	if (mod->numSurfaces) {
		for (int i = 0; i < mod->numSurfaces; i++) {
			if (mod->surfaces[i].numFaces) delete [] mod->surfaces[i].faces;
		}
		delete [] mod->surfaces;
	}
}
*/

float nullVerts[] = {
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f
};
float nullUVs[] = {
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
};
float nullNormals[] = {
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f
};

#define OBJ_MAX_INDICIES 1024
#define CMD_BUF_LEN 12
#define FLOAT_BUF_LEN 15

char const defaultShader[] = "textures/colors/c_grey";

objModel_t * Model_LoadObj(char const * name) {
	
	for (objModel_t * mod : loaded_models) {
		if (!strcmp(mod->name, name)) return mod;
	}
	
	fileHandle_t file;
	long len = FS_FOpenFileRead(name, &file, qfalse);
	
	if (len < 0) return nullptr;
	if (len == 0) {
		FS_FCloseFile(file);
		return nullptr;
	}
	
	char * obj_buf = new char[len];
	FS_Read(obj_buf, len, file);
	FS_FCloseFile(file);
	
	//objModel_t * mod = new objModel_t {};
	
	struct objWorkElement {
		int vert = 0;
		int uv = 0;
		int normal = 0;
	};
	
	struct objWorkFace {
		objWorkElement elements [3];
	};
	
	struct objWorkSurface {
		char shader[MAX_QPATH];
		int shaderIndex = 0;
		std::vector<objWorkFace> faces;
	};
	
	std::vector<float> verts;
	std::vector<float> uvs;
	std::vector<float> normals;
	std::vector<objWorkSurface> surfs;

	bool all_good = true;
	bool seekline = false;
	long obj_buf_i = 0;
	double scale = 1.0;
	
	objModel_t * mod = new objModel_t {};
	
	while (obj_buf_i < len && all_good) {
		
		switch (obj_buf[obj_buf_i]) {
		case '\r':
			obj_buf_i++;
			continue;
		case '\n':
			obj_buf_i++;
			seekline = false;
			continue;
		case '\0':
			all_good = false;
			continue;
		case '#':
			obj_buf_i++;
			seekline = true;
			continue;
		default:
			if (seekline) {
				obj_buf_i++;
				continue;
			}
			break;
		}
		
		char cmd_buf[CMD_BUF_LEN];
		int ci;
		for (ci = 0; ci < CMD_BUF_LEN; ci++) {
			switch(obj_buf[obj_buf_i+ci]) {
			case ' ':
				cmd_buf[ci] = '\0';
				break;
			default:
				cmd_buf[ci] = obj_buf[obj_buf_i+ci];
				continue;
			}
			break;
		}
		
		if (ci == CMD_BUF_LEN) {
			all_good = false;
			break;
		} else {
			obj_buf_i += ci + 1;
		}
		
		if (!strcmp(cmd_buf, "o")) {
			surfs.emplace_back();
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "v")) {
			char float_buf[FLOAT_BUF_LEN];
			for (int v = 0; v < 3; v++) {
				int vi;
				for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
					switch(obj_buf[obj_buf_i + vi]) {
					case '\r':
					case '\n':
					case ' ':
						float_buf[vi] = '\0';
						break;
					default:
						float_buf[vi] = obj_buf[obj_buf_i+vi];
						continue;
					}
					break;
				}
				obj_buf_i += vi + 1;
				float val = strtod(float_buf, nullptr) * size_mult * scale;
				verts.push_back(val);
				memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
			}
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "vn")) {
				char float_buf[FLOAT_BUF_LEN];
				for (int v = 0; v < 3; v++) {
					int vi;
					for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
						switch(obj_buf[obj_buf_i + vi]) {
						case '\r':
						case '\n':
						case ' ':
							float_buf[vi] = '\0';
							break;
						default:
							float_buf[vi] = obj_buf[obj_buf_i+vi];
							continue;
						}
						break;
					}
					obj_buf_i += vi + 1;
					float val = strtod(float_buf, nullptr);
					normals.push_back(val);
					memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
				}
				obj_buf_i -= 2;
				seekline = true;
				continue;
		} else if (!strcmp(cmd_buf, "vt")) {
			char float_buf[FLOAT_BUF_LEN];
			for (int v = 0; v < 2; v++) {
				int vi;
				for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
					switch(obj_buf[obj_buf_i + vi]) {
					case '\r':
					case '\n':
					case ' ':
						float_buf[vi] = '\0';
						break;
					default:
						float_buf[vi] = obj_buf[obj_buf_i+vi];
						continue;
					}
					break;
				}
				obj_buf_i += vi + 1;
				float val = strtod(float_buf, nullptr);
				if (v) val = 1 - val;
				uvs.push_back(val);
				memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
			}
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "f")) {
			objWorkSurface & cursurf = surfs.back();
			objWorkFace face;
			for (int fi = 0; fi < 3; fi++) {
				objWorkElement element;
				char int_buf[FLOAT_BUF_LEN];
				for (int v = 0; v < 3; v++) {
					int vi;
					for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
						switch(obj_buf[obj_buf_i + vi]) {
						case '\r':
						case '\n':
						case ' ':
							int_buf[vi] = '\0';
							v += 3;
							break;
						case '\\':
						case '/':
							int_buf[vi] = '\0';
							break;
						default:
							int_buf[vi] = obj_buf[obj_buf_i+vi];
							continue;
						}
						break;
					}
					obj_buf_i += vi + 1;
					int val = strtol(int_buf, nullptr, 10) - 1;
					switch(v) {
					case 0:
					case 3:
						element.vert = val;
					case 1:
					case 4:
						element.uv = val;
					case 2:
					case 5:
						element.normal = val;
					}
					memset(int_buf, FLOAT_BUF_LEN, sizeof(char));
				}
				face.elements[fi] = element;
			}
			cursurf.faces.push_back(face);
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "usemtl")) {
			objWorkSurface & cursurf = surfs.back();
			char nam_buf[MAX_QPATH];
			memset(nam_buf, '\0', MAX_QPATH);
			int ci = 0;
			while (true) {
				if (ci >= MAX_QPATH) Com_Error(ERR_DROP, "Obj model shader field exceeds MAX_QPATH(%i)", int(MAX_QPATH));
				switch(obj_buf[obj_buf_i]) {
				case '\r':
				case '\n':
				case ' ':
					nam_buf[ci++] = '\0';
					obj_buf_i++;
					break;
				case '\\':
					nam_buf[ci++] = '\0';
					obj_buf_i++;
					break;
				default:
					nam_buf[ci++] = obj_buf[obj_buf_i];
					obj_buf_i++;
					continue;
				}
				break;
			}

			strcpy(cursurf.shader, nam_buf);

			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "scale")) {
			
			std::array<char, 64> SCALE_BUF {};
			
			int ci = 0;
			while (true) {
				switch(obj_buf[obj_buf_i]) {
				case '\r':
				case '\n':
				case ' ':
					SCALE_BUF[ci++] = '\0';
					obj_buf_i++;
					break;
				default:
					SCALE_BUF[ci++] = obj_buf[obj_buf_i];
					obj_buf_i++;
					continue;
				}
				break;
			}

			scale = std::strtod(SCALE_BUF.data(), nullptr);

			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "physics")) {
			
			std::string buffer;
			buffer.reserve(64);
			
			while (true) {
				switch(obj_buf[obj_buf_i]) {
				case '\r':
				case '\n':
				case ' ':
					obj_buf_i++;
					break;
				default:
					buffer.push_back(obj_buf[obj_buf_i]);
					obj_buf_i++;
					continue;
				}
				break;
			}
			
			if (buffer == "sphere") {
				mod->physics = objModel_t::PhysicsType::SPHERE;
				buffer.clear();
				
				while (true) {
					switch(obj_buf[obj_buf_i]) {
					case '\r':
					case '\n':
					case ' ':
						obj_buf_i++;
						break;
					default:
						buffer.push_back(obj_buf[obj_buf_i]);
						obj_buf_i++;
						continue;
					}
					break;
				}
				
				mod->physics_sphere.radius = std::strtod(buffer.data(), nullptr) * size_mult * scale;
			}
			
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else {
			seekline = true;
			continue;
		}
	}
	
	if (all_good) {
	} else {
		Com_Printf("Obj Load Failed.\n");
		return nullptr;
	}
	
	VectorSet(mod->mins, verts[0], verts[1], verts[2]);
	VectorSet(mod->maxs, verts[0], verts[1], verts[2]);
	
	for (int i = 3; i < verts.size(); i += 3) {
		if (verts[i] < mod->mins[0]) mod->mins[0] = verts[i];
		else if (verts[i] > mod->maxs[0]) mod->maxs[0] = verts[i];
		if (verts[i+1] < mod->mins[1]) mod->mins[1] = verts[i+1];
		else if (verts[i+1] > mod->maxs[1]) mod->maxs[1] = verts[i+1];
		if (verts[i+2] < mod->mins[2]) mod->mins[2] = verts[i+2];
		else if (verts[i+2] > mod->maxs[2]) mod->maxs[2] = verts[i+2];
	}
	
	mod->numVerts = verts.size();
	mod->verts = new float [mod->numVerts];
	memcpy(mod->verts, verts.data(), mod->numVerts * sizeof(float));
	
	mod->numUVs = uvs.size();
	mod->UVs = new float [mod->numUVs];
	memcpy(mod->UVs, uvs.data(), mod->numUVs * sizeof(float));
	
	mod->numNormals = normals.size();
	mod->normals = new float [mod->numNormals];
	memcpy(mod->normals, normals.data(), mod->numNormals * sizeof(float));
	
	mod->numVerts /= 3;
	mod->numUVs /= 2;
	mod->numNormals /= 3;
	
	mod->numSurfaces = surfs.size();
	mod->surfaces = new objSurface_t [mod->numSurfaces];
	for (int s = 0; s < mod->numSurfaces; s++) {
		
		objSurface_t & surfTo = mod->surfaces[s];
		objWorkSurface & surfFrom = surfs[s];
		
		if (strlen(surfFrom.shader)) strcpy(surfTo.shader, surfFrom.shader);
		else strcpy(surfTo.shader, defaultShader);
		surfTo.shaderIndex = surfFrom.shaderIndex;
		
		surfTo.numFaces = surfFrom.faces.size();
		surfTo.faces = new objFace_t [surfTo.numFaces];
		for (int f = 0; f < surfTo.numFaces; f++) {
			
			objFace_t & faceT = surfTo.faces[f];
			objWorkFace & faceF = surfFrom.faces[f];
			
			faceT[0].vertex = &mod->verts[faceF.elements[2].vert * 3];
			faceT[1].vertex = &mod->verts[faceF.elements[1].vert * 3];
			faceT[2].vertex = &mod->verts[faceF.elements[0].vert * 3];
			
			faceT[0].uv = &mod->UVs[faceF.elements[2].uv * 2];
			faceT[1].uv = &mod->UVs[faceF.elements[1].uv * 2];
			faceT[2].uv = &mod->UVs[faceF.elements[0].uv * 2];
			
			faceT[0].normal = &mod->normals[faceF.elements[2].normal * 3];
			faceT[1].normal = &mod->normals[faceF.elements[1].normal * 3];
			faceT[2].normal = &mod->normals[faceF.elements[0].normal * 3];
		}
	}
	
	loaded_models.push_back(mod);
	
	return mod;
}

std::shared_ptr<GenericModel const> Model_LoadObj2(char const * name) {
	
	for (auto & mod : loaded_models2) {
		if (mod->name == name) return mod;
	}
	
	fileHandle_t file;
	long len = FS_FOpenFileRead(name, &file, qfalse);
	
	if (len < 0) return nullptr;
	if (len == 0) {
		FS_FCloseFile(file);
		return nullptr;
	}
	
	char * obj_buf = new char[len];
	FS_Read(obj_buf, len, file);
	FS_FCloseFile(file);
	
	//objModel_t * mod = new objModel_t {};
	
	struct objWorkElement {
		int vert = 0;
		int uv = 0;
		int normal = 0;
	};
	
	struct objWorkFace {
		objWorkElement elements [3];
	};
	
	struct objWorkSurface {
		char shader[MAX_QPATH];
		int shaderIndex = 0;
		std::vector<objWorkFace> faces;
	};
	
	std::vector<float> verts;
	std::vector<float> uvs;
	std::vector<float> normals;
	std::vector<objWorkSurface> surfs;

	bool all_good = true;
	bool seekline = false;
	long obj_buf_i = 0;
	
	while (obj_buf_i < len && all_good) {
		
		switch (obj_buf[obj_buf_i]) {
		case '\r':
			obj_buf_i++;
			continue;
		case '\n':
			obj_buf_i++;
			seekline = false;
			continue;
		case '\0':
			all_good = false;
			continue;
		case '#':
			obj_buf_i++;
			seekline = true;
			continue;
		default:
			if (seekline) {
				obj_buf_i++;
				continue;
			}
			break;
		}
		
		char cmd_buf[CMD_BUF_LEN];
		int ci;
		for (ci = 0; ci < CMD_BUF_LEN; ci++) {
			switch(obj_buf[obj_buf_i+ci]) {
			case ' ':
				cmd_buf[ci] = '\0';
				break;
			default:
				cmd_buf[ci] = obj_buf[obj_buf_i+ci];
				continue;
			}
			break;
		}
		
		if (ci == CMD_BUF_LEN) {
			all_good = false;
			break;
		} else {
			obj_buf_i += ci + 1;
		}
		
		if (!strcmp(cmd_buf, "o")) {
			surfs.emplace_back();
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "v")) {
			char float_buf[FLOAT_BUF_LEN];
			for (int v = 0; v < 3; v++) {
				int vi;
				for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
					switch(obj_buf[obj_buf_i + vi]) {
					case '\r':
					case '\n':
					case ' ':
						float_buf[vi] = '\0';
						break;
					default:
						float_buf[vi] = obj_buf[obj_buf_i+vi];
						continue;
					}
					break;
				}
				obj_buf_i += vi + 1;
				float val = strtod(float_buf, nullptr) * size_mult;
				verts.push_back(val);
				memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
			}
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "vn")) {
				char float_buf[FLOAT_BUF_LEN];
				for (int v = 0; v < 3; v++) {
					int vi;
					for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
						switch(obj_buf[obj_buf_i + vi]) {
						case '\r':
						case '\n':
						case ' ':
							float_buf[vi] = '\0';
							break;
						default:
							float_buf[vi] = obj_buf[obj_buf_i+vi];
							continue;
						}
						break;
					}
					obj_buf_i += vi + 1;
					float val = strtod(float_buf, nullptr);
					normals.push_back(val);
					memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
				}
				obj_buf_i -= 2;
				seekline = true;
				continue;
		} else if (!strcmp(cmd_buf, "vt")) {
			char float_buf[FLOAT_BUF_LEN];
			for (int v = 0; v < 2; v++) {
				int vi;
				for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
					switch(obj_buf[obj_buf_i + vi]) {
					case '\r':
					case '\n':
					case ' ':
						float_buf[vi] = '\0';
						break;
					default:
						float_buf[vi] = obj_buf[obj_buf_i+vi];
						continue;
					}
					break;
				}
				obj_buf_i += vi + 1;
				float val = strtod(float_buf, nullptr);
				if (v) val = 1 - val;
				uvs.push_back(val);
				memset(float_buf, FLOAT_BUF_LEN, sizeof(char));
			}
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "f")) {
			objWorkSurface & cursurf = surfs.back();
			objWorkFace face;
			for (int fi = 0; fi < 3; fi++) {
				objWorkElement element;
				char int_buf[FLOAT_BUF_LEN];
				for (int v = 0; v < 3; v++) {
					int vi;
					for(vi = 0; vi < FLOAT_BUF_LEN; vi++) {
						switch(obj_buf[obj_buf_i + vi]) {
						case '\r':
						case '\n':
						case ' ':
							int_buf[vi] = '\0';
							v += 3;
							break;
						case '\\':
						case '/':
							int_buf[vi] = '\0';
							break;
						default:
							int_buf[vi] = obj_buf[obj_buf_i+vi];
							continue;
						}
						break;
					}
					obj_buf_i += vi + 1;
					int val = strtol(int_buf, nullptr, 10) - 1;
					switch(v) {
					case 0:
					case 3:
						element.vert = val;
					case 1:
					case 4:
						element.uv = val;
					case 2:
					case 5:
						element.normal = val;
					}
					memset(int_buf, FLOAT_BUF_LEN, sizeof(char));
				}
				face.elements[fi] = element;
			}
			cursurf.faces.push_back(face);
			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else if (!strcmp(cmd_buf, "usemtl")) {
			objWorkSurface & cursurf = surfs.back();
			char nam_buf[MAX_QPATH];
			memset(nam_buf, '\0', MAX_QPATH);
			int ci = 0;
			while (true) {
				if (ci >= MAX_QPATH) Com_Error(ERR_DROP, "Obj model shader field exceeds MAX_QPATH(%i)", int(MAX_QPATH));
				switch(obj_buf[obj_buf_i]) {
				case '\r':
				case '\n':
				case ' ':
					nam_buf[ci++] = '\0';
					obj_buf_i++;
					break;
				case '\\':
					nam_buf[ci++] = '\0';
					obj_buf_i++;
					break;
				default:
					nam_buf[ci++] = obj_buf[obj_buf_i];
					obj_buf_i++;
					continue;
				}
				break;
			}

			strcpy(cursurf.shader, nam_buf);

			obj_buf_i -= 2;
			seekline = true;
			continue;
		} else {
			seekline = true;
			continue;
		}
	}
	
	if (all_good) {
	} else {
		Com_Printf("Obj Load Failed.\n");
		return nullptr;
	}
	
	std::shared_ptr<GenericModel> mod = std::make_shared<GenericModel>();
	
	mod->name = name;
	mod->maxs = mod->mins = { verts[0], verts[1], verts[2] };
	
	for (uint32_t i = 3; i < verts.size(); i += 3) {
		if (verts[i] < mod->mins[0]) mod->mins[0] = verts[i];
		else if (verts[i] > mod->maxs[0]) mod->maxs[0] = verts[i];
		if (verts[i+1] < mod->mins[1]) mod->mins[1] = verts[i+1];
		else if (verts[i+1] > mod->maxs[1]) mod->maxs[1] = verts[i+1];
		if (verts[i+2] < mod->mins[2]) mod->mins[2] = verts[i+2];
		else if (verts[i+2] > mod->maxs[2]) mod->maxs[2] = verts[i+2];
	}
	
	mod->verts.resize(verts.size() / 3);
	mod->uvs.resize(uvs.size() / 2);
	mod->normals.resize(normals.size() / 3);
	memcpy(reinterpret_cast<float *>(mod->verts.data()), verts.data(), verts.size() * sizeof(float));
	memcpy(reinterpret_cast<float *>(mod->uvs.data()), uvs.data(), uvs.size() * sizeof(float));
	memcpy(reinterpret_cast<float *>(mod->normals.data()), normals.data(), normals.size() * sizeof(float));
	
	/*
	mod->numSurfaces = surfs.size();
	mod->surfaces = new objSurface_t [mod->numSurfaces];
	for (int s = 0; s < mod->numSurfaces; s++) {
		
		objSurface_t & surfTo = mod->surfaces[s];
		objWorkSurface & surfFrom = surfs[s];
		
		if (strlen(surfFrom.shader)) strcpy(surfTo.shader, surfFrom.shader);
		else strcpy(surfTo.shader, defaultShader);
		surfTo.shaderIndex = surfFrom.shaderIndex;
		
		surfTo.numFaces = surfFrom.faces.size();
		surfTo.faces = new objFace_t [surfTo.numFaces];
		for (int f = 0; f < surfTo.numFaces; f++) {
			
			objFace_t & faceT = surfTo.faces[f];
			objWorkFace & faceF = surfFrom.faces[f];
			
			faceT[0].vertex = &mod->verts[faceF.elements[2].vert * 3];
			faceT[1].vertex = &mod->verts[faceF.elements[1].vert * 3];
			faceT[2].vertex = &mod->verts[faceF.elements[0].vert * 3];
			
			faceT[0].uv = &mod->UVs[faceF.elements[2].uv * 2];
			faceT[1].uv = &mod->UVs[faceF.elements[1].uv * 2];
			faceT[2].uv = &mod->UVs[faceF.elements[0].uv * 2];
			
			faceT[0].normal = &mod->normals[faceF.elements[2].normal * 3];
			faceT[1].normal = &mod->normals[faceF.elements[1].normal * 3];
			faceT[2].normal = &mod->normals[faceF.elements[0].normal * 3];
		}
	}
	
	loaded_models.push_back(mod);
	*/
	
	return mod;
}
