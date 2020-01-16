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

#ifdef XCVAR_PROTO
#undef XCVAR_PROTO
	#define XCVAR_DEF( name, defVal, flags, desc ) extern cvar_t * name;
#endif

#ifdef XCVAR_DECL
#undef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, flags, desc ) cvar_t * name;
#endif

#ifdef XCVAR_REGISTER
#undef XCVAR_REGISTER
	#define XCVAR_DEF( name, defVal, flags, desc ) name = ri.Cvar_Get ( #name , defVal , flags , desc );
#endif

XCVAR_DEF( r_debug,						"0",				CVAR_ARCHIVE,									"[DEBUG][binary]" )
XCVAR_DEF( r_lockpvs,					"0",				0,												"[DEBUG][binary]" )
XCVAR_DEF( r_patch_minsize,				"2",				CVAR_ARCHIVE,									"[VISUAL][float] Minimum size of a patch plane, lower is better." )
XCVAR_DEF( r_showedges,					"0",				CVAR_CHEAT,										"[DEBUG][binary]" )
XCVAR_DEF( r_shownormals,				"0",				0,												"[DEBUG][binary]" )
XCVAR_DEF( r_showtris,					"0",				CVAR_CHEAT,										"[DEBUG][binary] Draw wireframes." )
XCVAR_DEF( r_vis,						"1",				0,												"[DEBUG][binary] Toggle vis. [integer > 1] Show a specific vis index (index + 1)." )
XCVAR_DEF( r_viscachesize,				"1",				CVAR_ARCHIVE,									"[PERF][integer] Number of Index Buffers to cache for vis." )
XCVAR_DEF( r_whiteimage,				"0",				0,												"[DEBUG][binary]" )

// vanilla behavior toggles
XCVAR_DEF( r_vanilla_gridlighting,		"0",				CVAR_ARCHIVE,									"[VISUAL][binary] Use similar grid lighting to rd-vanilla." )

#ifdef _DEBUG
XCVAR_DEF( r_drawcalls,					"0",				0,												"[DEBUG][binary]" )
#endif

#undef XCVAR_DEF
