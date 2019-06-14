#pragma once

#include "qcommon/q_math2.hh"

#include "rd-common/tr_common.hh"
#include "rd-common/tr_public.hh"
#include "rd-common/tr_types.hh"

extern refimport_t ri;
extern vidconfig_t glConfig;
extern cvar_t * r_aspectCorrectFonts;

#define XCVAR_PROTO
#include "tr_xcvar.inl"

namespace howler {
	struct instance;
}

extern std::unique_ptr<howler::instance> hw_inst;

typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_OBJ,
/*
Ghoul2 Insert Start
*/
	SF_MDX,
/*
Ghoul2 Insert End
*/
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0xffffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct bmodel_s {
	vec3_t		bounds[2];		// for culling
	int32_t 	surf_idx;
	int32_t 	surf_num;
} bmodel_t;

static_assert(std::is_pod<bmodel_t>());

// for tr_font
void RE_SetColor ( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
qhandle_t RE_RegisterShaderNoMip ( const char *name );
