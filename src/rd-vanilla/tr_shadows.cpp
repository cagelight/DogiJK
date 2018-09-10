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

#include "tr_local.h"


/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

#define _STENCIL_REVERSE

typedef struct edgeDef_s {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES/3];
static	vec3_t		shadowXyz[SHADER_MAX_VERTEXES];

void R_AddEdgeDef( int i1, int i2, int facing ) {
	int		c;

	c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}

void R_RenderShadowEdges( void ) {
	int		i;
	int		c;
	int		j;
	int		i2;
#if 0
	int		c_edges, c_rejected;
	int		c2, k;
	int		hit[2];
#endif
#ifdef _STENCIL_REVERSE
	int		numTris;
	int		o1, o2, o3;
#endif

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
#if 0
	c_edges = 0;
	c_rejected = 0;
#endif

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			//with this system we can still get edges shared by more than 2 tris which
			//produces artifacts including seeing the shadow through walls. So for now
			//we are going to render all edges even though it is a tiny bit slower. -rww
#if 1
			i2 = edgeDefs[ i ][ j ].i2;
			glBegin( GL_TRIANGLE_STRIP );
				glVertex3fv( tess.xyz[ i ] );
				glVertex3fv( shadowXyz[ i ] );
				glVertex3fv( tess.xyz[ i2 ] );
				glVertex3fv( shadowXyz[ i2 ] );
			glEnd();
#else
			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (hit[1] != 1)
			{
				glBegin( GL_TRIANGLE_STRIP );
				glVertex3fv( tess.xyz[ i ] );
				glVertex3fv( shadowXyz[ i ] );
				glVertex3fv( tess.xyz[ i2 ] );
				glVertex3fv( shadowXyz[ i2 ] );
				glEnd();
				c_edges++;
			} else {
				c_rejected++;
			}
#endif
		}
	}

#ifdef _STENCIL_REVERSE
	//Carmack Reverse<tm> method requires that volumes
	//be capped properly -rww
	numTris = tess.numIndexes / 3;

	for ( i = 0 ; i < numTris ; i++ )
	{
		if ( !facing[i] )
		{
			continue;
		}

		o1 = tess.indexes[ i*3 + 0 ];
		o2 = tess.indexes[ i*3 + 1 ];
		o3 = tess.indexes[ i*3 + 2 ];

		glBegin(GL_TRIANGLES);
			glVertex3fv(tess.xyz[o1]);
			glVertex3fv(tess.xyz[o2]);
			glVertex3fv(tess.xyz[o3]);
		glEnd();
		glBegin(GL_TRIANGLES);
			glVertex3fv(shadowXyz[o3]);
			glVertex3fv(shadowXyz[o2]);
			glVertex3fv(shadowXyz[o1]);
		glEnd();
	}
#endif
}

//#define _DEBUG_STENCIL_SHADOWS

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_DoShadowTessEnd( vec3_t lightPos );
void RB_ShadowTessEnd( void )
{
#if 0
	if (backEnd.currentEntity &&
		(backEnd.currentEntity->directedLight[0] ||
			backEnd.currentEntity->directedLight[1] ||
			backEnd.currentEntity->directedLight[2]))
	{ //an ent that has its light set for it
		RB_DoShadowTessEnd(NULL);
		return;
	}

//	if (!tess.dlightBits)
//	{
//		return;
//	}

	int i = 0;
	dlight_t *dl;

	R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
/*	while (i < tr.refdef.num_dlights)
	{
		if (tess.dlightBits & (1 << i))
		{
			dl = &tr.refdef.dlights[i];

			RB_DoShadowTessEnd(dl->transformed);
		}

		i++;
	}
	*/
			dl = &tr.refdef.dlights[0];

			RB_DoShadowTessEnd(dl->transformed);

#else //old ents-only way
	RB_DoShadowTessEnd(NULL);
#endif
}

void RB_DoShadowTessEnd( vec3_t lightPos )
{
	int		i;
	int		numTris;
	vec3_t	lightDir;

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

#if 1 //controlled method - try to keep shadows in range so they don't show through so much -rww
	vec3_t	worldxyz;
	vec3_t	entLight;
	float	groundDist;

	VectorCopy( backEnd.currentEntity->lightDir, entLight );
	entLight[2] = 0.0f;
	VectorNormalize(entLight);

	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presets no chance of screwups and still looks better than a stupid
	//shader blob.
	VectorSet(lightDir, entLight[0]*0.3f, entLight[1]*0.3f, 1.0f);
	// project vertexes away from light direction
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		//add or.origin to vert xyz to end up with world oriented coord, then figure
		//out the ground pos for the vert to project the shadow volume to
		VectorAdd(tess.xyz[i], backEnd.ori.origin, worldxyz);
		groundDist = worldxyz[2] - backEnd.currentEntity->e.shadowPlane;
		groundDist += 16.0f; //fudge factor
		VectorMA( tess.xyz[i], -groundDist, lightDir, shadowXyz[i] );
	}
#else
	if (lightPos)
	{
		for ( i = 0 ; i < tess.numVertexes ; i++ )
		{
			shadowXyz[i][0] = tess.xyz[i][0]+(( tess.xyz[i][0]-lightPos[0] )*128.0f);
			shadowXyz[i][1] = tess.xyz[i][1]+(( tess.xyz[i][1]-lightPos[1] )*128.0f);
			shadowXyz[i][2] = tess.xyz[i][2]+(( tess.xyz[i][2]-lightPos[2] )*128.0f);
		}
	}
	else
	{
		VectorCopy( backEnd.currentEntity->lightDir, lightDir );

		// project vertexes away from light direction
		for ( i = 0 ; i < tess.numVertexes ; i++ ) {
			VectorMA( tess.xyz[i], -512, lightDir, shadowXyz[i] );
		}
	}
#endif
	// decide which triangles face the light
	memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) {
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		if (!lightPos)
		{
			VectorSubtract( v2, v1, d1 );
			VectorSubtract( v3, v1, d2 );
			CrossProduct( d1, d2, normal );

			d = DotProduct( normal, lightDir );
		}
		else
		{
			float planeEq[4];
			planeEq[0] = v1[1]*(v2[2]-v3[2]) + v2[1]*(v3[2]-v1[2]) + v3[1]*(v1[2]-v2[2]);
			planeEq[1] = v1[2]*(v2[0]-v3[0]) + v2[2]*(v3[0]-v1[0]) + v3[2]*(v1[0]-v2[0]);
			planeEq[2] = v1[0]*(v2[1]-v3[1]) + v2[0]*(v3[1]-v1[1]) + v3[0]*(v1[1]-v2[1]);
			planeEq[3] = -( v1[0]*( v2[1]*v3[2] - v3[1]*v2[2] ) +
						v2[0]*(v3[1]*v1[2] - v1[1]*v3[2]) +
						v3[0]*(v1[1]*v2[2] - v2[1]*v1[2]) );

			d = planeEq[0]*lightPos[0]+
				planeEq[1]*lightPos[1]+
				planeEq[2]*lightPos[2]+
				planeEq[3];
		}

		if ( d > 0 ) {
			facing[ i ] = 1;
		} else {
			facing[ i ] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	GL_Bind( tr.whiteImage );
	//glEnable( GL_CULL_FACE );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

#ifndef _DEBUG_STENCIL_SHADOWS
	glColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	glEnable( GL_STENCIL_TEST );
	glStencilFunc( GL_ALWAYS, 1, 255 );
#else
	glColor3f( 1.0f, 0.0f, 0.0f );
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDisable(GL_DEPTH_TEST);
#endif

#ifdef _STENCIL_REVERSE
	glDepthFunc(GL_LESS);

	//now using the Carmack Reverse<tm> -rww
	if ( backEnd.viewParms.isMirror ) {
		//glCullFace( GL_BACK );
		GL_Cull(CT_BACK_SIDED);
		glStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

		R_RenderShadowEdges();

		//glCullFace( GL_FRONT );
		GL_Cull(CT_FRONT_SIDED);
		glStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

		R_RenderShadowEdges();
	} else {
		//glCullFace( GL_FRONT );
		GL_Cull(CT_FRONT_SIDED);
		glStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

		R_RenderShadowEdges();

		//glCullFace( GL_BACK );
		GL_Cull(CT_BACK_SIDED);
		glStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

		R_RenderShadowEdges();
	}

	glDepthFunc(GL_LEQUAL);
#else
	// mirrors have the culling order reversed
	if ( backEnd.viewParms.isMirror ) {
		glCullFace( GL_FRONT );
		glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

		R_RenderShadowEdges();

		glCullFace( GL_BACK );
		glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

		R_RenderShadowEdges();
	} else {
		glCullFace( GL_BACK );
		glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

		R_RenderShadowEdges();

		glCullFace( GL_FRONT );
		glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

		R_RenderShadowEdges();
	}
#endif

	// reenable writing to the color buffer
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

#ifdef _DEBUG_STENCIL_SHADOWS
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void ) {
	if ( r_shadows->integer != 2 ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}

#ifdef _DEBUG_STENCIL_SHADOWS
	return;
#endif

	glEnable( GL_STENCIL_TEST );
	glStencilFunc( GL_NOTEQUAL, 0, 255 );

	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	bool planeZeroBack = false;
	if (glIsEnabled(GL_CLIP_PLANE0))
	{
		planeZeroBack = true;
		glDisable (GL_CLIP_PLANE0);
	}
	GL_Cull(CT_TWO_SIDED);
	//glDisable (GL_CULL_FACE);

	GL_Bind( tr.whiteImage );

	glPushMatrix();
    glLoadIdentity ();

//	glColor3f( 0.6f, 0.6f, 0.6f );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	glColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
	//GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	glBegin( GL_QUADS );
	glVertex3f( -100, 100, -10 );
	glVertex3f( 100, 100, -10 );
	glVertex3f( 100, -100, -10 );
	glVertex3f( -100, -100, -10 );
	glEnd ();

	glColor4f(1,1,1,1);
	glDisable( GL_STENCIL_TEST );
	if (planeZeroBack)
	{
		glEnable (GL_CLIP_PLANE0);
	}
	glPopMatrix();
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void ) {
	float	*xyz;
	int		i;
	float	h;
	vec3_t	ground;
	vec3_t	light;
	float	groundDist;
	float	d;
	vec3_t	lightDir;

	xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	groundDist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}

//update tr.screenImage
void RB_CaptureScreenImage(void)
{
	int radX = 2048;
	int radY = 2048;
	int x = glConfig.vidWidth/2;
	int y = glConfig.vidHeight/2;
	int cX, cY;

	GL_Bind( tr.screenImage );
	//using this method, we could pixel-filter the texture and all sorts of crazy stuff.
	//but, it is slow as hell.
	/*
	static byte *tmp = NULL;
	if (!tmp)
	{
		tmp = (byte *)Z_Malloc((sizeof(byte)*4)*(glConfig.vidWidth*glConfig.vidHeight), TAG_ICARUS, qtrue);
	}
	glReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	*/

	if (radX > glConfig.maxTextureSize)
	{
		radX = glConfig.maxTextureSize;
	}
	if (radY > glConfig.maxTextureSize)
	{
		radY = glConfig.maxTextureSize;
	}

	while (glConfig.vidWidth < radX)
	{
		radX /= 2;
	}
	while (glConfig.vidHeight < radY)
	{
		radY /= 2;
	}

	cX = x-(radX/2);
	cY = y-(radY/2);

	if (cX+radX > glConfig.vidWidth)
	{ //would it go off screen?
		cX = glConfig.vidWidth-radX;
	}
	else if (cX < 0)
	{ //cap it off at 0
		cX = 0;
	}

	if (cY+radY > glConfig.vidHeight)
	{ //would it go off screen?
		cY = glConfig.vidHeight-radY;
	}
	else if (cY < 0)
	{ //cap it off at 0
		cY = 0;
	}

	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, cX, cY, radX, radY, 0);
}

//yeah.. not really shadow-related.. but it's stencil-related. -rww
float tr_distortionAlpha = 1.0f; //opaque
float tr_distortionStretch = 0.0f; //no stretch override
qboolean tr_distortionPrePost = qfalse; //capture before postrender phase?
qboolean tr_distortionNegate = qfalse; //negative blend mode
void RB_DistortionFill(void)
{
	float alpha = tr_distortionAlpha;
	float spost = 0.0f;
	float spost2 = 0.0f;

	if ( glConfig.stencilBits < 4 )
	{
		return;
	}

	//ok, cap the stupid thing now I guess
	if (!tr_distortionPrePost)
	{
		RB_CaptureScreenImage();
	}

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	glDisable (GL_CLIP_PLANE0);
	GL_Cull( CT_TWO_SIDED );

	//reset the view matrices and go into ortho mode
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 32, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	if (tr_distortionStretch)
	{ //override
		spost = tr_distortionStretch;
		spost2 = tr_distortionStretch;
	}
	else
	{ //do slow stretchy effect
		spost = sin(tr.refdef.time*0.0005f);
		if (spost < 0.0f)
		{
			spost = -spost;
		}
		spost *= 0.2f;

		spost2 = sin(tr.refdef.time*0.0005f);
		if (spost2 < 0.0f)
		{
			spost2 = -spost2;
		}
		spost2 *= 0.08f;
	}

	if (alpha != 1.0f)
	{ //blend
		GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA);
	}
	else
	{ //be sure to reset the draw state
		GL_State(0);
	}

	glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glTexCoord2f(0+spost2, 1-spost);
		glVertex2f(0, 0);

		glTexCoord2f(0+spost2, 0+spost);
		glVertex2f(0, glConfig.vidHeight);

		glTexCoord2f(1-spost2, 0+spost);
		glVertex2f(glConfig.vidWidth, glConfig.vidHeight);

		glTexCoord2f(1-spost2, 1-spost);
		glVertex2f(glConfig.vidWidth, 0);
	glEnd();

	if (tr_distortionAlpha == 1.0f && tr_distortionStretch == 0.0f)
	{ //no overrides
		if (tr_distortionNegate)
		{ //probably the crazy alternate saber trail
			alpha = 0.8f;
			GL_State(GLS_SRCBLEND_ZERO|GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
		}
		else
		{
			alpha = 0.5f;
			GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA);
		}

		spost = sin(tr.refdef.time*0.0008f);
		if (spost < 0.0f)
		{
			spost = -spost;
		}
		spost *= 0.08f;

		spost2 = sin(tr.refdef.time*0.0008f);
		if (spost2 < 0.0f)
		{
			spost2 = -spost2;
		}
		spost2 *= 0.2f;

		glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glTexCoord2f(0+spost2, 1-spost);
			glVertex2f(0, 0);

			glTexCoord2f(0+spost2, 0+spost);
			glVertex2f(0, glConfig.vidHeight);

			glTexCoord2f(1-spost2, 0+spost);
			glVertex2f(glConfig.vidWidth, glConfig.vidHeight);

			glTexCoord2f(1-spost2, 1-spost);
			glVertex2f(glConfig.vidWidth, 0);
		glEnd();
	}

	//pop the view matrices back
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable( GL_STENCIL_TEST );
}

