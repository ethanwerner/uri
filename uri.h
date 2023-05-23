// uri.h - Minimal URI (de)serializer

// Ethan Werner
//
// URI Parser and Serializer
//
// The uri_t structure is intended to be used as a black box. The primary use
// of the header as a whole is to enable the fairly efficient creation of
// sequential URIs.
//
// Usage Notes
//  The internally held strings should remain opaque and only interacted with
//  through the given functions
//
//  Any string interacting with a uri_t is copied upon both entry and
//  exit, thus, all strings except those must be (de)allocated by the user.


#ifndef URI_H
#define URI_H


typedef enum
{
	BUILD,
	SCHEME,
	USERINFO,
	HOST,
	PORT,
	PATH,
	QUERY,
	FRAGMENT,
	URI_END
} uri_element_t;


typedef struct uri_t
{
	char *data[URI_END];
} uri_t;


uri_t uri_init( void );
void uri_set( uri_t *, uri_element_t, char const * );
char * uri_get( uri_t * );
uri_t * uri_parse( char const * );
void uri_print( uri_t const * );
void uri_build( uri_t * );


#endif // URI_H


#ifdef URI_IMPLEMENTATION


#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define copy_string( ptr, source ) {       \
	if( source )                           \
	{                                      \
		size_t n = strlen( source );       \
		ptr = malloc( n + 1 );             \
		ptr[n] = '\0';                     \
		strcpy( ptr, source );             \
	}                                      \
	else                                   \
	{                                      \
		ptr = NULL;                        \
	}                                      \
}


void uri_set( uri_t *u, uri_element_t e, char const *data )
{
	char *c;
	copy_string( c, data );

	if( u->data[e] )
		free( u->data[e] );

	u->data[e] = c;
}


char * uri_get( uri_t *u )
{
	uri_build( u );
	char *c;
	copy_string( c, u->data[BUILD] );

	return c;
}


char * uri_remove( uri_t *u, uri_element_t e )
{
	char *data = u->data[e];
	uri_set( u, e, NULL );

	return data;
}

// TODO: Account for / when path is missing in query and fragment
void uri_build( uri_t *u )
{
	int n = 0;

    // TODO: Convert to a for loop
	if( u->data[BUILD] )
		free( u->data[BUILD] );

	// Length of built string
	if( u->data[SCHEME] )
		n += strlen( u->data[SCHEME] ) + 1; // ":"

	if( u->data[USERINFO] )
		n += strlen( u->data[USERINFO] ) + 1; // "@"

	if( u->data[HOST] )
		n += strlen( u->data[HOST] ) + 2; // "//"

	if( u->data[PORT] )
		n += strlen( u->data[PORT] ) + 1; // ":"

	if( u->data[PATH] )
		n += strlen( u->data[PATH] );

	if( u->data[QUERY] )
		n += strlen( u->data[QUERY] ) + 1; // ?

	if( u->data[FRAGMENT] )
		n += strlen( u->data[FRAGMENT] ) + 1; // #

	char *build = ( char * ) malloc( n + 1 );
	build[n] = '\0';

	// Copy strings to BUILD
	strcpy( build, u->data[SCHEME] );
	strcat( build, ":" );

	if( u->data[HOST] )
	{
		strcat( build, "//" );

		if( u->data[USERINFO] )
		{
			strcat( build, u->data[USERINFO] );
			strcat( build, "@" );
		}

		strcat( build, u->data[HOST] );

		if( u->data[PORT] )
		{
			strcat( build, ":" );
			strcat( build, u->data[PORT] );
		}
	}
	if( u->data[PATH] )
	{
		strcat( build, u->data[PATH] );
	}
	if( u->data[QUERY] )
	{
		strcat( build, "?" );
		strcat( build, u->data[QUERY] );
	}
	if( u->data[FRAGMENT] )
	{
		strcat( build, "#" );
		strcat( build, u->data[FRAGMENT] );
	}

	u->data[BUILD] = build;
}


char * copy_substring( char const *from, char const *to )
{
	char *s = ( char * ) malloc( to - from + 1 );
	s[to - from] = 0;
	strncpy( s, from, to - from );

	return s;
}



uri_t * uri_parse( char const * uri_s )
{
	uri_t *uri = ( uri_t * ) calloc( 1, sizeof( uri_t ) );

	char const *current = uri_s;
	char const *working;
	char *token;

	// SCHEME
	working = strchr( current, ( int ) ':' );

	if( !working )
		return NULL;

	token = copy_substring( current, working );
	uri_set( uri, SCHEME, token );
	current = ++working;

	// AUTHORITY
	if( ( *( working ) == '/' ) && ( *( working + 1 ) == '/' ) )
	{
		current = working += 2;

		while( *working && *working != '@' && *working != ':' &&*working != '/' )
			working++;

		// USERINFO
		if( *working == '@' )
		{
			token = copy_substring( current, working );
			uri_set( uri, USERINFO, token );

			current = ++working;

			while( *working && *working != ':' && *working != '/' )
				working++;
		}

		// HOST
		token = copy_substring( current, working );
		uri_set( uri, HOST, token );

		if( !( *working ) )
			return uri;

		// PORT
		if( *working == ':' )
		{
			current = ++working;

			while( *working && *working != '/' )
				working++;

			token = copy_substring( current, working );
			uri_set( uri, PORT, token );
		}


		if( !( *working ) )
			return uri;

		current = working;

		while( *working && *working != '?' && *working != '#' )
			working++;
	}
	else
	{
		while( *working && *working != '/' )
			working++;
	}

	// PATH
	token = copy_substring( current, working );
	uri_set( uri, PATH, token );

	if( !( *working ) )
		return uri;

	current = working;

	// QUERY
	if( *working == '?' )
	{
		while( *working && *working != '#' )
			working++;

		token = copy_substring( current, working );
		uri_set( uri, QUERY, token );

		current = working;
	}

	// FRAGMENT
	if( *working == '#' )
	{
		while( *working )
			working++;

		token = copy_substring( current, working );
		uri_set( uri, FRAGMENT, token );
	}

	uri_build( uri );

	return uri;
}


void uri_print( uri_t const *u )
{
	if( u->data[BUILD] )
		fputs( u->data[BUILD], stdout );
}


void uri_print_elements( uri_t const *u )
{
	for( int i = 0; i < URI_END; i++ )
	{

		printf( "%d", i );

		if( u->data[i] )
		{
			printf( " - %s", u->data[i] );
		}

		putchar('\n');

	}
}


#endif // URI_IMPLEMENTATION
