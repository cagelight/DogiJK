#include "g2_local.hh"
#include "G2_gore.hh"

refexport_t g2_re;
refimport_t g2_ri;

cvar_t	*r_noServerGhoul2;
cvar_t	*r_Ghoul2AnimSmooth=0;
cvar_t	*r_Ghoul2UnSqashAfterSmooth=0;

cvar_t	*broadsword=0;
cvar_t	*broadsword_kickbones=0;
cvar_t	*broadsword_kickorigin=0;
cvar_t	*broadsword_playflop=0;
cvar_t	*broadsword_dontstopanim=0;
cvar_t	*broadsword_waitforshot=0;
cvar_t	*broadsword_smallbbox=0;
cvar_t	*broadsword_extra1=0;
cvar_t	*broadsword_extra2=0;

cvar_t	*broadsword_effcorr=0;
cvar_t	*broadsword_ragtobase=0;
cvar_t	*broadsword_dircap=0;

static g2export_t g2_ex;

static void G2API_BoltMatrixReconstruction( qboolean reconstruct ) { gG2_GBMNoReconstruct = (qboolean)!reconstruct; }
static void G2API_BoltMatrixSPMethod( qboolean spMethod ) { gG2_GBMUseSPMethod = spMethod; }

static void G2API_DeleteGoreTextureCoords(GoreTextureCoordinates * tex) {
	if (tex) (*tex).~GoreTextureCoordinates();
}

qboolean G2_IsValid( CGhoul2Info_v const & g) {
	return g.IsValid();
}

size_t G2_Size( CGhoul2Info_v const & g) {
	return g.size();
}

static CGhoul2Info & G2_At( CGhoul2Info_v const & g, size_t i ) {
	return const_cast<CGhoul2Info &>(g[i]);
}

extern "C" Q_EXPORT g2export_t * QDECL G2_GetInterface() {
	// G2API
	g2_ex.G2API_AddBolt						= G2API_AddBolt;
	g2_ex.G2API_AddBoltSurfNum					= G2API_AddBoltSurfNum;
	g2_ex.G2API_AddSurface						= G2API_AddSurface;
	g2_ex.G2API_AnimateG2ModelsRag				= G2API_AnimateG2ModelsRag;
	g2_ex.G2API_AttachEnt						= G2API_AttachEnt;
	g2_ex.G2API_AttachG2Model					= G2API_AttachG2Model;
	g2_ex.G2API_AttachInstanceToEntNum			= G2API_AttachInstanceToEntNum;
	g2_ex.G2API_AbsurdSmoothing				= G2API_AbsurdSmoothing;
	g2_ex.G2API_BoltMatrixReconstruction		= G2API_BoltMatrixReconstruction;
	g2_ex.G2API_BoltMatrixSPMethod				= G2API_BoltMatrixSPMethod;
	g2_ex.G2API_CleanEntAttachments			= G2API_CleanEntAttachments;
	g2_ex.G2API_CleanGhoul2Models				= G2API_CleanGhoul2Models;
	g2_ex.G2API_ClearAttachedInstance			= G2API_ClearAttachedInstance;
	g2_ex.G2API_CollisionDetect				= G2API_CollisionDetect;
	g2_ex.G2API_CollisionDetectCache			= G2API_CollisionDetectCache;
	g2_ex.G2API_CopyGhoul2Instance				= G2API_CopyGhoul2Instance;
	g2_ex.G2API_CopySpecificG2Model			= G2API_CopySpecificG2Model;
	g2_ex.G2API_DetachG2Model					= G2API_DetachG2Model;
	g2_ex.G2API_DoesBoneExist					= G2API_DoesBoneExist;
	g2_ex.G2API_DuplicateGhoul2Instance		= G2API_DuplicateGhoul2Instance;
	g2_ex.G2API_FreeSaveBuffer					= G2API_FreeSaveBuffer;
	g2_ex.G2API_GetAnimFileName				= G2API_GetAnimFileName;
	g2_ex.G2API_GetAnimFileNameIndex			= G2API_GetAnimFileNameIndex;
	g2_ex.G2API_GetAnimRange					= G2API_GetAnimRange;
	g2_ex.G2API_GetBoltMatrix					= G2API_GetBoltMatrix;
	g2_ex.G2API_GetBoneAnim					= G2API_GetBoneAnim;
	g2_ex.G2API_GetBoneIndex					= G2API_GetBoneIndex;
	g2_ex.G2API_GetGhoul2ModelFlags			= G2API_GetGhoul2ModelFlags;
	g2_ex.G2API_GetGLAName						= G2API_GetGLAName;
	g2_ex.G2API_GetModelName					= G2API_GetModelName;
	g2_ex.G2API_GetParentSurface				= G2API_GetParentSurface;
	g2_ex.G2API_GetRagBonePos					= G2API_GetRagBonePos;
	g2_ex.G2API_GetSurfaceIndex				= G2API_GetSurfaceIndex;
	g2_ex.G2API_GetSurfaceName					= G2API_GetSurfaceName;
	g2_ex.G2API_GetSurfaceOnOff				= G2API_GetSurfaceOnOff;
	g2_ex.G2API_GetSurfaceRenderStatus			= G2API_GetSurfaceRenderStatus;
	g2_ex.G2API_GetTime						= G2API_GetTime;
	g2_ex.G2API_Ghoul2Size						= G2API_Ghoul2Size;
	g2_ex.G2API_GiveMeVectorFromMatrix			= G2API_GiveMeVectorFromMatrix;
	g2_ex.G2API_HasGhoul2ModelOnIndex			= G2API_HasGhoul2ModelOnIndex;
	g2_ex.G2API_HaveWeGhoul2Models				= G2API_HaveWeGhoul2Models;
	g2_ex.G2API_IKMove							= G2API_IKMove;
	g2_ex.G2API_InitGhoul2Model				= G2API_InitGhoul2Model;
	g2_ex.G2API_IsGhoul2InfovValid				= G2API_IsGhoul2InfovValid;
	g2_ex.G2API_IsPaused						= G2API_IsPaused;
	g2_ex.G2API_ListBones						= G2API_ListBones;
	g2_ex.G2API_ListSurfaces					= G2API_ListSurfaces;
	g2_ex.G2API_LoadGhoul2Models				= G2API_LoadGhoul2Models;
	g2_ex.G2API_LoadSaveCodeDestructGhoul2Info	= G2API_LoadSaveCodeDestructGhoul2Info;
	g2_ex.G2API_OverrideServerWithClientData	= G2API_OverrideServerWithClientData;
	g2_ex.G2API_PauseBoneAnim					= G2API_PauseBoneAnim;
	g2_ex.G2API_PrecacheGhoul2Model			= G2API_PrecacheGhoul2Model;
	g2_ex.G2API_RagEffectorGoal				= G2API_RagEffectorGoal;
	g2_ex.G2API_RagEffectorKick				= G2API_RagEffectorKick;
	g2_ex.G2API_RagForceSolve					= G2API_RagForceSolve;
	g2_ex.G2API_RagPCJConstraint				= G2API_RagPCJConstraint;
	g2_ex.G2API_RagPCJGradientSpeed			= G2API_RagPCJGradientSpeed;
	g2_ex.G2API_RemoveBolt						= G2API_RemoveBolt;
	g2_ex.G2API_RemoveBone						= G2API_RemoveBone;
	g2_ex.G2API_RemoveGhoul2Model				= G2API_RemoveGhoul2Model;
	g2_ex.G2API_RemoveGhoul2Models				= G2API_RemoveGhoul2Models;
	g2_ex.G2API_RemoveSurface					= G2API_RemoveSurface;
	g2_ex.G2API_ResetRagDoll					= G2API_ResetRagDoll;
	g2_ex.G2API_SaveGhoul2Models				= G2API_SaveGhoul2Models;
	g2_ex.G2API_SetBoltInfo					= G2API_SetBoltInfo;
	g2_ex.G2API_SetBoneAngles					= G2API_SetBoneAngles;
	g2_ex.G2API_SetBoneAnglesIndex				= G2API_SetBoneAnglesIndex;
	g2_ex.G2API_SetBoneAnglesMatrix			= G2API_SetBoneAnglesMatrix;
	g2_ex.G2API_SetBoneAnglesMatrixIndex		= G2API_SetBoneAnglesMatrixIndex;
	g2_ex.G2API_SetBoneAnim					= G2API_SetBoneAnim;
	g2_ex.G2API_SetBoneAnimIndex				= G2API_SetBoneAnimIndex;
	g2_ex.G2API_SetBoneIKState					= G2API_SetBoneIKState;
	g2_ex.G2API_SetGhoul2ModelIndexes			= G2API_SetGhoul2ModelIndexes;
	g2_ex.G2API_SetGhoul2ModelFlags			= G2API_SetGhoul2ModelFlags;
	g2_ex.G2API_SetLodBias						= G2API_SetLodBias;
	g2_ex.G2API_SetNewOrigin					= G2API_SetNewOrigin;
	g2_ex.G2API_SetRagDoll						= G2API_SetRagDoll;
	g2_ex.G2API_SetRootSurface					= G2API_SetRootSurface;
	g2_ex.G2API_SetShader						= G2API_SetShader;
	g2_ex.G2API_SetSkin						= G2API_SetSkin;
	g2_ex.G2API_SetSurfaceOnOff				= G2API_SetSurfaceOnOff;
	g2_ex.G2API_SetTime						= G2API_SetTime;
	g2_ex.G2API_SkinlessModel					= G2API_SkinlessModel;
	g2_ex.G2API_StopBoneAngles					= G2API_StopBoneAngles;
	g2_ex.G2API_StopBoneAnglesIndex			= G2API_StopBoneAnglesIndex;
	g2_ex.G2API_StopBoneAnim					= G2API_StopBoneAnim;
	g2_ex.G2API_StopBoneAnimIndex				= G2API_StopBoneAnimIndex;

	#ifdef _G2_GORE
	g2_ex.G2API_GetNumGoreMarks				= G2API_GetNumGoreMarks;
	g2_ex.G2API_AddSkinGore					= G2API_AddSkinGore;
	g2_ex.G2API_ClearSkinGore					= G2API_ClearSkinGore;
	g2_ex.G2API_FindGoreRecord = FindGoreRecord;
	g2_ex.G2API_DeleteGoreTextureCoords = G2API_DeleteGoreTextureCoords;
	#endif // _SOF2
	
	g2_ex.G2_FindSurface = G2_FindSurface;
	g2_ex.G2_FindOverrideSurface = G2_FindOverrideSurface;
	g2_ex.G2_SetupModelPointers = G2_SetupModelPointers;
	g2_ex.G2_RootMatrix = G2_RootMatrix;
	g2_ex.G2_Sort_Models = G2_Sort_Models;
	g2_ex.G2_GenerateWorldMatrix = G2_GenerateWorldMatrix;
	g2_ex.G2_GetBoltMatrixLow = G2_GetBoltMatrixLow;
	g2_ex.G2_TransformGhoulBones = G2_TransformGhoulBones;
	g2_ex.G2_TransformBone = G2_TransformBone;
	g2_ex.G2_FindGoreSet = FindGoreSet;
	g2_ex.G2_IsValid = G2_IsValid;
	g2_ex.G2_Size = G2_Size;
	g2_ex.G2_At = G2_At;
	
	return &g2_ex;
}

extern "C" Q_EXPORT void QDECL G2_Init(refimport_t * ri, refexport_t * re) {
	g2_re = *re;
	g2_ri = *ri;
	
	broadsword							= g2_ri.Cvar_Get( "broadsword",						"0",						CVAR_ARCHIVE_ND, "" );
	broadsword_kickbones				= g2_ri.Cvar_Get( "broadsword_kickbones",				"1",						CVAR_NONE, "" );
	broadsword_kickorigin			= g2_ri.Cvar_Get( "broadsword_kickog2_rigin",			"1",						CVAR_NONE, "" );
	broadsword_dontstopanim				= g2_ri.Cvar_Get( "broadsword_dontstopanim",			"0",						CVAR_NONE, "" );
	broadsword_waitforshot				= g2_ri.Cvar_Get( "broadsword_waitforshot",			"0",						CVAR_NONE, "" );
	broadsword_playflop					= g2_ri.Cvar_Get( "broadsword_playflop",				"1",						CVAR_NONE, "" );
	broadsword_smallbbox				= g2_ri.Cvar_Get( "broadsword_smallbbox",				"0",						CVAR_NONE, "" );
	broadsword_extra1					= g2_ri.Cvar_Get( "broadsword_extra1",				"0",						CVAR_NONE, "" );
	broadsword_extra2					= g2_ri.Cvar_Get( "broadsword_extra2",				"0",						CVAR_NONE, "" );
	broadsword_effcorr					= g2_ri.Cvar_Get( "broadsword_effcorr",				"1",						CVAR_NONE, "" );
	broadsword_ragtobase				= g2_ri.Cvar_Get( "broadsword_ragtobase",				"2",						CVAR_NONE, "" );
	broadsword_dircap					= g2_ri.Cvar_Get( "broadsword_dircap",				"64",						CVAR_NONE, "" );
	
	r_noServerGhoul2					= g2_ri.Cvar_Get( "r_noserverghoul2",					"0",						CVAR_CHEAT, "" );
	r_Ghoul2AnimSmooth					= g2_ri.Cvar_Get( "r_ghoul2animsmooth",				"0.3",						CVAR_NONE, "" );
	r_Ghoul2UnSqashAfterSmooth			= g2_ri.Cvar_Get( "r_ghoul2unsqashaftersmooth",		"1",						CVAR_NONE, "" );
}

void QDECL Com_Printf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	g2_ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_OPrintf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	g2_ri.OPrintf("%s", text);
}

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	g2_ri.Error(level, "%s", text);
}
