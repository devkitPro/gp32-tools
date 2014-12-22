#ifndef _headers_h_included
#define _headers_h_included

#include "fio.h"

// FXP0 header..

#define FXE0_CRT0_SIZE			488
#define FXE0_FIX_DATASTART_POS 	12
#define FXE0_FIX_DATAEND_POS	16
#define FXE0_FIX_BSSSTART_POS	20
#define FXE0_FIX_BSSEND_POS		24

// FXP1 header..

#define FXE1_CRT0_SIZE 			668
#define FXE1_FIX_DATASTART_POS	0
#define FXE1_FIX_DATAEND_POS	4
#define FXE1_FIX_BSSSTART_POS	8
#define FXE1_FIX_BSSEND_POS		12

// FXP2 header..

#define FXE2_CRT0_SIZE 			744
#define FXE2_FIX_DATASTART_POS	12
#define FXE2_FIX_DATAEND_POS	16
#define FXE2_FIX_BSSSTART_POS	20
#define FXE2_FIX_BSSEND_POS		24

// FXP3 header..

#define FXE3_CRT0_SIZE 			840
#define FXE3_FIX_DATASTART_POS	12
#define FXE3_FIX_DATAEND_POS	16
#define FXE3_FIX_BSSSTART_POS	20
#define FXE3_FIX_BSSEND_POS		24

// Common stuff

#define FXE_HEADER_SIZE 1128
#define FXE_TOTALFILESIZE_POS 4
#define FXE_TITLE_POS 12
#define FXE_AUTHOR_POS 44
#define FXE_ICON_POS 92
#define FXE_FILESIZE_POS 1116

// config structure for header post fixing

struct fixInfo {
	long osize;		// original file size
	long csize;		// compressed file size
	long gap;		// gap between code & data sections
	long mhz;		// clockspeed
	long fac;		// factor..
	long flash;		// flash color reg

	fixInfo() {
		gap = 0;
		mhz = 0;
		fac = 0;
		flash = 0x0c000000;
	}
	~fixInfo() {}

};

//

namespace Headers {
	int saveFXEHeader( int, FWriter*, char*, char*, char*, char*, char* );
	int fixFXEHeader( int, FWriter*, long );
	int saveFXE0Header( FWriter* );
	int fixFXE0Header( FWriter*, long, const fixInfo* );
	int saveFXE1Header( FWriter* );
	int fixFXE1Header( FWriter*, long, const fixInfo* );
	int saveFXE2Header( FWriter* );
	int fixFXE2Header( FWriter*, long, const fixInfo* );
	int saveFXE3Header( FWriter* );
	int fixFXE3Header( FWriter*, long, const fixInfo* );

	//

	extern"C" unsigned char fxe0_crt0_bin[];
	extern"C" unsigned char fxe1_crt0_bin[];
	extern"C" unsigned char fxe2_crt0_bin[];
	extern"C" unsigned char fxe3_crt0_bin[];
	extern unsigned char FXEheader [];
};



#endif
