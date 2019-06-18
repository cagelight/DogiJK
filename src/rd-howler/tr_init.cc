#include "rd-common/tr_font.hh"
#include "tr_local.hh"

#include "hw_local.hh"

refimport_t ri;
vidconfig_t glConfig;

cvar_t * se_language;
cvar_t * r_aspectCorrectFonts;

#define XCVAR_DECL
#include "tr_xcvar.inl"

std::unique_ptr<howler::instance> hw_inst = nullptr;

void RE_Shutdown (qboolean destroyWindow, qboolean restarting) {
	hw_inst.reset( new howler::instance {} );
	if ( destroyWindow ) {
		ri.WIN_Shutdown();
	}
}

// ================================
// COMMANDS
// ================================

// stolen from rd-vanilla
static void R_ScreenshotFilename( char *buf, int bufSize, const char *ext ) {
	time_t rawtime;
	char timeStr[32] = {0}; // should really only reach ~19 chars

	time( &rawtime );
	strftime( timeStr, sizeof( timeStr ), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) ); // or gmtime

	Com_sprintf( buf, bufSize, "screenshots/shot%s%s", timeStr, ext );
}

static void CMD_screenshot() {
	char path[MAX_OSPATH];
	R_ScreenshotFilename(path, sizeof(path), ".png");
	hw_inst->screenshot(path);
}

struct console_command_t {
	const char	*cmd;
	xcommand_t	func;
};

static constexpr console_command_t commands [] = {
	{ "lightmap_atlas",			[](){hw_inst->save_lightmap_atlas();} },
	{ "screenshot",				CMD_screenshot }
};
static constexpr size_t commands_num = ARRAY_LEN ( commands );

// ================================
// INITIALIZE
// ================================

void RE_BeginRegistration (vidconfig_t *config) {
	
	se_language = ri.Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART, "");
	r_aspectCorrectFonts = ri.Cvar_Get("r_aspectCorrectFonts", "0", CVAR_ARCHIVE, "");
	
	#define XCVAR_REGISTER
	#include "tr_xcvar.inl"
	
	r_showedges->min = 1;
	r_showtris->min = 1;
	
	glConfig = *config;
	hw_inst->initialize_renderer();
	*config = glConfig;
	
	for ( size_t i = 0; i < commands_num; i++ )
		ri.Cmd_AddCommand( commands[i].cmd, commands[i].func, "" );
}

static inline qhandle_t RE_RegisterModel (const char *name) {
	return hw_inst->models.reg(name, false)->base.index;
}

static inline qhandle_t RE_RegisterServerModel(char const * name) {
	return hw_inst->models.reg(name, true)->base.index;
}

static qhandle_t scam = 0;
qhandle_t RE_RegisterSkin (const char *name) {
	return hw_inst->skins.reg(name, false)->index;
}

qhandle_t RE_RegisterServerSkin (const char *name) {
	return hw_inst->skins.reg(name, true)->index;
}

qhandle_t RE_RegisterShader (const char *name) {
	return hw_inst->shaders.reg(name, true)->index;
}

qhandle_t RE_RegisterShaderNoMip (const char *name) {
	return hw_inst->shaders.reg(name, false)->index;
}

const char * RE_ShaderNameFromIndex (int index) {
	return hw_inst->shaders.get(index)->name.c_str();
}

void RE_LoadWorldMap (const char *name) {
	hw_inst->load_world(name);
}

void RE_EndRegistration (void) {
	
}

void RE_ClearScene (void) {
	// TODO
	RE_SetColor(nullptr);
}

void RE_ClearDecals (void) {
	
}

void RE_AddRefEntityToScene (const refEntity_t *re) {
	
	if (!re || re->reType == RT_ENT_CHAIN) return;
	
	auto & obj = hw_inst->frame().emplace_3d<howler::cmd3d::basic_object>();
	obj.basemodel = hw_inst->models.get(re->hModel);
	qm::vec3_t origin = {re->origin[1], -re->origin[2], re->origin[0]};
	
	qm::mat4_t origin_m = qm::mat4_t::translate(origin);
	qm::mat4_t axis_conv = {
		re->axis[1][1], -re->axis[1][2], re->axis[1][0], 0,
		re->axis[2][1], -re->axis[2][2], re->axis[2][0], 0,
		re->axis[0][1], -re->axis[0][2], re->axis[0][0], 0,
		0, 0, 0, 1
	};
	
	obj.shader_color = {
		re->shaderRGBA[0] / 255.0f,
		re->shaderRGBA[1] / 255.0f,
		re->shaderRGBA[2] / 255.0f,
		re->shaderRGBA[3] / 255.0f,
	};
	obj.model_matrix = axis_conv * origin_m;
}

void RE_AddMiniRefEntityToScene (const miniRefEntity_t *re) {
	// TODO -- USED BY FX SYSTEM
}

void RE_AddPolyToScene (qhandle_t hShader , int numVerts, const polyVert_t *verts, int num) {
	// NOT IMPLEMENTED -- NOTHING USES THIS
}

void RE_AddDecalToScene (qhandle_t shader, const vec3_t origin, const vec3_t dir, float orientation, float r, float g, float b, float a, qboolean alphaFade, float radius, qboolean temporary) {

}

int R_LightForPoint (vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
	return 0;
}

void RE_AddLightToScene (const vec3_t org, float intensity, float r, float g, float b) {

}

void RE_AddAdditiveLightToScene (const vec3_t org, float intensity, float r, float g, float b) {

}

void RE_RenderScene (const refdef_t *fd) {
	hw_inst->frame().scene().ref = *fd;
	hw_inst->frame().new_scene();
}

static qm::vec4_t ui_color {1, 1, 1, 1};
void RE_SetColor (const float *rgba) {
	if (rgba) ui_color = { rgba[0], rgba[1], rgba[2], rgba[3] };
	else ui_color = {1, 1, 1, 1};
}

void RE_StretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) {
	if (!hShader) return;
	hw_inst->frame().c2ds.emplace_back( howler::cmd2d::stretchpic { x, y, w, h, s1 ,t1, s2, t2, hw_inst->shaders.get(hShader), ui_color });
}

void RE_RotatePic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader) {

}

void RE_RotatePic2 (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float a1, qhandle_t hShader) {

}

void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {

}

void RE_UploadCinematic (int cols, int rows, const byte *data, int client, qboolean dirty) {

}

void RE_BeginFrame (stereoFrame_t stereoFrame) {
	hw_inst->begin_frame();
}

void RE_EndFrame (int *frontEndMsec, int *backEndMsec) {
	ui_color = {1, 1, 1, 1};
	hw_inst->end_frame((ri.Milliseconds() * ri.Cvar_VariableValue("timescale")) / 1000.0f);
}

int R_MarkFragments (int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer) {
	return 0;
}

static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

int R_LerpTag (orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagName) {
	md3Tag_t	*start, *end;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	howler::q3basemodel_ptr bmod = hw_inst->models.get(handle);
	if (!bmod) return 0;
	model = &bmod->base;
	if ( !model->md3[0] ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return qfalse;
	}

	start = R_GetTag( model->md3[0], startFrame, tagName );
	end = R_GetTag( model->md3[0], endFrame, tagName );
	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return qfalse;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}

model_t * R_GetModelByHandle (qhandle_t index);
void R_ModelBounds (qhandle_t handle, vec3_t mins, vec3_t maxs) {
	model_t		*model;
	md3Header_t	*header;
	md3Frame_t	*frame;

	model = R_GetModelByHandle( handle );

	if ( model->bmodel ) {
		VectorCopy( model->bmodel->bounds[0], mins );
		VectorCopy( model->bmodel->bounds[1], maxs );
		return;
	}

	if ( !model->md3[0] ) {
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}

	header = model->md3[0];

	frame = (md3Frame_t *)( (byte *)header + header->ofsFrames );

	VectorCopy( frame->bounds[0], mins );
	VectorCopy( frame->bounds[1], maxs );
}

void R_RemapShader (const char *oldShader, const char *newShader, const char *offsetTime) {

}

qboolean R_GetEntityToken (char *buffer, int size) {
	return hw_inst->world_get_entity_token(buffer, size);
}

qboolean R_inPVS (const vec3_t p1, const vec3_t p2, byte *mask) {
	return qtrue;
}

void RE_GetLightStyle (int style, color4ub_t color) {

}

void RE_SetLightStyle (int style, int color) {

}

void RE_GetBModelVerts (int bmodelIndex, vec3_t *vec, vec3_t normal) {

}

void SetRangedFog (float range) {

}

void SetRefractionProperties (float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate) {

}

float GetDistanceCull (void) {
	return 120000;
}

void GetRealRes (int *w, int *h) {
	*w = glConfig.vidWidth;
	*h = glConfig.vidHeight;
}

void R_AutomapElevationAdjustment (float newHeight) {

}

qboolean R_InitializeWireframeAutomap (void) {
	return qtrue;
}

void RE_AddWeatherZone (vec3_t mins, vec3_t maxs) {

}

void RE_WorldEffectCommand (const char *command) {

}

static int levelNum = 0;
void RE_RegisterMedia_LevelLoadBegin (const char *psMapName, ForceReload_e eForceReload) {
	levelNum++;
}

void RE_RegisterMedia_LevelLoadEnd (void) {

}

int RE_RegisterMedia_GetLevel (void) {
	return levelNum;
}

qboolean RE_RegisterImages_LevelLoadEnd (void) {
	return qtrue;
}

qboolean RE_RegisterModels_LevelLoadEnd (qboolean bDeleteEverythingNotUsedThisLevel) {
	return qtrue;
}

model_t * R_GetModelByHandle (qhandle_t index) {
	return &hw_inst->models.get(index)->base;
}

skin_t * R_GetSkinByHandle (qhandle_t hSkin) {
	return &hw_inst->skins.get(hSkin)->skin;
}

qboolean ShaderHashTableExists (void) {
	return hw_inst != nullptr;
}

void RE_TakeVideoFrame (int h, int w, byte* captureBuffer, byte *encodeBuffer, qboolean motionJpeg) {

}

void R_InitSkins (void) {

}

void R_InitShaders (qboolean server) {

}

void R_SVModelInit (void) {

}

void RE_HunkClearCrap (void) {
	
}

qboolean G2_HackadelicOnClient (void) {
	return hw_inst != nullptr;
}

extern "C" Q_EXPORT refexport_t* QDECL GetRefAPI( int apiVersion, refimport_t *rimp ) {
	static refexport_t re;

	assert( rimp );
	ri = *rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf( PRINT_ALL,  "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown 							= RE_Shutdown;
	re.BeginRegistration					= RE_BeginRegistration;
	re.RegisterModel						= RE_RegisterModel;
	re.RegisterServerModel					= RE_RegisterServerModel;
	re.RegisterSkin							= RE_RegisterSkin;
	re.RegisterServerSkin					= RE_RegisterServerSkin;
	re.RegisterShader						= RE_RegisterShader;
	re.RegisterShaderNoMip					= RE_RegisterShaderNoMip;
	re.ShaderNameFromIndex					= RE_ShaderNameFromIndex;
	re.LoadWorld							= RE_LoadWorldMap;
	re.EndRegistration						= RE_EndRegistration;
	re.BeginFrame							= RE_BeginFrame;
	re.EndFrame								= RE_EndFrame;
	re.MarkFragments						= R_MarkFragments;
	re.LerpTag								= R_LerpTag;
	re.ModelBounds							= R_ModelBounds;
	re.DrawRotatePic						= RE_RotatePic;
	re.DrawRotatePic2						= RE_RotatePic2;
	re.ClearScene							= RE_ClearScene;
	re.ClearDecals							= RE_ClearDecals;
	re.AddRefEntityToScene					= RE_AddRefEntityToScene;
	re.AddMiniRefEntityToScene				= RE_AddMiniRefEntityToScene;
	re.AddPolyToScene						= RE_AddPolyToScene;
	re.AddDecalToScene						= RE_AddDecalToScene;
	re.LightForPoint						= R_LightForPoint;
	re.AddLightToScene						= RE_AddLightToScene;
	re.AddAdditiveLightToScene				= RE_AddAdditiveLightToScene;

	re.RenderScene							= RE_RenderScene;
	re.SetColor								= RE_SetColor;
	re.DrawStretchPic						= RE_StretchPic;
	re.DrawStretchRaw						= RE_StretchRaw;
	re.UploadCinematic						= RE_UploadCinematic;

	re.RegisterFont							= RE_RegisterFont;
	re.Font_StrLenPixels					= RE_Font_StrLenPixels;
	re.Font_StrLenChars						= RE_Font_StrLenChars;
	re.Font_HeightPixels					= RE_Font_HeightPixels;
	re.Font_DrawString						= RE_Font_DrawString;
	re.Language_IsAsian						= Language_IsAsian;
	re.Language_UsesSpaces					= Language_UsesSpaces;
	re.AnyLanguage_ReadCharFromString		= AnyLanguage_ReadCharFromString;

	re.RemapShader							= R_RemapShader;
	re.GetEntityToken						= R_GetEntityToken;
	re.inPVS								= R_inPVS;
	re.GetLightStyle						= RE_GetLightStyle;
	re.SetLightStyle						= RE_SetLightStyle;
	re.GetBModelVerts						= RE_GetBModelVerts;

	// missing from 1.01
	re.SetRangedFog							= SetRangedFog;
	re.SetRefractionProperties				= SetRefractionProperties;
	re.GetDistanceCull						= GetDistanceCull;
	re.GetRealRes							= GetRealRes;
	re.AutomapElevationAdjustment			= R_AutomapElevationAdjustment; //tr_world.cpp
	re.InitializeWireframeAutomap			= R_InitializeWireframeAutomap; //tr_world.cpp
	re.AddWeatherZone						= RE_AddWeatherZone;
	re.WorldEffectCommand					= RE_WorldEffectCommand;
	re.RegisterMedia_LevelLoadBegin			= RE_RegisterMedia_LevelLoadBegin;
	re.RegisterMedia_LevelLoadEnd			= RE_RegisterMedia_LevelLoadEnd;
	re.RegisterMedia_GetLevel				= RE_RegisterMedia_GetLevel;
	re.RegisterImages_LevelLoadEnd			= RE_RegisterImages_LevelLoadEnd;
	re.RegisterModels_LevelLoadEnd			= RE_RegisterModels_LevelLoadEnd;

	// AVI recording
	re.TakeVideoFrame						= RE_TakeVideoFrame;

	// G2 stuff
	re.GetModelByHandle						= R_GetModelByHandle;
	re.GetSkinByHandle						= R_GetSkinByHandle;
	re.ShaderHashTableExists				= ShaderHashTableExists;
	re.InitSkins							= R_InitSkins;
	re.InitShaders							= R_InitShaders;
	re.SVModelInit							= R_SVModelInit;
	re.HunkClearCrap						= RE_HunkClearCrap;
	
	re.G2_HackadelicOnClient 				= G2_HackadelicOnClient;

	// this is set in R_Init
	//re.G2VertSpaceServer	= G2VertSpaceServer;

	re.ext.Font_StrLenPixels				= RE_Font_StrLenPixelsNew;

	hw_inst.reset( new howler::instance {} );
	
	return &re;
}
