//
//
// - Requires C99 compliant compiler.. or a twisted gcc
//
//
//

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "disasm.h"
#include "bitutils.h"

//
//

static int fixed = 0;
static uint8_t *key = NULL;
static uint8_t *buf = NULL;
static long keylen;
static long buflen;



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

#define FXE_HEADER_SIZE (4+4+4+4+4+32+32+16+1024)
#define CH(p) ( isprint(*(p)) ? *(p) : '.' )


//
// Decrypt buffer fxe data with the supplied key..
//
//
//

void decrypt_buffer( long klen, const uint8_t *key, 
					 long blen, unsigned char *buf ) {
	long n = 0;

	while (n < blen) {
		unsigned char c = buf[n] ^ 0xff;	// MVN :)
		buf[n] = c ^ key[n % klen];
		n++;
	}
}

//
//
//
//
//
//

int checkrom( struct disassemble_info *dd, int verbose ) {
	uint8_t *p;
	long x;
	int encrypted = 0;

  // Check for Software Recongnition Code..

  if (strncmp("fxe ",dd->buffer,4)) {
    if (verbose) { printf("Not a FXE file. Base address is set to %X\n",dd->buffer_vma); }
    return -1;
  } else {
    if (verbose) { printf("The file is a FXE file...\n"); }
  }

  //

	p = dd->buffer+4;
	GETL_LE(p,x);
	if (verbose) printf("Total FXE file size is %d bytes\n",x);
	GETL_LE(p,x);
	if (verbose) printf("FXE info size is %d bytes\n",x);
	if (verbose) printf("Game title:  %-32s\n",p); p += 32;
	if (verbose) printf("Game author: %-32s\n",p); p += 32;
	if (verbose) printf("Reserved:    ");
	if (verbose) printf("%02X%02X%02X%02X %02X%02X%02X%02X "
          "%02X%02X%02X%02X %02X%02X%02X%02X  ",
          *(p+0),*(p+1),*(p+2),*(p+3),*(p+4),*(p+5),*(p+6),*(p+7),
          *(p+8),*(p+9),*(p+10),*(p+11),*(p+12),*(p+13),*(p+14),*(p+15));

   	if (verbose) printf("'%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c'\n",
          CH(p+0),CH(p+1),CH(p+2),CH(p+3),CH(p+4),CH(p+5),CH(p+6),CH(p+7),
          CH(p+8),CH(p+9),CH(p+10),CH(p+11),
          CH(p+12),CH(p+13),CH(p+14),CH(p+15));
	p += 16;

	// Icon.. size is 32x32 == 1024 bytes

	p += 1024;

	// file size
	
	buf = p;
	GETL_LE(p,x);
	buflen = x;
	if (verbose) printf("FXE file data size is %d bytes\n",x);
	
	// keysize

	GETL_LE(p,x);
	keylen = x;
	key = p;
	buf = key + keylen;

	if (verbose) printf("FXE encryption key size is %d bytes\n",x);

	while (x-- > 0) {
		if (*p++ != 0xff) {
			if (verbose) printf("FXE is ecrypted and thus won't disassemble correctly!\n");
			encrypted = 1;
			break;
		}
	}

	if (encrypted == 0) {
		x = p - dd->buffer;

		if (verbose) { 
			printf("FXE is not really encrypted. Code starts at %08X\n",
					dd->buffer_vma + x);
		}
		return 1;
	} else {
		return 0;
	}
}

int fixfxe( struct disassemble_info *dd ) {
	int n;

	if (fixed) {
		printf("FXE file has already been decrypted and memory address fixed..\n");
		return 1;
	}
	
	n = checkrom(dd,1);
	
	if (n < 0) { return 1; }
	if (n == 0) { decrypt_buffer(keylen,key,buflen,buf); }

	// fix base..

	dd->buffer_vma = 0x0c000000 - (buf - dd->buffer);

	printf("\nFXE code start fixed to 0x0c000000, memory starts now from %08X\n",
		dd->buffer_vma);
	fixed = 1;

	return 0;
}

