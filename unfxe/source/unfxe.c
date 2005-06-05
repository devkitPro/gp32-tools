//
// UnFxe v0.2 (c) 2002 Jouni 'Mr.Spiv' Korhonen
//
//
// - Requires C99 compliant compiler.. or a twisted gcc
//
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdint.h>
#include <ctype.h>
#include <unistd.h>

#include "bitutils.h"

//
// Fxe-header structure - all numbers are in little-endian:
//
//    +---+---+---+---+
//    |'f'|'x'|'e'|' '|
//    +---+---+---+---+
//
//    +---+---+---+---+
//    | TOTALFILESIZE | == 4 + 4 + 4 + 32 + 32 + 16 + 1024 + 4 + 4 + FILESIZE == FILESIZE + 0x464
//    +---+---+---+---+
//
//    +---+---+---+---+
//    |   INFOSIZE    | == 4 + 32 + 32 + 16 + 1024 = 0x454
//    +---+---+---+---+
//
//    +-============================-+
//    | 32 Bytes Game name string    |
//    +-============================-+
//
//    +-============================-+
//    | 32 Bytes Author name string  |
//    +-============================-+
//
//    +-=======================-+
//    | 16 Bytes Reserved Area  |
//    +-=======================-+
//
//    +-============================-+
//    | 1024 Bytes 32x32 pixels icon |
//    +-============================-+
//    
//      The icon is a BMP without any headers.   
//      The BMP depth is 8bits.
//
//    +---+---+---+---+
//    |   FILESIZE    |
//    +---+---+---+---+
//
//    +---+---+---+---+
//    |    KEYSIZE    |
//    +---+---+---+---+
//
//    +-===============================-+
//    | KEYSIZE Bytes of encryption key |
//    +-===============================-+
//
//    +-===============================-+
//    | FILESIZE Bytes of encryted data |
//    +-===============================-+
//
//    And when decoded into memory..
//
//    +-===============================================-+
//    | KEYSIZE Bytes of encryption key XORed with 0xff |
//    +-===============================================-+
//
//
//
//
//
//
//

#define FXE_HEADER_SIZE (4+4+4+4+4+32+32+16+1024)
#define CH(p) ( isprint(*(p)) ? *(p) : '.' )

// 

static char author[32];
static char title[32];
static unsigned char reserved[16];
static unsigned char icon[1024];
static unsigned char *key = NULL;
static unsigned char *fxedata = NULL;
static int keylen;
static int filelen;
static FILE *fh = NULL;
static FILE *fh_gxb = NULL;
static FILE *fh_icn = NULL;
static FILE *fh_key = NULL;
static int addkey = 0;





//
// Decrypt buffer fxe data with the supplied key..
//
//
//


static void fix_key( long klen, unsigned char *key ) {
	long n = 0;
	while (n < klen) {
		key[n++] ^= 0xff;
	}
}

static void decrypt_buffer( long klen, const unsigned char *key, 
					 long blen, unsigned char *buf ) {
	long n = 0;
	
	while (n < blen) {
		//unsigned char c = buf[n] ^ 0xff;	// MVN :)
		unsigned char c = buf[n];
		buf[n] = c ^ key[n % klen];
		n++;
	}
}

//
// cleanup..
//
//
//
//

static void cleanup( void ) {
	if (key) { free(key); }
	if (fxedata) { free(fxedata); }
	if (fh) { fclose(fh); }
	if (fh_gxb) { fclose(fh_gxb); }
	if (fh_icn) { fclose(fh_icn); }
	if (fh_key) { fclose(fh_key); }
}


//
// parse fxe-header..
//
//
//
//

static int parse_fxe( FILE *fh ) {
	unsigned char buf[FXE_HEADER_SIZE];
	unsigned long x;
	unsigned char *p;
	int n;

	if (fread(buf,1,FXE_HEADER_SIZE,fh) != FXE_HEADER_SIZE) {
		// read error or something..
		fprintf(stderr,"*** failed to read %d bytes..\n",FXE_HEADER_SIZE);
		return -1;
	}

	// check if it's fxe..

	if (strncmp(buf,"fxe ",4)) {
		fprintf(stderr,"*** Not an FXE file..\n");
		return -1;
	}

	// read & output some info stuff..

	p = buf+4;
	GETL_LE(p,x); printf("Total FXE file size is %d bytes\n",x);
	GETL_LE(p,x); printf("FXE info size is %d bytes\n",x);
	memcpy(title,p,32); p += 32;
	memcpy(author,p,32);  p += 32;
	memcpy(reserved,p,16);
	printf("Game title:  %-32s\n",title);
	printf("Game author: %-32s\n",author);
	printf("Reserved:    ");
	printf("%02X%02X%02X%02X %02X%02X%02X%02X "
           "%02X%02X%02X%02X %02X%02X%02X%02X  ",
           *(p+0),*(p+1),*(p+2),*(p+3),*(p+4),*(p+5),*(p+6),*(p+7),
           *(p+8),*(p+9),*(p+10),*(p+11),*(p+12),*(p+13),*(p+14),*(p+15));

    printf("'%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c'\n",
           CH(p+0),CH(p+1),CH(p+2),CH(p+3),CH(p+4),CH(p+5),CH(p+6),CH(p+7),
           CH(p+8),CH(p+9),CH(p+10),CH(p+11),
           CH(p+12),CH(p+13),CH(p+14),CH(p+15));
	p += 16;

	// Icon.. size is 32x32 == 1024 bytes
	
	memcpy(icon,p,1024); p += 1024;

	// file size
	
	GETL_LE(p,x); printf("FXE file data size is %d bytes\n",x);
	filelen = x;
	
	// keysize

	GETL_LE(p,x); printf("FXE encryption key size is %d bytes\n",x);
	keylen = x;

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Usage..
//
// Parameters:
// Returns:
//
//////////////////////////////////////////////////////////////////////////////

static void usage( char **argv ) {
  fprintf(stderr,	"Usage: %s [-options] fxe-file\n"
 					"  Options are:\n"
 					"    o filename - output file name for AFX/GXB file\n"
 					"    k filename - save encryption key (XORed with 0xff)\n"
 					"    i filename - save icon\n"
 					"    g          - add encrytion key to tail of GXB file\n"
 					"    h          - help -> this output\n",argv[0]);
  exit(1);
}

//
// Main.. tube & spaghetti code :)
//
//
//

int main( int argc, char **argv ) {

	int ch;

	if (atexit(cleanup)) {
		fprintf(stderr,"*** Failed to install cleanup function..\n");
		exit(1);
	}

	//
	// Check commandline options..
	//

	while ((ch = getopt(argc,argv,"k:i:go:h")) != -1) {
		switch (ch) {
		case 'k':	// Open key file..
			if ((fh_key = fopen(optarg,"w+b")) == NULL) {
				fprintf(stderr,"*** Failed to open file '%s'..\n",argv[4]);
				exit(1);
			}
			break;
		case 'i':	// Open icon file..
			if ((fh_icn = fopen(optarg,"w+b")) == NULL) {
				fprintf(stderr,"*** Failed to open file '%s'..\n",argv[3]);
				exit(1);
			}
			break;
		case 'g':
			addkey = 1;
			break;
		case 'o':	// Open afx/gxb file..
			if ((fh_gxb = fopen(optarg,"w+b")) == NULL) {
				fprintf(stderr,"*** Failed to open file '%s'..\n",argv[2]);
				exit(1);
			}
			break;
		case '?':
		case 'h':
		default:
			usage(argv);
		}
	}

	if (optind >= argc) {
		usage(argv);
		return 0;
	}

	//
	// Greetings..
	//

	printf("UnFxe v0.2 (c) 2002 Jouni 'Mr.Spiv' Korhonen\n\n");

	// Open fxe..
	
	if ((fh = fopen(argv[optind],"r+b")) == NULL) {
		fprintf(stderr,"*** Failed to open file '%s'..\n",argv[1]);
		exit(1);
	}


	// parse the fxe header..
		
	if (parse_fxe(fh)) { exit(1); }

	// write icon..

	if (fh_icn) {
		if (fwrite(icon,1,1024,fh_icn) != 1024) {
			fprintf(stderr,"*** Failed to write icon data..\n");
			exit(1);
		}
	}

	//
	// Read in the encryption key...
	//

	if ((key = (unsigned char *)malloc(keylen)) == NULL) {
		fprintf(stderr,"*** malloc() failed..\n");
		exit(1);
	}
	if (fread(key,1,keylen,fh) != keylen) {
		fprintf(stderr,"*** Failed to read encryption key..\n");
		exit(1);
	}

	fix_key(keylen,key);

	if (fh_key) {
		if (fwrite(key,1,keylen,fh_key) != keylen) {
			fprintf(stderr,"*** Failed to write encryption key..\n");
			exit(1);
		}
	}

	//
	// Read data & decrypt it..
	//

	if ((fxedata = (unsigned char *)malloc(filelen)) == NULL) {
		fprintf(stderr,"*** malloc() failed..\n");
		exit(1);
	}
	if (fread(fxedata,1,filelen,fh) != filelen) {
		fprintf(stderr,"*** Failed to read fxe data..\n");
		exit(1);
	}

	decrypt_buffer( keylen, key, filelen, fxedata ); 

	if (fh_gxb) {
		if (fwrite(fxedata,1,filelen,fh_gxb) != filelen) {
			fprintf(stderr,"*** Failed to write encrypted fxe data..\n");
			exit(1);
		}
		if (addkey) {
			if (fwrite(key,1,keylen,fh_gxb) != keylen) {
				fprintf(stderr,"*** Failed to write encryption key..\n");
				exit(1);
			}
		}
	}



	// done..
	
	printf("Done..\n");

	return 0;
}
