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

#include <libbsp/reader.hh>

#include "qcommon/q_shared.hh"
#include "qfiles.hh"

typedef struct cNode_s {
	cplane_t	*plane;
	int32_t		children[2];		// negative numbers are leafs
	qm::ivec3_t	mins, maxs;
} cNode_t;

typedef struct cLeaf_s {
	int32_t		cluster;
	int32_t		area;
	
	qm::ivec3_t	mins, maxs;

	ptrdiff_t	firstLeafBrush;
	int32_t		numLeafBrushes;

	ptrdiff_t	firstLeafSurface;
	int32_t		numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s {
	vec3_t		mins, maxs;
	cLeaf_t		leaf;			// submodels don't reference the main tree
	int32_t		firstNode;		// only for cmodel[0] (for the main and bsp instances)
} cmodel_t;

typedef struct cbrushside_s {
	cplane_t	*plane;
	int32_t		shaderNum;
	int32_t		surfNum;
} cbrushside_t;

typedef struct cbrush_s {
	int32_t				shaderNum;		// the shader that determined the contents
	int32_t				contents;
	vec3_t				bounds[2];
	cbrushside_t		*sides;
	unsigned short		numsides;
} cbrush_t;

// a trace is returned when a box is swept through the world
typedef struct trace_s {
	byte			allsolid;	// if true, plane is not valid
	byte			startsolid;	// if true, the initial point was in a solid area
	int16_t			entityNum;	// entity the contacted sirface is a part of

	float			fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t			endpos;		// final position
	cplane_t		plane;		// surface normal at impact, transformed to world space
	cbrushside_t *	brushside;
	int32_t			surfaceFlags;	// surface hit
	int32_t			brushNum;
	int32_t			contents;	// contents on other side of surface hit
/*
Ghoul2 Insert Start
*/
	//rww - removed this for now, it's just wasting space in the trace structure.
//	CollisionRecord_t G2CollisionMap[MAX_G2_COLLISIONS];	// map that describes all of the parts of ghoul2 models that got hit
/*
Ghoul2 Insert End
*/
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

class CCMShader
{
public:
	char		shader[MAX_QPATH];
	class CCMShader	*mNext;
	int			surfaceFlags;
	int			contentFlags;

	const char	*GetName(void) const { return(shader); }
	class CCMShader *GetNext(void) const { return(mNext); }
	void	SetNext(class CCMShader *next) { mNext = next; }
	void	Destroy(void) { }
};

typedef struct cPatch_s {
	int			surfaceFlags;
	int			contents;
	struct patchCollide_s	*pc;
} cPatch_t;


typedef struct cArea_s {
	int			floodnum;
	int			floodvalid;
} cArea_t;

typedef struct clipMap_s {
	char		name[MAX_QPATH];

	int			numShaders;
	CCMShader	*shaders;

	int			numBrushSides;
	cbrushside_t *brushsides;

	int			numPlanes;
	cplane_t	*planes;

	int			numNodes;
	cNode_t		*nodes;

	int			numLeafs;
	cLeaf_t		*leafs;

	int			numLeafBrushes;
	int			*leafbrushes;

	int			numLeafSurfaces;
	int			*leafsurfaces;

	int			numSubModels;
	cmodel_t	*cmodels;

	int			numBrushes;
	cbrush_t	*brushes;

	int			numClusters;
	int			clusterBytes;
	byte		*visibility;
	qboolean	vised;			// if false, visibility is just a single cluster of ffs

	int			numEntityChars;
	char		*entityString;

	int			numAreas;
	cArea_t		*areas;
	int			*areaPortals;	// [ numAreas*numAreas ] reference counts

	int			numSurfaces;
	cPatch_t	**surfaces;			// non-patches will be NULL

	int			floodvalid;
} clipMap_t;

clipMap_t const * CM_Get();

BSP::Reader CM_Read();

void		CM_LoadMap( const char *name, qboolean clientload, int *checksum);

void		CM_ClearMap( void );
clipHandle_t CM_InlineModel( int index );		// 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule );

void		CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

int			CM_NumInlineModels( void );
char		*CM_EntityString (void);

// returns an ORed contents mask
int			CM_PointContents( const vec3_t p, clipHandle_t model );
int			CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );

void		CM_BoxTrace ( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, int capsule );
void		CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles, int capsule );

byte		*CM_ClusterPVS (int cluster);

int			CM_PointLeafnum( const vec3_t p );

// only returns non-solid leafs
// overflow if return listsize and if *lastLeaf != list[listsize-1]
int			CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *boxList,
		 					int listsize, int *lastLeaf );
//rwwRMG - changed to boxList to not conflict with list type

int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_AdjustAreaPortalState( int area1, int area2, qboolean open );
qboolean	CM_AreasConnected( int area1, int area2 );

int			CM_WriteAreaBits( byte *buffer, int area );

//rwwRMG - added:
bool		CM_GenericBoxCollide(const vec3pair_t abounds, const vec3pair_t bbounds);
void		CM_CalcExtents(const vec3_t start, const vec3_t end, const struct traceWork_s *tw, vec3pair_t bounds);

// cm_tag.c
int			CM_LerpTag( orientation_t *tag,  clipHandle_t model, int startFrame, int endFrame,
					 float frac, const char *tagName );


// cm_marks.c
int	CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

// cm_patch.c
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, float *points) );

// cm_trace.cpp
bool CM_CullWorldBox (const cplane_t *frustum, const vec3pair_t bounds);
