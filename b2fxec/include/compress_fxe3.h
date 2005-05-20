#ifndef _compress_fxp3_h_included
#define _compress_fxp3_h_included

//
// This include file is also loadable from a
// C99 compliant C-compiler.. 
// Though when included from a C-compiler the only
// thing you will get are defines!
//

#ifdef __cplusplus
#include <stdexcept>
#include <new>

#include "b2fxec_io.h"
#include "cb.h"
#include "fxe_def.h"
#include "lz2min.h"
#include "compress.h"
#include "bitutils.h"
#include "chuff.h"

#endif

//
// This is the total huffman version.. no bitstrings!
//
//
//
//
// FXE3 uses new way of encoding offsets compared to FXE2.
// Total 3 huffman trees are required:
//
// Literals_match: 512 symbols
//  0-255   literal
//  256     EOF
//  257-511 Match_length-256-2
//
//
// Offset_hgh: 256 symbols
//  0-127   Offset-1    -> offset+1
//  128-255 Offset_hgh  -> (offset-128) << 8
//
// Offset_low: 256 symbols (if offset_hgh > 127)
//  0-255   offset low 8 bits
//
//  Offset _> (offset_hgh - 128) << 8 | offset_low
//
//


#define AFTERMATCHFREQ		16384
#define AFTERLITERALFREQ	16384
#define LOWOFFSETFREQ		16384
#define HIGHOFFSETFREQ		16384
#define ESCAPEFREQ			16384

#define OLDOFFSETLONG	128
#define MTFSIZE			16


#define FXE3_OFFSET_0   1
#define FXE3_OFFSET_1   2
#define FXE3_OFFSET_2   4
#define FXE3_OFFSET_3   8
#define FXE3_OFFSET_4   16
#define FXE3_OFFSET_5   32
#define FXE3_OFFSET_6   64
#define FXE3_OFFSET_7   128
#define FXE3_OFFSET_8   256
#define FXE3_OFFSET_9   512
#define FXE3_OFFSET_10  1024
#define FXE3_OFFSET_11  2048
#define FXE3_OFFSET_12  4096
#define FXE3_OFFSET_13  8192
#define FXE3_OFFSET_14  16384
#define FXE3_OFFSET_15  32768	//

#define FXE3_STRTO  0
//!#define FXE3_STOPO  14
#define FXE3_STOPO  15

#define FXE3_SLIDING_WINDOW ( \
				+FXE3_OFFSET_0+FXE3_OFFSET_1+FXE3_OFFSET_2+FXE3_OFFSET_3\
				+FXE3_OFFSET_4+FXE3_OFFSET_5+FXE3_OFFSET_6+FXE3_OFFSET_7\
				+FXE3_OFFSET_8+FXE3_OFFSET_9+FXE3_OFFSET_10+FXE3_OFFSET_11\
				+FXE3_OFFSET_12+FXE3_OFFSET_13+FXE3_OFFSET_14)
#define FXE3_BLOCK_SIZE			FXE3_SLIDING_WINDOW	// 32768
#define FXE3_MAX_MATCH			(256-1)
#define FXE3_GOOD_MATCH			FXE3_MAX_MATCH

#define FXE3_EOB				256
#define FXE3_PMR				257
#define FXE3_HUFFBASE			(257+1)


#define FXE3_PRE_SYMBOLS		16
#define FXE3_MAX_CODELEN		15
#define FXE3_NUM_LITMATCH		(512)
#define FXE3_NUM_OFFLOW			256
#define FXE3_NUM_OFFHGH			256
#define FXE3_NUM_SYMBOLS		(FXE3_NUM_LITMATCH+FXE3_NUM_OFFLOW+FXE3_NUM_OFFHGH)

//
// Description of the binary encoding of the FXE3 algorithm..
//
//  (nn) means a complete byte encoded into byte boundary.
//  [nn] means a 'nn' encoded bits.
//  [H]  means a Huffman encoded tag
//
// Literals:
//

//
// Matches:
//

//
// Offsets:
//
//
//
// Algorithm for decompression:
//
//
// Header is as follows (all in little endian):
//
//   +---+---+
//   | OSIZE | Original uncompressed block size
//   +---+---+

//
//   Block MUST be aligned to 16 bits boundary
//
//
//
//
//
//
//
//

#ifdef __cplusplus
class CompressFXE3 : public Compress, private LZ2min {
 private:

	// Huffman trees..
	
	EncHuf* _prehf;
	EncHuf* _litmatchhf;
	EncHuf* _offhghhf;
	EncHuf* _offlowhf;
	unsigned char _preCodes[FXE3_PRE_SYMBOLS];
	unsigned char _blockCodes[FXE3_NUM_SYMBOLS];
	unsigned char _deltaCodes[FXE3_NUM_SYMBOLS];
	unsigned char T[FXE3_NUM_SYMBOLS];				// for the MTF


	// Bit manipulation stuff..

	int _bc;            // bit buffer counter
	unsigned long _bb;  // bit buffer

	// 16bits versions..

	void _putBits( unsigned char*& b, unsigned short w, int c ) {
		_bb <<= c;
		_bb |= w;

		if (_bc <= c) {
			c -= _bc;
			PUTW_LE(b,(_bb >> c));
			_bc = 16;
		}
		_bc -= c;
	}

	void _putByte( unsigned char*& p, unsigned char b ) {
		_putBits(p,b,8);
	}

	void _flushBits( unsigned char*& b ) {
		if (_bc & 15) {
			_putBits(b,0,_bc & 15);
		}
	}

	void _initBitOutput( unsigned char*& b ) {
		_bc = 16;
		_bb = 0;
	}

	//       ^..^
	//       (oo)
	

 private:
  // Useless statistics

	long _compressedSize;
	long _originalSize;
	long _numMatches;
	long _numLiterals;
	long _numMatchedBytes;

	//

	//Reader* _r;
	//CallBack* _c;
 public:
	virtual ~CompressFXE3();
	CompressFXE3( int=1 )
    throw(std::bad_alloc, std::invalid_argument);

	void init( const Params* );
	long compressBlock( const Params* , bool& );

	// statistics stuff..

	long getNumMatches() const { return _numMatches; }
	long getNumLiterals() const { return _numLiterals; }
	long getNumMatchedBytes() const { return _numMatchedBytes; }

	// long long for NGPC?? :) :)

	long getCompressedSize() const { return _compressedSize; }
	long getOriginalSize() const { return _originalSize; }

	// some ID etc stuff

	unsigned long getTypeID() const { return FXEP_TYPE3_ID; }

	//
	
};
#endif
#endif

