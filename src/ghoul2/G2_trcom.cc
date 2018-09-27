/*
===========================================================================
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

#include "client/client.hh"	//FIXME!! EVIL - just include the definitions needed
#include "qcommon/matcomp.hh"
#include "qcommon/qcommon.hh"
#include "ghoul2/g2_local.hh"
#ifdef _G2_GORE
#include "ghoul2/G2_gore.hh"
#endif

#include "qcommon/disablewarnings.hh"

#define	LL(x) x=LittleLong(x)
#define	LS(x) x=LittleShort(x)
#define	LF(x) x=LittleFloat(x)

//rww - RAGDOLL_BEGIN
#ifdef __linux__
#include <math.h>
#else
#include <float.h>
#endif

//rww - RAGDOLL_END

qboolean G2_SetupModelPointers(CGhoul2Info *ghlInfo);
qboolean G2_SetupModelPointers(CGhoul2Info_v &ghoul2);

extern cvar_t	*r_Ghoul2AnimSmooth;
extern cvar_t	*r_Ghoul2UnSqashAfterSmooth;

#if 0
static inline int G2_Find_Bone_ByNum(const model_t *mod, boneInfo_v &blist, const int boneNum)
{
	size_t i = 0;

	while (i < blist.size())
	{
		if (blist[i].boneNumber == boneNum)
		{
			return i;
		}
		i++;
	}

	return -1;
}
#endif

const static mdxaBone_t		identityMatrix =
{
	{
		{ 0.0f, -1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f }
	}
};

// I hate doing this, but this is the simplest way to get this into the routines it needs to be
mdxaBone_t		worldMatrix;
mdxaBone_t		worldMatrixInv;
#ifdef _G2_GORE

#endif

void RemoveBoneCache(CBoneCache *boneCache)
{
#ifdef _FULL_G2_LEAK_CHECKING
	g_Ghoul2Allocations -= sizeof(*boneCache);
#endif

	delete boneCache;
}

#ifdef _G2_LISTEN_SERVER_OPT
void CopyBoneCache(CBoneCache *to, CBoneCache *from)
{
	memcpy(to, from, sizeof(CBoneCache));
}
#endif

const mdxaBone_t &EvalBoneCache(int index,CBoneCache *boneCache)
{
	assert(boneCache);
	return boneCache->Eval(index);
}

//rww - RAGDOLL_BEGIN
const mdxaHeader_t *G2_GetModA(CGhoul2Info &ghoul2)
{
	if (!ghoul2.mBoneCache)
	{
		return 0;
	}

	CBoneCache &boneCache=*ghoul2.mBoneCache;
	return boneCache.header;
}

int G2_GetBoneDependents(CGhoul2Info &ghoul2,int boneNum,int *tempDependents,int maxDep)
{
	// fixme, these should be precomputed
	if (!ghoul2.mBoneCache||!maxDep)
	{
		return 0;
	}

	CBoneCache &boneCache=*ghoul2.mBoneCache;
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	int i;
	int ret=0;
	for (i=0;i<skel->numChildren;i++)
	{
		if (!maxDep)
		{
			return i; // number added
		}
		*tempDependents=skel->children[i];
		assert(*tempDependents>0&&*tempDependents<boneCache.header->numBones);
		maxDep--;
		tempDependents++;
		ret++;
	}
	for (i=0;i<skel->numChildren;i++)
	{
		int num=G2_GetBoneDependents(ghoul2,skel->children[i],tempDependents,maxDep);
		tempDependents+=num;
		ret+=num;
		maxDep-=num;
		assert(maxDep>=0);
		if (!maxDep)
		{
			break;
		}
	}
	return ret;
}

bool G2_WasBoneRendered(CGhoul2Info &ghoul2,int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return false;
	}
	CBoneCache &boneCache=*ghoul2.mBoneCache;

	return boneCache.WasRendered(boneNum);
}

void G2_GetBoneBasepose(CGhoul2Info &ghoul2,int boneNum,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	if (!ghoul2.mBoneCache)
	{
		// yikes
		retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	retBasepose=&skel->BasePoseMat;
	retBaseposeInv=&skel->BasePoseMatInv;
}

char *G2_GetBoneNameFromSkel(CGhoul2Info &ghoul2, int boneNum)
{
	if (!ghoul2.mBoneCache)
	{
		return NULL;
	}
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);

	return skel->name;
}

void G2_RagGetBoneBasePoseMatrixLow(CGhoul2Info &ghoul2, int boneNum, mdxaBone_t &boneMatrix, mdxaBone_t &retMatrix, vec3_t scale)
{
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	Multiply_3x4Matrix(&retMatrix, &boneMatrix, &skel->BasePoseMat);

	if (scale[0])
	{
		retMatrix.matrix[0][3] *= scale[0];
	}
	if (scale[1])
	{
		retMatrix.matrix[1][3] *= scale[1];
	}
	if (scale[2])
	{
		retMatrix.matrix[2][3] *= scale[2];
	}

	VectorNormalize((float*)&retMatrix.matrix[0]);
	VectorNormalize((float*)&retMatrix.matrix[1]);
	VectorNormalize((float*)&retMatrix.matrix[2]);
}

void G2_GetBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	if (!ghoul2.mBoneCache)
	{
		retMatrix=identityMatrix;
		// yikes
		retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
		retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		return;
	}
	mdxaBone_t bolt;
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	assert(boneNum>=0&&boneNum<boneCache.header->numBones);

	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
	skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boneNum]);
	Multiply_3x4Matrix(&bolt, (mdxaBone_t *)&boneCache.Eval(boneNum), &skel->BasePoseMat); // DEST FIRST ARG
	retBasepose=&skel->BasePoseMat;
	retBaseposeInv=&skel->BasePoseMatInv;

	if (scale[0])
	{
		bolt.matrix[0][3] *= scale[0];
	}
	if (scale[1])
	{
		bolt.matrix[1][3] *= scale[1];
	}
	if (scale[2])
	{
		bolt.matrix[2][3] *= scale[2];
	}
	VectorNormalize((float*)&bolt.matrix[0]);
	VectorNormalize((float*)&bolt.matrix[1]);
	VectorNormalize((float*)&bolt.matrix[2]);

	Multiply_3x4Matrix(&retMatrix,&worldMatrix, &bolt);

#ifdef _DEBUG
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			assert( !Q_isnan(retMatrix.matrix[i][j]));
		}
	}
#endif// _DEBUG
}

int G2_GetParentBoneMatrixLow(CGhoul2Info &ghoul2,int boneNum,const vec3_t scale,mdxaBone_t &retMatrix,mdxaBone_t *&retBasepose,mdxaBone_t *&retBaseposeInv)
{
	int parent=-1;
	if (ghoul2.mBoneCache)
	{
		CBoneCache &boneCache=*ghoul2.mBoneCache;
		assert(boneCache.mod);
		assert(boneNum>=0&&boneNum<boneCache.header->numBones);
		parent=boneCache.GetParent(boneNum);
		if (parent<0||parent>=boneCache.header->numBones)
		{
			parent=-1;
			retMatrix=identityMatrix;
			// yikes
			retBasepose=const_cast<mdxaBone_t *>(&identityMatrix);
			retBaseposeInv=const_cast<mdxaBone_t *>(&identityMatrix);
		}
		else
		{
			G2_GetBoneMatrixLow(ghoul2,parent,scale,retMatrix,retBasepose,retBaseposeInv);
		}
	}
	return parent;
}
//rww - RAGDOLL_END

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

//======================================================================
//
// Bone Manipulation code


void G2_CreateQuaterion(mdxaBone_t *mat, vec4_t quat)
{
	// this is revised for the 3x4 matrix we use in G2.
    float t = 1 + mat->matrix[0][0] + mat->matrix[1][1] + mat->matrix[2][2];
	float s;

    //If the trace of the matrix is greater than zero, then
    //perform an "instant" calculation.
    //Important note wrt. rouning errors:
    //Test if ( T > 0.00000001 ) to avoid large distortions!
	if (t > 0.00000001)
	{
      s = sqrt(t) * 2;
      quat[0] = ( mat->matrix[1][2] - mat->matrix[2][1] ) / s;
      quat[1] = ( mat->matrix[2][0] - mat->matrix[0][2] ) / s;
      quat[2] = ( mat->matrix[0][1] - mat->matrix[1][0] ) / s;
      quat[3] = 0.25 * s;
	}
	else
	{
		//If the trace of the matrix is equal to zero then identify
		//which major diagonal element has the greatest value.

		//Depending on this, calculate the following:

		if ( mat->matrix[0][0] > mat->matrix[1][1] && mat->matrix[0][0] > mat->matrix[2][2] )  {	// Column 0:
			s  = sqrt( 1.0 + mat->matrix[0][0] - mat->matrix[1][1] - mat->matrix[2][2])* 2;
			quat[0] = 0.25 * s;
			quat[1] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[2] = (mat->matrix[2][0] + mat->matrix[0][2] ) / s;
			quat[3] = (mat->matrix[1][2] - mat->matrix[2][1] ) / s;

		} else if ( mat->matrix[1][1] > mat->matrix[2][2] ) {			// Column 1:
			s  = sqrt( 1.0 + mat->matrix[1][1] - mat->matrix[0][0] - mat->matrix[2][2] ) * 2;
			quat[0] = (mat->matrix[0][1] + mat->matrix[1][0] ) / s;
			quat[1] = 0.25 * s;
			quat[2] = (mat->matrix[1][2] + mat->matrix[2][1] ) / s;
			quat[3] = (mat->matrix[2][0] - mat->matrix[0][2] ) / s;

		} else {						// Column 2:
			s  = sqrt( 1.0 + mat->matrix[2][2] - mat->matrix[0][0] - mat->matrix[1][1] ) * 2;
			quat[0] = (mat->matrix[2][0]+ mat->matrix[0][2] ) / s;
			quat[1] = (mat->matrix[1][2] + mat->matrix[2][1] ) / s;
			quat[2] = 0.25 * s;
			quat[3] = (mat->matrix[0][1] - mat->matrix[1][0] ) / s;
		}
	}
}

void G2_CreateMatrixFromQuaterion(mdxaBone_t *mat, vec4_t quat)
{

    float xx      = quat[0] * quat[0];
    float xy      = quat[0] * quat[1];
    float xz      = quat[0] * quat[2];
    float xw      = quat[0] * quat[3];

    float yy      = quat[1] * quat[1];
    float yz      = quat[1] * quat[2];
    float yw      = quat[1] * quat[3];

    float zz      = quat[2] * quat[2];
    float zw      = quat[2] * quat[3];

    mat->matrix[0][0]  = 1 - 2 * ( yy + zz );
    mat->matrix[1][0]  =     2 * ( xy - zw );
    mat->matrix[2][0]  =     2 * ( xz + yw );

    mat->matrix[0][1]  =     2 * ( xy + zw );
    mat->matrix[1][1]  = 1 - 2 * ( xx + zz );
    mat->matrix[2][1]  =     2 * ( yz - xw );

    mat->matrix[0][2]  =     2 * ( xz - yw );
    mat->matrix[1][2]  =     2 * ( yz + xw );
    mat->matrix[2][2]  = 1 - 2 * ( xx + yy );

    mat->matrix[0][3]  = mat->matrix[1][3] = mat->matrix[2][3] = 0;
}

#define DEBUG_G2_TIMING (0)
#define DEBUG_G2_TIMING_RENDER_ONLY (1)

void G2_TimingModel(boneInfo_t &bone,int currentTime,int numFramesInFile,int &currentFrame,int &newFrame,float &lerp)
{
	assert(bone.startFrame>=0);
	assert(bone.startFrame<=numFramesInFile);
	assert(bone.endFrame>=0);
	assert(bone.endFrame<=numFramesInFile);

	// yes - add in animation speed to current frame
	float	animSpeed = bone.animSpeed;
	float	time;
	if (bone.pauseTime)
	{
		time = (bone.pauseTime - bone.startTime) / 50.0f;
	}
	else
	{
		time = (currentTime - bone.startTime) / 50.0f;
	}
	if (time<0.0f)
	{
		time=0.0f;
	}
	float	newFrame_g = bone.startFrame + (time * animSpeed);

	int		animSize = bone.endFrame - bone.startFrame;
	float	endFrame = (float)bone.endFrame;
	// we are supposed to be animating right?
	if (animSize)
	{
		// did we run off the end?
		if (((animSpeed > 0.0f) && (newFrame_g > endFrame - 1)) ||
			((animSpeed < 0.0f) && (newFrame_g < endFrame+1)))
		{
			// yep - decide what to do
			if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
			{
				// get our new animation frame back within the bounds of the animation set
				if (animSpeed < 0.0f)
				{
					// we don't use this case, or so I am told
					// if we do, let me know, I need to insure the mod works

					// should we be creating a virtual frame?
					if ((newFrame_g < endFrame+1) && (newFrame_g >= endFrame))
					{
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = float(endFrame+1)-newFrame_g;
						// frames are easy to calculate
						currentFrame = endFrame;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					else
					{
						if (newFrame_g <= endFrame+1)
						{
							newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
						}
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (ceil(newFrame_g)-newFrame_g);
						// frames are easy to calculate
						currentFrame = ceil(newFrame_g);
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						// should we be creating a virtual frame?
						if (currentFrame <= endFrame+1 )
						{
							newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						else
						{
							newFrame = currentFrame - 1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				else
				{
					// should we be creating a virtual frame?
					if ((newFrame_g > endFrame - 1) && (newFrame_g < endFrame))
					{
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (newFrame_g - (int)newFrame_g);
						// frames are easy to calculate
						currentFrame = (int)newFrame_g;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					else
					{
						if (newFrame_g >= endFrame)
						{
							newFrame_g=endFrame+fmod(newFrame_g-endFrame,animSize)-animSize;
						}
						// now figure out what we are lerping between
						// delta is the fraction between this frame and the next, since the new anim is always at a .0f;
						lerp = (newFrame_g - (int)newFrame_g);
						// frames are easy to calculate
						currentFrame = (int)newFrame_g;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
						// should we be creating a virtual frame?
						if (newFrame_g >= endFrame - 1)
						{
							newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						else
						{
							newFrame = currentFrame + 1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				// sanity check
				assert (((newFrame < endFrame) && (newFrame >= bone.startFrame)) || (animSize < 10));
			}
			else
			{
				if (((bone.flags & (BONE_ANIM_OVERRIDE_FREEZE)) == (BONE_ANIM_OVERRIDE_FREEZE)))
				{
					// if we are supposed to reset the default anim, then do so
					if (animSpeed > 0.0f)
					{
						currentFrame = bone.endFrame - 1;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
					}
					else
					{
						currentFrame = bone.endFrame+1;
						assert(currentFrame>=0&&currentFrame<numFramesInFile);
					}

					newFrame = currentFrame;
					assert(newFrame>=0&&newFrame<numFramesInFile);
					lerp = 0;
				}
				else
				{
					bone.flags &= ~(BONE_ANIM_TOTAL);
				}

			}
		}
		else
		{
			if (animSpeed> 0.0)
			{
				// frames are easy to calculate
				currentFrame = (int)newFrame_g;

				// figure out the difference between the two frames	- we have to decide what frame and what percentage of that
				// frame we want to display
				lerp = (newFrame_g - currentFrame);

				assert(currentFrame>=0&&currentFrame<numFramesInFile);

				newFrame = currentFrame + 1;
				// are we now on the end frame?
				assert((int)endFrame<=numFramesInFile);
				if (newFrame >= (int)endFrame)
				{
					// we only want to lerp with the first frame of the anim if we are looping
					if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
					{
					  	newFrame = bone.startFrame;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
					// if we intend to end this anim or freeze after this, then just keep on the last frame
					else
					{
						newFrame = bone.endFrame-1;
						assert(newFrame>=0&&newFrame<numFramesInFile);
					}
				}
				assert(newFrame>=0&&newFrame<numFramesInFile);
			}
			else
			{
				lerp = (ceil(newFrame_g)-newFrame_g);
				// frames are easy to calculate
				currentFrame = ceil(newFrame_g);
				if (currentFrame>bone.startFrame)
				{
					currentFrame=bone.startFrame;
					newFrame = currentFrame;
					lerp=0.0f;
				}
				else
				{
					newFrame=currentFrame-1;
					// are we now on the end frame?
					if (newFrame < endFrame+1)
					{
						// we only want to lerp with the first frame of the anim if we are looping
						if (bone.flags & BONE_ANIM_OVERRIDE_LOOP)
						{
					  		newFrame = bone.startFrame;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
						// if we intend to end this anim or freeze after this, then just keep on the last frame
						else
						{
							newFrame = bone.endFrame+1;
							assert(newFrame>=0&&newFrame<numFramesInFile);
						}
					}
				}
				assert(currentFrame>=0&&currentFrame<numFramesInFile);
				assert(newFrame>=0&&newFrame<numFramesInFile);
			}
		}
	}
	else
	{
		if (animSpeed<0.0)
		{
			currentFrame = bone.endFrame+1;
		}
		else
		{
			currentFrame = bone.endFrame-1;
		}
		if (currentFrame<0)
		{
			currentFrame=0;
		}
		assert(currentFrame>=0&&currentFrame<numFramesInFile);
		newFrame = currentFrame;
		assert(newFrame>=0&&newFrame<numFramesInFile);
		lerp = 0;

	}
	assert(currentFrame>=0&&currentFrame<numFramesInFile);
	assert(newFrame>=0&&newFrame<numFramesInFile);
	assert(lerp>=0.0f&&lerp<=1.0f);
}

#ifdef _RAG_PRINT_TEST
void G2_RagPrintMatrix(mdxaBone_t *mat);
#endif
//basically construct a seperate skeleton with full hierarchy to store a matrix
//off which will give us the desired settling position given the frame in the skeleton
//that should be used -rww
int G2_Add_Bone (const model_t *mod, boneInfo_v &blist, const char *boneName);
int G2_Find_Bone(const model_t *mod, boneInfo_v &blist, const char *boneName);

void G2_SetUpBolts( mdxaHeader_t *header, CGhoul2Info &ghoul2, mdxaBone_v &bonePtr, boltInfo_v &boltList)
{
	mdxaSkel_t		*skel;
	mdxaSkelOffsets_t *offsets;
	offsets = (mdxaSkelOffsets_t *)((byte *)header + sizeof(mdxaHeader_t));

	for (size_t i=0; i<boltList.size(); i++)
	{
		if (boltList[i].boneNumber != -1)
		{
			// figure out where the bone hirearchy info is
			skel = (mdxaSkel_t *)((byte *)header + sizeof(mdxaHeader_t) + offsets->offsets[boltList[i].boneNumber]);
			Multiply_3x4Matrix(&boltList[i].position, &bonePtr[boltList[i].boneNumber].second, &skel->BasePoseMat);
		}
	}
}

//rww - RAGDOLL_BEGIN
#define		GHOUL2_RAG_STARTED						0x0010
//rww - RAGDOLL_END
//rwwFIXMEFIXME: Move this into the stupid header or something.

void G2_TransformGhoulBones(boneInfo_v &rootBoneList,mdxaBone_t &rootMatrix, CGhoul2Info &ghoul2, int time,bool smooth)
{

	/*
	model_t			*currentModel;
	model_t			*animModel;
	mdxaHeader_t	*aHeader;

	//currentModel = R_GetModelByHandle(RE_RegisterModel(ghoul2.mFileName));
	currentModel = R_GetModelByHandle(ghoul2.mModel);
	assert(currentModel);
	assert(currentModel->mdxm);

	animModel =  R_GetModelByHandle(currentModel->mdxm->animIndex);
	assert(animModel);
	aHeader = animModel->mdxa;
	assert(aHeader);
	*/
	model_t			*currentModel = (model_t *)ghoul2.currentModel;
	mdxaHeader_t	*aHeader = (mdxaHeader_t *)ghoul2.aHeader;


	assert(ghoul2.aHeader);
	assert(ghoul2.currentModel);
	assert(ghoul2.currentModel->mdxm);
	if (!aHeader->numBones)
	{
		assert(0); // this would be strange
		return;
	}
	if (!ghoul2.mBoneCache)
	{
		ghoul2.mBoneCache=new CBoneCache(currentModel,aHeader);

#ifdef _FULL_G2_LEAK_CHECKING
		g_Ghoul2Allocations += sizeof(*ghoul2.mBoneCache);
#endif
	}
	ghoul2.mBoneCache->mod=currentModel;
	ghoul2.mBoneCache->header=aHeader;
	assert(ghoul2.mBoneCache->mBones.size()==(unsigned)aHeader->numBones);

	ghoul2.mBoneCache->mSmoothingActive=false;
	ghoul2.mBoneCache->mUnsquash=false;

	// master smoothing control
	if (g2_re.G2_HackadelicOnClient() && smooth && !g2_ri.Cvar_VariableIntegerValue( "dedicated" ))
	{
		ghoul2.mBoneCache->mLastTouch=ghoul2.mBoneCache->mLastLastTouch;
		/*
		float val=r_Ghoul2AnimSmooth->value;
		if (smooth&&val>0.0f&&val<1.0f)
		{
		//	if (HackadelicOnClient)
		//	{
				ghoul2.mBoneCache->mLastTouch=ghoul2.mBoneCache->mLastLastTouch;
		//	}

			ghoul2.mBoneCache->mSmoothFactor=val;
			ghoul2.mBoneCache->mSmoothingActive=true;
			if (r_Ghoul2UnSqashAfterSmooth->integer)
			{
				ghoul2.mBoneCache->mUnsquash=true;
			}
		}
		else
		{
			ghoul2.mBoneCache->mSmoothFactor=1.0f;
		}
		*/

		// master smoothing control
		float val=r_Ghoul2AnimSmooth->value;
		if (val>0.0f&&val<1.0f)
		{
			//if (ghoul2.mFlags&GHOUL2_RESERVED_FOR_RAGDOLL)
#if 1
			if(ghoul2.mFlags & GHOUL2_CRAZY_SMOOTH)
			{
				val = 0.9f;
			}
			else if(ghoul2.mFlags & GHOUL2_RAG_STARTED)
			{
				for (size_t k=0;k<rootBoneList.size();k++)
				{
					boneInfo_t &bone=rootBoneList[k];
					if (bone.flags&BONE_ANGLES_RAGDOLL)
					{
						if (bone.firstCollisionTime &&
							bone.firstCollisionTime>time-250 &&
							bone.firstCollisionTime<time)
						{
							val=0.9f;//(val+0.8f)/2.0f;
						}
						else if (bone.airTime > time)
						{
							val = 0.2f;
						}
						else
						{
							val = 0.8f;
						}
						break;
					}
				}
			}
#endif

//			ghoul2.mBoneCache->mSmoothFactor=(val + 1.0f-pow(1.0f-val,50.0f/dif))/2.0f;  // meaningless formula
			ghoul2.mBoneCache->mSmoothFactor=val;  // meaningless formula
			ghoul2.mBoneCache->mSmoothingActive=true;

			if (r_Ghoul2UnSqashAfterSmooth->integer)
			{
				ghoul2.mBoneCache->mUnsquash=true;
			}
		}
	}
	else
	{
		ghoul2.mBoneCache->mSmoothFactor=1.0f;
	}

	ghoul2.mBoneCache->mCurrentTouch++;

//rww - RAGDOLL_BEGIN
	if (g2_re.G2_HackadelicOnClient())
	{
		ghoul2.mBoneCache->mLastLastTouch=ghoul2.mBoneCache->mCurrentTouch;
		ghoul2.mBoneCache->mCurrentTouchRender=ghoul2.mBoneCache->mCurrentTouch;
	}
	else
	{
		ghoul2.mBoneCache->mCurrentTouchRender=0;
	}
//rww - RAGDOLL_END

	ghoul2.mBoneCache->frameSize = 0;// can be deleted in new G2 format	//(size_t)( &((mdxaFrame_t *)0)->boneIndexes[ ghoul2.aHeader->numBones ] );

	ghoul2.mBoneCache->rootBoneList=&rootBoneList;
	ghoul2.mBoneCache->rootMatrix=rootMatrix;
	ghoul2.mBoneCache->incomingTime=time;

	SBoneCalc &TB=ghoul2.mBoneCache->Root();
	TB.newFrame=0;
	TB.currentFrame=0;
	TB.backlerp=0.0f;
	TB.blendFrame=0;
	TB.blendOldFrame=0;
	TB.blendMode=false;
	TB.blendLerp=0;
}


#define MDX_TAG_ORIGIN 2

//======================================================================
//
// Surface Manipulation code


// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt(mdxaBone_v &bonePtr, mdxmSurface_t *surface, int boltNum, boltInfo_v &boltList, surfaceInfo_t *surfInfo, model_t *mod)
{
 	mdxmVertex_t 	*v, *vert0, *vert1, *vert2;
 	matrix3_t		axes, sides;
 	float			pTri[3][3], d;
 	int				j, k;

	// now there are two types of tag surface - model ones and procedural generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int	polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t	*originalSurf = (mdxmSurface_t *)G2_FindSurface(mod, surfNumber, surfInfo->genLod);
		mdxmTriangle_t	*originalTriangleIndexes = (mdxmTriangle_t *)((byte*)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are

 		vert0 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert0+= index0;

 		vert1 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert1+= index1;

 		vert2 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert2+= index2;

		// clear out the triangle verts to be
 	   	VectorClear( pTri[0] );
 	   	VectorClear( pTri[1] );
 	   	VectorClear( pTri[2] );

//		mdxmWeight_t	*w;

		int *piBoneRefs = (int*) ((byte*)originalSurf + originalSurf->ofsBoneReferences);

		// now go and transform just the points we need from the surface that was hit originally
//		w = vert0->weights;
		float fTotalWeight = 0.0f;
		int iNumWeights = G2_GetVertWeights( vert0 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert0, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert0, k, fTotalWeight, iNumWeights );

 			pTri[0][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[0][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[0][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert0->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}
//		w = vert1->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert1 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert1, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert1, k, fTotalWeight, iNumWeights );

 			pTri[1][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[1][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[1][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert1->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}
//		w = vert2->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert2 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert2, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert2, k, fTotalWeight, iNumWeights );

 			pTri[2][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 			pTri[2][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 			pTri[2][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], vert2->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
		}

   		vec3_t normal;
		vec3_t up;
		vec3_t right;
		vec3_t vec0, vec1;
		// work out baryCentricK
		float baryCentricK = 1.0 - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		boltList[boltNum].position.matrix[0][3] = (pTri[0][0] * surfInfo->genBarycentricI) + (pTri[1][0] * surfInfo->genBarycentricJ) + (pTri[2][0] * baryCentricK);
		boltList[boltNum].position.matrix[1][3] = (pTri[0][1] * surfInfo->genBarycentricI) + (pTri[1][1] * surfInfo->genBarycentricJ) + (pTri[2][1] * baryCentricK);
		boltList[boltNum].position.matrix[2][3] = (pTri[0][2] * surfInfo->genBarycentricI) + (pTri[1][2] * surfInfo->genBarycentricJ) + (pTri[2][2] * baryCentricK);

		// generate a normal to this new triangle
		VectorSubtract(pTri[0], pTri[1], vec0);
		VectorSubtract(pTri[2], pTri[1], vec1);

		CrossProduct(vec0, vec1, normal);
		VectorNormalize(normal);

		// forward vector
		boltList[boltNum].position.matrix[0][0] = normal[0];
		boltList[boltNum].position.matrix[1][0] = normal[1];
		boltList[boltNum].position.matrix[2][0] = normal[2];

		// up will be towards point 0 of the original triangle.
		// so lets work it out. Vector is hit point - point 0
		up[0] = boltList[boltNum].position.matrix[0][3] - pTri[0][0];
		up[1] = boltList[boltNum].position.matrix[1][3] - pTri[0][1];
		up[2] = boltList[boltNum].position.matrix[2][3] - pTri[0][2];

		// normalise it
		VectorNormalize(up);

		// that's the up vector
		boltList[boltNum].position.matrix[0][1] = up[0];
		boltList[boltNum].position.matrix[1][1] = up[1];
		boltList[boltNum].position.matrix[2][1] = up[2];

		// right is always straight

		CrossProduct( normal, up, right );
		// that's the up vector
		boltList[boltNum].position.matrix[0][2] = right[0];
		boltList[boltNum].position.matrix[1][2] = right[1];
		boltList[boltNum].position.matrix[2][2] = right[2];


	}
	// no, we are looking at a normal model tag
	else
	{
		int *piBoneRefs = (int*) ((byte*)surface + surface->ofsBoneReferences);

	 	// whip through and actually transform each vertex
 		v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
 		for ( j = 0; j < 3; j++ )
 		{
// 			mdxmWeight_t	*w;

 			VectorClear( pTri[j] );
 //			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );
			float fTotalWeight = 0.0f;
 			for ( k = 0 ; k < iNumWeights ; k++ )
 			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

 				//bone = bonePtr + piBoneRefs[w->boneIndex];
 				pTri[j][0] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[0][3] );
 				pTri[j][1] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[1][3] );
 				pTri[j][2] += fBoneWeight * ( DotProduct( bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2], v->vertCoords ) + bonePtr[piBoneRefs[iBoneIndex]].second.matrix[2][3] );
 			}

 			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
 		}

 		// clear out used arrays
 		memset( axes, 0, sizeof( axes ) );
 		memset( sides, 0, sizeof( sides ) );

 		// work out actual sides of the tag triangle
 		for ( j = 0; j < 3; j++ )
 		{
 			sides[j][0] = pTri[(j+1)%3][0] - pTri[j][0];
 			sides[j][1] = pTri[(j+1)%3][1] - pTri[j][1];
 			sides[j][2] = pTri[(j+1)%3][2] - pTri[j][2];
 		}

 		// do math trig to work out what the matrix will be from this triangle's translated position
 		VectorNormalize2( sides[iG2_TRISIDE_LONGEST], axes[0] );
 		VectorNormalize2( sides[iG2_TRISIDE_SHORTEST], axes[1] );

 		// project shortest side so that it is exactly 90 degrees to the longer side
 		d = DotProduct( axes[0], axes[1] );
 		VectorMA( axes[0], -d, axes[1], axes[0] );
 		VectorNormalize2( axes[0], axes[0] );

 		CrossProduct( sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2] );
 		VectorNormalize2( axes[2], axes[2] );

 		// set up location in world space of the origin point in out going matrix
 		boltList[boltNum].position.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 		boltList[boltNum].position.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 		boltList[boltNum].position.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

 		// copy axis to matrix - do some magic to orient minus Y to positive X and so on so bolt on stuff is oriented correctly
		boltList[boltNum].position.matrix[0][0] = axes[1][0];
		boltList[boltNum].position.matrix[0][1] = axes[0][0];
		boltList[boltNum].position.matrix[0][2] = -axes[2][0];

		boltList[boltNum].position.matrix[1][0] = axes[1][1];
		boltList[boltNum].position.matrix[1][1] = axes[0][1];
		boltList[boltNum].position.matrix[1][2] = -axes[2][1];

		boltList[boltNum].position.matrix[2][0] = axes[1][2];
		boltList[boltNum].position.matrix[2][1] = axes[0][2];
		boltList[boltNum].position.matrix[2][2] = -axes[2][2];
	}

}


// now go through all the generated surfaces that aren't included in the model surface hierarchy and create the correct bolt info for them
void G2_ProcessGeneratedSurfaceBolts(CGhoul2Info &ghoul2, mdxaBone_v &bonePtr, model_t *mod_t)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.Start();
#endif
	// look through the surfaces off the end of the pre-defined model surfaces
	for (size_t i=0; i< ghoul2.mSlist.size(); i++)
	{
		// only look for bolts if we are actually a generated surface, and not just an overriden one
		if (ghoul2.mSlist[i].offFlags & G2SURFACEFLAG_GENERATED)
		{
	   		// well alrighty then. Lets see if there is a bolt that is attempting to use it
			int boltNum = G2_Find_Bolt_Surface_Num(ghoul2.mBltlist, i, G2SURFACEFLAG_GENERATED);
			// yes - ok, processing time.
			if (boltNum != -1)
			{
				G2_ProcessSurfaceBolt(bonePtr, NULL, boltNum, ghoul2.mBltlist, &ghoul2.mSlist[i], mod_t);
			}
		}
	}
#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ProcessGeneratedSurfaceBolts += G2PerformanceTimer_G2_ProcessGeneratedSurfaceBolts.End();
#endif
}

// Go through the model and deal with just the surfaces that are tagged as bolt on points - this is for the server side skeleton construction
void ProcessModelBoltSurfaces(int surfaceNum, surfaceInfo_v &rootSList,
					mdxaBone_v &bonePtr, model_t *currentModel, int lod, boltInfo_v &boltList)
{
	int			i;
	int			offFlags = 0;

	// back track and get the surfinfo struct for this surface
	mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)currentModel, surfaceNum, 0);
	mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)currentModel->mdxm + sizeof(mdxmHeader_t));
	mdxmSurfHierarchy_t		*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);

	// see if we have an override surface in the surface list
	surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(surfaceNum, rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// is this surface considered a bolt surface?
	if (surfInfo->flags & G2SURFACEFLAG_ISBOLT)
	{
		// well alrighty then. Lets see if there is a bolt that is attempting to use it
		int boltNum = G2_Find_Bolt_Surface_Num(boltList, surfaceNum, 0);
		// yes - ok, processing time.
		if (boltNum != -1)
		{
			G2_ProcessSurfaceBolt(bonePtr, surface, boltNum, boltList, surfOverride, currentModel);
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
		ProcessModelBoltSurfaces(surfInfo->childIndexes[i], rootSList, bonePtr, currentModel, lod, boltList);
	}
}


// build the used bone list so when doing bone transforms we can determine if we need to do it or not
void G2_ConstructUsedBoneList(CConstructBoneList &CBL)
{
	int	 		i, j;
	int			offFlags = 0;

	// back track and get the surfinfo struct for this surface
	const mdxmSurface_t			*surface = (mdxmSurface_t *)G2_FindSurface((void *)CBL.currentModel, CBL.surfaceNum, 0);
	const mdxmHierarchyOffsets_t	*surfIndexes = (mdxmHierarchyOffsets_t *)((byte *)CBL.currentModel->mdxm + sizeof(mdxmHeader_t));
	const mdxmSurfHierarchy_t	*surfInfo = (mdxmSurfHierarchy_t *)((byte *)surfIndexes + surfIndexes->offsets[surface->thisSurfaceIndex]);
	const model_t				*mod_a = g2_re.GetModelByHandle(CBL.currentModel->mdxm->animIndex);
	const mdxaSkelOffsets_t		*offsets = (mdxaSkelOffsets_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t));
	const mdxaSkel_t			*skel, *childSkel;

	// see if we have an override surface in the surface list
	const surfaceInfo_t	*surfOverride = G2_FindOverrideSurface(CBL.surfaceNum, CBL.rootSList);

	// really, we should use the default flags for this surface unless it's been overriden
	offFlags = surfInfo->flags;

	// set the off flags if we have some
	if (surfOverride)
	{
		offFlags = surfOverride->offFlags;
	}

	// if this surface is not off, add it to the shader render list
	if (!(offFlags & G2SURFACEFLAG_OFF))
	{
		int	*bonesReferenced = (int *)((byte*)surface + surface->ofsBoneReferences);
		// now whip through the bones this surface uses
		for (i=0; i<surface->numBoneReferences;i++)
		{
			int iBoneIndex = bonesReferenced[i];
			CBL.boneUsedList[iBoneIndex] = 1;

			// now go and check all the descendant bones attached to this bone and see if any have the always flag on them. If so, activate them
 			skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iBoneIndex]);

			// for every child bone...
			for (j=0; j< skel->numChildren; j++)
			{
				// get the skel data struct for each child bone of the referenced bone
 				childSkel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[skel->children[j]]);

				// does it have the always on flag on?
				if (childSkel->flags & G2BONEFLAG_ALWAYSXFORM)
				{
					// yes, make sure it's in the list of bones to be transformed.
					CBL.boneUsedList[skel->children[j]] = 1;
				}
			}

			// now we need to ensure that the parents of this bone are actually active...
			//
			int iParentBone = skel->parent;
			while (iParentBone != -1)
			{
				if (CBL.boneUsedList[iParentBone])	// no need to go higher
					break;
				CBL.boneUsedList[iParentBone] = 1;
				skel = (mdxaSkel_t *)((byte *)mod_a->mdxa + sizeof(mdxaHeader_t) + offsets->offsets[iParentBone]);
				iParentBone = skel->parent;
			}
		}
	}
 	else
	// if we are turning off all descendants, then stop this recursion now
	if (offFlags & G2SURFACEFLAG_NODESCENDANTS)
	{
		return;
	}

	// now recursively call for the children
	for (i=0; i< surfInfo->numChildren; i++)
	{
		CBL.surfaceNum = surfInfo->childIndexes[i];
		G2_ConstructUsedBoneList(CBL);
	}
}


// sort all the ghoul models in this list so if they go in reference order. This will ensure the bolt on's are attached to the right place
// on the previous model, since it ensures the model being attached to is built and rendered first.

// NOTE!! This assumes at least one model will NOT have a parent. If it does - we are screwed
void G2_Sort_Models(CGhoul2Info_v &ghoul2, int * const modelList, int * const modelCount)
{
	int		startPoint, endPoint;
	int		i, boltTo, j;

	*modelCount = 0;

	// first walk all the possible ghoul2 models, and stuff the out array with those with no parents
	for (i=0; i<ghoul2.size();i++)
	{
		// have a ghoul model here?
		if (ghoul2[i].mModelindex == -1)
		{
			continue;
		}

		if (!ghoul2[i].mValid)
		{
			continue;
		}

		// are we attached to anything?
		if (ghoul2[i].mModelBoltLink == -1)
		{
			// no, insert us first
			modelList[(*modelCount)++] = i;
	 	}
	}

	startPoint = 0;
	endPoint = *modelCount;

	// now, using that list of parentless models, walk the descendant tree for each of them, inserting the descendents in the list
	while (startPoint != endPoint)
	{
		for (i=0; i<ghoul2.size(); i++)
		{
			// have a ghoul model here?
			if (ghoul2[i].mModelindex == -1)
			{
				continue;
			}

			if (!ghoul2[i].mValid)
			{
				continue;
			}

			// what does this model think it's attached to?
			if (ghoul2[i].mModelBoltLink != -1)
			{
				boltTo = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				// is it any of the models we just added to the list?
				for (j=startPoint; j<endPoint; j++)
				{
					// is this my parent model?
					if (boltTo == modelList[j])
					{
						// yes, insert into list and exit now
						modelList[(*modelCount)++] = i;
						break;
					}
				}
			}
		}
		// update start and end points
		startPoint = endPoint;
		endPoint = *modelCount;
	}
}

void *G2_FindSurface_BC(const model_s *mod, int index, int lod)
{
	assert(mod);
	assert(mod->mdxm);

	// point at first lod list
	byte	*current = (byte*)((intptr_t)mod->mdxm + (intptr_t)mod->mdxm->ofsLODs);
	int i;

	//walk the lods
	assert(lod>=0&&lod<mod->mdxm->numLODs);
	for (i=0; i<lod; i++)
	{
		mdxmLOD_t *lodData = (mdxmLOD_t *)current;
		current += lodData->ofsEnd;
	}

	// avoid the lod pointer data structure
	current += sizeof(mdxmLOD_t);

	mdxmLODSurfOffset_t *indexes = (mdxmLODSurfOffset_t *)current;
	// we are now looking at the offset array
	assert(index>=0&&index<mod->mdxm->numSurfaces);
	current += indexes->offsets[index];

	return (void *)current;
}

//#define G2EVALRENDER

// We've come across a surface that's designated as a bolt surface, process it and put it in the appropriate bolt place
void G2_ProcessSurfaceBolt2(CBoneCache &boneCache, const mdxmSurface_t *surface, int boltNum, boltInfo_v &boltList, const surfaceInfo_t *surfInfo, const model_t *mod,mdxaBone_t &retMatrix)
{
 	mdxmVertex_t 	*v, *vert0, *vert1, *vert2;
 	matrix3_t		axes, sides;
 	float			pTri[3][3], d;
 	int				j, k;

	// now there are two types of tag surface - model ones and procedural generated types - lets decide which one we have here.
	if (surfInfo && surfInfo->offFlags == G2SURFACEFLAG_GENERATED)
	{
		int surfNumber = surfInfo->genPolySurfaceIndex & 0x0ffff;
		int	polyNumber = (surfInfo->genPolySurfaceIndex >> 16) & 0x0ffff;

		// find original surface our original poly was in.
		mdxmSurface_t	*originalSurf = (mdxmSurface_t *)G2_FindSurface_BC(mod, surfNumber, surfInfo->genLod);
		mdxmTriangle_t	*originalTriangleIndexes = (mdxmTriangle_t *)((byte*)originalSurf + originalSurf->ofsTriangles);

		// get the original polys indexes
		int index0 = originalTriangleIndexes[polyNumber].indexes[0];
		int index1 = originalTriangleIndexes[polyNumber].indexes[1];
		int index2 = originalTriangleIndexes[polyNumber].indexes[2];

		// decide where the original verts are
 		vert0 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert0+=index0;

		vert1 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert1+=index1;

		vert2 = (mdxmVertex_t *) ((byte *)originalSurf + originalSurf->ofsVerts);
		vert2+=index2;

		// clear out the triangle verts to be
 	   	VectorClear( pTri[0] );
 	   	VectorClear( pTri[1] );
 	   	VectorClear( pTri[2] );
		int *piBoneReferences = (int*) ((byte*)originalSurf + originalSurf->ofsBoneReferences);

//		mdxmWeight_t	*w;

		// now go and transform just the points we need from the surface that was hit originally
//		w = vert0->weights;
		float fTotalWeight = 0.0f;
		int iNumWeights = G2_GetVertWeights( vert0 );
 		for ( k = 0 ; k < iNumWeights ; k++ )
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert0, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert0, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

			pTri[0][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert0->vertCoords ) + bone.matrix[0][3] );
 			pTri[0][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert0->vertCoords ) + bone.matrix[1][3] );
 			pTri[0][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert0->vertCoords ) + bone.matrix[2][3] );
		}

//		w = vert1->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert1 );
 		for ( k = 0 ; k < iNumWeights ; k++)
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert1, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert1, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 			pTri[1][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert1->vertCoords ) + bone.matrix[0][3] );
 			pTri[1][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert1->vertCoords ) + bone.matrix[1][3] );
 			pTri[1][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert1->vertCoords ) + bone.matrix[2][3] );
		}

//		w = vert2->weights;
		fTotalWeight = 0.0f;
		iNumWeights = G2_GetVertWeights( vert2 );
 		for ( k = 0 ; k < iNumWeights ; k++)
 		{
			int		iBoneIndex	= G2_GetVertBoneIndex( vert2, k );
			float	fBoneWeight	= G2_GetVertBoneWeight( vert2, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
			const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
			const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 			pTri[2][0] += fBoneWeight * ( DotProduct( bone.matrix[0], vert2->vertCoords ) + bone.matrix[0][3] );
 			pTri[2][1] += fBoneWeight * ( DotProduct( bone.matrix[1], vert2->vertCoords ) + bone.matrix[1][3] );
 			pTri[2][2] += fBoneWeight * ( DotProduct( bone.matrix[2], vert2->vertCoords ) + bone.matrix[2][3] );
		}

   		vec3_t normal;
		vec3_t up;
		vec3_t right;
		vec3_t vec0, vec1;
		// work out baryCentricK
		float baryCentricK = 1.0 - (surfInfo->genBarycentricI + surfInfo->genBarycentricJ);

		// now we have the model transformed into model space, now generate an origin.
		retMatrix.matrix[0][3] = (pTri[0][0] * surfInfo->genBarycentricI) + (pTri[1][0] * surfInfo->genBarycentricJ) + (pTri[2][0] * baryCentricK);
		retMatrix.matrix[1][3] = (pTri[0][1] * surfInfo->genBarycentricI) + (pTri[1][1] * surfInfo->genBarycentricJ) + (pTri[2][1] * baryCentricK);
		retMatrix.matrix[2][3] = (pTri[0][2] * surfInfo->genBarycentricI) + (pTri[1][2] * surfInfo->genBarycentricJ) + (pTri[2][2] * baryCentricK);

		// generate a normal to this new triangle
		VectorSubtract(pTri[0], pTri[1], vec0);
		VectorSubtract(pTri[2], pTri[1], vec1);

		CrossProduct(vec0, vec1, normal);
		VectorNormalize(normal);

		// forward vector
		retMatrix.matrix[0][0] = normal[0];
		retMatrix.matrix[1][0] = normal[1];
		retMatrix.matrix[2][0] = normal[2];

		// up will be towards point 0 of the original triangle.
		// so lets work it out. Vector is hit point - point 0
		up[0] = retMatrix.matrix[0][3] - pTri[0][0];
		up[1] = retMatrix.matrix[1][3] - pTri[0][1];
		up[2] = retMatrix.matrix[2][3] - pTri[0][2];

		// normalise it
		VectorNormalize(up);

		// that's the up vector
		retMatrix.matrix[0][1] = up[0];
		retMatrix.matrix[1][1] = up[1];
		retMatrix.matrix[2][1] = up[2];

		// right is always straight

		CrossProduct( normal, up, right );
		// that's the up vector
		retMatrix.matrix[0][2] = right[0];
		retMatrix.matrix[1][2] = right[1];
		retMatrix.matrix[2][2] = right[2];


	}
	// no, we are looking at a normal model tag
	else
	{
	 	// whip through and actually transform each vertex
 		v = (mdxmVertex_t *) ((byte *)surface + surface->ofsVerts);
		int *piBoneReferences = (int*) ((byte*)surface + surface->ofsBoneReferences);
 		for ( j = 0; j < 3; j++ )
 		{
// 			mdxmWeight_t	*w;

 			VectorClear( pTri[j] );
 //			w = v->weights;

			const int iNumWeights = G2_GetVertWeights( v );

			float fTotalWeight = 0.0f;
 			for ( k = 0 ; k < iNumWeights ; k++)
 			{
				int		iBoneIndex	= G2_GetVertBoneIndex( v, k );
				float	fBoneWeight	= G2_GetVertBoneWeight( v, k, fTotalWeight, iNumWeights );

#ifdef G2EVALRENDER
				const mdxaBone_t &bone=boneCache.EvalRender(piBoneReferences[iBoneIndex]);
#else
				const mdxaBone_t &bone=boneCache.Eval(piBoneReferences[iBoneIndex]);
#endif

 				pTri[j][0] += fBoneWeight * ( DotProduct( bone.matrix[0], v->vertCoords ) + bone.matrix[0][3] );
 				pTri[j][1] += fBoneWeight * ( DotProduct( bone.matrix[1], v->vertCoords ) + bone.matrix[1][3] );
 				pTri[j][2] += fBoneWeight * ( DotProduct( bone.matrix[2], v->vertCoords ) + bone.matrix[2][3] );
 			}

 			v++;// = (mdxmVertex_t *)&v->weights[/*v->numWeights*/surface->maxVertBoneWeights];
 		}

 		// clear out used arrays
 		memset( axes, 0, sizeof( axes ) );
 		memset( sides, 0, sizeof( sides ) );

 		// work out actual sides of the tag triangle
 		for ( j = 0; j < 3; j++ )
 		{
 			sides[j][0] = pTri[(j+1)%3][0] - pTri[j][0];
 			sides[j][1] = pTri[(j+1)%3][1] - pTri[j][1];
 			sides[j][2] = pTri[(j+1)%3][2] - pTri[j][2];
 		}

 		// do math trig to work out what the matrix will be from this triangle's translated position
 		VectorNormalize2( sides[iG2_TRISIDE_LONGEST], axes[0] );
 		VectorNormalize2( sides[iG2_TRISIDE_SHORTEST], axes[1] );

 		// project shortest side so that it is exactly 90 degrees to the longer side
 		d = DotProduct( axes[0], axes[1] );
 		VectorMA( axes[0], -d, axes[1], axes[0] );
 		VectorNormalize2( axes[0], axes[0] );

 		CrossProduct( sides[iG2_TRISIDE_LONGEST], sides[iG2_TRISIDE_SHORTEST], axes[2] );
 		VectorNormalize2( axes[2], axes[2] );

 		// set up location in world space of the origin point in out going matrix
 		retMatrix.matrix[0][3] = pTri[MDX_TAG_ORIGIN][0];
 		retMatrix.matrix[1][3] = pTri[MDX_TAG_ORIGIN][1];
 		retMatrix.matrix[2][3] = pTri[MDX_TAG_ORIGIN][2];

 		// copy axis to matrix - do some magic to orient minus Y to positive X and so on so bolt on stuff is oriented correctly
		retMatrix.matrix[0][0] = axes[1][0];
		retMatrix.matrix[0][1] = axes[0][0];
		retMatrix.matrix[0][2] = -axes[2][0];

		retMatrix.matrix[1][0] = axes[1][1];
		retMatrix.matrix[1][1] = axes[0][1];
		retMatrix.matrix[1][2] = -axes[2][1];

		retMatrix.matrix[2][0] = axes[1][2];
		retMatrix.matrix[2][1] = axes[0][2];
		retMatrix.matrix[2][2] = -axes[2][2];
	}

}

void G2_GetBoltMatrixLow(CGhoul2Info &ghoul2,int boltNum,const vec3_t scale,mdxaBone_t &retMatrix)
{
	if (!ghoul2.mBoneCache)
	{
		retMatrix=identityMatrix;
		return;
	}
	assert(ghoul2.mBoneCache);
	CBoneCache &boneCache=*ghoul2.mBoneCache;
	assert(boneCache.mod);
	boltInfo_v &boltList=ghoul2.mBltlist;

	//Raz: This was causing a client crash when rendering a model with no valid g2 bolts, such as Ragnos =]
	if ( boltList.size() < 1 ) {
		retMatrix=identityMatrix;
		return;
	}

	assert(boltNum>=0&&boltNum<(int)boltList.size());
#if 0 //rwwFIXMEFIXME: Disable this before release!!!!!! I am just trying to find a crash bug.
	if (boltNum < 0 || boltNum >= boltList.size())
	{
		char fName[MAX_QPATH];
		char mName[MAX_QPATH];
		int bLink = ghoul2.mModelBoltLink;

		if (ghoul2.currentModel)
		{
			strcpy(mName, ghoul2.currentModel->name);
		}
		else
		{
			strcpy(mName, "NULL!");
		}

		if (ghoul2.mFileName && ghoul2.mFileName[0])
		{
			strcpy(fName, ghoul2.mFileName);
		}
		else
		{
			strcpy(fName, "None?!");
		}

		Com_Error(ERR_DROP, "Write down or save this error message, show it to Rich\nBad bolt index on model %s (filename %s), index %i boltlink %i\n", mName, fName, boltNum, bLink);
	}
#endif
	if (boltList[boltNum].boneNumber>=0)
	{
		mdxaSkel_t		*skel;
		mdxaSkelOffsets_t *offsets;
		offsets = (mdxaSkelOffsets_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t));
		skel = (mdxaSkel_t *)((byte *)boneCache.header + sizeof(mdxaHeader_t) + offsets->offsets[boltList[boltNum].boneNumber]);
		Multiply_3x4Matrix(&retMatrix, (mdxaBone_t *)&boneCache.EvalUnsmooth(boltList[boltNum].boneNumber), &skel->BasePoseMat);
	}
	else if (boltList[boltNum].surfaceNumber>=0)
	{
		const surfaceInfo_t *surfInfo=0;
		{
			for (size_t i=0;i<ghoul2.mSlist.size();i++)
			{
				surfaceInfo_t &t=ghoul2.mSlist[i];
				if (t.surface==boltList[boltNum].surfaceNumber)
				{
					surfInfo=&t;
				}
			}
		}
		mdxmSurface_t *surface = 0;
		if (!surfInfo)
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(boneCache.mod,boltList[boltNum].surfaceNumber, 0);
		}
		if (!surface&&surfInfo&&surfInfo->surface<10000)
		{
			surface = (mdxmSurface_t *)G2_FindSurface_BC(boneCache.mod,surfInfo->surface, 0);
		}
		G2_ProcessSurfaceBolt2(boneCache,surface,boltNum,boltList,surfInfo,(model_t *)boneCache.mod,retMatrix);
	}
	else
	{
		 // we have a bolt without a bone or surface, not a huge problem but we ought to at least clear the bolt matrix
		retMatrix=identityMatrix;
	}
}

void G2_RootMatrix(CGhoul2Info_v &ghoul2,int time,const vec3_t scale,mdxaBone_t &retMatrix)
{
	int i;
	for (i=0; i<ghoul2.size(); i++)
	{
		if (ghoul2[i].mModelindex != -1 && ghoul2[i].mValid)
		{
			if (ghoul2[i].mFlags & GHOUL2_NEWORIGIN)
			{
				mdxaBone_t bolt;
				mdxaBone_t		tempMatrix;

				G2_ConstructGhoulSkeleton(ghoul2,time,false,scale);
				G2_GetBoltMatrixLow(ghoul2[i],ghoul2[i].mNewOrigin,scale,bolt);
				tempMatrix.matrix[0][0]=1.0f;
				tempMatrix.matrix[0][1]=0.0f;
				tempMatrix.matrix[0][2]=0.0f;
				tempMatrix.matrix[0][3]=-bolt.matrix[0][3];
				tempMatrix.matrix[1][0]=0.0f;
				tempMatrix.matrix[1][1]=1.0f;
				tempMatrix.matrix[1][2]=0.0f;
				tempMatrix.matrix[1][3]=-bolt.matrix[1][3];
				tempMatrix.matrix[2][0]=0.0f;
				tempMatrix.matrix[2][1]=0.0f;
				tempMatrix.matrix[2][2]=1.0f;
				tempMatrix.matrix[2][3]=-bolt.matrix[2][3];
//				Inverse_Matrix(&bolt, &tempMatrix);
				Multiply_3x4Matrix(&retMatrix, &tempMatrix, (mdxaBone_t*)&identityMatrix);
				return;
			}
		}
	}
	retMatrix=identityMatrix;
}

#ifdef _G2_LISTEN_SERVER_OPT
qboolean G2API_OverrideServerWithClientData(CGhoul2Info *serverInstance);
#endif

bool G2_NeedsRecalc(CGhoul2Info *ghlInfo,int frameNum)
{
	G2_SetupModelPointers(ghlInfo);
	// not sure if I still need this test, probably
	if (ghlInfo->mSkelFrameNum!=frameNum||
		!ghlInfo->mBoneCache||
		ghlInfo->mBoneCache->mod!=ghlInfo->currentModel)
	{
#ifdef _G2_LISTEN_SERVER_OPT
		if (ghlInfo->entityNum != ENTITYNUM_NONE &&
			G2API_OverrideServerWithClientData(ghlInfo))
		{ //if we can manage this, then we don't have to reconstruct
			return false;
		}
#endif
		ghlInfo->mSkelFrameNum=frameNum;
		return true;
	}
	return false;
}

/*
==============
G2_ConstructGhoulSkeleton - builds a complete skeleton for all ghoul models in a CGhoul2Info_v class	- using LOD 0
==============
*/
void G2_ConstructGhoulSkeleton( CGhoul2Info_v &ghoul2,const int frameNum,bool checkForNewOrigin,const vec3_t scale)
{
#ifdef G2_PERFORMANCE_ANALYSIS
	G2PerformanceTimer_G2_ConstructGhoulSkeleton.Start();
#endif
	int				i, j;
	int				modelCount;
	mdxaBone_t		rootMatrix;

	int modelList[256];
	assert(ghoul2.size()<=255);
	modelList[255]=548;

	if (checkForNewOrigin)
	{
		G2_RootMatrix(ghoul2,frameNum,scale,rootMatrix);
	}
	else
	{
		rootMatrix = identityMatrix;
	}

	G2_Sort_Models(ghoul2, modelList, &modelCount);
	assert(modelList[255]==548);

	for (j=0; j<modelCount; j++)
	{
		// get the sorted model to play with
		i = modelList[j];

		if (ghoul2[i].mValid)
		{
			if (j&&ghoul2[i].mModelBoltLink != -1)
			{
				int	boltMod = (ghoul2[i].mModelBoltLink >> MODEL_SHIFT) & MODEL_AND;
				int	boltNum = (ghoul2[i].mModelBoltLink >> BOLT_SHIFT) & BOLT_AND;

				mdxaBone_t bolt;
				G2_GetBoltMatrixLow(ghoul2[boltMod],boltNum,scale,bolt);
				G2_TransformGhoulBones(ghoul2[i].mBlist,bolt,ghoul2[i],frameNum,checkForNewOrigin);
			}
#ifdef _G2_LISTEN_SERVER_OPT
			else if (ghoul2[i].entityNum == ENTITYNUM_NONE || ghoul2[i].mSkelFrameNum != frameNum)
#else
			else
#endif
			{
				G2_TransformGhoulBones(ghoul2[i].mBlist,rootMatrix,ghoul2[i],frameNum,checkForNewOrigin);
			}
		}
	}
#ifdef G2_PERFORMANCE_ANALYSIS
	G2Time_G2_ConstructGhoulSkeleton += G2PerformanceTimer_G2_ConstructGhoulSkeleton.End();
#endif
}
