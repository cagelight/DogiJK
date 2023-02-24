#include "hw_local.hh"
using namespace howler;

instance::shader_registry::shader_registry() {
	
	q3shader_ptr & ds = shaders.emplace_back( make_q3shader() );
	ds->name = "*default";
	ds->index = 0;
	ds->sort = q3shader::q3sort_opaque;
	lookup[ds->name] = ds;
	ds->stages.emplace_back();
	ds->stages[0].gen_map = q3stage::map_gen::mnoise;
	ds->stages[0].depthwrite = true;
	
	int num_shader_files;
	char * * shfiles = ri.FS_ListFiles("shaders", ".shader", &num_shader_files);
	for (int i = 0; i < num_shader_files; i++) {
		
		fileHandle_t f;
		char const * token, * p, * pOld;
		istring name;
		std::string shdata;
		int line;
		
		int len = ri.FS_FOpenFileRead(va("shaders/%s", shfiles[i]), &f, qfalse);
		std::string ff;
		ff.resize(len);
		ri.FS_Read(&ff[0], len, f);
		
		COM_BeginParseSession(shfiles[i]);
		p = ff.c_str();
		while (true) {
			token = COM_ParseExt(&p, qtrue);
			line = COM_GetCurrentParseLine();
			if (!token || !token[0]) break;
			if (token[0] == '#' || (token[0] == '/' && token[1] == '/')) {
				SkipRestOfLine(&p);
				continue;
			}
			name = token;
			pOld = p;
			token = COM_ParseExt(&p, qtrue);
			if (token[0] != '{' || token[1] != '\0') {
				Com_Printf(S_COLOR_RED "ERROR: Shader (\"%s\") without brace section on line %i, aborting.\n", name.c_str(), line);
				break;
			}
			if (!SkipBracedSection(&p, 1)) {
				Com_Printf(S_COLOR_RED "ERROR: Shader (\"%s\") seems to be missing closing brace, aborting.\n", name.c_str());
				break;
			}
			while (true) {
				switch (pOld [0]) {
					default:
						pOld++;
						assert(pOld < p);
						continue;
					case '{':
						break;
				}
				break;
			}
			
			char sname [MAX_QPATH];
			COM_StripExtension(name.c_str(), sname, MAX_QPATH);
			source_lookup[sname] = {pOld, p};
		}
		ri.FS_FCloseFile(f);
	}
	ri.FS_FreeFileList(shfiles);
}

q3shader_ptr instance::shader_registry::reg(istring const & name_in, bool mipmaps, default_shader_mode dmode) {
	char name_stripped [MAX_QPATH];
	COM_StripExtension(name_in.c_str(), name_stripped, MAX_QPATH);
	istring name = name_stripped;
	
	if (dmode == default_shader_mode::lightmap && (!hw_inst->m_world || !hw_inst->m_world->m_lightmap)) dmode = default_shader_mode::basic;
	
	auto m = lookup.find(name);
	if (m != lookup.end()) return m->second;
	
	q3shader_ptr shad = shaders.emplace_back(make_q3shader());
	shad->index = shaders.size() - 1;
	shad->name = name;
	shad->mips = mipmaps;
	shad->dmode = dmode;
	
	lookup[name] = shad;
	
	if (!hw_inst->renderer_initialized)
		waiting_shaders.push_back(shad);
	else
		load_shader(shad);
	
	return shad;
}

q3shader_ptr instance::shader_registry::get(qhandle_t h) {
	if (h < 0 || h >= static_cast<int32_t>(shaders.size())) return nullptr;
	return shaders[h];
}

void instance::shader_registry::process_waiting() {
	for (q3shader_ptr & shad : waiting_shaders)
		load_shader(shad);
	waiting_shaders.clear();
}

void instance::shader_registry::load_shader(q3shader_ptr shad) {
	auto src = source_lookup.find(shad->name);
	if (src != source_lookup.end()) {
		
		if (shad->parse_shader(src->second.c_str(), shad->mips))
			return;
		
		Com_Printf(S_COLOR_RED "ERROR: Could not parse shader '%s'!\n", shad->name.c_str());
		shad->index = 0;
		return;
	}
	
	q3texture_ptr diffuse = hw_inst->textures.reg(shad->name, shad->mips);
	if (!diffuse) {
		Com_Printf(S_COLOR_RED "ERROR: Could not find image for shader '%s'!\n", shad->name.c_str());
		shad->index = 0;
		return;
	}
	
	if (shad->dmode == default_shader_mode::diffuse) {
		q3stage & stg = shad->stages.emplace_back();
		stg.diffuse = diffuse;
		stg.gridlit = true;
		stg.gen_rgb = q3stage::gen_type::diffuse_lighting;
	} else {
		q3stage & stg = shad->stages.emplace_back();
		stg.diffuse = diffuse;
		if (diffuse->is_transparent() && !shad->mips /* vanilla behavior, transparency only automatically used if no mipmaps */) {
			stg.blend_src = GL_SRC_ALPHA;
			stg.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
			stg.blend = true;
			stg.clamp = true;
			shad->sort = q3shader::q3sort_basetrans;
		}
		if (shad->dmode == default_shader_mode::lightmap) { // STAGE 1
			q3stage & stg = shad->stages.emplace_back();
			stg.gen_map = q3stage::map_gen::lightmap;
			stg.blend_src = GL_DST_COLOR;
			stg.blend_dst = GL_ZERO;
			stg.blend = true;
			stg.clamp = true;
		}
	}
	

	shad->validate();
}

//================================================================
// STATIC HELPERS
//================================================================

static std::unordered_map<istring, q3stage::alpha_func> alphafunc_map = {
	{"GT0",			q3stage::alpha_func::gt0},
	{"LT128",		q3stage::alpha_func::lt128},
	{"GE128",		q3stage::alpha_func::ge128},
	{"GE192",		q3stage::alpha_func::ge192},
};

static std::unordered_map<istring, GLenum> blendfunc_map = {
	{"GL_ONE",						GL_ONE},
	{"GL_ZERO",						GL_ZERO},
	{"GL_SRC_COLOR",				GL_SRC_COLOR},
	{"GL_DST_COLOR",				GL_DST_COLOR},
	{"GL_SRC_ALPHA",				GL_SRC_ALPHA},
	{"GL_DST_ALPHA",				GL_DST_ALPHA},
	{"GL_ONE_MINUS_SRC_COLOR",		GL_ONE_MINUS_SRC_COLOR},
	{"GL_ONE_MINUS_DST_COLOR",		GL_ONE_MINUS_DST_COLOR},
	{"GL_ONE_MINUS_SRC_ALPHA",		GL_ONE_MINUS_SRC_ALPHA},
	{"GL_ONE_MINUS_DST_ALPHA",		GL_ONE_MINUS_DST_ALPHA},
};

static std::unordered_map<istring, std::pair<GLenum, GLenum>> named_blendfunc_map = {
	{"add",			{GL_ONE, GL_ONE}},
	{"filter",		{GL_DST_COLOR, GL_ZERO}},
	{"blend",		{GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA}},
};

static GLenum blendfunc_str2enum(char const * str) {
	if (!str) return GL_ONE;
	auto const & i = blendfunc_map.find(str);
	if (i == blendfunc_map.end()) {
		Com_Printf(S_COLOR_RED "ERROR: invalid blendfunc requested: (\"%s\"), defaulting to GL_ONE.\n", str);
		return GL_ONE;
	}
	else return i->second;
}

static std::unordered_map<istring, q3stage::gen_func> genfunc_map = {
	{"sin",					q3stage::gen_func::sine},
	{"square",				q3stage::gen_func::square},
	{"triangle",			q3stage::gen_func::triangle},
	{"sawtooth",			q3stage::gen_func::sawtooth},
	{"inverse_sawtooth",	q3stage::gen_func::inverse_sawtooth},
	{"noise",				q3stage::gen_func::noise},
	{"random",				q3stage::gen_func::random},
};

static q3stage::gen_func genfunc_str2enum(char const * str) {
	if (!str) return q3stage::gen_func::random;
	auto const & i = genfunc_map.find(str);
	if (i == genfunc_map.end()) {
		Com_Printf(S_COLOR_RED "ERROR: invalid genfunc requested: (\"%s\"), defaulting to random.\n", str);
		return q3stage::gen_func::random;
	}
	else return i->second;
}

static qboolean parse_vector ( const char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
 	if ( strcmp( token, "(" ) ) {
		//ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "ERROR: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			//ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "ERROR: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		//ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "ERROR: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}

static std::unordered_map<istring, q3shader::cull_type> const cull_type_lookup {
	{"front",		q3shader::cull_type::front},
	{"frontside",	q3shader::cull_type::front},
	{"frontsided",	q3shader::cull_type::front},
	{"back",		q3shader::cull_type::back},
	{"backside",	q3shader::cull_type::back},
	{"backsided",	q3shader::cull_type::back},
	{"none",		q3shader::cull_type::both},
	{"twosided",	q3shader::cull_type::both},
	{"disable",		q3shader::cull_type::both},
};

static inline q3shader::cull_type parse_cull(char const * name, char const * token) {
	auto f = cull_type_lookup.find(token);
	if (f != cull_type_lookup.end()) return f->second;
	Com_Printf(S_COLOR_YELLOW "WARNING: shader (\"%s\") has unknown/invalid cull type (\"%s\").\n", name, token);
	return q3shader::cull_type::front;
}

static std::unordered_map<istring, float> const named_sorts {
	{"portal", 1.0f},
	{"sky", 2.0f},
	{"opaque", q3shader::q3sort_opaque},
	{"decal", 4.0f},
	{"seeThrough", q3shader::q3sort_seethrough},
	{"banner", 6.0f},
	{"inside", 7.0f},
	{"mid_inside", 8.0f},
	{"middle", 9.0f},
	{"mid_outside", 10.0f},
	{"outside", 11.0f},
	{"underwater", 13.0f},
	{"additive", q3shader::q3sort_basetrans + 1},
	{"nearest", 21.0f},
};

static inline float parse_sort(char const * token) {
	auto s = named_sorts.find(token);
	if (s != named_sorts.end()) return s->second;
	float v = std::strtof(token, nullptr);
	if (!v) v = q3shader::q3sort_opaque;
	return v;
}

//================================================================
// SHADER PARSING
//================================================================



bool q3shader::parse_shader(istring const & src, bool mips) {
	
	char const * token, * p = src.c_str();
	COM_BeginParseSession("shader");
	
	token = COM_ParseExt(&p, qtrue);
	if (token[0] != '{') {
		Com_Printf(S_COLOR_RED "ERROR: shader (\"%s\") missing definition.\n", name.c_str());
		return false;
	}
	
	while (true) {
		
		token = COM_ParseExt(&p, qtrue);
		
		if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: shader (\"%s\") ends abruptly.\n", name.c_str());
			return false;
		} 
		else if (token[0] == '}')
			goto end;
		else if (token[0] == '{') {
			q3stage & stg = stages.emplace_back();
			if (!parse_stage(stg, p, mips)) {
				Com_Printf(S_COLOR_RED "ERROR: shader stage for (\"%s\") failed to parse.\n", name.c_str());
				return false;
			}
			goto next;
		} else if (!Q_stricmp(token, "detail")) {
			// IGNORED
			goto next;
		} else if (!Q_stricmp(token, "nopicmip")) {
			// TODO -- the hell is a picmip
			goto next;
		} else if (!Q_stricmp(token, "nomips") || !Q_stricmp(token, "nomipmaps")) {
			mips = false;
			goto next;
		} else if (!Q_stricmp(token, "cull")) {
			token = COM_ParseExt(&p, qfalse);
			cull = parse_cull(name.c_str(), token);
			goto next;
		} else if ( !Q_stricmp(token, "polygonOffset" )) {
			polygon_offset = true;
			goto next;
		} else if (!Q_stricmp(token, "sort")) {
			token = COM_ParseExt(&p, qfalse);
			sort = parse_sort(token);
			goto next;
		} else if (!Q_stricmp(token, "notc")) {
			// IGNORED
			goto next;
		} else if (!Q_stricmp(token, "surfaceParm")) {
			token = COM_ParseExt(&p, qfalse);
			if (!Q_stricmp(token, "nodraw")) {
				nodraw = true;
			} else {
				//Com_Printf(S_COLOR_YELLOW "ERROR: shader (\"%s\") has unknown/invalid surfaceParm (\"%s\").\n", name.c_str(), token);
			}
			goto next;
		} else if ( !Q_stricmp( token, "deformVertexes" ) || !Q_stricmp( token, "deform" ) ) {
			
			token = COM_ParseExt( &p, qfalse );
			
			if ( !Q_stricmp( token, "autosprite" ) ) {
				sprite_mode = sprite_mode_t::sprite;
				goto next;
			}
			
			if ( !Q_stricmp( token, "autosprite2" ) ) {
				sprite_mode = sprite_mode_t::line;
				goto next;
			}
			
		} else if ( !Q_stricmp( token, "skyparms" ) ) {
			
			static constexpr char const * suf[6] {"rt", "lf", "bk", "ft", "up", "dn"};
			sky_parms = std::make_unique<sky_parms_t>();
			
			token = COM_ParseExt(&p, qfalse);
			if (!token[0]) goto next;
			if (strcmp(token, "-" )) {
				char path [MAX_QPATH];
				for (auto i = 0; i < 6; i++) {
					Com_sprintf(path, sizeof(path), "%s_%s", token, suf[i]);
					sky_parms->sides[i] = hw_inst->textures.reg(path, qtrue);
					if (!sky_parms->sides[i] && i) sky_parms->sides[i] = sky_parms->sides[i-1];
				}
			}
			
			token = COM_ParseExt(&p, qfalse);
			if (!token[0]) goto next;
			sky_parms->cloud_height = strtof(token, nullptr);
			if (!sky_parms->cloud_height) sky_parms->cloud_height = 512;
			
			token = COM_ParseExt(&p, qfalse);
			if (!token[0]) goto next;
			if (strcmp(token, "-" )) {
				Com_Printf(S_COLOR_YELLOW "WARNING: shader (\"%s\") skyparms have an inner box, this is not yet supported.\n", name.c_str());
			}
			
			goto next;
			
		} else if (!Q_stricmpn(token, "qer_", 4) || !Q_stricmpn(token, "q3map_", 6)) {
			goto next;
		} else {
			Com_Printf(S_COLOR_YELLOW "WARNING: shader (\"%s\") has unknown/invalid key (\"%s\").\n", name.c_str(), token);
			goto next;
		}
		
		next:
		SkipRestOfLine(&p);
		continue;
		end:
		break;
	}
	
	validate();
	
	return true;
}

void q3shader::validate() {
	opaque = false;
	blended = false;
	depthwrite = false;
	gridlit = false;
	
	for (q3stage & stg : stages) {
		stg.validate();
		if (stg.depthwrite) depthwrite = true;
		if (stg.opaque) opaque = true;
		if (stg.blend) blended = true;
		if (stg.gridlit) gridlit = true;
	}
	
	if (!sort) {
		if (polygon_offset)
			sort = q3sort_decal;
		else if (opaque)
			sort = q3sort_opaque;
		else
			sort = depthwrite ? q3sort_seethrough : q3sort_basetrans;
	}
	
	valid = true;
}

bool q3shader::parse_stage(q3stage & stg, char const * & sptr, bool mips) {
	
	char const * token;
	
	while (true) {
		
		token = COM_ParseExt(&sptr, qtrue);
		
		if (!token[0]) {
			
			Com_Printf(S_COLOR_RED "ERROR: shader stage for (\"%s\") ends abruptly.\n", name.c_str());
			goto fail;
			
		} else if (token[0] == '}')
			
			goto end;
		
		//================================
		// MAP
		//================================
		
		else if (!Q_stricmp("map", token) || !Q_stricmp("clampmap", token)) {
			
			stg.clamp = !Q_stricmp("clampmap", token);
			token = COM_ParseExt(&sptr, qfalse);
			if (!Q_stricmp("$lightmap", token)) {
				stg.gen_map = q3stage::map_gen::lightmap;
				stg.clamp = true;
			} else if (!Q_stricmp("$noise", token)) {
				stg.gen_map = q3stage::map_gen::mnoise;
			} else if (!Q_stricmp("$alphanoise", token)) {
				stg.gen_map = q3stage::map_gen::anoise;
			} else {
				stg.diffuse = hw_inst->textures.reg(token, mips);
				if (!stg.has_diffuse()) {
					Com_Printf(S_COLOR_RED "ERROR: shader stage for (\"%s\") has invalid map (\"%s\"), could not find this image.\n", name.c_str(), token);
					goto fail;
				}
			}
			
		} else if (!Q_stricmp("animmap", token) || !Q_stricmp("clampanimmap", token) || !Q_stricmp("oneshotanimmap", token)) {
			
			stg.clamp = !Q_stricmp("clampanimmap", token);
			q3stage::diffuse_anim_ptr anim = std::make_shared<q3stage::diffuse_anim_t>();
			
			token = COM_ParseExt(&sptr, qfalse);
			anim->speed = std::strtof(token, nullptr);
			
			while (true) {
				token = COM_ParseExt(&sptr, qfalse);
				if (!token[0]) break;
				
				q3texture_ptr tex = hw_inst->textures.reg(token, mips);
				if (!tex) {
					Com_Printf(S_COLOR_RED "ERROR: animated map for (\"%s\") has invalid frame (\"%s\"), could not find this image.\n", name.c_str(), token);
					continue;
				}
				anim->maps.push_back(tex);
				if (tex->is_transparent()) anim->is_transparent = true;
			}
			
			if (!anim->maps.size()) {
				Com_Printf(S_COLOR_RED "ERROR: shader stage for (\"%s\") has invalid animated map (\"%s\"), could not load any frames.\n", name.c_str(), token);
				goto fail;
			}
			
			stg.diffuse = anim;
			
		} else if (!Q_stricmp("videomap", token)) {
			
			token = COM_ParseExt(&sptr, qfalse);
			q3stage::diffuse_cinematic_t & cin = stg.diffuse.emplace<q3stage::diffuse_cinematic_t>();
			cin.handle = ri.CIN_PlayCinematic( token, 0, 0, 512, 512, (CIN_loop | CIN_silent | CIN_shader));
			
		//================================
			
		} else if (!Q_stricmp("alphaFunc", token)) {
			
			token = COM_ParseExt(&sptr, qfalse);
			auto iter = alphafunc_map.find(token);
			if (iter == alphafunc_map.end()) {
				Com_Printf(S_COLOR_RED "ERROR: shader stage for (\"%s\") has invalid alphaFunc (\"%s\"), could not find this image.\n", name.c_str(), token);
				goto fail;
			}
			stg.alpha_test = iter->second;
			
		} else if (!Q_stricmp("blendFunc", token)) {
			
			token = COM_ParseExt(&sptr, qfalse);
			auto iter = named_blendfunc_map.find(token);
			if (iter != named_blendfunc_map.end()) {
				stg.blend_src = iter->second.first;
				stg.blend_dst = iter->second.second;
			} else {
				stg.blend_src = blendfunc_str2enum(token);
				token = COM_ParseExt(&sptr, qfalse);
				stg.blend_dst = blendfunc_str2enum(token);
			}
			stg.blend = true;
			
		} else if (!Q_stricmp(token, "detail")) {
			
			// IGNORED
			
		} else if (!Q_stricmp("rgbGen", token) || !Q_stricmp("alphaGen", token)) {
			
			bool alpha = !Q_stricmp("alphaGen", token);
			q3stage::gen_type & gen = alpha ? stg.gen_alpha : stg.gen_rgb;
			q3stage::wave_func_t & wave = alpha ? stg.wave_alpha : stg.wave_rgb;
			
			token = COM_ParseExt(&sptr, qfalse);
			
			auto iter = q3stage::gen_type_lookup.find(token);
			if (iter != q3stage::gen_type_lookup.end()) {
				gen = iter->second;
			} else {
				Com_Printf(S_COLOR_YELLOW "WARNING: shader stage for (\"%s\") has unknown/invalid %s (\"%s\").\n", name.c_str(), alpha ? "alphaGen" : "rgbGen", token);
				goto next;
			}
			
			switch (gen) {
				
				default:
					break;
				case q3stage::gen_type::constant:
					if (alpha) {
						token = COM_ParseExt(&sptr, qfalse);
						stg.const_color[3] = strtof(token, nullptr);
					} else {
						vec3_t color;
						parse_vector(&sptr, 3, color);
						stg.const_color[0] = color[0];
						stg.const_color[1] = color[1];
						stg.const_color[2] = color[2];
					}
					break;
				case q3stage::gen_type::vertex:
					if (!alpha && stg.gen_alpha == q3stage::gen_type::identity) 
						stg.gen_alpha = q3stage::gen_type::vertex;
					break;
				case q3stage::gen_type::diffuse_lighting:
				case q3stage::gen_type::diffuse_lighting_entity:
				case q3stage::gen_type::specular_lighting:
					stg.gridlit = true;
					break;
				case q3stage::gen_type::wave:
					token = COM_ParseExt( &sptr, qfalse ); if (!token[0]) {
						Com_Printf(S_COLOR_YELLOW "WARNING: missing alphaGen/rgbGen wave parm \"genfunc\" in shader (\"%s\")\n", name.c_str());
						goto next;
					}
					wave.func = genfunc_str2enum(token);
					token = COM_ParseExt( &sptr, qfalse ); if (!token[0]) {
						Com_Printf(S_COLOR_YELLOW "WARNING: missing alphaGen/rgbGen wave parm \"base\" in shader (\"%s\")\n", name.c_str());
						goto next;
					}
					wave.base = strtof(token, nullptr);
					token = COM_ParseExt( &sptr, qfalse ); if (!token[0]) {
						Com_Printf(S_COLOR_YELLOW "WARNING: missing alphaGen/rgbGen wave parm \"amplitude\" in shader (\"%s\")\n", name.c_str());
						goto next;
					}
					wave.amplitude = strtof(token, nullptr);
					token = COM_ParseExt( &sptr, qfalse ); if (!token[0]) {
						Com_Printf(S_COLOR_YELLOW "WARNING: missing alphaGen/rgbGen wave parm \"phase\" in shader (\"%s\")\n", name.c_str());
						goto next;
					}
					wave.phase= strtof(token, nullptr);
					token = COM_ParseExt( &sptr, qfalse ); if (!token[0]) {
						Com_Printf(S_COLOR_YELLOW "WARNING: missing alphaGen/rgbGen wave parm \"frequency\" in shader (\"%s\")\n", name.c_str());
						goto next;
					}
					wave.frequency = strtof(token, nullptr);
					break;
			}
			
		} else if (!Q_stricmp(token, "tcGen")) {
			
			token = COM_ParseExt(&sptr, qfalse);
			if (!Q_stricmp("environment", token)) {
				stg.gen_tc = q3stage::tcgen::environment;
				goto next;
			} else if (!Q_stricmp("lightmap", token)) {
				stg.gen_tc = q3stage::tcgen::lightmap;
				goto next;
			} else {
				Com_Printf(S_COLOR_YELLOW "WARNING: shader stage for (\"%s\") has unknown/invalid tcGen (\"%s\").\n", name.c_str(), token);
				goto next;
			}
			
		} else if (!Q_stricmp(token, "tcMod")) {
			
			char buffer [1024] = "";
			while (true) {
				token = COM_ParseExt(&sptr, qfalse);
				if (!token[0]) break;
				Q_strcat( buffer, sizeof( buffer ), token );
				Q_strcat( buffer, sizeof( buffer ), " " );
			}
			parse_texmod(stg, buffer);
			
		} else if (!Q_stricmp(token, "glow")) {
			
			//stg.glow = true;
			//TODO
			
		} else if (!Q_stricmp( token, "depthwrite" )) {
			
			stg.depthwrite = true;
			
		} else if (!Q_stricmp( token, "depthfunc" )) {
			
			static std::unordered_map<istring, GLenum> const depth_func_map {
				{"less",		GL_LESS},
				{"equal",		GL_EQUAL},
				{"lequal",		GL_LEQUAL},
				{"greater",		GL_GREATER},
				{"notequal",	GL_NOTEQUAL},
				{"gequal",		GL_GEQUAL},
				{"disable",		GL_NEVER},
				{"never",		GL_NEVER},
				{"always",		GL_ALWAYS},
			};
			
			token = COM_ParseExt(&sptr, qfalse);
			auto iter = depth_func_map.find(token);
			if (iter == depth_func_map.end()) {
				Com_Printf(S_COLOR_YELLOW "WARNING: shader stage for (\"%s\") has unknown/invalid depthFunc (\"%s\").\n", name.c_str(), token);
			}
			
			stg.depth_func = iter->second;
			
		} else {
			
			Com_Printf(S_COLOR_YELLOW "WARNING: shader stage for (\"%s\") has unknown/invalid key (\"%s\").\n", name.c_str(), token);
			goto next;
			
		}
		
		continue;
		next:
		SkipRestOfLine(&sptr);
		continue;
		end:
		break;
		fail:
		return false;
	}
	
	return true;
}

void q3stage::validate() {
	
	opaque = true;
	
	if (blend) {
		if (blend_dst == GL_ONE_MINUS_SRC_ALPHA && has_diffuse() && diffuse_has_transparency()) opaque = false;
		else if (blend_dst != GL_ZERO) opaque = false;
		else if (
			blend_src == GL_DST_ALPHA ||
			blend_src == GL_DST_COLOR ||
			blend_src == GL_ONE_MINUS_DST_ALPHA ||
			blend_src == GL_ONE_MINUS_DST_COLOR
		) opaque = false;
	}
}

void q3shader::parse_texmod(q3stage & stg, char const * ptr) {
	
	char const * token = COM_ParseExt( &ptr, qfalse );
	
	if (!Q_stricmp( token, "turb")) {
		
		auto & turb = std::get<q3stage::tx_turb>(stg.texmods.emplace_back( q3stage::tx_turb {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod turb parm \"base\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		turb.base = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod turb parm \"amplitude\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		turb.amplitude = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod turb parm \"phase\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		turb.phase = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod turb parm \"frequency\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		turb.frequency = strtof(token, nullptr);
		
	} else if (!Q_stricmp( token, "scale")) {
		
		auto & scale = std::get<q3stage::tx_scale>(stg.texmods.emplace_back( q3stage::tx_scale {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod scale parm \"x\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		scale.value[0] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod scale parm \"y\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		scale.value[1] = strtof(token, nullptr);
		
	} else if (!Q_stricmp( token, "scroll")) {
		
		auto & scroll = std::get<q3stage::tx_scroll>(stg.texmods.emplace_back( q3stage::tx_scroll {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod scroll parm \"x\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		scroll.value[0] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod scroll parm \"y\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		scroll.value[1] = strtof(token, nullptr);
		
	} else if (!Q_stricmp( token, "stretch")) {
		
		auto & stretch = std::get<q3stage::tx_stretch>(stg.texmods.emplace_back( q3stage::tx_stretch {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod stretch parm \"genfunc\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		stretch.gfunc = genfunc_str2enum(token);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod stretch parm \"base\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		stretch.base = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod stretch parm \"amplitude\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		stretch.amplitude = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod stretch parm \"phase\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		stretch.phase= strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod stretch parm \"frequency\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		stretch.frequency = strtof(token, nullptr);
		
	} else if (!Q_stricmp( token, "transform")) {

		auto & transform = std::get<q3stage::tx_transform>(stg.texmods.emplace_back( q3stage::tx_transform {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[0][0]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[0][0] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[0][1]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[0][1] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[1][0]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[1][0] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[1][1]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[1][1] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[2][0]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[2][0] = strtof(token, nullptr);
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod transform parm \"[2][1]\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		transform.matrix[2][1] = strtof(token, nullptr);
		
	} else if (!Q_stricmp( token, "rotate")) {

		auto & rotate = std::get<q3stage::tx_rotate>(stg.texmods.emplace_back( q3stage::tx_rotate {} ));
		
		token = COM_ParseExt( &ptr, qfalse ); if (!token[0]) {
			Com_Printf(S_COLOR_RED "ERROR: missing tcMod rotate parm \"value\" in shader (\"%s\")\n", name.c_str());
			return;
		}
		rotate.angle = strtof(token, nullptr);
		rotate.angle = qm::deg2rad(rotate.angle);
	} else {
		Com_Printf(S_COLOR_RED "ERROR: unknown tcMod parameter \"%s\" in shader (\"%s\")\n", token, name.c_str());
		return;
	}
}

//================================================================
// DRAW PREPARATON
//================================================================

void q3shader::setup_draw() const {
	
	gl::polygon_offset_fill(polygon_offset);
	if (polygon_offset) {
		gl::polygon_offset(-1, -2);
	}
	
	switch (cull) {
		case q3shader::cull_type::front:
			gl::cull(true);
			gl::cull_face(GL_FRONT);
			break;
		case q3shader::cull_type::back:
			gl::cull(true);
			gl::cull_face(GL_BACK);
			break;
		case q3shader::cull_type::both:
			gl::cull(false);
			break;
	}
}

static inline float Q_fractional(float v) {
	return v - static_cast<uint_fast64_t>(v);
}

static float gen_func_do(q3stage::gen_func func, float in, float base, float amplitude, float phase, float frequency) {
	switch (func) {
		case q3stage::gen_func::sine:
			return base + std::sin((in * frequency + phase) * qm::pi * 2) * amplitude;
		case q3stage::gen_func::square:
			return base + (Q_fractional((in + phase) * frequency) > 0.5f ? 1.0f : -1.0f) * amplitude;
		case q3stage::gen_func::triangle:
			return 0; // TODO
		case q3stage::gen_func::sawtooth:
			return base + Q_fractional((in + phase) * frequency) * amplitude;
		case q3stage::gen_func::inverse_sawtooth:
			return base + (1.0f - Q_fractional((in + phase) * frequency)) * amplitude;
		case q3stage::gen_func::noise:
			return 0; // TODO
		case q3stage::gen_func::random:
			return base + Q_flrand(0.0f, 1.0f) * amplitude;
	}
	return 0;
}

void q3stage::setup_draw(setup_draw_parameters_t const & parm) const {
	
	gl::depth_func(depth_func);
	gl::depth_write(opaque || depthwrite);
	
	switch (alpha_test) {
		case alpha_func::none:
			gl::alpha_test(false);
			break;
		case alpha_func::gt0:
			gl::alpha_test(true);
			gl::alpha_func(GL_GREATER, 0.0f);
			break;
		case alpha_func::lt128:
			gl::alpha_test(true);
			gl::alpha_func(GL_LESS, 0.5f);
			break;
		case alpha_func::ge128:
			gl::alpha_test(true);
			gl::alpha_func(GL_GEQUAL, 0.5f);
			break;
		case alpha_func::ge192:
			gl::alpha_test(true);
			gl::alpha_func(GL_GEQUAL, 0.75f);
			break;
	}
	
	gl::blend(blend);
	gl::blend(blend_src, blend_dst);
	
	hw_inst->main_sampler->wrap(clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	
	float mult;
	
	qm::vec4_t q3color {1, 1, 1, 1};
	switch (gen_rgb) {
		case q3stage::gen_type::diffuse_lighting_entity:
		case q3stage::gen_type::entity:
			q3color[0] = parm.shader_color[0];
			q3color[1] = parm.shader_color[1];
			q3color[2] = parm.shader_color[2];
			break;
		case q3stage::gen_type::vertex:
		case q3stage::gen_type::vertex_exact:
		case q3stage::gen_type::vertex_one_minus:
			q3color = hw_inst->m_shader_color;
			break;
		case q3stage::gen_type::identity:
			if (hw_inst->m_ui_draw) q3color = hw_inst->m_shader_color;
			break;
		case q3stage::gen_type::constant:
			q3color = const_color;
			break;
		case q3stage::gen_type::wave:
			mult = gen_func_do(wave_rgb.func, parm.time, wave_rgb.base, wave_rgb.amplitude, wave_rgb.phase, wave_rgb.frequency);
			q3color = const_color * mult;
			break;
		case q3stage::gen_type::specular_lighting:
		case q3stage::gen_type::diffuse_lighting:
			// handled in shader
			break;
	}
	
	switch (gen_alpha) {
		case q3stage::gen_type::diffuse_lighting_entity:
		case q3stage::gen_type::entity:
			q3color[3] = parm.shader_color[3];
			break;
		case q3stage::gen_type::vertex:
		case q3stage::gen_type::vertex_exact:
		case q3stage::gen_type::vertex_one_minus:
			q3color[3] = hw_inst->m_shader_color[3];
			break;
		case q3stage::gen_type::identity:
			if (hw_inst->m_ui_draw) q3color[3] = hw_inst->m_shader_color[3];
			break;
		case q3stage::gen_type::constant:
			q3color[3] = const_color[3];
			break;
		case q3stage::gen_type::wave:
			mult = gen_func_do(wave_alpha.func, parm.time, wave_alpha.base, wave_alpha.amplitude, wave_alpha.phase, wave_alpha.frequency);
			q3color[3] = const_color[3] * mult;
			break;
		case q3stage::gen_type::specular_lighting:
		case q3stage::gen_type::diffuse_lighting: 
			// handled in shader
			break;
	}
	
	bool turb = false;
	q3stage::tx_turb const * turb_data = nullptr;
	
	qm::mat3_t uvm2 = parm.uvm;
	for (texmod const & tx : texmods) {
		std::visit(lambda_visit{
			[&](tx_turb const & value){
				turb = true;
				turb_data = &value;
			},
			[&](tx_scale const & value){
				uvm2 *= qm::mat3_t::scale(value.value[0], value.value[1]);
			},
			[&](tx_scroll const & value){
				uvm2 *= qm::mat3_t::translate(value.value[0] * parm.time, value.value[1] * parm.time);
			},
			[&](tx_stretch const & value){
				float p = 1.0f / gen_func_do(value.gfunc, parm.time, value.base, value.amplitude, value.phase, value.frequency);
				qm::mat3_t mat = qm::mat3_t::identity();
				mat[0][0] = mat[1][1] = p;
				mat[2][0] = mat[2][1] = 0.5f - 0.5f * p;
				uvm2 *= mat;
			},
			[&](tx_transform const & value){
				uvm2 *= value.matrix;
			},
			[&](tx_rotate const & value){
				uvm2 *= qm::mat3_t::translate(-0.5, -0.5);
				uvm2 *= qm::mat3_t::rotate(value.angle * parm.time);
				uvm2 *= qm::mat3_t::translate(0.5, 0.5);
			}
		}, tx);
	}
	
	if (has_diffuse() && gen_map == map_gen::diffuse)
		if ((r_shownormals->integer || r_whiteimage->integer) && !parm.is_2d) {
			hw_inst->textures.whiteimage->bind(BINDING_DIFFUSE);
		} else
			std::visit( lambda_visit {
				[&](q3texture_ptr const & tex) { tex->bind(BINDING_DIFFUSE); },
				[&](diffuse_anim_ptr const & anim) {
					uint_fast16_t index = std::floor(parm.time * anim->speed);
					index %= anim->maps.size();
					anim->maps[index]->bind(BINDING_DIFFUSE);
				},
				[&](diffuse_cinematic_t const & cin) {
					ri.CIN_RunCinematic(cin.handle);
					ri.CIN_UploadCinematic(cin.handle);
					if (cin.handle >= (int)hw_inst->cinematic_frames.size()) return;
					hw_inst->cinematic_frames[cin.handle]->bind(BINDING_DIFFUSE);
				}
			}, diffuse);
	else
		q3texture::unbind(BINDING_DIFFUSE);
	
	hw_inst->q3mainprog->bind();
	hw_inst->q3mainprog->time(parm.time);
	hw_inst->q3mainprog->mvp(parm.mvp);
	hw_inst->q3mainprog->uvm(uvm2);
	hw_inst->q3mainprog->color(q3color);
	hw_inst->q3mainprog->turb(turb);
	hw_inst->q3mainprog->tcgen(gen_tc);
	hw_inst->q3mainprog->mapgen(gen_map);
	hw_inst->q3mainprog->viewpos(parm.view_origin);
	
	if (hw_inst->m_ui_draw && gen_rgb == gen_type::vertex) // when drawing UI elements, "vertex" is supposed to mean using the global color, not the actual vertex colors of the quad
		hw_inst->q3mainprog->cgen(gen_type::identity);
	else
		hw_inst->q3mainprog->cgen(gen_rgb);
	
	if (hw_inst->m_ui_draw && gen_alpha == gen_type::vertex) // when drawing UI elements, "vertex" is supposed to mean using the global alpha, not the actual vertex alphas of the quad
		hw_inst->q3mainprog->agen(gen_type::identity);
	else
		hw_inst->q3mainprog->agen(gen_alpha);
	
	hw_inst->q3mainprog->gridlighting(gridlit ? parm.gridlight : nullptr);
	
	if (turb) {
		hw_inst->q3mainprog->turb_data(*turb_data);
	}
	
	if (gridlit) {
		hw_inst->q3mainprog->itm(parm.itm);
	}
	
	//if (gridlit || gen_tc == tcgen::environment) {
		hw_inst->q3mainprog->m(parm.m);
	//}
	
	if (parm.bone_weights)
		hw_inst->q3mainprog->bone_matricies(parm.bone_weights->data(), parm.bone_weights->size());
	else
		hw_inst->q3mainprog->bone_matricies(nullptr, 0);
	
	if (parm.mesh_uniforms)
		hw_inst->q3mainprog->lm_mode(parm.mesh_uniforms->lm_mode);
	else
		hw_inst->q3mainprog->lm_mode(0);
			
}
