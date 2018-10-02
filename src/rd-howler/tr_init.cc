#include "rd-common/tr_font.hh"
#include "tr_local.hh"

refimport_t ri;
glconfig_t glConfig;

cvar_t * se_language;
cvar_t * r_aspectCorrectFonts;

std::unique_ptr<modelbank> mbank;
std::unique_ptr<rend> r;
std::shared_ptr<rframe> frame;

rend::~rend() {
	R_ShutdownFonts();
	this->destruct_shader();
	this->destruct_texture();
}

void rend::initialize() {
	if (initialized) return;
	
	windowDesc_t windowDesc = { GRAPHICS_API_OPENGL };

	window = ri.WIN_Init(&windowDesc, &glConfig);
	gladLoadGLLoader(ri.GL_GetProcAddress);

	// get our config strings
	glConfig.vendor_string = (const char *)glGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *)glGetString (GL_RENDERER);
	glConfig.version_string = (const char *)glGetString (GL_VERSION);
	glConfig.extensions_string = (const char *)glGetString (GL_EXTENSIONS);
	glConfig.isFullscreen = qfalse;
	glConfig.stereoEnabled = qfalse;

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	glConfig.maxTextureSize = Q_max(0, glConfig.maxTextureSize);
	
	if (GLAD_GL_EXT_texture_filter_anisotropic)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy);
	else glConfig.maxTextureFilterAnisotropy = 0;
	
	Com_Printf("Vendor: %s\nRenderer: %s\nVersion: %s\n", glConfig.vendor_string, glConfig.renderer_string, glConfig.version_string);
	
	glViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	glScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	
	glEnable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	this->initialize_texture();
	this->initialize_shader();
	this->initialize_model();
	
	R_InitFonts();
	
	initialized = true;
}

void rend::swap() {
	ri.WIN_Present(&window);
}

void RE_Shutdown (qboolean destroyWindow, qboolean restarting) {
	if ( destroyWindow ) {
		ri.WIN_Shutdown();
	}
	r.reset();
	mbank = std::make_unique<modelbank>();
}

void RE_BeginRegistration (glconfig_t *config) {
	se_language = ri.Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART, "");
	r_aspectCorrectFonts = ri.Cvar_Get("r_aspectCorrectFonts", "0", CVAR_ARCHIVE, "");
	glConfig = *config;
	r = std::make_unique<rend>();
	r->initialize();
	*config = glConfig;
}

static inline qhandle_t RE_RegisterModel (const char *name) {
	return mbank->register_model(name, false);
}

static inline qhandle_t RE_RegisterServerModel(char const * name) {
	return mbank->register_model(name, true);
}

qhandle_t RE_RegisterSkin (const char *name) {
	return 0;
}

qhandle_t RE_RegisterServerSkin (const char *name) {
	return 0;
}

qhandle_t RE_RegisterShader (const char *name) {
	return r->register_shader(name);
}

qhandle_t RE_RegisterShaderNoMip (const char *name) {
	return r->register_shader(name, false);
}

const char * RE_ShaderNameFromIndex (int index) {
	return "";
}

void RE_LoadWorldMap (const char *name) {

}

void RE_SetWorldVisData (const byte *vis) {

}

// EndRegistration will draw a tiny polygon with each texture, forcing
// them to be loaded into card memory
void RE_EndRegistration (void) {

}

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void RE_ClearScene (void) {
	
}

void RE_ClearDecals (void) {

}

void RE_AddRefEntityToScene (const refEntity_t *re) {

}

void RE_AddMiniRefEntityToScene (const miniRefEntity_t *re) {

}

void RE_AddPolyToScene (qhandle_t hShader , int numVerts, const polyVert_t *verts, int num) {

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
	
}

void RE_SetColor (const float *rgba) {
	rcmd & cmd = frame->d_2d.emplace_back(rcmd::mode_e::color_2d);
	if (rgba) cmd.color_2d = {rgba[0], rgba[1], rgba[2], rgba[3]};
	else cmd.color_2d = {1, 1, 1, 1};
}

void RE_StretchPic (float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) {
	rcmd & cmd = frame->d_2d.emplace_back(rcmd::mode_e::stretch_pic);
	cmd.stretch_pic = {
		x, y, w, h, s1 ,t1, s2, t2, hShader
	};
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	frame = std::make_unique<rframe>();
	frame->shader_time = (ri.Milliseconds() * ri.Cvar_VariableValue("timescale")) / 1000.0f;
	RE_SetColor(nullptr);
}

void RE_EndFrame (int *frontEndMsec, int *backEndMsec) {
	r->draw(frame);
	r->swap();
}

int R_MarkFragments (int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer) {
	return 0;
}

int R_LerpTag (orientation_t *tag, qhandle_t model, int startFrame, int endFrame, float frac, const char *tagName) {
	return 0;
}

void R_ModelBounds (qhandle_t model, vec3_t mins, vec3_t maxs) {

}

void R_RemapShader (const char *oldShader, const char *newShader, const char *offsetTime) {

}

qboolean R_GetEntityToken (char *buffer, int size) {
	return qfalse;
}

qboolean R_inPVS (const vec3_t p1, const vec3_t p2, byte *mask) {
	return qfalse;
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
	return 0;
}

void GetRealRes (int *w, int *h) {

}

void R_AutomapElevationAdjustment (float newHeight) {

}

qboolean R_InitializeWireframeAutomap (void) {
	return qfalse;
}

void RE_AddWeatherZone (vec3_t mins, vec3_t maxs) {

}

void RE_WorldEffectCommand (const char *command) {

}

void RE_RegisterMedia_LevelLoadBegin (const char *psMapName, ForceReload_e eForceReload) {

}

void RE_RegisterMedia_LevelLoadEnd (void) {

}

int RE_RegisterMedia_GetLevel (void) {
	return 0;
}

qboolean RE_RegisterImages_LevelLoadEnd (void) {
	return qfalse;
}

qboolean RE_RegisterModels_LevelLoadEnd (qboolean bDeleteEverythingNotUsedThisLevel) {
	return qfalse;
}

model_t * R_GetModelByHandle (qhandle_t index) {
	return mbank->get_model(index);
}

skin_t * R_GetSkinByHandle (qhandle_t hSkin) {
	return nullptr;
}

qboolean ShaderHashTableExists (void) {
	return r != nullptr;
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
	return qfalse;
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
	re.SetWorldVisData						= RE_SetWorldVisData;
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

	mbank = std::make_unique<modelbank>();
	
	return &re;
}
