#include "tr_local.hh"
#include "ghoul2/g2_public.hh"

bool HackadelicOnClient=false; // means this is a render traversal

qboolean G2_HackadelicOnClient() {
	return HackadelicOnClient;
}

qhandle_t		goreShader=-1;

class CRenderSurface
{
public:
	int				surfaceNum;
	surfaceInfo_v	&rootSList;
	shader_t		*cust_shader;
	int				fogNum;
	qboolean		personalModel;
	CBoneCache		*boneCache;
	int				renderfx;
	skin_t			*skin;
	model_t			*currentModel;
	int				lod;
	boltInfo_v		&boltList;
#ifdef _G2_GORE
	shader_t		*gore_shader;
	CGoreSet		*gore_set;
#endif

	CRenderSurface(
	int				initsurfaceNum,
	surfaceInfo_v	&initrootSList,
	shader_t		*initcust_shader,
	int				initfogNum,
	qboolean		initpersonalModel,
	CBoneCache		*initboneCache,
	int				initrenderfx,
	skin_t			*initskin,
	model_t			*initcurrentModel,
	int				initlod,
#ifdef _G2_GORE
	boltInfo_v		&initboltList,
	shader_t		*initgore_shader,
	CGoreSet		*initgore_set):
#else
	boltInfo_v		&initboltList):
#endif

	surfaceNum(initsurfaceNum),
	rootSList(initrootSList),
	cust_shader(initcust_shader),
	fogNum(initfogNum),
	personalModel(initpersonalModel),
	boneCache(initboneCache),
	renderfx(initrenderfx),
	skin(initskin),
	currentModel(initcurrentModel),
	lod(initlod),
#ifdef _G2_GORE
	boltList(initboltList),
	gore_shader(initgore_shader),
	gore_set(initgore_set)
#else
	boltList(initboltList)
#endif
	{}
};

#ifdef _G2_GORE
#define MAX_RENDER_SURFACES (2048)
static CRenderableSurface RSStorage[MAX_RENDER_SURFACES];
static unsigned int NextRS=0;

CRenderableSurface *AllocRS()
{
	CRenderableSurface *ret=&RSStorage[NextRS];
	ret->Init();
	NextRS++;
	NextRS%=MAX_RENDER_SURFACES;
	return ret;
}
#endif


/*
=============
R_ACullModel
=============
*/
static int R_GCullModel( trRefEntity_t *ent ) {

	// scale the radius if need be
	float largestScale = ent->e.modelScale[0];

	if (ent->e.modelScale[1] > largestScale)
	{
		largestScale = ent->e.modelScale[1];
	}
	if (ent->e.modelScale[2] > largestScale)
	{
		largestScale = ent->e.modelScale[2];
	}
	if (!largestScale)
	{
		largestScale = 1;
	}

	// cull bounding sphere
  	switch ( R_CullLocalPointAndRadius( vec3_origin,  ent->e.radius * largestScale) )
  	{
  	case CULL_OUT:
  		tr.pc.c_sphere_cull_md3_out++;
  		return CULL_OUT;

	case CULL_IN:
		tr.pc.c_sphere_cull_md3_in++;
		return CULL_IN;

	case CULL_CLIP:
		tr.pc.c_sphere_cull_md3_clip++;
		return CULL_IN;
 	}
	return CULL_IN;
}


/*
=================
R_AComputeFogNum

=================
*/
static int R_GComputeFogNum( trRefEntity_t *ent ) {

	int				i, j;
	fog_t			*fog;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

// work out lod for this entity.
static int G2_ComputeLOD( trRefEntity_t *ent, const model_t *currentModel, int lodBias )
{
	float flod, lodscale;
	float projectedRadius;
	int lod;

	if ( currentModel->numLods < 2 )
	{	// model has only 1 LOD level, skip computations and bias
		return(0);
	}

	if ( r_lodbias->integer > lodBias )
	{
		lodBias = r_lodbias->integer;
	}

	// scale the radius if need be
	float largestScale = ent->e.modelScale[0];

	if (ent->e.modelScale[1] > largestScale)
	{
		largestScale = ent->e.modelScale[1];
	}
	if (ent->e.modelScale[2] > largestScale)
	{
		largestScale = ent->e.modelScale[2];
	}
	if (!largestScale)
	{
		largestScale = 1;
	}

	if ( ( projectedRadius = ProjectRadius( 0.75*largestScale*ent->e.radius, ent->e.origin ) ) != 0 )	//we reduce the radius to make the LOD match other model types which use the actual bound box size
 	{
 		lodscale = (r_lodscale->value+r_autolodscalevalue->value);
 		if ( lodscale > 20 )
		{
			lodscale = 20;
		}
		else if ( lodscale < 0 )
		{
			lodscale = 0;
		}
 		flod = 1.0f - projectedRadius * lodscale;
 	}
 	else
 	{
 		// object intersects near view plane, e.g. view weapon
 		flod = 0;
 	}
 	flod *= currentModel->numLods;
	lod = Q_ftol( flod );

 	if ( lod < 0 )
 	{
 		lod = 0;
 	}
 	else if ( lod >= currentModel->numLods )
 	{
 		lod = currentModel->numLods - 1;
 	}


	lod += lodBias;

	if ( lod >= currentModel->numLods )
		lod = currentModel->numLods - 1;
	if ( lod < 0 )
		lod = 0;

	return lod;
}


void RenderSurfaces(CRenderSurface &RS) //also ended up just ripping right from SP.
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_RenderSurfaces.Start();
#endif
	int			i;
	const shader_t	*shader = 0;
	int			offFlags = 0;
#ifdef _G2_GORE
	bool		drawGore = true;
#endif

	assert(RS.currentModel);
	assert(RS.currentModel->mdxm);
	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = (mdxmSurface_t *)ri.G2_FindSurface(RS.currentModel, RS.surfaceNum, RS.lod);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)RS.currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = ri.G2_FindOverrideSurface(RS.surfaceNum, RS.rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, add it to the shader render list
	if (!offFlags)
	{
 		if ( RS.cust_shader )
		{
			shader = RS.cust_shader;
		}
		else if ( RS.skin )
		{
			int		j;

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < RS.skin->numSurfaces ; j++ )
			{
				// the names have both been lowercased
				if ( !strcmp( RS.skin->surfaces[j]->name, surfInfo->name ) )
				{
					shader = (shader_t*)RS.skin->surfaces[j]->shader;
					break;
				}
			}
		}
		else
		{
			shader = R_GetShaderByHandle( surfInfo->shaderIndex );
		}

		//rww - catch surfaces with bad shaders
		//assert(shader != tr.defaultShader);
		//Alright, this is starting to annoy me because of the state of the assets. Disabling for now.
		// we will add shadows even if the main object isn't visible in the view
		// stencil shadows can't do personal models unless I polyhedron clip
		//using z-fail now so can do personal models -rww
		if ( /*!RS.personalModel
			&& */r_shadows->integer == 2
//			&& RS.fogNum == 0
			&& (RS.renderfx & RF_SHADOW_PLANE )
			&& !(RS.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			&& shader->sort == SS_OPAQUE )
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			if (surface->numVerts >= SHADER_MAX_VERTEXES/2)
			{ //we need numVerts*2 xyz slots free in tess to do shadow, if this surf is going to exceed that then let's try the lowest lod -rww
				mdxmSurface_t *lowsurface = (mdxmSurface_t *)ri.G2_FindSurface(RS.currentModel, RS.surfaceNum, RS.currentModel->numLods-1);
				newSurf->surfaceData = lowsurface;
			}
			else
			{
				newSurf->surfaceData = surface;
			}
			newSurf->boneCache = RS.boneCache;
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
//			&& RS.fogNum == 0
			&& (RS.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE )
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
			R_AddDrawSurf( (surfaceType_t *)newSurf, tr.projectionShadowShader, 0, qfalse );
		}

		// don't add third_person objects if not viewing through a portal
		if ( !RS.personalModel )
		{		// set the surface info to point at the where the transformed bone list is going to be for when the surface gets rendered out
			CRenderableSurface *newSurf = new CRenderableSurface;
			newSurf->surfaceData = surface;
			newSurf->boneCache = RS.boneCache;
			R_AddDrawSurf( (surfaceType_t *)newSurf, (shader_t *)shader, RS.fogNum, qfalse );

#ifdef _G2_GORE
			if (RS.gore_set && drawGore)
			{
				int curTime = ri.G2API_GetTime(tr.refdef.time);
				std::pair<std::multimap<int,SGoreSurface>::iterator,std::multimap<int,SGoreSurface>::iterator> range=
					RS.gore_set->mGoreRecords.equal_range(RS.surfaceNum);
				std::multimap<int,SGoreSurface>::iterator k,kcur;
				CRenderableSurface *last=newSurf;
				for (k=range.first;k!=range.second;)
				{
					kcur=k;
					++k;
					GoreTextureCoordinates *tex=ri.G2API_FindGoreRecord((*kcur).second.mGoreTag);
					if (!tex ||											 // it is gone, lets get rid of it
						(kcur->second.mDeleteTime && curTime>=kcur->second.mDeleteTime)) // out of time
					{
						ri.G2API_DeleteGoreTextureCoords(tex);
						RS.gore_set->mGoreRecords.erase(kcur);
					}
					else if (tex->tex[RS.lod])
					{
						CRenderableSurface *newSurf2 = AllocRS();
						*newSurf2=*newSurf;
						newSurf2->goreChain=0;
						newSurf2->alternateTex=tex->tex[RS.lod];
						newSurf2->scale=1.0f;
						newSurf2->fade=1.0f;
						newSurf2->impactTime=1.0f;	// done with
						int magicFactor42=500; // ms, impact time
						if (curTime>(*kcur).second.mGoreGrowStartTime && curTime<(*kcur).second.mGoreGrowStartTime+magicFactor42)
						{
							newSurf2->impactTime=float(curTime-(*kcur).second.mGoreGrowStartTime)/float(magicFactor42);  // linear
						}
						if (curTime<(*kcur).second.mGoreGrowEndTime)
						{
							newSurf2->scale=1.0f/((curTime-(*kcur).second.mGoreGrowStartTime)*(*kcur).second.mGoreGrowFactor + (*kcur).second.mGoreGrowOffset);
							if (newSurf2->scale<1.0f)
							{
								newSurf2->scale=1.0f;
							}
						}
						shader_t *gshader;
						if ((*kcur).second.shader)
						{
 							gshader=R_GetShaderByHandle((*kcur).second.shader);
						}
						else
						{
							gshader=R_GetShaderByHandle(goreShader);
						}

						// Set fade on surf.
						//Only if we have a fade time set, and let us fade on rgb if we want -rww
						if ((*kcur).second.mDeleteTime && (*kcur).second.mFadeTime)
						{
							if ((*kcur).second.mDeleteTime - curTime < (*kcur).second.mFadeTime)
							{
								newSurf2->fade=(float)((*kcur).second.mDeleteTime - curTime)/(*kcur).second.mFadeTime;
								if ((*kcur).second.mFadeRGB)
								{ //RGB fades are scaled from 2.0f to 3.0f (simply to differentiate)
									newSurf2->fade += 2.0f;

									if (newSurf2->fade < 2.01f)
									{
										newSurf2->fade = 2.01f;
									}
								}
							}
						}

						last->goreChain=newSurf2;
						last=newSurf2;
						R_AddDrawSurf( (surfaceType_t *)newSurf2,gshader, RS.fogNum, qfalse );
					}
				}
			}
#endif
		}
	}

	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		RS.surfaceNum = surfInfo->childIndexes[i];
		RenderSurfaces(RS);
	}

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_RenderSurfaces += G2PerformanceTimer_RenderSurfaces.End();
#endif
}

extern cvar_t	*r_shadowRange;
static inline bool bInShadowRange(vec3_t location)
{
	const float c = DotProduct( tr.viewParms.ori.axis[0], tr.viewParms.ori.origin );
	const float dist = DotProduct( tr.viewParms.ori.axis[0], location ) - c;

//	return (dist < tr.distanceCull/1.5f);
	return (dist < r_shadowRange->value);
}

/*
==============
R_AddGHOULSurfaces
==============
*/
void R_AddGhoulSurfaces( trRefEntity_t *ent ) {
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_R_AddGHOULSurfaces.Start();
#endif
	shader_t		*cust_shader = 0;
#ifdef _G2_GORE
	shader_t		*gore_shader = 0;
#endif
	int				fogNum = 0;
	qboolean		personalModel;
	int				cull;
	int				i, whichLod, j;
	skin_t			*skin;
	int				modelCount;
	mdxaBone_t		rootMatrix;
	CGhoul2Info_v	&ghoul2 = *((CGhoul2Info_v *)ent->e.ghoul2);

	if ( !ri.G2_IsValid(ghoul2) )
	{
		return;
	}
	// if we don't want server ghoul2 models and this is one, or we just don't want ghoul2 models at all, then return
	if (!ri.G2_SetupModelPointers(ghoul2))
	{
		return;
	}

	int currentTime=ri.G2API_GetTime(tr.refdef.time);


	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	cull = R_GCullModel (ent );
	if ( cull == CULL_OUT )
	{
		return;
	}
	HackadelicOnClient=true;
	// are any of these models setting a new origin?
	ri.G2_RootMatrix(ghoul2,currentTime, ent->e.modelScale,rootMatrix);

   	// don't add third_person objects if not in a portal
	personalModel = (qboolean)((ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal);

	int modelList[256];
	assert(ri.G2_Size(ghoul2)<=255);
	modelList[255]=548;

	// set up lighting now that we know we aren't culled
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	// see if we are in a fog volume
	fogNum = R_GComputeFogNum( ent );

	// order sort the ghoul 2 models so bolt ons get bolted to the right model
	ri.G2_Sort_Models(ghoul2, modelList, &modelCount);
	assert(modelList[255]==548);

#ifdef _G2_GORE
	if (goreShader == -1)
	{
		goreShader=RE_RegisterShader("gfx/damage/burnmark1");
	}
#endif

	// construct a world matrix for this entity
	ri.G2_GenerateWorldMatrix(ent->e.angles, ent->e.origin);

	// walk each possible model for this entity and try rendering it out
	for (j=0; j<modelCount; j++)
	{
		i = modelList[j];
		if (ri.G2_At(ghoul2, i).mValid&&!(ri.G2_At(ghoul2, i).mFlags & GHOUL2_NOMODEL)&&!(ri.G2_At(ghoul2, i).mFlags & GHOUL2_NORENDER))
		{
			//
			// figure out whether we should be using a custom shader for this model
			//
			skin = NULL;
			if (ent->e.customShader)
			{
				cust_shader = R_GetShaderByHandle(ent->e.customShader );
			}
			else
			{
				cust_shader = NULL;
				// figure out the custom skin thing
				if (ri.G2_At(ghoul2, i).mCustomSkin)
				{
					skin = R_GetSkinByHandle(ri.G2_At(ghoul2, i).mCustomSkin );
				}
				else if (ent->e.customSkin)
				{
					skin = R_GetSkinByHandle(ent->e.customSkin );
				}
				else if ( ri.G2_At(ghoul2, i).mSkin > 0 && ri.G2_At(ghoul2, i).mSkin < tr.numSkins )
				{
					skin = R_GetSkinByHandle( ri.G2_At(ghoul2, i).mSkin );
				}
			}

			if (j&&ri.G2_At(ghoul2, i).mModelBoltLink != -1)
			{
				int	boltMod = (ri.G2_At(ghoul2, i).mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				int	boltNum = (ri.G2_At(ghoul2, i).mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;
				mdxaBone_t bolt;
				ri.G2_GetBoltMatrixLow(ri.G2_At(ghoul2, boltMod),boltNum,ent->e.modelScale,bolt);
				ri.G2_TransformGhoulBones(ri.G2_At(ghoul2, i).mBlist,bolt, ri.G2_At(ghoul2, i),currentTime, true);
			}
			else
			{
				ri.G2_TransformGhoulBones(ri.G2_At(ghoul2, i).mBlist, rootMatrix, ri.G2_At(ghoul2, i),currentTime, true);
			}
			whichLod = G2_ComputeLOD( ent, ri.G2_At(ghoul2, i).currentModel, ri.G2_At(ghoul2, i).mLodBias );
			ri.G2_FindOverrideSurface(-1,ri.G2_At(ghoul2, i).mSlist); //reset the quick surface override lookup;

#ifdef _G2_GORE
			CGoreSet *gore=0;
			if (ri.G2_At(ghoul2, i).mGoreSetTag)
			{
				gore=ri.G2_FindGoreSet(ri.G2_At(ghoul2, i).mGoreSetTag);
				if (!gore) // my gore is gone, so remove it
				{
					ri.G2_At(ghoul2, i).mGoreSetTag=0;
				}
			}

			CRenderSurface RS(ri.G2_At(ghoul2, i).mSurfaceRoot, ri.G2_At(ghoul2, i).mSlist, cust_shader, fogNum, personalModel, ri.G2_At(ghoul2, i).mBoneCache, ent->e.renderfx, skin, (model_t *)ri.G2_At(ghoul2, i).currentModel, whichLod, ri.G2_At(ghoul2, i).mBltlist, gore_shader, gore);
#else
			CRenderSurface RS(ri.G2_At(ghoul2, i).mSurfaceRoot, ri.G2_At(ghoul2, i).mSlist, cust_shader, fogNum, personalModel, ri.G2_At(ghoul2, i).mBoneCache, ent->e.renderfx, skin, (model_t *)ri.G2_At(ghoul2, i).currentModel, whichLod, ri.G2_At(ghoul2, i).mBltlist);
#endif
			if (!personalModel && (RS.renderfx & RF_SHADOW_PLANE) && !bInShadowRange(ent->e.origin))
			{
				RS.renderfx |= RF_NOSHADOW;
			}
			RenderSurfaces(RS);
		}
	}
	HackadelicOnClient=false;

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_R_AddGHOULSurfaces += G2PerformanceTimer_R_AddGHOULSurfaces.End();
#endif
}

static inline float G2_GetVertBoneWeightNotSlow( const mdxmVertex_t *pVert, const int iWeightNum)
{
	float fBoneWeight;

	int iTemp = pVert->BoneWeightings[iWeightNum];

	iTemp|= (pVert->uiNmWeightsAndBoneIndexes >> (iG2_BONEWEIGHT_TOPBITS_SHIFT+(iWeightNum*2)) ) & iG2_BONEWEIGHT_TOPBITS_AND;

	fBoneWeight = fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp;

	return fBoneWeight;
}

//This is a slightly mangled version of the same function from the sof2sp base.
//It provides a pretty significant performance increase over the existing one.
void RB_SurfaceGhoul( CRenderableSurface *surf )
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_RB_SurfaceGhoul.Start();
#endif

	static int				j, k;
	static int				baseIndex, baseVertex;
	static int				numVerts;
	static mdxmVertex_t 	*v;
	static int				*triangles;
	static int				indexes;
	static glIndex_t		*tessIndexes;
	static mdxmVertexTexCoord_t *pTexCoords;
	static int				*piBoneReferences;

#ifdef _G2_GORE
	if (surf->alternateTex)
	{
		// a gore surface ready to go.

		/*
			sizeof(int)+ // num verts
			sizeof(int)+ // num tris
			sizeof(int)*newNumVerts+ // which verts to copy from original surface
			sizeof(float)*4*newNumVerts+ // storgage for deformed verts
			sizeof(float)*4*newNumVerts+ // storgage for deformed normal
			sizeof(float)*2*newNumVerts+ // texture coordinates
			sizeof(int)*newNumTris*3;  // new indecies
		*/

		int *data=(int *)surf->alternateTex;
		numVerts=*data++;
		indexes=(*data++);
		// first up, sanity check our numbers
		RB_CheckOverflow(numVerts,indexes);
		indexes*=3;

		data+=numVerts;

		baseIndex = tess.numIndexes;
		baseVertex = tess.numVertexes;

		memcpy(&tess.xyz[baseVertex][0],data,sizeof(float)*4*numVerts);
		data+=4*numVerts;
		memcpy(&tess.normal[baseVertex][0],data,sizeof(float)*4*numVerts);
		data+=4*numVerts;
		assert(numVerts>0);

		//float *texCoords = tess.texCoords[0][baseVertex];
		float *texCoords = tess.texCoords[baseVertex][0];
		int hack = baseVertex;
		//rww - since the array is arranged as such we cannot increment
		//the relative memory position to get where we want. Maybe this
		//is why sof2 has the texCoords array reversed. In any case, I
		//am currently too lazy to get around it.
		//Or can you += array[.][x]+2?
		if (surf->scale>1.0f)
		{
			for ( j = 0; j < numVerts; j++)
			{
				texCoords[0]=((*(float *)data)-0.5f)*surf->scale+0.5f;
				data++;
				texCoords[1]=((*(float *)data)-0.5f)*surf->scale+0.5f;
				data++;
				//texCoords+=2;// Size of gore (s,t).
				hack++;
				texCoords = tess.texCoords[hack][0];
			}
		}
		else
		{
			for (j=0;j<numVerts;j++)
			{
				texCoords[0]=*(float *)(data++);
				texCoords[1]=*(float *)(data++);
//				texCoords+=2;// Size of gore (s,t).
				hack++;
				texCoords = tess.texCoords[hack][0];
			}
		}

		//now check for fade overrides -rww
		if (surf->fade)
		{
			static int lFade;
			static int j;

			if (surf->fade<1.0)
			{
				tess.fading = true;
				lFade = Q_ftol(254.4f*surf->fade);

				for (j=0;j<numVerts;j++)
				{
					tess.svars.colors[j+baseVertex][3] = lFade;
				}
			}
			else if (surf->fade > 2.0f && surf->fade < 3.0f)
			{ //hack to fade out on RGB if desired (don't want to add more to CRenderableSurface) -rww
				tess.fading = true;
				lFade = Q_ftol(254.4f*(surf->fade-2.0f));

				for (j=0;j<numVerts;j++)
				{
					if (lFade < tess.svars.colors[j+baseVertex][0])
					{ //don't set it unless the fade is less than the current r value (to avoid brightening suddenly before we start fading)
						tess.svars.colors[j+baseVertex][0] = tess.svars.colors[j+baseVertex][1] = tess.svars.colors[j+baseVertex][2] = lFade;
					}

					//Set the alpha as well I suppose, no matter what
					tess.svars.colors[j+baseVertex][3] = lFade;
				}
			}
		}

		glIndex_t *indexPtr = &tess.indexes[baseIndex];
		triangles = data;
		for (j = indexes ; j ; j--)
		{
			*indexPtr++ = baseVertex + (*triangles++);
		}
		tess.numIndexes += indexes;
		tess.numVertexes += numVerts;
		return;
	}
#endif

	// grab the pointer to the surface info within the loaded mesh file
	mdxmSurface_t	*surface = surf->surfaceData;

	CBoneCache *bones = surf->boneCache;

#ifndef _G2_GORE //we use this later, for gore
	delete surf;
#endif

	// first up, sanity check our numbers
	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	//
	// deform the vertexes by the lerped bones
	//

	// first up, sanity check our numbers
	baseVertex = tess.numVertexes;
	triangles = (int *) ((byte *)surface + surface->ofsTriangles);
	baseIndex = tess.numIndexes;
#if 0
	indexes = surface->numTriangles * 3;
	for (j = 0 ; j < indexes ; j++) {
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;
#else
	indexes = surface->numTriangles; //*3;	//unrolled 3 times, don't multiply
	tessIndexes = &tess.indexes[baseIndex];
	for (j = 0 ; j < indexes ; j++) {
		*tessIndexes++ = baseVertex + *triangles++;
		*tessIndexes++ = baseVertex + *triangles++;
		*tessIndexes++ = baseVertex + *triangles++;
	}
	tess.numIndexes += indexes*3;
#endif

	numVerts = surface->numVerts;

	piBoneReferences = (int*) ((byte*)surface + surface->ofsBoneReferences);
	baseVertex = tess.numVertexes;
	v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
	pTexCoords = (mdxmVertexTexCoord_t *) &v[numVerts];

//	if (r_ghoul2fastnormals&&r_ghoul2fastnormals->integer==0)
#if 0
	if (0)
	{
		for ( j = 0; j < numVerts; j++, baseVertex++,v++ )
		{
			const int iNumWeights = G2_GetVertWeights( v );

			float fTotalWeight = 0.0f;

			k=0;
			int		iBoneIndex = G2_GetVertBoneIndex( v, k );
			float	fBoneWeight = G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );
			const mdxaBone_t *bone = &bones->EvalRender(piBoneReferences[iBoneIndex]);

			tess.xyz[baseVertex][0] = fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
			tess.xyz[baseVertex][1] = fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
			tess.xyz[baseVertex][2] = fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );

			tess.normal[baseVertex][0] = fBoneWeight * DotProduct( bone->matrix[0], v->normal );
			tess.normal[baseVertex][1] = fBoneWeight * DotProduct( bone->matrix[1], v->normal );
			tess.normal[baseVertex][2] = fBoneWeight * DotProduct( bone->matrix[2], v->normal );

			for ( k++ ; k < iNumWeights ; k++)
			{
				iBoneIndex	= G2_GetVertBoneIndex( v, k );
				fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

				bone = &bones->EvalRender(piBoneReferences[iBoneIndex]);

				tess.xyz[baseVertex][0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
				tess.xyz[baseVertex][1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
				tess.xyz[baseVertex][2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );

				tess.normal[baseVertex][0] += fBoneWeight * DotProduct( bone->matrix[0], v->normal );
				tess.normal[baseVertex][1] += fBoneWeight * DotProduct( bone->matrix[1], v->normal );
				tess.normal[baseVertex][2] += fBoneWeight * DotProduct( bone->matrix[2], v->normal );
			}

			tess.texCoords[baseVertex][0][0] = pTexCoords[j].texCoords[0];
			tess.texCoords[baseVertex][0][1] = pTexCoords[j].texCoords[1];
		}
	}
	else
	{
#endif
		float fTotalWeight;
		float fBoneWeight;
		float t1;
		float t2;
		const mdxaBone_t *bone;
		const mdxaBone_t *bone2;
		for ( j = 0; j < numVerts; j++, baseVertex++,v++ )
		{

			bone = &bones->EvalRender(piBoneReferences[G2_GetVertBoneIndex( v, 0 )]);
			int iNumWeights = G2_GetVertWeights( v );
			tess.normal[baseVertex][0] = DotProduct( bone->matrix[0], v->normal );
			tess.normal[baseVertex][1] = DotProduct( bone->matrix[1], v->normal );
			tess.normal[baseVertex][2] = DotProduct( bone->matrix[2], v->normal );

			if (iNumWeights==1)
			{
				tess.xyz[baseVertex][0] = ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
				tess.xyz[baseVertex][1] = ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
				tess.xyz[baseVertex][2] = ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );
			}
			else
			{
				fBoneWeight = G2_GetVertBoneWeightNotSlow( v, 0);
				if (iNumWeights==2)
				{
					bone2 = &bones->EvalRender(piBoneReferences[G2_GetVertBoneIndex( v, 1 )]);
					/*
					useless transposition
					tess.xyz[baseVertex][0] =
					v[0]*(w*(bone->matrix[0][0]-bone2->matrix[0][0])+bone2->matrix[0][0])+
					v[1]*(w*(bone->matrix[0][1]-bone2->matrix[0][1])+bone2->matrix[0][1])+
					v[2]*(w*(bone->matrix[0][2]-bone2->matrix[0][2])+bone2->matrix[0][2])+
					w*(bone->matrix[0][3]-bone2->matrix[0][3]) + bone2->matrix[0][3];
					*/
					t1 = ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
					t2 = ( DotProduct( bone2->matrix[0], v->vertCoords ) + bone2->matrix[0][3] );
					tess.xyz[baseVertex][0] = fBoneWeight * (t1-t2) + t2;
					t1 = ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
					t2 = ( DotProduct( bone2->matrix[1], v->vertCoords ) + bone2->matrix[1][3] );
					tess.xyz[baseVertex][1] = fBoneWeight * (t1-t2) + t2;
					t1 = ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );
					t2 = ( DotProduct( bone2->matrix[2], v->vertCoords ) + bone2->matrix[2][3] );
					tess.xyz[baseVertex][2] = fBoneWeight * (t1-t2) + t2;
				}
				else
				{

					tess.xyz[baseVertex][0] = fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
					tess.xyz[baseVertex][1] = fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
					tess.xyz[baseVertex][2] = fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );

					fTotalWeight=fBoneWeight;
					for (k=1; k < iNumWeights-1 ; k++)
					{
						bone = &bones->EvalRender(piBoneReferences[G2_GetVertBoneIndex( v, k )]);
						fBoneWeight = G2_GetVertBoneWeightNotSlow( v, k);
						fTotalWeight += fBoneWeight;

						tess.xyz[baseVertex][0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
						tess.xyz[baseVertex][1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
						tess.xyz[baseVertex][2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );
					}
					bone = &bones->EvalRender(piBoneReferences[G2_GetVertBoneIndex( v, k )]);
					fBoneWeight	= 1.0f-fTotalWeight;

					tess.xyz[baseVertex][0] += fBoneWeight * ( DotProduct( bone->matrix[0], v->vertCoords ) + bone->matrix[0][3] );
					tess.xyz[baseVertex][1] += fBoneWeight * ( DotProduct( bone->matrix[1], v->vertCoords ) + bone->matrix[1][3] );
					tess.xyz[baseVertex][2] += fBoneWeight * ( DotProduct( bone->matrix[2], v->vertCoords ) + bone->matrix[2][3] );
				}
			}

			tess.texCoords[baseVertex][0][0] = pTexCoords[j].texCoords[0];
			tess.texCoords[baseVertex][0][1] = pTexCoords[j].texCoords[1];
		}
#if 0
	}
#endif

#ifdef _G2_GORE
	CRenderableSurface *storeSurf = surf;

	while (surf->goreChain)
	{
		surf=(CRenderableSurface *)surf->goreChain;
		if (surf->alternateTex)
		{
			// get a gore surface ready to go.

			/*
				sizeof(int)+ // num verts
				sizeof(int)+ // num tris
				sizeof(int)*newNumVerts+ // which verts to copy from original surface
				sizeof(float)*4*newNumVerts+ // storgage for deformed verts
				sizeof(float)*4*newNumVerts+ // storgage for deformed normal
				sizeof(float)*2*newNumVerts+ // texture coordinates
				sizeof(int)*newNumTris*3;  // new indecies
			*/

			int *data=(int *)surf->alternateTex;
			int gnumVerts=*data++;
			data++;

			float *fdata=(float *)data;
			fdata+=gnumVerts;
			for (j=0;j<gnumVerts;j++)
			{
				assert(data[j]>=0&&data[j]<numVerts);
				memcpy(fdata,&tess.xyz[tess.numVertexes+data[j]][0],sizeof(float)*3);
				fdata+=4;
			}
			for (j=0;j<gnumVerts;j++)
			{
				assert(data[j]>=0&&data[j]<numVerts);
				memcpy(fdata,&tess.normal[tess.numVertexes+data[j]][0],sizeof(float)*3);
				fdata+=4;
			}
		}
		else
		{
			assert(0);
		}

	}

	// NOTE: This is required because a ghoul model might need to be rendered twice a frame (don't cringe,
	// it's not THAT bad), so we only delete it when doing the glow pass. Warning though, this assumes that
	// the glow is rendered _second_!!! If that changes, change this!
	extern bool g_bRenderGlowingObjects;
	extern bool g_bDynamicGlowSupported;
	if ( !tess.shader->hasGlow || g_bRenderGlowingObjects || !g_bDynamicGlowSupported || !r_DynamicGlow->integer )
	{
		delete storeSurf;
	}
#endif

	tess.numVertexes += surface->numVerts;

#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_RB_SurfaceGhoul += G2PerformanceTimer_RB_SurfaceGhoul.End();
#endif
}


/*
=================
R_LoadMDXM - load a Ghoul 2 Mesh file
=================
*/

/*

Some information used in the creation of the JK2 - JKA bone remap table

These are the old bones:
Complete list of all 72 bones:

*/

int OldToNewRemapTable[72] = {
0,// Bone 0:   "model_root":           Parent: ""  (index -1)
1,// Bone 1:   "pelvis":               Parent: "model_root"  (index 0)
2,// Bone 2:   "Motion":               Parent: "pelvis"  (index 1)
3,// Bone 3:   "lfemurYZ":             Parent: "pelvis"  (index 1)
4,// Bone 4:   "lfemurX":              Parent: "pelvis"  (index 1)
5,// Bone 5:   "ltibia":               Parent: "pelvis"  (index 1)
6,// Bone 6:   "ltalus":               Parent: "pelvis"  (index 1)
6,// Bone 7:   "ltarsal":              Parent: "pelvis"  (index 1)
7,// Bone 8:   "rfemurYZ":             Parent: "pelvis"  (index 1)
8,// Bone 9:   "rfemurX":	            Parent: "pelvis"  (index 1)
9,// Bone10:   "rtibia":	            Parent: "pelvis"  (index 1)
10,// Bone11:   "rtalus":	            Parent: "pelvis"  (index 1)
10,// Bone12:   "rtarsal":              Parent: "pelvis"  (index 1)
11,// Bone13:   "lower_lumbar":         Parent: "pelvis"  (index 1)
12,// Bone14:   "upper_lumbar":	        Parent: "lower_lumbar"  (index 13)
13,// Bone15:   "thoracic":	            Parent: "upper_lumbar"  (index 14)
14,// Bone16:   "cervical":	            Parent: "thoracic"  (index 15)
15,// Bone17:   "cranium":              Parent: "cervical"  (index 16)
16,// Bone18:   "ceyebrow":	            Parent: "face_always_"  (index 71)
17,// Bone19:   "jaw":                  Parent: "face_always_"  (index 71)
18,// Bone20:   "lblip2":	            Parent: "face_always_"  (index 71)
19,// Bone21:   "leye":		            Parent: "face_always_"  (index 71)
20,// Bone22:   "rblip2":	            Parent: "face_always_"  (index 71)
21,// Bone23:   "ltlip2":               Parent: "face_always_"  (index 71)
22,// Bone24:   "rtlip2":	            Parent: "face_always_"  (index 71)
23,// Bone25:   "reye":		            Parent: "face_always_"  (index 71)
24,// Bone26:   "rclavical":            Parent: "thoracic"  (index 15)
25,// Bone27:   "rhumerus":             Parent: "thoracic"  (index 15)
26,// Bone28:   "rhumerusX":            Parent: "thoracic"  (index 15)
27,// Bone29:   "rradius":              Parent: "thoracic"  (index 15)
28,// Bone30:   "rradiusX":             Parent: "thoracic"  (index 15)
29,// Bone31:   "rhand":                Parent: "thoracic"  (index 15)
29,// Bone32:   "mc7":                  Parent: "thoracic"  (index 15)
34,// Bone33:   "r_d5_j1":              Parent: "thoracic"  (index 15)
35,// Bone34:   "r_d5_j2":              Parent: "thoracic"  (index 15)
35,// Bone35:   "r_d5_j3":              Parent: "thoracic"  (index 15)
30,// Bone36:   "r_d1_j1":              Parent: "thoracic"  (index 15)
31,// Bone37:   "r_d1_j2":              Parent: "thoracic"  (index 15)
31,// Bone38:   "r_d1_j3":              Parent: "thoracic"  (index 15)
32,// Bone39:   "r_d2_j1":              Parent: "thoracic"  (index 15)
33,// Bone40:   "r_d2_j2":              Parent: "thoracic"  (index 15)
33,// Bone41:   "r_d2_j3":              Parent: "thoracic"  (index 15)
32,// Bone42:   "r_d3_j1":			    Parent: "thoracic"  (index 15)
33,// Bone43:   "r_d3_j2":		        Parent: "thoracic"  (index 15)
33,// Bone44:   "r_d3_j3":              Parent: "thoracic"  (index 15)
34,// Bone45:   "r_d4_j1":              Parent: "thoracic"  (index 15)
35,// Bone46:   "r_d4_j2":	            Parent: "thoracic"  (index 15)
35,// Bone47:   "r_d4_j3":		        Parent: "thoracic"  (index 15)
36,// Bone48:   "rhang_tag_bone":	    Parent: "thoracic"  (index 15)
37,// Bone49:   "lclavical":            Parent: "thoracic"  (index 15)
38,// Bone50:   "lhumerus":	            Parent: "thoracic"  (index 15)
39,// Bone51:   "lhumerusX":	        Parent: "thoracic"  (index 15)
40,// Bone52:   "lradius":	            Parent: "thoracic"  (index 15)
41,// Bone53:   "lradiusX":	            Parent: "thoracic"  (index 15)
42,// Bone54:   "lhand":	            Parent: "thoracic"  (index 15)
42,// Bone55:   "mc5":		            Parent: "thoracic"  (index 15)
43,// Bone56:   "l_d5_j1":	            Parent: "thoracic"  (index 15)
44,// Bone57:   "l_d5_j2":	            Parent: "thoracic"  (index 15)
44,// Bone58:   "l_d5_j3":	            Parent: "thoracic"  (index 15)
43,// Bone59:   "l_d4_j1":	            Parent: "thoracic"  (index 15)
44,// Bone60:   "l_d4_j2":	            Parent: "thoracic"  (index 15)
44,// Bone61:   "l_d4_j3":	            Parent: "thoracic"  (index 15)
45,// Bone62:   "l_d3_j1":	            Parent: "thoracic"  (index 15)
46,// Bone63:   "l_d3_j2":	            Parent: "thoracic"  (index 15)
46,// Bone64:   "l_d3_j3":	            Parent: "thoracic"  (index 15)
45,// Bone65:   "l_d2_j1":	            Parent: "thoracic"  (index 15)
46,// Bone66:   "l_d2_j2":	            Parent: "thoracic"  (index 15)
46,// Bone67:   "l_d2_j3":	            Parent: "thoracic"  (index 15)
47,// Bone68:   "l_d1_j1":				Parent: "thoracic"  (index 15)
48,// Bone69:   "l_d1_j2":	            Parent: "thoracic"  (index 15)
48,// Bone70:   "l_d1_j3":				Parent: "thoracic"  (index 15)
52// Bone71:   "face_always_":			Parent: "cranium"  (index 17)
};


/*

Bone   0:   "model_root":
            Parent: ""  (index -1)
            #Kids:  1
            Child 0: (index 1), name "pelvis"

Bone   1:   "pelvis":
            Parent: "model_root"  (index 0)
            #Kids:  4
            Child 0: (index 2), name "Motion"
            Child 1: (index 3), name "lfemurYZ"
            Child 2: (index 7), name "rfemurYZ"
            Child 3: (index 11), name "lower_lumbar"

Bone   2:   "Motion":
            Parent: "pelvis"  (index 1)
            #Kids:  0

Bone   3:   "lfemurYZ":
            Parent: "pelvis"  (index 1)
            #Kids:  3
            Child 0: (index 4), name "lfemurX"
            Child 1: (index 5), name "ltibia"
            Child 2: (index 49), name "ltail"

Bone   4:   "lfemurX":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  0

Bone   5:   "ltibia":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  1
            Child 0: (index 6), name "ltalus"

Bone   6:   "ltalus":
            Parent: "ltibia"  (index 5)
            #Kids:  0

Bone   7:   "rfemurYZ":
            Parent: "pelvis"  (index 1)
            #Kids:  3
            Child 0: (index 8), name "rfemurX"
            Child 1: (index 9), name "rtibia"
            Child 2: (index 50), name "rtail"

Bone   8:   "rfemurX":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  0

Bone   9:   "rtibia":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  1
            Child 0: (index 10), name "rtalus"

Bone  10:   "rtalus":
            Parent: "rtibia"  (index 9)
            #Kids:  0

Bone  11:   "lower_lumbar":
            Parent: "pelvis"  (index 1)
            #Kids:  1
            Child 0: (index 12), name "upper_lumbar"

Bone  12:   "upper_lumbar":
            Parent: "lower_lumbar"  (index 11)
            #Kids:  1
            Child 0: (index 13), name "thoracic"

Bone  13:   "thoracic":
            Parent: "upper_lumbar"  (index 12)
            #Kids:  5
            Child 0: (index 14), name "cervical"
            Child 1: (index 24), name "rclavical"
            Child 2: (index 25), name "rhumerus"
            Child 3: (index 37), name "lclavical"
            Child 4: (index 38), name "lhumerus"

Bone  14:   "cervical":
            Parent: "thoracic"  (index 13)
            #Kids:  1
            Child 0: (index 15), name "cranium"

Bone  15:   "cranium":
            Parent: "cervical"  (index 14)
            #Kids:  1
            Child 0: (index 52), name "face_always_"

Bone  16:   "ceyebrow":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  17:   "jaw":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  18:   "lblip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  19:   "leye":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  20:   "rblip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  21:   "ltlip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  22:   "rtlip2":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  23:   "reye":
            Parent: "face_always_"  (index 52)
            #Kids:  0

Bone  24:   "rclavical":
            Parent: "thoracic"  (index 13)
            #Kids:  0

Bone  25:   "rhumerus":
            Parent: "thoracic"  (index 13)
            #Kids:  2
            Child 0: (index 26), name "rhumerusX"
            Child 1: (index 27), name "rradius"

Bone  26:   "rhumerusX":
            Parent: "rhumerus"  (index 25)
            #Kids:  0

Bone  27:   "rradius":
            Parent: "rhumerus"  (index 25)
            #Kids:  9
            Child 0: (index 28), name "rradiusX"
            Child 1: (index 29), name "rhand"
            Child 2: (index 30), name "r_d1_j1"
            Child 3: (index 31), name "r_d1_j2"
            Child 4: (index 32), name "r_d2_j1"
            Child 5: (index 33), name "r_d2_j2"
            Child 6: (index 34), name "r_d4_j1"
            Child 7: (index 35), name "r_d4_j2"
            Child 8: (index 36), name "rhang_tag_bone"

Bone  28:   "rradiusX":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  29:   "rhand":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  30:   "r_d1_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  31:   "r_d1_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  32:   "r_d2_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  33:   "r_d2_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  34:   "r_d4_j1":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  35:   "r_d4_j2":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  36:   "rhang_tag_bone":
            Parent: "rradius"  (index 27)
            #Kids:  0

Bone  37:   "lclavical":
            Parent: "thoracic"  (index 13)
            #Kids:  0

Bone  38:   "lhumerus":
            Parent: "thoracic"  (index 13)
            #Kids:  2
            Child 0: (index 39), name "lhumerusX"
            Child 1: (index 40), name "lradius"

Bone  39:   "lhumerusX":
            Parent: "lhumerus"  (index 38)
            #Kids:  0

Bone  40:   "lradius":
            Parent: "lhumerus"  (index 38)
            #Kids:  9
            Child 0: (index 41), name "lradiusX"
            Child 1: (index 42), name "lhand"
            Child 2: (index 43), name "l_d4_j1"
            Child 3: (index 44), name "l_d4_j2"
            Child 4: (index 45), name "l_d2_j1"
            Child 5: (index 46), name "l_d2_j2"
            Child 6: (index 47), name "l_d1_j1"
            Child 7: (index 48), name "l_d1_j2"
            Child 8: (index 51), name "lhang_tag_bone"

Bone  41:   "lradiusX":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  42:   "lhand":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  43:   "l_d4_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  44:   "l_d4_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  45:   "l_d2_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  46:   "l_d2_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  47:   "l_d1_j1":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  48:   "l_d1_j2":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  49:   "ltail":
            Parent: "lfemurYZ"  (index 3)
            #Kids:  0

Bone  50:   "rtail":
            Parent: "rfemurYZ"  (index 7)
            #Kids:  0

Bone  51:   "lhang_tag_bone":
            Parent: "lradius"  (index 40)
            #Kids:  0

Bone  52:   "face_always_":
            Parent: "cranium"  (index 15)
            #Kids:  8
            Child 0: (index 16), name "ceyebrow"
            Child 1: (index 17), name "jaw"
            Child 2: (index 18), name "lblip2"
            Child 3: (index 19), name "leye"
            Child 4: (index 20), name "rblip2"
            Child 5: (index 21), name "ltlip2"
            Child 6: (index 22), name "rtlip2"
            Child 7: (index 23), name "reye"



*/


qboolean R_LoadMDXM( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {
	int					i,l, j;
	mdxmHeader_t		*pinmodel, *mdxm;
	mdxmLOD_t			*lod;
	mdxmSurface_t		*surf;
	int					version;
	int					size;
	mdxmSurfHierarchy_t	*surfInfo;

#ifdef Q3_BIG_ENDIAN
	int					k;
	mdxmTriangle_t		*tri;
	mdxmVertex_t		*v;
	int					*boneRef;
	mdxmLODSurfOffset_t	*indexes;
	mdxmVertexTexCoord_t	*pTexCoords;
	mdxmHierarchyOffsets_t	*surfIndexes;
#endif

	pinmodel= (mdxmHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXM_VERSION) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "R_LoadMDXM: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXM_VERSION);
		return qfalse;
	}

	mod->type	   = MOD_MDXM;
	mod->dataSize += size;

	qboolean bAlreadyFound = qfalse;
	mdxm = mod->mdxm = (mdxmHeader_t*) //Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size, buffer, mod_name, &bAlreadyFound, TAG_MODEL_GLM);

	assert(bAlreadyCached == bAlreadyFound);

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxm == buffer );
//		memcpy( mdxm, buffer, size );	// and don't do this now, since it's the same thing

		LL(mdxm->ident);
		LL(mdxm->version);
		LL(mdxm->numBones);
		LL(mdxm->numLODs);
		LL(mdxm->ofsLODs);
		LL(mdxm->numSurfaces);
		LL(mdxm->ofsSurfHierarchy);
		LL(mdxm->ofsEnd);
	}

	// first up, go load in the animation file we need that has the skeletal animation info for this model
	mdxm->animIndex = RE_RegisterModel(va ("%s.gla",mdxm->animName));

	if (!mdxm->animIndex)
	{
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "R_LoadMDXM: missing animation file %s for mesh %s\n", mdxm->animName, mdxm->name);
		return qfalse;
	}

	mod->numLods = mdxm->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if (bAlreadyFound)
	{
		return qtrue;	// All done. Stop, go no further, do not LittleLong(), do not pass Go...
	}

	bool isAnOldModelFile = false;
	if (mdxm->numBones == 72 && strstr(mdxm->animName,"_humanoid") )
	{
		isAnOldModelFile = true;
	}

	surfInfo = (mdxmSurfHierarchy_t *)( (byte *)mdxm + mdxm->ofsSurfHierarchy);
#ifdef Q3_BIG_ENDIAN
	surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)mdxm + sizeof(mdxmHeader_t));
#endif
 	for ( i = 0 ; i < mdxm->numSurfaces ; i++)
	{
		LL(surfInfo->flags);
		LL(surfInfo->numChildren);
		LL(surfInfo->parentIndex);

		Q_strlwr(surfInfo->name);	//just in case
		if ( !strcmp( &surfInfo->name[strlen(surfInfo->name)-4],"_off") )
		{
			surfInfo->name[strlen(surfInfo->name)-4]=0;	//remove "_off" from name
		}

		// do all the children indexs
		for (j=0; j<surfInfo->numChildren; j++)
		{
			LL(surfInfo->childIndexes[j]);
		}

		shader_t	*sh;
		// get the shader name
		sh = R_FindShader( surfInfo->shader, lightmapsNone, stylesDefault, qtrue );
		// insert it in the surface list
		if ( sh->defaultShader )
		{
			surfInfo->shaderIndex = 0;
		}
		else
		{
			surfInfo->shaderIndex = sh->index;
		}

		RE_RegisterModels_StoreShaderRequest(mod_name, &surfInfo->shader[0], &surfInfo->shaderIndex);

#ifdef Q3_BIG_ENDIAN
		// swap the surface offset
		LL(surfIndexes->offsets[i]);
		assert(surfInfo == (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[i]));
#endif

		// find the next surface
		surfInfo = (mdxmSurfHierarchy_t *)( (byte *)surfInfo + (size_t)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ surfInfo->numChildren ] ));
  	}

	// swap all the LOD's	(we need to do the middle part of this even for intel, because of shader reg and err-check)
	lod = (mdxmLOD_t *) ( (byte *)mdxm + mdxm->ofsLODs );
	for ( l = 0 ; l < mdxm->numLODs ; l++)
	{
		int	triCount = 0;

		LL(lod->ofsEnd);
		// swap all the surfaces
		surf = (mdxmSurface_t *) ( (byte *)lod + sizeof (mdxmLOD_t) + (mdxm->numSurfaces * sizeof(mdxmLODSurfOffset_t)) );
		for ( i = 0 ; i < mdxm->numSurfaces ; i++)
		{
			LL(surf->thisSurfaceIndex);
			LL(surf->ofsHeader);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numBoneReferences);
			LL(surf->ofsBoneReferences);
			LL(surf->ofsEnd);

			triCount += surf->numTriangles;

			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				Com_Error (ERR_DROP, "R_LoadMDXM: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
				Com_Error (ERR_DROP, "R_LoadMDXM: %s has more than %i triangles on a surface (%i)",
					mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			}

			// change to surface identifier
			surf->ident = SF_MDX;
			// register the shaders

			if (isAnOldModelFile)
			{
				int *boneRef = (int *) ( (byte *)surf + surf->ofsBoneReferences );
				for ( j = 0 ; j < surf->numBoneReferences ; j++ )
				{
					assert(boneRef[j] >= 0 && boneRef[j] < 72);
					if (boneRef[j] >= 0 && boneRef[j] < 72)
					{
						boneRef[j]=OldToNewRemapTable[boneRef[j]];
					}
					else
					{
						boneRef[j]=0;
					}
				}
			}
			// find the next surface
			surf = (mdxmSurface_t *)( (byte *)surf + surf->ofsEnd );
		}
		// find the next LOD
		lod = (mdxmLOD_t *)( (byte *)lod + lod->ofsEnd );
	}
	return qtrue;
}

/*
=================
R_LoadMDXA - load a Ghoul 2 animation file
=================
*/
qboolean R_LoadMDXA( model_t *mod, void *buffer, const char *mod_name, qboolean &bAlreadyCached ) {

	mdxaHeader_t		*pinmodel, *mdxa;
	int					version;
	int					size;

 	pinmodel = (mdxaHeader_t *)buffer;
	//
	// read some fields from the binary, but only LittleLong() them when we know this wasn't an already-cached model...
	//
	version = (pinmodel->version);
	size	= (pinmodel->ofsEnd);

	if (!bAlreadyCached)
	{
		LL(version);
		LL(size);
	}

	if (version != MDXA_VERSION) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "R_LoadMDXA: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MDXA_VERSION);
		return qfalse;
	}

	mod->type		= MOD_MDXA;
	mod->dataSize  += size;

	qboolean bAlreadyFound = qfalse;

	mdxa = mod->mdxa = (mdxaHeader_t*) //Hunk_Alloc( size );
										RE_RegisterModels_Malloc(size,
										#ifdef CREATE_LIMB_HIERARCHY
											NULL,	// I think this'll work, can't really test on PC
										#else
											buffer,
										#endif
										mod_name, &bAlreadyFound, TAG_MODEL_GLA);

	assert(bAlreadyCached == bAlreadyFound);	// I should probably eliminate 'bAlreadyFound', but wtf?

	if (!bAlreadyFound)
	{
		// horrible new hackery, if !bAlreadyFound then we've just done a tag-morph, so we need to set the
		//	bool reference passed into this function to true, to tell the caller NOT to do an ri.FS_Freefile since
		//	we've hijacked that memory block...
		//
		// Aaaargh. Kill me now...
		//
		bAlreadyCached = qtrue;
		assert( mdxa == buffer );
//		memcpy( mdxa, buffer, size );	// and don't do this now, since it's the same thing
		LL(mdxa->ident);
		LL(mdxa->version);
		//LF(mdxa->fScale);
		LL(mdxa->numFrames);
		LL(mdxa->ofsFrames);
		LL(mdxa->numBones);
		LL(mdxa->ofsCompBonePool);
		LL(mdxa->ofsSkel);
		LL(mdxa->ofsEnd);
	}

 	if ( mdxa->numFrames < 1 ) {
		ri.Printf( PRINT_ALL, S_COLOR_YELLOW  "R_LoadMDXA: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (bAlreadyFound)
	{
		return qtrue;	// All done, stop here, do not LittleLong() etc. Do not pass go...
	}

	return qtrue;
}

void G2_TransformBone(int index,CBoneCache &CB) {
	ri.G2_TransformBone(index, CB);
}
