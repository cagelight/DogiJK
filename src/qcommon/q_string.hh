#pragma once

#include "q_platform.hh"

#include <string>

struct insensitive_char_traits : public std::char_traits<char> {
	
	static bool eq(char c1, char c2) {
		return std::toupper(c1) == std::toupper(c2);
	}
	static bool lt(char c1, char c2) {
		return std::toupper(c1) <  std::toupper(c2);
	}
	static int compare(const char* s1, const char* s2, size_t n) {
		while ( n-- != 0 ) {
			if ( std::toupper(*s1) < std::toupper(*s2) ) return -1;
			if ( std::toupper(*s1) > std::toupper(*s2) ) return 1;
			++s1; ++s2;
		}
		return 0;
	}
	static char const * find(const char* s, int n, char a) {
		auto const ua (std::toupper(a));
		while ( n-- != 0 ) 
		{
			if (std::toupper(*s) == ua)
				return s;
			s++;
		}
		return nullptr;
	}
};

typedef std::basic_string<char, insensitive_char_traits> istring;

inline std::string i2s(istring const & str) { return {str.c_str()}; }
inline istring s2i(std::string const & str) { return {str.c_str()}; }

namespace std {
	
	template <> struct hash<istring> {
		size_t operator() (istring const & str) const {
			size_t h = 14695981039346656037UL;
			for (char c : str) {
				h ^= std::toupper(c);
				h *= 1099511628211UL;
			}
			return h;
		}
	};
	
}

int Q_isprint( int c );
int Q_isprintext( int c );
int Q_isgraph( int c );
int Q_islower( int c );
int Q_isupper( int c );
int Q_isalpha( int c );
qboolean Q_isanumber( const char *s );
qboolean Q_isintegral( float f );

// portable case insensitive compare
int Q_stricmp(const char *s1, const char *s2);
int	Q_strncmp(const char *s1, const char *s2, int n);
int	Q_stricmpn(const char *s1, const char *s2, int n);
char *Q_strlwr( char *s1 );
char *Q_strupr( char *s1 );
char *Q_strrchr( const char* string, int c );

// buffer size safe library replacements
void Q_strncpyz( char *dest, const char *src, int destsize );
void Q_strcat( char *dest, int size, const char *src );

const char *Q_stristr( const char *s, const char *find);

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );

// removes color sequences from string
char *Q_CleanStr( char *string );
void Q_StripColor(char *text);
const char *Q_strchrs( const char *string, const char *search );

void Q_strstrip( char *string, const char *strip, const char *repl );

#if defined (_MSC_VER)
	// vsnprintf is ISO/IEC 9899:1999
	// abstracting this to make it portable
	int Q_vsnprintf( char *str, size_t size, const char *format, va_list args );
#else // not using MSVC
	#define Q_vsnprintf vsnprintf
#endif
