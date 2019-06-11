#include "tr_local.hh"

static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *(const unsigned char* /*eurofix*/)data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' )
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

static bool setup_skin( q3skin & q3s, char const * path, bool server )
{
	char			*text, *text_p;
	char			*token;
	char			surfName[MAX_QPATH];

	// load and parse the skin file
	ri.FS_ReadFile( path, (void **)&text );
	if ( !text ) {
		return false;
	}

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( !strncmp( token, "tag_", 4 ) ) {	//these aren't in there, but just in case you load an id style one...
			continue;
		}

		// parse the shader name
		token = CommaParse( &text_p );

		if ( !strcmp( &surfName[strlen(surfName)-4], "_off") )
		{
			if ( !strcmp( token ,"*off" ) )
			{
				continue;	//don't need these double offs
			}
			surfName[strlen(surfName)-4] = 0;	//remove the "_off"
		}
		if ( (unsigned)q3s.skin.numSurfaces >= ARRAY_LEN( q3s.skin.surfaces ) )
		{
			assert( ARRAY_LEN( q3s.skin.surfaces ) > (unsigned)q3s.skin.numSurfaces );
			ri.Printf( PRINT_ALL, "WARNING: setup_skin( '%s' ) more than %u surfaces!\n", path, (unsigned int )ARRAY_LEN( q3s.skin.surfaces ) );
			break;
		}
		skinSurface_t & surf = q3s.surfs.emplace_back();
		q3s.skin.surfaces[q3s.skin.numSurfaces] = (_skinSurface_t *)&surf;

		Q_strncpyz( surf.name, surfName, sizeof( surf.name ) );

		if (!server) surf.shader = r->shader_register(token, true)->index;
		else surf.shader = 0;
		
		q3s.skin.numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders
	if ( q3s.skin.numSurfaces == 0 ) {
		return false;		// use default skin
	}
	
	q3s.skin.numSurfaces = q3s.surfs.size();

	return true;
}

static bool RE_SplitSkins(const char *INname, char *skinhead, char *skintorso, char *skinlower)
{	//INname= "models/players/jedi_tf/|head01_skin1|torso01|lower01";
	if (strchr(INname, '|'))
	{
		char name[MAX_QPATH];
		strcpy(name, INname);
		char *p = strchr(name, '|');
		*p=0;
		p++;
		//fill in the base path
		strcpy (skinhead, name);
		strcpy (skintorso, name);
		strcpy (skinlower, name);

		//now get the the individual files

		//advance to second
		char *p2 = strchr(p, '|');
		if (!p2)
		{
			return false;
		}
		*p2=0;
		p2++;
		strcat (skinhead, p);
		strcat (skinhead, ".skin");


		//advance to third
		p = strchr(p2, '|');
		if (!p)
		{
			return false;
		}
		*p=0;
		p++;
		strcat (skintorso,p2);
		strcat (skintorso, ".skin");

		strcat (skinlower,p);
		strcat (skinlower, ".skin");

		return true;
	}
	return false;
}

qhandle_t skinbank::register_skin(char const * name, bool server) {
	if (!name || !name[0]) return 0;
	
	auto const & find = skin_lookup.find(name);
	if (find != skin_lookup.end()) return find->second;
	
	qhandle_t handle = -1;
	
	for (size_t i = 0; i < skins.size(); i++) {
		if (!skins[i]) {
			handle = i;
		}
	}
	
	if (handle == -1) {
		handle = skins.size();
		skins.emplace_back( skins.emplace_back( std::make_shared<q3skin>() ));
	}
	
	q3skin_ptr & q3s = skins[handle];
	Q_strncpyz( q3s->skin.name, name, sizeof( q3s->skin.name ) );
	q3s->surfs.reserve(128);
	
	char skinhead[MAX_QPATH]={0};
	char skintorso[MAX_QPATH]={0};
	char skinlower[MAX_QPATH]={0};
	if ( RE_SplitSkins(name, (char*)&skinhead, (char*)&skintorso, (char*)&skinlower ) ) {
		bool status = setup_skin(*q3s, skinhead, server);
		if (status) status = setup_skin(*q3s, skintorso, server);
		if (status) status = setup_skin(*q3s, skinlower, server);
		if (status) handle = ++hcounter;
	} else {
		if (setup_skin(*q3s, name, server))
			handle = ++hcounter;
	}
	
	skin_lookup[name] = handle;
	return handle;
}

q3skin_ptr skinbank::get_skin(qhandle_t h) {
	if (h < 0 || h >= skins.size()) return nullptr;
	return skins[h];
}
