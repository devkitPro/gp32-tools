#ifndef _compress_fxp0_h_included
#define _compress_fxp0_h_included

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
#endif
//
//
//
//

#define FXE0_OFFSET_8   256
#define FXE0_OFFSET_9   512
#define FXE0_OFFSET_10  1024
#define FXE0_OFFSET_11  2048
#define FXE0_OFFSET_12  4096
#define FXE0_OFFSET_13  8192

#define FXE0_STRTO  8
#define FXE0_STOPO  13

#define FXE0_SLIDING_WINDOW (FXE0_OFFSET_8+FXE0_OFFSET_9+FXE0_OFFSET_10+FXE0_OFFSET_11\
				+FXE0_OFFSET_12+FXE0_OFFSET_13)
#define FXE0_BLOCK_SIZE FXE0_SLIDING_WINDOW
#define FXE0_GOOD_MATCH           15+254


//
// Description of the binary encoding of the NGPack4 algorithm..
//
//  (nn) means a complete byte encoded into byte boundary.
//  [nn] means a 'nn' encoded bits.
//
// Literals:
//
//  1 + ([8])             -> one literal
//
// Matches:
//
//  0 + D + C
//
//  C:
//  1    + [1]     -> 2-3
//  01   + [2]     -> 4-7
//  001  + [3]     -> 8-15
//  0001 + [4]     -> 16-31
//  0000 + (n*[8]) -> 32-2048 (if [8] == 0xff then read next [8] etc..)
//
//  D:
//  ([8]) + 1           -> 1-256       ;  9 bits
//  ([8]) + 01    + [1] -> 257-768     ; 11 bits
//  ([8]) + 001   + [2] -> 769-1792    ; 13 bits
//  ([8]) + 0001  + [3] -> 1793-3840   ; 15 bits
//  ([8]) + 00001 + [4] -> 3841-7936   ; 17 bits
//  ([8]) + 00000 + [5] -> 7939-16128  ; 18 bits
//
//
// minimum encodings: match 2 is benefical to encode
//  match 2-3 offset 1-256    -> 1+2+9  == 12 bits
//  match 2-3 offset 257-768  -> 1+2+11 == 14 bits
//  match 2-3 offset 769-1792 -> 1+2+13 == 16 bits
//
// minimum encodings: match 3 is benefical to encode
//  match 2-3 offset ...
//  match 2-3 offset 7939-16128 -> 1+2+18 == 21 bits
//

#ifdef __cplusplus
class CompressFXE0 : public Compress, private LZ2min {
 private:
  // Bit manipulation stuff..

  int _bc;            // bit buffer counter
  unsigned long _bb;  // bit buffer
  unsigned char* _cp; // control output pointer..

  //

  void _putBits( unsigned char*& b, unsigned short w, int c ) {
	  _bb <<= c;
	  _bb |= w;

	  if (_bc <= c) {
		  c -= _bc;
		  PUTB(_cp,_bb >> c);
		  _bc = 8;
      _cp = b++;
	  }
	  _bc -= c;
  }

  void _putByte( unsigned char*& p, unsigned char b ) {
    PUTB(p,b);
  }

  void _flushBitsNoUpdate( unsigned char*& b ) {
	  if (_bc & 7) {
		  _putBits(b,0,_bc & 7);
      b--;
	  }
  }

  void _initBitOutput( unsigned char*& b ) {
    _cp = b++;
	  _bc = 8;
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
	virtual ~CompressFXE0() {}
	CompressFXE0( int=1 )
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

	unsigned long getTypeID() const { return FXEP_TYPE0_ID; }

	//
	
};
#endif
#endif

