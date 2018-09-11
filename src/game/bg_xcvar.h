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
	#define XCVAR_DEF( name, defVal, update, flags, announce ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
	#define XCVAR_DEF( name, defVal, update, flags, announce ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
#ifdef _CGAME
	#define XCVAR_DEF( name, defVal, update, flags, accounce ) { & name , #name , defVal , update , flags | CVAR_SYSTEMINFO },
#elif defined(_GAME)
	#define XCVAR_DEF( name, defVal, update, flags, announce ) { & name , #name , defVal , update , flags | CVAR_SYSTEMINFO , announce },
#endif
#endif

XCVAR_DEF( bg_fighterAltControl,		"0",			NULL,				0,										qtrue )
XCVAR_DEF( bg_infammo,					"0",			NULL,				CVAR_ARCHIVE,							qtrue )
XCVAR_DEF( bg_gunrate,					"1",			NULL,				CVAR_ARCHIVE,							qtrue )

XCVAR_DEF( weap_conc_bounceAlt,			"0",			NULL,				CVAR_ARCHIVE,							qtrue )
XCVAR_DEF( weap_snip_bounce,			"0",			NULL,				CVAR_ARCHIVE,							qtrue )

#undef XCVAR_DEF
