#ifndef _port_h_included
#define _port_h_included

/////////////////////////////////////////////////////////////////////////////
//
// Module:
//  port.h
//
// Description:
//  Defines prototypes for commont portability layer functions.
//  Currently these are for posix <-> win32 api portability.
//
// Author:
//  (c) Jouni 'mr.spiv' Korhonen
//
// Version:
//  v0.0l 7-Sep-2004 JiK - initial version
//  
//
/////////////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(WINNT) || defined(__WIN32__) || defined(__WIN32) || defined(__linux__)
#include "getopt.h"
#endif


int b2fxec_strncasecmp( const char *A, const char * B, size_t n );
int b2fxec_truncate( const char *s, size_t n );
char *b2fxec_mktemp( char *tmpstr );
int b2fxec_unlink( const char *A );


#endif



