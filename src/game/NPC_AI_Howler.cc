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

#include "b_local.hh"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1
#define LSTATE_FLEE			2
#define LSTATE_BERZERK		3

#define HOWLER_RETREAT_DIST	300.0f
#define HOWLER_PANIC_HEALTH	10

extern qboolean PM_InKnockDown( playerState_t *ps );
extern int PM_AnimLength( int index, animNumber_t anim );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex );
extern void G_TossTheMofo(gentity_t *ent, vec3_t tossDir, float tossStr);
extern qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern qboolean NPC_TryJumpToEnt(gentity_t *goal, float max_xy_dist, float max_z_diff);
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );

void Howler_SetBolts( gentity_t *self )
{
	if ( self && self->client )
	{
		renderInfo_t *ri = &self->client->renderInfo;
		ri->genericBolt1 = trap->G2API_AddBolt(self->ghoul2, 0, "Tongue01");
		ri->genericBolt2 = trap->G2API_AddBolt(self->ghoul2, 0, "Tongue08");
	}
}

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
	int i;
	//G_SoundIndex( "sound/chars/howler/howl.mp3" );
	G_EffectIndex( "howler/sonic" );
	G_SoundIndex( "sound/chars/howler/howl.mp3" );
	for ( i = 1; i < 3; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/idle_hiss%d.mp3", i ) );
	}
	for ( i = 1; i < 6; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/howl_talk%d.mp3", i ) );
		G_SoundIndex( va( "sound/chars/howler/howl_yell%d.mp3", i ) );
	}
}

void Howler_ClearTimers( gentity_t *self )
{
	//clear all my timers
	TIMER_Set( self, "flee", -level.time );
	TIMER_Set( self, "retreating", -level.time );
	TIMER_Set( self, "standing", -level.time );
	TIMER_Set( self, "walking", -level.time );
	TIMER_Set( self, "running", -level.time );
	TIMER_Set( self, "aggressionDecay", -level.time );
	TIMER_Set( self, "speaking", -level.time );
}

static void Howler_TryDamage( int damage, qboolean tongue, qboolean knockdown )
{
	vec3_t	start, end, dir;
	trace_t	tr;

	if ( tongue )
	{
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->client->renderInfo.genericBolt1, start, 0 );
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->client->renderInfo.genericBolt2, end, 0 );
		VectorSubtract( end, start, dir );
		float dist = VectorNormalize( dir );
		VectorMA( start, dist+16, dir, end );
	}
	else
	{
		VectorCopy( NPCS.NPC->r.currentOrigin, start );
		AngleVectors( NPCS.NPC->r.currentAngles, dir, NULL, NULL );
		VectorMA( start, MIN_DISTANCE*2, dir, end );
	}
	// Should probably trace from the mouth, but, ah well.
	trap->Trace( &tr, start, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, 0, 0, 0 );

	if ( tr.entityNum < ENTITYNUM_WORLD )
	{//hit *something*
		gentity_t *victim = &g_entities[tr.entityNum];
		if ( !victim->client
			|| victim->client->NPC_class != CLASS_HOWLER )
		{//not another howler

			if ( knockdown && victim->client )
			{//only do damage if victim isn't knocked down.  If he isn't, knock him down
				if ( PM_InKnockDown( &victim->client->ps ) )
				{
					return;
				}
			}
			G_Damage( victim, NPCS.NPC, NPCS.NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			if ( knockdown && victim->health > 0 )
			{//victim still alive
				G_TossTheMofo( victim, NPCS.NPC->client->ps.velocity, 500 );
			}
		}
	}
}


static void Howler_Howl( void )
{
	gentity_t	*radiusEnts[ 128 ];
	int			radiusEntNums[128];
	int			numEnts;
	const float	radius = (NPCS.NPC->spawnflags&1)?256:128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	//AddSoundEvent( NPCS.NPC, NPCS.NPC->r.currentOrigin, 512, AEL_DANGER, qfalse, qtrue );

	numEnts = NPC_GetEntsNearBolt( radiusEntNums, radius, NPCS.NPC->client->renderInfo.handLBolt, boltOrg );
	

	for ( i = 0; i < numEnts; i++ )
	{
		radiusEnts[i] = &g_entities[radiusEntNums[i]];
		
		if ( !radiusEnts[i]->inuse )
		{
			continue;
		}

		if ( radiusEnts[i] == NPCS.NPC )
		{//Skip the rancor ent
			continue;
		}

		if ( radiusEnts[i]->client == NULL )
		{//must be a client
			continue;
		}

		if ( radiusEnts[i]->client->NPC_class == CLASS_HOWLER )
		{//other howlers immune
			continue;
		}

		distSq = DistanceSquared( radiusEnts[i]->r.currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				G_Damage( radiusEnts[i], NPCS.NPC, NPCS.NPC, vec3_origin, NPCS.NPC->r.currentOrigin, 1, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			}
			if ( radiusEnts[i]->health > 0
				&& radiusEnts[i]->client
				&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
				&& radiusEnts[i]->client->NPC_class != CLASS_ATST
				&& !PM_InKnockDown( &radiusEnts[i]->client->ps ) )
			{
				if ( /*PM_HasAnimation( radiusEnts[i], BOTH_SONICPAIN_START )*/ qtrue )
				{
					if ( radiusEnts[i]->client->ps.torsoAnim != BOTH_SONICPAIN_START
						&& radiusEnts[i]->client->ps.torsoAnim != BOTH_SONICPAIN_HOLD )
					{
						NPC_SetAnim( radiusEnts[i], SETANIM_LEGS, BOTH_SONICPAIN_START, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( radiusEnts[i], SETANIM_TORSO, BOTH_SONICPAIN_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						radiusEnts[i]->client->ps.torsoTimer += 100;
						radiusEnts[i]->client->ps.weaponTime = radiusEnts[i]->client->ps.torsoTimer;
					}
					else if ( radiusEnts[i]->client->ps.torsoTimer <= 100 )
					{//at the end of the sonic pain start or hold anim
						NPC_SetAnim( radiusEnts[i], SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( radiusEnts[i], SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						radiusEnts[i]->client->ps.torsoTimer += 100;
						radiusEnts[i]->client->ps.weaponTime = radiusEnts[i]->client->ps.torsoTimer;
					}
				}
				/*
				else if ( distSq < halfRadSquared
					&& radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE
					&& !Q_irand( 0, 10 ) )//FIXME: base on skill
				{//within range
					G_Knockdown( radiusEnts[i], NPC, vec3_origin, 500, qfalse );
				}
				*/
			}
		}
	}

	/*
	float playerDist = NPC_EntRangeFromBolt( player, NPC->genericBolt1 );
	if ( playerDist < 256.0f )
	{
		CGCam_Shake( 1.0f*playerDist/128.0f, 200 );
	}
	*/
}

static void Howler_Attack( float enemyDist, qboolean howl = qfalse )
{
	int dmg = (NPCS.NPCInfo->localState==LSTATE_BERZERK)?5:2;

	if ( !TIMER_Exists( NPCS.NPC, "attacking" ))
	{
		int attackAnim = BOTH_GESTURE1;
		// Going to do an attack
		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && PM_InKnockDown( &NPCS.NPC->enemy->client->ps )
			&& enemyDist <= MIN_DISTANCE )
		{
			attackAnim = BOTH_ATTACK2;
		}
		else if ( !Q_irand( 0, 4 ) || howl )
		{//howl attack
			//G_SoundOnEnt( NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
		}
		else if ( enemyDist > MIN_DISTANCE && Q_irand( 0, 1 ) )
		{//lunge attack
			//jump foward
			vec3_t	fwd, yawAng = {0, NPCS.NPC->client->ps.viewangles[YAW], 0};
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, (enemyDist*3.0f), NPCS.NPC->client->ps.velocity );
			NPCS.NPC->client->ps.velocity[2] = 200;
			NPCS.NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;

			attackAnim = BOTH_ATTACK1;
		}
		else
		{//tongue attack
			attackAnim = BOTH_ATTACK2;
		}

		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, attackAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART );
		if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
		{//attack again right away
			TIMER_Set( NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer );
		}
		else
		{
			TIMER_Set( NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer + Q_irand( 0, 1500 ) );//FIXME: base on skill
			TIMER_Set( NPCS.NPC, "standing", -level.time );
			TIMER_Set( NPCS.NPC, "walking", -level.time );
			TIMER_Set( NPCS.NPC, "running", NPCS.NPC->client->ps.legsTimer + 5000 );
		}

		TIMER_Set( NPCS.NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	switch ( NPCS.NPC->client->ps.legsAnim )
	{
	case BOTH_ATTACK1:
	case BOTH_MELEE1:
		if ( NPCS.NPC->client->ps.legsTimer > 650//more than 13 frames left
			/*&& PM_AnimLength( 0, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 800*/  )//at least 16 frames into anim // FIXME
		{
			Howler_TryDamage( dmg, qfalse, qfalse );
		}
		break;
	case BOTH_ATTACK2:
	case BOTH_MELEE2:
		if ( NPCS.NPC->client->ps.legsTimer > 350//more than 7 frames left
			/*&& PM_AnimLength( 0, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 550*/ )//at least 11 frames into anim // FIXME
		{
			Howler_TryDamage( dmg, qtrue, qfalse );
		}
		break;
	case BOTH_GESTURE1:
		{
			if ( NPCS.NPC->client->ps.legsTimer > 1800//more than 36 frames left
				/*&& PM_AnimLength( 0, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 950*/ )//at least 19 frames into anim // FIXME
			{
				Howler_Howl();
				if ( !NPCS.NPC->count )
				{
					G_PlayEffectID( G_EffectIndex( "howler/sonic" ), NPCS.NPC->r.currentOrigin, vec3_origin );
					G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
					NPCS.NPC->count = 1;
				}
			}
		}
		break;
	default:
		//anims seem to get reset after a load, so just stop attacking and it will restart as needed.
		TIMER_Remove( NPCS.NPC, "attacking" );
		break;
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPCS.NPC, "attacking", qtrue );
}

/*
-------------------------
Howler_Move
-------------------------
*/
qboolean NPC_Howler_Move( int randomJumpChance = 0 )
{
	if ( !TIMER_Done( NPCS.NPC, "standing" ) )
	{//standing around
		return qfalse;
	}
	if ( NPCS.client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in air, don't do anything
		return qfalse;
	}
	if ( (!NPCS.NPC->enemy&&TIMER_Done( NPCS.NPC, "running" )) || !TIMER_Done( NPCS.NPC, "walking" ) )
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
	}
	if ( (!randomJumpChance||Q_irand( 0, randomJumpChance ))
		&& NPC_MoveToGoal( qtrue ) )
	{
		if ( VectorCompare( NPCS.client->ps.moveDir, vec3_origin )
			|| !NPCS.client->ps.speed )
		{//uh.... wtf?  Got there?
			if ( NPCS.NPCInfo->goalEntity )
			{
				NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qfalse );
			}
			else
			{
				NPC_UpdateAngles( qfalse, qtrue );
			}
			return qtrue;
		}
		//TEMP: don't want to strafe
		VectorClear( NPCS.client->ps.moveDir );
		NPCS.ucmd.rightmove = 0.0f;
//		Com_Printf( "Howler moving %d\n",NPCS.ucmd.forwardmove );
		//if backing up, go slow...
		if ( NPCS.ucmd.forwardmove < 0.0f )
		{
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			//if ( NPCS.client->ps.speed > NPCS.NPCInfo->stats.walkSpeed )
			{//don't walk faster than I'm allowed to
				NPCS.client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
		}
		else
		{
			if ( (NPCS.ucmd.buttons&BUTTON_WALKING) )
			{
				NPCS.client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
			else
			{
				NPCS.client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
			}
		}
		NPCS.NPCInfo->lockedDesiredYaw = NPCS.NPCInfo->desiredYaw = NPCS.NPCInfo->lastPathAngles[YAW];
		NPC_UpdateAngles( qfalse, qtrue );
	}
	else if ( NPCS.NPCInfo->goalEntity )
	{//couldn't get where we wanted to go, try to jump there
		NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qfalse );
		//FIXME NPC_TryJump( NPCS.NPCInfo->goalEntity, 400.0f, -256.0f );
	}
	return qtrue;
}

static qboolean Howler_Move( qboolean visible )
{
	if ( NPCS.NPCInfo->localState != LSTATE_WAITING )
	{
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
		return NPC_Howler_Move( 30 );
	}
	return qfalse;
}

/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle( void ) {
}


/*
-------------------------
Howler_Patrol
-------------------------
*/
void Howler_Patrol( void )
{
	NPCS.NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPC_Howler_Move( 100 );
	}

	vec3_t dif;
	VectorSubtract( g_entities[0].r.currentOrigin, NPCS.NPC->r.currentOrigin, dif );

	if ( VectorLengthSquared( dif ) < 256 * 256 )
	{
		G_SetEnemy( NPCS.NPC, &g_entities[0] );
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}

	Howler_Attack( 0.0f, qtrue );
}

//---------------------------------------------------------
void Howler_TryDamage( gentity_t *enemy, int damage )
{
	vec3_t	end, dir;
	trace_t	tr;

	if ( !enemy )
	{
		return;
	}

	AngleVectors( NPCS.NPC->client->ps.viewangles, dir, NULL, NULL );
	VectorMA( NPCS.NPC->r.currentOrigin, MIN_DISTANCE, dir, end );

	// Should probably trace from the mouth, but, ah well.
	trap->Trace( &tr, NPCS.NPC->r.currentOrigin, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0 );

	if ( tr.entityNum != ENTITYNUM_WORLD )
	{
		G_Damage( &g_entities[tr.entityNum], NPCS.NPC, NPCS.NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
}

//----------------------------------
void Howler_Combat( void )
{
	qboolean	faced = qfalse;
	float		distance;
	qboolean	advance = qfalse;
	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//not on the ground
		if ( NPCS.NPC->client->ps.legsAnim == BOTH_JUMP1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_INAIR1 )
		{//flying through the air with the greatest of ease, etc
			Howler_TryDamage( 10, qfalse, qfalse );
		}
	}
	else
	{//not in air, see if we should attack or advance
		// If we cannot see our target or we have somewhere to go, then do that
		if ( !NPC_ClearLOS4( NPCS.NPC->enemy ) )//|| UpdateGoal( ))
		{
			NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
			NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

			if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
			{
				NPC_Howler_Move( 3 );
			}
			else
			{
				NPC_Howler_Move( 10 );
			}
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}

		distance = DistanceHorizontal( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );

		if ( NPCS.NPC->enemy &&NPCS. NPC->enemy->client && PM_InKnockDown( &NPCS.NPC->enemy->client->ps ) )
		{//get really close to knocked down enemies
			advance = (qboolean)( distance > MIN_DISTANCE ? qtrue : qfalse  );
		}
		else
		{
			advance = (qboolean)( distance > MAX_DISTANCE ? qtrue : qfalse  );//MIN_DISTANCE
		}

		if (( advance || NPCS.NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPCS.NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPCS.NPC, "takingPain", qtrue ))
			{
				NPCS.NPCInfo->localState = LSTATE_CLEAR;
			}
			else if ( TIMER_Done( NPCS.NPC, "standing" ) )
			{
				faced = Howler_Move( qtrue );
			}
		}
		else
		{
			Howler_Attack( distance );
		}
	}

	if ( !faced )
	{
		if ( //TIMER_Done( NPC, "standing" ) //not just standing there
			//!advance //not moving
			TIMER_Done( NPCS.NPC, "attacking" ) )// not attacking
		{//not standing around
			// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qfalse, qtrue );
		}
	}
}

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
void NPC_Howler_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( damage >= 10 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Set( self, "takingPain", 2900 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		if ( self->NPC )
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
void NPC_BSHowler_Default( void )
{
	if ( NPCS.NPC->client->ps.legsAnim != BOTH_GESTURE1 )
	{
		NPCS.NPC->count = 0;
	}
	//FIXME: if in jump, do damage in front and maybe knock them down?
	if ( !TIMER_Done( NPCS.NPC, "attacking" ) )
	{
		if ( NPCS.NPC->enemy )
		{
			//NPC_FaceEnemy( qfalse );
			Howler_Attack( Distance( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) );
		}
		else
		{
			//NPC_UpdateAngles( qfalse, qtrue );
			Howler_Attack( 0.0f );
		}
		NPC_UpdateAngles( qfalse, qtrue );
		return;
	}

	if ( NPCS.NPC->enemy )
	{
		if ( NPCS.NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPCS.NPC, "aggressionDecay" ) )
			{
				NPCS.NPCInfo->stats.aggression--;
				TIMER_Set( NPCS.NPC, "aggressionDecay", 500 );
			}
		}
		if ( !TIMER_Done( NPCS.NPC, "flee" )
			/*&& NPC_BSFlee() FIXME*/ )	//this can clear ENEMY
		{//successfully trying to run away
			return;
		}
		if ( NPCS.NPC->enemy == NULL)
		{
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}
		if ( NPCS.NPCInfo->localState == LSTATE_FLEE )
		{//we were fleeing, now done (either timer ran out or we cannot flee anymore
			if ( NPC_ClearLOS4( NPCS.NPC->enemy ) )
			{//if enemy is still around, go berzerk
				NPCS.NPCInfo->localState = LSTATE_BERZERK;
			}
			else
			{//otherwise, lick our wounds?
				NPCS.NPCInfo->localState = LSTATE_CLEAR;
				TIMER_Set( NPCS.NPC, "standing", Q_irand( 3000, 10000 ) );
			}
		}
		else if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
		{//go nuts!
		}
		else if ( NPCS.NPCInfo->stats.aggression >= Q_irand( 75, 125 ) )
		{//that's it, go nuts!
			NPCS.NPCInfo->localState = LSTATE_BERZERK;
		}
		else if ( !TIMER_Done( NPCS.NPC, "retreating" ) )
		{//trying to back off
			NPC_FaceEnemy( qtrue );
			if ( NPCS.NPC->client->ps.speed > NPCS.NPCInfo->stats.walkSpeed )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			if ( Distance( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < HOWLER_RETREAT_DIST )
			{//enemy is close
				vec3_t moveDir;
				AngleVectors( NPCS.NPC->r.currentAngles, moveDir, NULL, NULL );
				VectorScale( moveDir, -1, moveDir );
				if ( !NAV_DirSafe( NPCS.NPC, moveDir, 8 ) )
				{//enemy is backing me up against a wall or ledge!  Start to get really mad!
					NPCS.NPCInfo->stats.aggression += 2;
				}
				else
				{//back off
					NPCS.ucmd.forwardmove = -127;
				}
				//enemy won't leave me alone, get mad...
				NPCS.NPCInfo->stats.aggression++;
			}
			return;
		}
		else if ( TIMER_Done( NPCS.NPC, "standing" ) )
		{//not standing around
			if ( !(NPCS.NPCInfo->last_ucmd.forwardmove)
				&& !(NPCS.NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPCS.NPC, "walking" )
					&& TIMER_Done( NPCS.NPC, "running" ) )
				{//not walking or running
					if ( Q_irand( 0, 2 ) )
					{//run for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 4000, 8000 ) );
					}
					else
					{//walk for a bit
						TIMER_Set( NPCS.NPC, "running", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else if ( (NPCS.NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 5 ) || DistanceSquared( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//run for a while
						TIMER_Set( NPCS.NPC, "running", Q_irand( 4000, 20000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPCS.NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 8 ) || DistanceSquared( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//walk for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 3000, 10000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
		}
		if ( NPC_ValidEnemy( NPCS.NPC->enemy ) == qfalse )
		{
			TIMER_Remove( NPCS.NPC, "lookForNewEnemy" );//make them look again right now
			if ( !NPCS.NPC->enemy->inuse || level.time - NPCS.NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
			{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
				NPCS.NPC->enemy = NULL;
				Howler_Patrol();
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
		if ( TIMER_Done( NPCS.NPC, "lookForNewEnemy" ) )
		{
			gentity_t *sav_enemy = NPCS.NPC->enemy;//FIXME: what about NPCS.NPC->lastEnemy?
			NPCS.NPC->enemy = NULL;
			gentity_t *newEnemy = NPC_CheckEnemy( (qboolean)(NPCS.NPCInfo->confusionTime < level.time), qfalse, qfalse );
			NPCS.NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
				G_SetEnemy( NPCS.NPC, newEnemy );
				if ( NPCS.NPC->enemy != NPCS.NPC->lastEnemy )
				{//clear this so that we only sniff the player the first time we pick them up
					NPCS.NPC->useDebounceTime = 0;
				}
				//hold this one for at least 5-15 seconds
				TIMER_Set( NPCS.NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			}
			else
			{//look again in 2-5 secs
				TIMER_Set( NPCS.NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
			}
		}
		Howler_Combat();
		if ( TIMER_Done( NPCS.NPC, "speaking" ) )
		{
			if ( !TIMER_Done( NPCS.NPC, "standing" )
				|| !TIMER_Done( NPCS.NPC, "retreating" ))
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else if ( !TIMER_Done( NPCS.NPC, "walking" )
				|| NPCS.NPCInfo->localState == LSTATE_FLEE )
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_yell%d.mp3", Q_irand( 1, 5 ) ) );
			}
			if ( NPCS.NPCInfo->localState == LSTATE_BERZERK
				|| NPCS.NPCInfo->localState == LSTATE_FLEE )
			{
				TIMER_Set( NPCS.NPC, "speaking", Q_irand( 1000, 4000 ) );
			}
			else
			{
				TIMER_Set( NPCS.NPC, "speaking", Q_irand( 3000, 8000 ) );
			}
		}
		return;
	}
	else
	{
		if ( TIMER_Done( NPCS.NPC, "speaking" ) )
		{
			if ( !Q_irand( 0, 3 ) )
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			TIMER_Set( NPCS.NPC, "speaking", Q_irand( 4000, 12000 ) );
		}
		if ( NPCS.NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPCS.NPC, "aggressionDecay" ) )
			{
				NPCS.NPCInfo->stats.aggression--;
				TIMER_Set( NPCS.NPC, "aggressionDecay", 200 );
			}
		}
		if ( TIMER_Done( NPCS.NPC, "standing" ) )
		{//not standing around
			if ( !(NPCS.NPCInfo->last_ucmd.forwardmove)
				&& !(NPCS.NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPCS.NPC, "walking" )
					&& TIMER_Done( NPCS.NPC, "running" ) )
				{//not walking or running
					if ( NPCS.NPCInfo->goalEntity )
					{//have somewhere to go
						if ( Q_irand( 0, 2 ) )
						{//walk for a while
							TIMER_Set( NPCS.NPC, "walking", Q_irand( 3000, 10000 ) );
						}
						else
						{//run for a bit
							TIMER_Set( NPCS.NPC, "running", Q_irand( 2500, 5000 ) );
						}
					}
				}
			}
			else if ( (NPCS.NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 3 ) )
					{//run for a while
						TIMER_Set( NPCS.NPC, "running", Q_irand( 3000, 6000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPCS.NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 2 ) )
					{//walk for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 6000, 15000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 4000, 6000 ) );
					}
				}
			}
		}
		if ( NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Howler_Patrol();
		}
		else
		{
			Howler_Idle();
		}
	}

	NPC_UpdateAngles( qfalse, qtrue );
}
