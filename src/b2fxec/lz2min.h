#ifndef _lz2min_h_included
#define _lz2min_h_included

//
//
//
//
//
//
//
//
//

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cassert>

#include "b2fxec_io.h"
#include "cb.h"
#include "params.h"

//
//
//

#define LZ2_NIL 0
#define LZ2_NO_LAZY 0
#define LZ2_MAX_LAZY 8
#define LZ2_MAX_MATCH 32767
#define LZ2_MIN_MATCH 2
#define LZ2_MAX_HASH 65536
#define LZ2_MAX_OPTIMAL 16
#define LZ2_NO_MATCH 0
#define LZ2_THRESHOLD 64
#define LZ2_GAP 8

//
//
//

struct LZParams : public Params {
	long goodMatch;
	long maxMatch;
	long minMatchThres;	// match < minMatchThresh
	long minOffsetThres;	// offset < minOffsetThresh
	int maxChain;
	int numLazy;
	int numDistBits;

	// static area..
	
	long windowSize;
	Reader* r;				//
	CallBack* c;			//
};


//
//
//

class LZ2min {
  struct Hash {
    long prev;
    long llen;
  };
  struct Prev {  // force 8 bytes alignement
    long prev;
    unsigned short best;
    unsigned short hash;  // also acts as a context!!!
  };
  struct NonGreedy {
    int offset;
    int length;
  };

  //

  Hash* _head;
  Prev* _prev;
  unsigned char* _b;
  
  //int _maxChainLen;
  int _windowSize;
  int _blockSize;
  int _totalBufferSize;
  int _blockEndPos;
  //int _maxNonGreedy;
  //int _goodMatchLen;
  //int _maxMatchLen;
  int _readSize;
  int _pos;
  int _len;
  int _fakeLen;
  int _n, _nn;
  unsigned short _context;
  bool _firstTime;
  bool _eof;

  //
  int _longestMatch( int&, int&, int, const LZParams* ) const;
  //
  unsigned short _hash( const unsigned char* b ) {
    // For a minimum match of 2 we do direct table lookup..
    // This requires a 64K table though..
    // And because the hash is 2 bytes we can also use the hash
    // as a context for context confirmation..

    unsigned short h = (_context << 8) | *(b+1); _context = h;
    return h;
  }
  void _initHash( const unsigned char* b ) {
    _context = *b;
  }
  //
  void _insert( unsigned short hash ) {
    Hash* head = _head;
    Prev* prev = _prev;
    int h = head[hash].prev;

    // Delete old node..

#if 1
    if (_nn < _windowSize) {
      _nn++;
    } else {
      int oldh = prev[_n].hash;
      head[oldh].llen--;
    }
#else
    head[prev[_n].hash].llen--;
#endif

    // Insert new node..

    prev[_n].prev = h;
    prev[_n].best = LZ2_MAX_MATCH+1;  // this node has not been visited..
    prev[_n].hash = hash;             // Also the context!!

    // Update hash head..

    head[hash].prev = _n;
    head[hash].llen++;

    // update counters.. and check for overflow..

    _pos++;
  
    // Position 0 in the prev[] table means an empty/NIL position..
    // So the start of hash chain starts from 1..

    if (++_n > _windowSize) { _n = 1; }
  }

  //


protected:
  const unsigned char* const peekCurrentBuffer() const {return _b+_windowSize;}
  long getCurrentBufferLength() const { return _len; }
protected:
  virtual ~LZ2min();
  //  history, block, nblocks 
  LZ2min( int, int, int=1 )
    throw ( std::bad_alloc, std::invalid_argument );
protected:
  long processBlock( Reader*, CallBack* );
  long processBlockLazy( const LZParams*  );
  long processBlockNG( const LZParams* );
  void lzinit( const LZParams* );
  void done();
};



#endif






