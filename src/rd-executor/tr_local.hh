#pragma once

#include "rd-common/tr_common.hh"
#include "rd-common/tr_public.hh"
#include "rd-common/tr_types.hh"

extern refimport_t ri;
extern vidconfig_t glConfig;
extern cvar_t * r_aspectCorrectFonts;

// fore tr_font
void RE_SetColor ( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
qhandle_t RE_RegisterShaderNoMip ( const char *name );
