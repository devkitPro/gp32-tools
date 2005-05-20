/////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
//
//
//
//
/////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(WIN32) || defined(WINNT) || defined(__WIN32__) || defined(__WIN32)
#include <windows.h>
#include <io.h>
#define  mktemp _mktemp
#define  unlink _unlink
#else
#include <unistd.h>
#endif


//
//
//

int b2fxec_strncasecmp( const char *A, const char * B, size_t n ) {
#if defined(WIN32) || defined(WINNT) || defined(__WIN32__) || defined(__WIN32)
  return _strnicmp(reinterpret_cast<const char*>(A),B,n);
#else
  return strncasecmp(reinterpret_cast<const char*>(A),B,n);
#endif
}

//
//
//

int b2fxec_truncate( const char *s, size_t n ) {
#if defined(WIN32) || defined(WINNT) || defined(__WIN32__) || defined(__WIN32)
  HANDLE h;
  if ((h = CreateFile(s,GENERIC_READ,FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0)) != INVALID_HANDLE_VALUE) {
    SetFilePointer(h,n,0,FILE_BEGIN);
    SetEndOfFile(h);
    CloseHandle(h);
    return 0;
  } else {
    return -1;
  }
#else
  return truncate(s,n);
#endif
}

//
//
//

char *b2fxec_mktemp( char *tmpstr ) {
  return mktemp(tmpstr);
}

//
//
//

int b2fxec_unlink( const char *A ) {
  return unlink(A);
}


