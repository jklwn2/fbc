/*
 *  libfb - FreeBASIC's runtime library
 *	Copyright (C) 2004-2005 Andre V. T. Vicentini (av1ctor@yahoo.com.br) and others.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * libfb_file_dir.c -- dir$ implementation
 *
 * chng: jan/2005 written [lillo]
 *
 */

#include <stdlib.h>
#include <string.h>
#include "fb.h"


/*:::::*/
static void close_dir ( void )
{
	FB_DIRCTX *ctx = FB_TLSGETCTX( DIR );
#ifdef TARGET_WIN32
    _findclose( ctx->handle );
#else
    FindClose( ctx->handle );
#endif
	ctx->in_use = FALSE;
}


/*:::::*/
static char *find_next ( void )
{
	char *name = NULL;
	FB_DIRCTX *ctx = FB_TLSGETCTX( DIR );

#ifdef TARGET_WIN32
	do
	{
		if( _findnext( ctx->handle, &ctx->data ) )
		{
			close_dir( );
			name = NULL;
			break;
		}
        name = ctx->data.name;
	}
	while( ctx->data.attrib & ~ctx->attrib );
#else
    do {
        if( !FindNextFile( ctx->handle, &ctx->data ) ) {
            close_dir();
            name = NULL;
            break;
        }
        name = ctx->data.cFileName;
    } while( ctx->data.dwFileAttributes & ~ctx->attrib );
#endif

	return name;
}


/*:::::*/
FBCALL FBSTRING *fb_Dir ( FBSTRING *filespec, int attrib )
{
	FB_DIRCTX	*ctx;
	FBSTRING	*res;
	int			len;
    char		*name;
    int         handle_ok;

	len = FB_STRSIZE( filespec );
	name = NULL;

	ctx = FB_TLSGETCTX( DIR );

	if( len > 0 )
	{
		/* findfirst */
		if( ctx->in_use )
			close_dir( );

#ifdef TARGET_WIN32
        ctx->handle = _findfirst( filespec->data, &ctx->data );
        handle_ok = ctx->handle != -1;
#else
        ctx->handle = FindFirstFile( filespec->data, &ctx->data );
        handle_ok = ctx->handle != INVALID_HANDLE_VALUE;
#endif
		if( handle_ok )
		{
			/* Handle any other possible bits different Windows versions could return */
			ctx->attrib = attrib | 0xFFFFFF00;

			if( (attrib & 0x10) == 0 )
				ctx->attrib |= 0x20;
#ifdef TARGET_WIN32
			if( ctx->data.attrib & ~ctx->attrib )
				name = find_next( );
			else
                name = ctx->data.name;
#else
			if( ctx->data.dwFileAttributes & ~ctx->attrib )
				name = find_next( );
			else
                name = ctx->data.cFileName;
#endif
			if( name )
				ctx->in_use = TRUE;
		}
	}
	else {

		/* findnext */
		if( ctx->in_use )
			name = find_next( );
	}

	FB_STRLOCK();

	/* store filename if found */
	if( name )
	{
        len = strlen( name );
        res = fb_hStrAllocTemp_NoLock( NULL, len );
		if( res )
		{
			fb_hStrCopy( res->data, name, len );
		}
		else
			res = &fb_strNullDesc;
	}
	else
		res = &fb_strNullDesc;

	fb_hStrDelTemp_NoLock( filespec );

	FB_STRUNLOCK();

	return res;
}
