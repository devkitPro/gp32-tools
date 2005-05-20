//
// UnFxe v0.2 (c) 2002 Jouni 'Mr.Spiv' Korhonen
//
//
// - Requires C99 compliant compiler.. or a twisted gcc
//
//
//

#include <iostream>
#include <cstring>
#include "unfxe.h"
#include "bitutils.h"
#include "port.h"

using namespace std;

//
// Fxe-header structure - all numbers are in little-endian:
//
//    +---+---+---+---+
//    |'f'|'x'|'e'|' '|
//    +---+---+---+---+
//
//    +---+---+---+---+
//    | TOTALFILESIZE | == 4 + 4 + 4 + 32 + 32 + 16 + 1024 + 4 + 4 + FILESIZE + KEYSIZE
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
// 



//
// Decrypt buffer fxe data with the supplied key..
//
//
//


void UnFXE::fix_key( long klen, unsigned char *key ) {
	long n = 0;
	while (n < klen) {
		key[n++] ^= 0xff;
	}
}

unsigned char* UnFXE::decrypt_buffer( long klen, const unsigned char *key, 
					 long blen, unsigned char *buf, unsigned char* dst ) {
	long n = 0;
	
	while (n < blen) {
		unsigned char c = buf[n];
		dst[n] = c ^ key[n % klen];
		n++;
	}
	return dst+blen;
}


//
// parse fxe-header..
//
// Returns:
//   NULL if not an FXE, otherwise a pointer to the key.
//

unsigned char* UnFXE::parse_fxe( unsigned char *p, int fxelen ) {
	// check if it's fxe..

	if (b2fxec_strncasecmp(reinterpret_cast<const char*>(p),"fxe ",4)) {
		return NULL;
	}
	if (!strncmp(reinterpret_cast<const char*>(p+76),"**StoneCracker**",16)) {
		cout << "++ Warning: the FXE is already compressed with b2fxec.." << endl;
	}
	

	// read & output some info stuff..

	memcpy(m_gamename,p+12,32);
	memcpy(m_authorname,p+12+32,32);
	memcpy(m_icon,p+12+32+32+16,1024);

	//

	p = p+4+4+4+32+32+16+1024;
	//filelen = *(long *)p;
	//p += 4;
	GETL_LE(p,m_filelen);
	
	// keysize

	//keylen = *(long *)p;
	//p += 4;
	GETL_LE(p,m_keylen);

	//cerr << "title: " << gamename << endl;
	//cerr << "author: " << authorname << endl;
	//cerr << "filelen: " << filelen << endl;
	//cerr << "keylen: " << keylen << endl;


	return p;
}


//
// Main.. tube & spaghetti code :)
//
//
//

unsigned char* UnFXE::unfxe( unsigned char *dst, int& datalen ) {
	// parse the fxe header..
	unsigned char *data;

	m_fxedata = NULL;

	//

	if ((data = parse_fxe(dst,datalen)) == NULL) { 
		return NULL;
	}
	
	m_key = new unsigned char[m_keylen];
	memcpy(m_key,data,m_keylen);
	m_fxedata = data+m_keylen;

	//
	// key points now to the encryption key...
	// data points to GXB data now..
	//

	fix_key(m_keylen,m_key);

	//
	// Read data & decrypt it..
	//

	data = decrypt_buffer( m_keylen, m_key, m_filelen, m_fxedata, dst ); 

	// move key after the GXB.. as it is the data section..

	memcpy(data,m_key,m_keylen);
	delete[] m_key;

	//

	datalen = m_filelen + m_keylen;
	return dst;
}
