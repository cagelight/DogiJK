/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// tr_noise.c
#include "tr_common.hh"
#include "qcommon/q_simplex.hh"

#define NOISE_SIZE 256
#define NOISE_MASK ( NOISE_SIZE - 1 )

#define VAL( a ) s_noise_perm[ ( a ) & ( NOISE_MASK )]

static float s_noise_table[NOISE_SIZE];
static int s_noise_perm[NOISE_SIZE];

static qm::simplex s_simplex;

float GetNoiseTime( int t )
{
	int index = VAL( t );

	return (1 + s_noise_table[index]);
}

void R_NoiseInit( void )
{
	int i;

	for ( i = 0; i < NOISE_SIZE; i++ )
	{
		s_noise_table[i] = Q_flrand(-1, 1);
		s_noise_perm[i] = Q_irand(0, 255);
	}
	
	s_simplex.seed();
}

float R_NoiseGet4f( float x, float y, float z, float t )
{
	return s_simplex.generate(x, y, z, t);
}
