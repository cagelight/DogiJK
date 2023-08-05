/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// tr_init.c -- functions that are not called every frame

#include "tr_local.hh"
#include "../rd-common/tr_common.hh"
#include "qcommon/MiniHeap.hh"
#include "ghoul2/g2_local.hh"
#include <algorithm>


cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;

cvar_t	*r_skipBackEnd;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
// rjr - removed for hacking cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;
cvar_t	*r_autolodscalevalue;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_drawfog;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_cullRoofFaces; //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
cvar_t	*r_roofCullCeilDist; //ceiling distance cull tolerance -rww
cvar_t	*r_roofCullFloorDist; //floor distance cull tolerance -rww
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_autoMap; //automap renderside toggle for debugging -rww
cvar_t	*r_autoMapBackAlpha; //alpha of automap bg -rww
cvar_t	*r_autoMapDisable; //don't calc it (since it's slow in debug) -rww

cvar_t	*r_dlightStyle;
cvar_t	*r_surfaceSprites;
cvar_t	*r_surfaceWeather;

cvar_t	*r_windSpeed;
cvar_t	*r_windAngle;
cvar_t	*r_windGust;
cvar_t	*r_windDampFactor;
cvar_t	*r_windPointForce;
cvar_t	*r_windPointX;
cvar_t	*r_windPointY;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_compressed_lightmaps;
cvar_t	*r_ext_preferred_tc_method;
cvar_t	*r_ext_gamma_control;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;

cvar_t	*r_DynamicGlow;
cvar_t	*r_DynamicGlowPasses;
cvar_t	*r_DynamicGlowDelta;
cvar_t	*r_DynamicGlowIntensity;
cvar_t	*r_DynamicGlowSoft;
cvar_t	*r_DynamicGlowWidth;
cvar_t	*r_DynamicGlowHeight;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_stereo;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t	*r_texturebitslm;

cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_shadowRange;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_markcount;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen = 0;
cvar_t	*r_noborder;
cvar_t	*r_centerWindow;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;

cvar_t	*r_overBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;

// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	DEFAULT_MAX_POLYS		600
#define	DEFAULT_MAX_POLYVERTS	3000
cvar_t	*r_maxpolys;
cvar_t	*r_maxpolyverts;
int		max_polys;
int		max_polyverts;

cvar_t	*r_modelpoolmegs;

/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
cvar_t	*r_noPrecacheGLA;
#endif

/*
Ghoul2 Insert End
*/

cvar_t *r_aviMotionJpegQuality;
cvar_t *r_screenshotJpegQuality;
cvar_t *r_screenshotJXLQuality;
cvar_t *r_screenshotJXLEffort;

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
} vidmode_t;

const vidmode_t r_vidModes[] = {
    { "Mode  0: 320x240",		320,	240 },
    { "Mode  1: 400x300",		400,	300 },
    { "Mode  2: 512x384",		512,	384 },
    { "Mode  3: 640x480",		640,	480 },
    { "Mode  4: 800x600",		800,	600 },
    { "Mode  5: 960x720",		960,	720 },
    { "Mode  6: 1024x768",		1024,	768 },
    { "Mode  7: 1152x864",		1152,	864 },
    { "Mode  8: 1280x1024",		1280,	1024 },
    { "Mode  9: 1600x1200",		1600,	1200 },
    { "Mode 10: 2048x1536",		2048,	1536 },
    { "Mode 11: 856x480 (wide)", 856,	 480 },
    { "Mode 12: 2400x600(surround)",2400,600 }
};
static const int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, int mode ) {
	const vidmode_t	*vm;

    if ( mode < -1 ) {
        return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		return qtrue;
	}

	vm = &r_vidModes[mode];

    *width  = vm->width;
    *height = vm->height;

    return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	Com_Printf ("\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		Com_Printf ("%s\n", r_vidModes[i].description );
	}
	Com_Printf ("\n" );
}

typedef struct consoleCommand_s {
	const char	*cmd;
	xcommand_t	func;
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "modellist",			R_Modellist_f },
	{ "modelist",			R_ModeList_f },
	{ "modelcacheinfo",		RE_RegisterModels_Info_f },
};

static const size_t numCommands = ARRAY_LEN( commands );

#ifdef _DEBUG
#define MIN_PRIMITIVES -1
#else
#define MIN_PRIMITIVES 0
#endif
#define MAX_PRIMITIVES 3

#ifdef _WIN32
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE_ND
#else
#define SWAPINTERVAL_FLAGS CVAR_ARCHIVE_ND | CVAR_LATCH
#endif

/*
===============
R_Register
===============
*/
void R_Register( void )
{
	//
	// latched and archived variables
	//
	r_allowExtensions					= ri.Cvar_Get( "r_allowExtensions",				"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_compressed_textures			= ri.Cvar_Get( "r_ext_compress_textures",			"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_compressed_lightmaps			= ri.Cvar_Get( "r_ext_compress_lightmaps",			"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_preferred_tc_method			= ri.Cvar_Get( "r_ext_preferred_tc_method",		"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_gamma_control					= ri.Cvar_Get( "r_ext_gamma_control",				"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_multitexture					= ri.Cvar_Get( "r_ext_multitexture",				"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_compiled_vertex_array			= ri.Cvar_Get( "r_ext_compiled_vertex_array",		"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_texture_env_add				= ri.Cvar_Get( "r_ext_texture_env_add",			"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ext_texture_filter_anisotropic	= ri.Cvar_Get( "r_ext_texture_filter_anisotropic",	"16",						CVAR_ARCHIVE_ND, "" );
	r_DynamicGlow						= ri.Cvar_Get( "r_DynamicGlow",					"0",						CVAR_ARCHIVE_ND, "" );
	r_DynamicGlowPasses					= ri.Cvar_Get( "r_DynamicGlowPasses",				"5",						CVAR_ARCHIVE_ND, "" );
	r_DynamicGlowDelta					= ri.Cvar_Get( "r_DynamicGlowDelta",				"0.8f",						CVAR_ARCHIVE_ND, "" );
	r_DynamicGlowIntensity				= ri.Cvar_Get( "r_DynamicGlowIntensity",			"1.13f",					CVAR_ARCHIVE_ND, "" );
	r_DynamicGlowSoft					= ri.Cvar_Get( "r_DynamicGlowSoft",				"1",						CVAR_ARCHIVE_ND, "" );
	r_DynamicGlowWidth					= ri.Cvar_Get( "r_DynamicGlowWidth",				"320",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_DynamicGlowHeight					= ri.Cvar_Get( "r_DynamicGlowHeight",				"240",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_picmip							= ri.Cvar_Get( "r_picmip",							"1",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	ri.Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_colorMipLevels					= ri.Cvar_Get( "r_colorMipLevels",					"0",						CVAR_LATCH, "" );
	r_detailTextures					= ri.Cvar_Get( "r_detailtextures",					"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_texturebits						= ri.Cvar_Get( "r_texturebits",					"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_texturebitslm						= ri.Cvar_Get( "r_texturebitslm",					"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_colorbits							= ri.Cvar_Get( "r_colorbits",						"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_stereo							= ri.Cvar_Get( "r_stereo",							"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_stencilbits						= ri.Cvar_Get( "r_stencilbits",					"8",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_depthbits							= ri.Cvar_Get( "r_depthbits",						"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_overBrightBits					= ri.Cvar_Get( "r_overBrightBits",					"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_ignorehwgamma						= ri.Cvar_Get( "r_ignorehwgamma",					"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_mode								= ri.Cvar_Get( "r_mode",							"4",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_fullscreen						= ri.Cvar_Get( "r_fullscreen",						"0",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_noborder							= ri.Cvar_Get( "r_noborder",						"0",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_centerWindow						= ri.Cvar_Get( "r_centerWindow",					"0",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_customwidth						= ri.Cvar_Get( "r_customwidth",					"1600",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_customheight						= ri.Cvar_Get( "r_customheight",					"1024",						CVAR_ARCHIVE|CVAR_LATCH, "" );
	r_simpleMipMaps						= ri.Cvar_Get( "r_simpleMipMaps",					"1",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_vertexLight						= ri.Cvar_Get( "r_vertexLight",					"0",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_uiFullScreen						= ri.Cvar_Get( "r_uifullscreen",					"0",						CVAR_NONE, "" );
	r_subdivisions						= ri.Cvar_Get( "r_subdivisions",					"4",						CVAR_ARCHIVE_ND|CVAR_LATCH, "" );
	r_displayRefresh					= ri.Cvar_Get( "r_displayRefresh",					"0",						CVAR_LATCH, "" );
	ri.Cvar_CheckRange( r_displayRefresh, 0, 200, qtrue );
	r_fullbright						= ri.Cvar_Get( "r_fullbright",						"0",						CVAR_CHEAT, "" );
	r_intensity							= ri.Cvar_Get( "r_intensity",						"1",						CVAR_LATCH, "" );
	r_singleShader						= ri.Cvar_Get( "r_singleShader",					"0",						CVAR_CHEAT|CVAR_LATCH, "" );
	r_lodCurveError						= ri.Cvar_Get( "r_lodCurveError",					"250",						CVAR_ARCHIVE_ND, "" );
	r_lodbias							= ri.Cvar_Get( "r_lodbias",						"0",						CVAR_ARCHIVE_ND, "" );
	r_autolodscalevalue					= ri.Cvar_Get( "r_autolodscalevalue",				"0",						CVAR_ROM, "" );
	r_flares							= ri.Cvar_Get( "r_flares",							"1",						CVAR_ARCHIVE_ND, "" );
	r_znear								= ri.Cvar_Get( "r_znear",							"4",						CVAR_ARCHIVE_ND, "" );
	ri.Cvar_CheckRange( r_znear, 0.001f, 10, qfalse );
	r_ignoreGLErrors					= ri.Cvar_Get( "r_ignoreGLErrors",					"1",						CVAR_ARCHIVE_ND, "" );
	r_fastsky							= ri.Cvar_Get( "r_fastsky",						"0",						CVAR_ARCHIVE_ND, "" );
	r_inGameVideo						= ri.Cvar_Get( "r_inGameVideo",					"1",						CVAR_ARCHIVE_ND, "" );
	r_drawSun							= ri.Cvar_Get( "r_drawSun",						"0",						CVAR_ARCHIVE_ND, "" );
	r_dynamiclight						= ri.Cvar_Get( "r_dynamiclight",					"1",						CVAR_ARCHIVE, "" );
	// rjr - removed for hacking
//	r_dlightBacks						= ri.Cvar_Get( "r_dlightBacks",					"1",						CVAR_CHEAT, "" );
	r_finish							= ri.Cvar_Get( "r_finish",							"0",						CVAR_ARCHIVE_ND, "" );
	r_textureMode						= ri.Cvar_Get( "r_textureMode",					"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE, "" );
	r_swapInterval						= ri.Cvar_Get( "r_swapInterval",					"0",						SWAPINTERVAL_FLAGS, "" );
	r_markcount							= ri.Cvar_Get( "r_markcount",						"100",						CVAR_ARCHIVE_ND, "" );
	r_gamma								= ri.Cvar_Get( "r_gamma",							"1",						CVAR_ARCHIVE_ND, "" );
	r_facePlaneCull						= ri.Cvar_Get( "r_facePlaneCull",					"1",						CVAR_ARCHIVE_ND, "" );
	r_cullRoofFaces						= ri.Cvar_Get( "r_cullRoofFaces",					"0",						CVAR_CHEAT, "" ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullCeilDist					= ri.Cvar_Get( "r_roofCullCeilDist",				"256",						CVAR_CHEAT, "" ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullFloorDist					= ri.Cvar_Get( "r_roofCeilFloorDist",				"128",						CVAR_CHEAT, "" ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_primitives						= ri.Cvar_Get( "r_primitives",						"0",						CVAR_ARCHIVE_ND, "" );
	ri.Cvar_CheckRange( r_primitives, MIN_PRIMITIVES, MAX_PRIMITIVES, qtrue );
	r_ambientScale						= ri.Cvar_Get( "r_ambientScale",					"0.6",						CVAR_CHEAT, "" );
	r_directedScale						= ri.Cvar_Get( "r_directedScale",					"1",						CVAR_CHEAT, "" );
	r_autoMap							= ri.Cvar_Get( "r_autoMap",						"0",						CVAR_ARCHIVE_ND, "" ); //automap renderside toggle for debugging -rww
	r_autoMapBackAlpha					= ri.Cvar_Get( "r_autoMapBackAlpha",				"0",						CVAR_NONE, "" ); //alpha of automap bg -rww
	r_autoMapDisable					= ri.Cvar_Get( "r_autoMapDisable",					"1",						CVAR_NONE, "" );
	r_showImages						= ri.Cvar_Get( "r_showImages",						"0",						CVAR_CHEAT, "" );
	r_debugLight						= ri.Cvar_Get( "r_debuglight",						"0",						CVAR_TEMP, "" );
	r_debugSort							= ri.Cvar_Get( "r_debugSort",						"0",						CVAR_CHEAT, "" );
	r_dlightStyle						= ri.Cvar_Get( "r_dlightStyle",					"1",						CVAR_TEMP, "" );
	r_surfaceSprites					= ri.Cvar_Get( "r_surfaceSprites",					"1",						CVAR_TEMP, "" );
	r_surfaceWeather					= ri.Cvar_Get( "r_surfaceWeather",					"0",						CVAR_TEMP, "" );
	r_windSpeed							= ri.Cvar_Get( "r_windSpeed",						"0",						CVAR_NONE, "" );
	r_windAngle							= ri.Cvar_Get( "r_windAngle",						"0",						CVAR_NONE, "" );
	r_windGust							= ri.Cvar_Get( "r_windGust",						"0",						CVAR_NONE, "" );
	r_windDampFactor					= ri.Cvar_Get( "r_windDampFactor",					"0.1",						CVAR_NONE, "" );
	r_windPointForce					= ri.Cvar_Get( "r_windPointForce",					"0",						CVAR_NONE, "" );
	r_windPointX						= ri.Cvar_Get( "r_windPointX",						"0",						CVAR_NONE, "" );
	r_windPointY						= ri.Cvar_Get( "r_windPointY",						"0",						CVAR_NONE, "" );
	r_nocurves							= ri.Cvar_Get( "r_nocurves",						"0",						CVAR_CHEAT, "" );
	r_drawworld							= ri.Cvar_Get( "r_drawworld",						"1",						CVAR_CHEAT, "" );
	r_drawfog							= ri.Cvar_Get( "r_drawfog",						"2",						CVAR_CHEAT, "" );
	r_lightmap							= ri.Cvar_Get( "r_lightmap",						"0",						CVAR_CHEAT, "" );
	r_portalOnly						= ri.Cvar_Get( "r_portalOnly",						"0",						CVAR_CHEAT, "" );
	r_skipBackEnd						= ri.Cvar_Get( "r_skipBackEnd",					"0",						CVAR_CHEAT, "" );
	r_measureOverdraw					= ri.Cvar_Get( "r_measureOverdraw",				"0",						CVAR_CHEAT, "" );
	r_lodscale							= ri.Cvar_Get( "r_lodscale",						"5",						CVAR_NONE, "" );
	r_norefresh							= ri.Cvar_Get( "r_norefresh",						"0",						CVAR_CHEAT, "" );
	r_drawentities						= ri.Cvar_Get( "r_drawentities",					"1",						CVAR_CHEAT, "" );
	r_ignore							= ri.Cvar_Get( "r_ignore",							"1",						CVAR_CHEAT, "" );
	r_nocull							= ri.Cvar_Get( "r_nocull",							"0",						CVAR_CHEAT, "" );
	r_novis								= ri.Cvar_Get( "r_novis",							"0",						CVAR_CHEAT, "" );
	r_showcluster						= ri.Cvar_Get( "r_showcluster",					"0",						CVAR_CHEAT, "" );
	r_speeds							= ri.Cvar_Get( "r_speeds",							"0",						CVAR_CHEAT, "" );
	r_verbose							= ri.Cvar_Get( "r_verbose",						"0",						CVAR_CHEAT, "" );
	r_logFile							= ri.Cvar_Get( "r_logFile",						"0",						CVAR_CHEAT, "" );
	r_debugSurface						= ri.Cvar_Get( "r_debugSurface",					"0",						CVAR_CHEAT, "" );
	r_nobind							= ri.Cvar_Get( "r_nobind",							"0",						CVAR_CHEAT, "" );
	r_showtris							= ri.Cvar_Get( "r_showtris",						"0",						CVAR_CHEAT, "" );
	r_showsky							= ri.Cvar_Get( "r_showsky",						"0",						CVAR_CHEAT, "" );
	r_shownormals						= ri.Cvar_Get( "r_shownormals",					"0",						CVAR_CHEAT, "" );
	r_clear								= ri.Cvar_Get( "r_clear",							"0",						CVAR_CHEAT, "" );
	r_offsetFactor						= ri.Cvar_Get( "r_offsetfactor",					"-1",						CVAR_CHEAT, "" );
	r_offsetUnits						= ri.Cvar_Get( "r_offsetunits",					"-5",						CVAR_CHEAT, "" );
	r_lockpvs							= ri.Cvar_Get( "r_lockpvs",						"0",						CVAR_CHEAT, "" );
	r_noportals							= ri.Cvar_Get( "r_noportals",						"0",						CVAR_CHEAT, "" );
	r_shadows							= ri.Cvar_Get( "cg_shadows",						"1",						CVAR_NONE, "" );
	r_shadowRange						= ri.Cvar_Get( "r_shadowRange",					"1000",						CVAR_NONE, "" );
	r_maxpolys							= ri.Cvar_Get( "r_maxpolys",						XSTRING( DEFAULT_MAX_POLYS ),		CVAR_NONE, "" );
	r_maxpolyverts						= ri.Cvar_Get( "r_maxpolyverts",					XSTRING( DEFAULT_MAX_POLYVERTS ),	CVAR_NONE, "" );
/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
	r_noPrecacheGLA						= ri.Cvar_Get( "r_noPrecacheGLA",					"0",						CVAR_CHEAT, "" );
#endif
/*
Ghoul2 Insert End
*/
	r_modelpoolmegs = ri.Cvar_Get("r_modelpoolmegs", "20", CVAR_ARCHIVE, "" );
	if (ri.Sys_LowPhysicalMemory() )
		ri.Cvar_Set("r_modelpoolmegs", "0");

	for ( size_t i = 0; i < numCommands; i++ )
		ri.Cmd_AddCommand( commands[i].cmd, commands[i].func, "" );
}


/*
===============
R_Init
===============
*/
extern void R_InitWorldEffects(void); //tr_WorldEffects.cpp
void R_Init( void ) {
	int i;
	byte *ptr;

//	Com_Printf ("----- R_Init -----\n" );
	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );

//	Swap_Init();

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}
	R_Register();

	max_polys = Q_min( r_maxpolys->integer, DEFAULT_MAX_POLYS );
	max_polyverts = Q_min( r_maxpolyverts->integer, DEFAULT_MAX_POLYVERTS );

	ptr = (byte *)Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);

	R_ModelInit();

//	Com_Printf ("----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow, qboolean restarting ) {

//	Com_Printf ("RE_Shutdown( %i )\n", destroyWindow );

	for ( size_t i = 0; i < numCommands; i++ )
		ri.Cmd_RemoveCommand( commands[i].cmd );

	tr.registered = qfalse;
}

extern void R_SVModelInit( void ); //tr_model.cpp
extern qboolean gG2_GBMNoReconstruct;
extern qboolean gG2_GBMUseSPMethod;
extern qhandle_t RE_RegisterServerSkin( const char *name );

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t re;

	assert( rimp );
	ri = *rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		Com_Printf ( "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.RegisterMedia_LevelLoadBegin			= RE_RegisterMedia_LevelLoadBegin;
	re.RegisterMedia_LevelLoadEnd			= RE_RegisterMedia_LevelLoadEnd;
	re.RegisterMedia_GetLevel				= RE_RegisterMedia_GetLevel;
	re.RegisterModels_LevelLoadEnd			= RE_RegisterModels_LevelLoadEnd;

	re.RegisterServerSkin					= RE_RegisterServerSkin;
	re.RegisterServerModel					= RE_RegisterServerModel;

	// G2 stuff
	re.GetModelByHandle						= R_GetModelByHandle;
	re.GetSkinByHandle						= R_GetSkinByHandle;
	re.ShaderHashTableExists				= ShaderHashTableExists;
	re.InitSkins							= R_InitSkins;
	re.InitShaders							= R_InitShaders;
	re.SVModelInit							= R_SVModelInit;
	re.HunkClearCrap						= RE_HunkClearCrap;
	
	re.G2_HackadelicOnClient 				= G2_HackadelicOnClient;
	
	//G2_Init(rimp, &re);

	return &re;
}
