#ifndef _compress_fxp2_h_included
#define _compress_fxp2_h_included

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
//
//
//

#define FXE2_OFFSET_0   1
#define FXE2_OFFSET_1   2
#define FXE2_OFFSET_2   4
#define FXE2_OFFSET_3   8
#define FXE2_OFFSET_4   16
#define FXE2_OFFSET_5   32
#define FXE2_OFFSET_6   64
#define FXE2_OFFSET_7   128
#define FXE2_OFFSET_8   256
#define FXE2_OFFSET_9   512
#define FXE2_OFFSET_10  1024
#define FXE2_OFFSET_11  2048
#define FXE2_OFFSET_12  4096
#define FXE2_OFFSET_13  8192
#define FXE2_OFFSET_14  16384
#define FXE2_OFFSET_15  32768	//

#define FXE2_STRTO  0
//!#define FXE2_STOPO  14
#define FXE2_STOPO  15

#define FXE2_SLIDING_WINDOW ( \
				+FXE2_OFFSET_0+FXE2_OFFSET_1+FXE2_OFFSET_2+FXE2_OFFSET_3\
				+FXE2_OFFSET_4+FXE2_OFFSET_5+FXE2_OFFSET_6+FXE2_OFFSET_7\
				+FXE2_OFFSET_8+FXE2_OFFSET_9+FXE2_OFFSET_10+FXE2_OFFSET_11\
				+FXE2_OFFSET_12+FXE2_OFFSET_13+FXE2_OFFSET_14)
#define FXE2_BLOCK_SIZE			FXE2_SLIDING_WINDOW	// 32768
#define FXE2_MAX_MATCH			256
#define FXE2_GOOD_MATCH			FXE2_MAX_MATCH

#define FXE2_EOB				256
#define FXE2_HUFFBASE			257


#define FXE2_PRE_SYMBOLS		16
#define FXE2_MAX_CODELEN		15
#define FXE2_NUM_SYMBOLS		256+8*15+1
//#define FXE2_NUM_SYMBOLS		256+8*16+1

//
// Description of the binary encoding of the FXE2 algorithm..
//
//  (nn) means a complete byte encoded into byte boundary.
//  [nn] means a 'nn' encoded bits.
//  [H]  means a Huffman encoded tag
//
// Literals:
//
//  [H] < 256        -> H = one literal
//
// Matches:
//
//  [H] 256 = end of block
//
//  [H] 257->261     -> C
//      C = 0 -> match 2
//      C = 1 -> match 3-4
//      C = 2 -> match 5-8
//      C = 3 -> match 9-16
//      C = 4 -> match 17-32
//      C = 5 -> match 33-64
//      C = 6 -> match 65-128
//      C = 7 -> match 129-256
//
// Offsets:
//  [H] 0->13
//      O = 0  -> offset 1
//      O = 1  -> offset 2-3
//      O = 2  -> offset 4-7
//      O = 3  -> offset 8-15
//      O = 4  -> offset 16-31
//      O = 5  -> offset 32-63
//      O = 6  -> offset 64-127
//      O = 7  -> offset 128-255
//      O = 8  -> offset 256-511
//      O = 9  -> offset 512-1023
//      O = 0  -> offset 1024-2047
//      O = 11 -> offset 2048-4095
//      O = 12 -> offset 4096-8191
//      O = 13 -> offset 8192-16383
//      O = 14 -> offset 16383-32767
//
//
//
// Algorithm for decompression:
//
// while (stuff_to_decompress) {
//   h = huffdecode();
//
//   if (h < 256) {
//     output_literal(h);
//   } else {
//     h -= 256;
//     if (h < 4) {
//     } else {
//     }
// ...
//     while (length--) { copy_from_offset( offset ); }
//   }
// }
//
// Header is as follows (all in little endian):
//
//   +---+---+
//   | OSIZE | Original uncompressed block size
//   +---+---+
//
//   +---+---+---+---+---+---+
//   |PREHUFFMAN TREE (3BITS)| 16x3 bits..
//   +---+---+---+---+---+---+
//
//   +-=======================-+
//   |MATCHHUFFMAN TREE (4BITS)| 264x4 bits encoded with prehuffman tree
//   +-=======================-+
//
//   +-========================-+
//   |OFFSETHUFFMAN TREE (4BITS)| 14x4 bits encoded with prehuffman tree
//   +-========================-+
//
//   +-===============-+
//   | compressed data |
//   +-===============-+
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
class CompressFXE2 : public Compress, private LZ2min {
 private:
	// Canonical Huffman
	
	EncHuf* _prehf;
	EncHuf* _dechf;
	unsigned char _preCodes[FXE2_PRE_SYMBOLS];
	unsigned char _blockCodes[FXE2_NUM_SYMBOLS];
	unsigned char _deltaCodes[FXE2_NUM_SYMBOLS];

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

	//
	//
	//

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
	virtual ~CompressFXE2();
	CompressFXE2( int=1 )
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

	unsigned long getTypeID() const { return FXEP_TYPE2_ID; }

	//
	
};
#endif
#endif

