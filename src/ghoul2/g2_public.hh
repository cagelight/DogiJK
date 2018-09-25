#pragma once

#include "qcommon/MiniHeap.hh"
#include "qcommon/qcommon.hh"
#include "ghoul2_shared.hh"

#include "rd-common/tr_public.hh"

typedef struct g2export_s {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void				(*Shutdown)								( qboolean destroyWindow, qboolean restarting );
	
	// G2API
	int					(*G2API_AddBolt)						( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName );
	int					(*G2API_AddBoltSurfNum)					( CGhoul2Info *ghlInfo, const int surfIndex );
	int					(*G2API_AddSurface)						( CGhoul2Info *ghlInfo, int surfaceNumber, int polyNumber, float BarycentricI, float BarycentricJ, int lod );
	void				(*G2API_AnimateG2ModelsRag)				( CGhoul2Info_v &ghoul2, int AcurrentTime, CRagDollUpdateParams *params );
	qboolean			(*G2API_AttachEnt)						( int *boltInfo, CGhoul2Info_v& ghoul2, int modelIndex, int toBoltIndex, int entNum, int toModelNum );
	qboolean			(*G2API_AttachG2Model)					( CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int toBoltIndex, int toModel );
	void				(*G2API_AttachInstanceToEntNum)			( CGhoul2Info_v &ghoul2, int entityNum, qboolean server );
	void				(*G2API_AbsurdSmoothing)				( CGhoul2Info_v &ghoul2, qboolean status );
	void				(*G2API_BoltMatrixReconstruction)		( qboolean reconstruct );
	void				(*G2API_BoltMatrixSPMethod)				( qboolean spMethod );
	void				(*G2API_CleanEntAttachments)			( void );
	void				(*G2API_CleanGhoul2Models)				( CGhoul2Info_v **ghoul2Ptr );
	void				(*G2API_ClearAttachedInstance)			( int entityNum );
	void				(*G2API_CollisionDetect)				( CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius );
	void				(*G2API_CollisionDetectCache)			( CollisionRecord_t *collRecMap, CGhoul2Info_v &ghoul2, const vec3_t angles, const vec3_t position, int frameNumber, int entNum, vec3_t rayStart, vec3_t rayEnd, vec3_t scale, IHeapAllocator *G2VertSpace, int traceFlags, int useLod, float fRadius );
	int					(*G2API_CopyGhoul2Instance)				( CGhoul2Info_v &g2From, CGhoul2Info_v &g2To, int modelIndex );
	void				(*G2API_CopySpecificG2Model)			( CGhoul2Info_v &ghoul2From, int modelFrom, CGhoul2Info_v &ghoul2To, int modelTo );
	qboolean			(*G2API_DetachG2Model)					( CGhoul2Info *ghlInfo );
	qboolean			(*G2API_DoesBoneExist)					( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName );
	void				(*G2API_DuplicateGhoul2Instance)		( CGhoul2Info_v &g2From, CGhoul2Info_v **g2To );
	void				(*G2API_FreeSaveBuffer)					( char *buffer );
	qboolean			(*G2API_GetAnimFileName)				( CGhoul2Info *ghlInfo, char **filename );
	char *				(*G2API_GetAnimFileNameIndex)			( qhandle_t modelIndex );
	qboolean			(*G2API_GetAnimRange)					( CGhoul2Info *ghlInfo, const char *boneName, int *startFrame, int *endFrame );
	qboolean			(*G2API_GetBoltMatrix)					( CGhoul2Info_v &ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix, const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale );
	qboolean			(*G2API_GetBoneAnim)					( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName, const int currentTime, float *currentFrame, int *startFrame, int *endFrame, int *flags, float *animSpeed, qhandle_t *modelList );
	int					(*G2API_GetBoneIndex)					( CGhoul2Info *ghlInfo, const char *boneName );
	int					(*G2API_GetGhoul2ModelFlags)			( CGhoul2Info *ghlInfo );
	char *				(*G2API_GetGLAName)						( CGhoul2Info_v &ghoul2, int modelIndex );
	const char *		(*G2API_GetModelName)					( CGhoul2Info_v& ghoul2, int modelIndex );
	int					(*G2API_GetParentSurface)				( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_GetRagBonePos)					( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos, vec3_t entAngles, vec3_t entPos, vec3_t entScale );
	int					(*G2API_GetSurfaceIndex)				( CGhoul2Info *ghlInfo, const char *surfaceName );
	char *				(*G2API_GetSurfaceName)					( CGhoul2Info_v& ghlInfo, int modelIndex, int surfNumber );
	int					(*G2API_GetSurfaceOnOff)				( CGhoul2Info *ghlInfo, const char *surfaceName );
	int					(*G2API_GetSurfaceRenderStatus)			( CGhoul2Info_v& ghoul2, int modelIndex, const char *surfaceName );
	int					(*G2API_GetTime)						( int argTime );
	int					(*G2API_Ghoul2Size)						( CGhoul2Info_v &ghoul2 );
	void				(*G2API_GiveMeVectorFromMatrix)			( mdxaBone_t *boltMatrix, Eorientations flags, vec3_t vec );
	qboolean			(*G2API_HasGhoul2ModelOnIndex)			( CGhoul2Info_v **ghlRemove, const int modelIndex );
	qboolean			(*G2API_HaveWeGhoul2Models)				( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_IKMove)							( CGhoul2Info_v &ghoul2, int time, sharedIKMoveParams_t *params );
	int					(*G2API_InitGhoul2Model)				( CGhoul2Info_v **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin, qhandle_t customShader, int modelFlags, int lodBias );
	qboolean			(*G2API_IsGhoul2InfovValid)				( CGhoul2Info_v& ghoul2 );
	qboolean			(*G2API_IsPaused)						( CGhoul2Info *ghlInfo, const char *boneName );
	void				(*G2API_ListBones)						( CGhoul2Info *ghlInfo, int frame );
	void				(*G2API_ListSurfaces)					( CGhoul2Info *ghlInfo );
	void				(*G2API_LoadGhoul2Models)				( CGhoul2Info_v &ghoul2, char *buffer );
	void				(*G2API_LoadSaveCodeDestructGhoul2Info)	( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_OverrideServerWithClientData)	( CGhoul2Info_v& serverInstance, int modelIndex );
	qboolean			(*G2API_PauseBoneAnim)					( CGhoul2Info *ghlInfo, const char *boneName, const int currentTime );
	qhandle_t			(*G2API_PrecacheGhoul2Model)			( const char *fileName );
	qboolean			(*G2API_RagEffectorGoal)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t pos );
	qboolean			(*G2API_RagEffectorKick)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t velocity );
	qboolean			(*G2API_RagForceSolve)					( CGhoul2Info_v &ghoul2, qboolean force );
	qboolean			(*G2API_RagPCJConstraint)				( CGhoul2Info_v &ghoul2, const char *boneName, vec3_t min, vec3_t max );
	qboolean			(*G2API_RagPCJGradientSpeed)			( CGhoul2Info_v &ghoul2, const char *boneName, const float speed );
	qboolean			(*G2API_RemoveBolt)						( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_RemoveBone)						( CGhoul2Info_v& ghoul2, int modelIndex, const char *boneName );
	qboolean			(*G2API_RemoveGhoul2Model)				( CGhoul2Info_v **ghlRemove, const int modelIndex );
	qboolean			(*G2API_RemoveGhoul2Models)				( CGhoul2Info_v **ghlRemove );
	qboolean			(*G2API_RemoveSurface)					( CGhoul2Info *ghlInfo, const int index );
	void				(*G2API_ResetRagDoll)					( CGhoul2Info_v &ghoul2 );
	qboolean			(*G2API_SaveGhoul2Models)				( CGhoul2Info_v &ghoul2, char **buffer, int *size );
	void				(*G2API_SetBoltInfo)					( CGhoul2Info_v &ghoul2, int modelIndex, int boltInfo );
	qboolean			(*G2API_SetBoneAngles)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const vec3_t angles, const int flags, const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t *modelList, int blendTime, int currentTime  );
	qboolean			(*G2API_SetBoneAnglesIndex)				( CGhoul2Info *ghlInfo, const int index, const vec3_t angles, const int flags, const Eorientations yaw, const Eorientations pitch, const Eorientations roll, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnglesMatrix)			( CGhoul2Info *ghlInfo, const char *boneName, const mdxaBone_t &matrix, const int flags, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnglesMatrixIndex)		( CGhoul2Info *ghlInfo, const int index, const mdxaBone_t &matrix, const int flags, qhandle_t *modelList, int blendTime, int currentTime );
	qboolean			(*G2API_SetBoneAnim)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame /*= -1*/, const int blendTime /*= -1*/ );
	qboolean			(*G2API_SetBoneAnimIndex)				( CGhoul2Info *ghlInfo, const int index, const int startFrame, const int endFrame, const int flags, const float animSpeed, const int currentTime, const float setFrame, const int blendTime );
	qboolean			(*G2API_SetBoneIKState)					( CGhoul2Info_v &ghoul2, int time, const char *boneName, int ikState, sharedSetBoneIKStateParams_t *params );
	qboolean			(*G2API_SetGhoul2ModelFlags)			( CGhoul2Info *ghlInfo, const int flags );
	void				(*G2API_SetGhoul2ModelIndexes)			( CGhoul2Info_v &ghoul2, qhandle_t *modelList, qhandle_t *skinList );
	qboolean			(*G2API_SetLodBias)						( CGhoul2Info *ghlInfo, int lodBias );
	qboolean			(*G2API_SetNewOrigin)					( CGhoul2Info_v &ghoul2, const int boltIndex );
	void				(*G2API_SetRagDoll)						( CGhoul2Info_v &ghoul2, CRagDollParams *parms );
	qboolean			(*G2API_SetRootSurface)					( CGhoul2Info_v &ghoul2, const int modelIndex, const char *surfaceName );
	qboolean			(*G2API_SetShader)						( CGhoul2Info *ghlInfo, qhandle_t customShader );
	qboolean			(*G2API_SetSkin)						( CGhoul2Info_v& ghoul2, int modelIndex, qhandle_t customSkin, qhandle_t renderSkin );
	qboolean			(*G2API_SetSurfaceOnOff)				( CGhoul2Info_v &ghoul2, const char *surfaceName, const int flags );
	void				(*G2API_SetTime)						( int currentTime, int clock );
	qboolean			(*G2API_SkinlessModel)					( CGhoul2Info_v& ghoul2, int modelIndex );
	qboolean			(*G2API_StopBoneAngles)					( CGhoul2Info *ghlInfo, const char *boneName );
	qboolean			(*G2API_StopBoneAnglesIndex)			( CGhoul2Info *ghlInfo, const int index );
	qboolean			(*G2API_StopBoneAnim)					( CGhoul2Info *ghlInfo, const char *boneName );
	qboolean			(*G2API_StopBoneAnimIndex)				( CGhoul2Info *ghlInfo, const int index );

	#ifdef _G2_GORE
	int					(*G2API_GetNumGoreMarks)				( CGhoul2Info_v& ghoul2, int modelIndex );
	void				(*G2API_AddSkinGore)					( CGhoul2Info_v &ghoul2, SSkinGoreData &gore );
	void				(*G2API_ClearSkinGore)					( CGhoul2Info_v &ghoul2 );
	#endif // _G2_GORE

} g2export_t;

void G2_Init(refimport_t * ri, refexport_t * re);
