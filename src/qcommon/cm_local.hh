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

#pragma once

#include "cm_polylib.hh"
#include "cm_public.hh"
#include "qcommon/qcommon.hh"
#include "qcommon/q_math2.hh"

#define	MAX_SUBMODELS			512
#define	BOX_MODEL_HANDLE		(MAX_SUBMODELS-1)
#define CAPSULE_MODEL_HANDLE	(MAX_SUBMODELS-2)

struct Point
{
	long x, y;
};

// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define	SURFACE_CLIP_EPSILON	(0.125)

extern	clipMap_t	cmg; //rwwRMG - changed from cm
extern	int			c_pointcontents;
extern	int			c_traces, c_brush_traces, c_patch_traces;
extern	cvar_t		*cm_noAreas;
extern	cvar_t		*cm_noCurves;
extern	cvar_t		*cm_playerCurveClip;
extern	cvar_t		*cm_extraVerbose;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct sphere_s {
	qboolean	use;
	float		radius;
	float		halfheight;
	vec3_t		offset;
} sphere_t;

typedef struct traceWork_s { //rwwRMG - modified
	vec3_t		start;
	vec3_t		end;
	vec3_t		size[2];	// size of the box being swept through the model
	vec3_t		offsets[8];	// [signbits][x] = either size[0][x] or size[1][x]
	float		maxOffset;	// longest corner length from origin
	vec3_t		extents;	// greatest of abs(size[0]) and abs(size[1])
	vec3_t		modelOrigin;// origin of the model tracing through
	int			contents;	// ored contents of the model tracing through
	qboolean	isPoint;	// optimized case
//	trace_t		trace;		// returned from trace call
	sphere_t	sphere;		// sphere for oriendted capsule collision

	//rwwRMG - added:
	vec3pair_t		bounds;			// enclosing box of start and end surrounding by size
	vec3pair_t		localBounds;	// enclosing box of start and end surrounding by size for a segment

	float			baseEnterFrac;	// global enter fraction (before processing subsections of the brush)
	float			baseLeaveFrac;	// global leave fraction (before processing subsections of the brush)
	float			enterFrac;		// fraction where the ray enters the brush
	float			leaveFrac;		// fraction where the ray leaves the brush
	cbrushside_t	*leadside;
	cplane_t		*clipplane;
	bool			startout;
	bool			getout;

} traceWork_t;

typedef struct leafList_s {
	int		count;
	int		maxcount;
	qboolean	overflowed;
	int		*list;
	vec3_t	bounds[2];
	int		lastLeaf;		// for overflows where each leaf can't be stored individually
	void	(*storeLeafs)( struct leafList_s *ll, int nodenum );
} leafList_t;

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t	*CM_ClipHandleToModel( clipHandle_t handle, clipMap_t **clipMap = 0 );

// cm_patch.c

struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, trace_t &trace, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_ClearLevelPatches( void );

// cm_shader.cpp
void CM_SetupShaderProperties( void );
void CM_ShutdownShaderProperties(void);
CCMShader *CM_GetShaderInfo( const char *name );
CCMShader *CM_GetShaderInfo( int shaderNum );
void		CM_GetModelFormalName ( const char* model, const char* skin, char* name, int size );

// cm_load.cpp
void CM_GetWorldBounds ( vec3_t mins, vec3_t maxs );
