//////////////////////////////////////////////////////////////////////////////
//
// Description:
//   This (protected) class does basic string matching for LZ-type
//   compressors. Following techniques are included into this class:
//
//    - Minimum match length of two characters
//    - Direct hashing i.e. two concatenated bytes are used as hash
//    - Greedy or Lazy Evaluation matching (like gzip & variants)
//    - Gutman's halfmatch.. not used b'cos of zero benefit!
//    - Context confirmation (based on the direct hash)
//    - Circle history buffer (0 for NIL) i.e. no hash buffer copying etc.
//    - Encode everything as literal strings or matches i.e. runs of not
//      matched literals are passed to 'upper layers' as a one string
//    - Link list search cut based on 'good match' and
//      'no better match expected'..
//    - Buffered file handling i.e. data is read in small blocks
//    - Complete data source abstraction i.e. the search engine does not
//      know or care where the data come from
//
//   Still to implement: two level hashing, which would give a major
//   speedup (I'm talking about 4-15 times faster searches here..)
//
// Version:
//   0.1/??-???-1999 JiK - initial
//   0.2/10-Dec-2001 JiK - Some cleanup & speedups..
//   0.3/15-Dec-2001 JiK - Removed unnecessary Context Confirmation as
//                         Direct Hashing includes same functionality
//                         implicitely..
//   0.4/13-Jan-2002 JiK - Added read buffering.. saves IO and memory copies..
//   0.5/18-Jan-2002 JiK - Optimized the _longestMatch()..
//   0.6/21-Jan-2002 JiK - Fixed a bug that in _longestMatch()..
//   0.7/22-Jan-2002 JiK - Yet another bug in _longestMatch().. shouldn't have
//                         go on optimizing the _longestMatch() ofter v0.4
//
// Author:
//   Jouni 'Mr.Spiv' Korhonen (jouni.korhonen@iki.fi)
//
//////////////////////////////////////////////////////////////////////////////

#include <new>
#include "lz2min.h"

//////////////////////////////////////////////////////////////////////////////
//
//
//
// Parameters:
//   hsize  - [in] History or sliding window size in bytes. Must be a
//                 power of 2.
//   bsize  - [in] Block size i.e. anount of data loaded once. Must be
//                 divideable by hsize.
//   mchain - [in] Maximum hash chain length.
//   mmatch - [in] Maximum match length.
//   gmatch - [in] Goog match length.
//   nblks  - [in] Number of blocks read once - save some io..
//
//

// Constructor
//////////////////////////////////////////////////////////////////////////////

LZ2min::LZ2min( int hsize, int bsize, int nblks )
	throw ( std::bad_alloc, std::invalid_argument ) {

  // in case of bad_alloc clear pointers..

  _head = NULL;
  _prev = NULL;
  _b = NULL;

  //

  if (nblks < 1 || bsize > hsize ) {
    throw std::invalid_argument("LZ2min::LZ2min - bad arguments.");
  }

  //

  //_maxChainLen = mchain;
  _windowSize = hsize;
  _blockSize = bsize;
  _totalBufferSize = _windowSize + _blockSize * nblks;
  //_maxMatchLen = mmatch;
  //_maxNonGreedy = 0;
  //_goodMatchLen = gmatch;
  _readSize = _blockSize * nblks;

  //

  try {
    _head = new Hash[LZ2_MAX_HASH];
    _prev = new Prev[_windowSize+1];  // reserve extra entry for NIL nodes..
    _b =    new unsigned char[_totalBufferSize+LZ2_GAP];
  } catch (std::bad_alloc) {
    delete[] _head;
    delete[] _prev;
    delete[] _b;
    throw;
  } catch (...) {
    cerr << "Unknown exception inside LZ2min::LZ2min()" << endl;
  }
}

// destructor

LZ2min::~LZ2min() {
  delete[] _head;
  delete[] _prev;
  delete[] _b;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//

void LZ2min::lzinit( const LZParams* info ) {
  int n;

  _firstTime = true;
  _eof = false;
  _fakeLen = 0;
  _len = 0;
  _pos = _windowSize;

  // nodes stuff..

  _n = 1;
  _nn = 0;

  for (n = 0; n < LZ2_MAX_HASH; n++) {
    _head[n].prev = 0;
    _head[n].llen = 0;
  }
#if 0
  for (n = 0; n < _windowSize; n++) {
    _prev[n].prev = 0;
  }
#endif
  memset(_b+_totalBufferSize,0,LZ2_GAP);

}


//////////////////////////////////////////////////////////////////////////////
//
// Description:
//   This method does simple LZ matching. No bells'n'whistles..
//
// Parameters:
//   r [in] - ptr to Reader used for reading next block data to compress
//   c [in] - ptr to CallBack used for passing match information
//
// Returns:
//   Number of read bytes, or -1 if error.
//
//////////////////////////////////////////////////////////////////////////////

long LZ2min::processBlock( Reader* r, CallBack* c ) {
  int localLen;
  int lastPos;

  if (_firstTime) {
    _firstTime = false;
  } else if ( _fakeLen == 0 && !_eof ) {
    memmove(_b,_b+_len,_windowSize);
    _len = 0;
  }

  // Because of Context Confirmation and in general of proper hashing
  // we do a one byte peek.. not the prettiest solution..

  if ( _fakeLen == 0 ) {
    _len = r->read( _b+_windowSize,_readSize+1 );
    if (_len <= 0) { return _len; }
    if (_len > _readSize) {
      if (r->seek(-1,IO_SEEK_CUR) < 0) { return -1; }
      _len--;
    } else {
      _eof = true;
    }

    _fakeLen = _len;
    _pos = _windowSize;
  }

  if (_fakeLen >= _blockSize) {
    localLen = _blockSize;
  } else {
    localLen = _fakeLen;
  }

  // Reset position within the sliding window 
  // and setup values for the newly read block..

  //_pos = _windowSize;
  _blockEndPos = _pos + localLen;
  lastPos = _pos;
  _initHash(_b+_pos);

  // Do not attemp matching if the read block size is
  // is below the LZ2_THRESHOLD threshold.. Just update
  // the hash table and handle all data as literals..
  // The code is actually same for both LITERAL_STRINGS
  // case and the normal 'single' literal case..

  if (localLen < LZ2_THRESHOLD) {
    c->callback( localLen, LZ2_NO_MATCH, _b+_pos );

    // and update the hash table...

    for (int n = 0; n < localLen; n++) {
      unsigned short h = _hash(_b+_pos);
      _insert(h);
    }
    _fakeLen -= localLen;
    return localLen;
  }

  // Process the newly loaded block of data..

  while (_pos < _blockEndPos) {
    int length, offset;
    unsigned short h = _hash(_b+_pos);
    int p = _head[h].prev;

    // Initially the minimum length MUST be set to 
    // LZ2_MIN_MATCH-1.. otherwise the matching method
    // will bug..

    length = LZ2_MIN_MATCH-1;
    offset = _longestMatch(p,length,_head[h].llen,  NULL);
    
    if (length >= LZ2_MIN_MATCH) {
      if (_pos - lastPos > 0) {
        c->callback( _pos-lastPos, LZ2_NO_MATCH, _b+lastPos );
      }

      // And inform about the match..

      c->callback(offset,length,_b+_pos);

      // Update the hash chain about the best match so far 
      // in thisd chain..

      _prev[p].best = length;

      // Insert remaining strings into the hash table..

      for (int n = 1; n < length; n++) {
        _insert(h);
        h = _hash(_b+_pos);
      }
      lastPos = _pos+1;
    }

    // insert this/last string into to hash table

    _insert(h);
  }

  if (lastPos < _pos) {
    c->callback( _pos-lastPos, LZ2_NO_MATCH, _b+lastPos );
  }

  _fakeLen -= localLen;
  return localLen;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//   This method does LZ matching with lazy evaluation.. i.e. non-greedy
//   matching with one character history (a'la gzip :)
//   This is a specially optimized version of non-greedy matching
//   algorithm. The general purpose non-greedy matching method is
//   the processBlockNG() method..
//
// Parameters:
//   r [in] - ptr to Reader used for reading next block data to compress
//   c [in] - ptr to CallBack used for passing match information
//
// Returns:
//   Number of read bytes, or -1 if error.
//
//////////////////////////////////////////////////////////////////////////////

long LZ2min::processBlockLazy( const LZParams* lzp ) {
	int localLen;
	int lastPos;
	Reader* r = lzp->r;
	CallBack* c = lzp->c;

	//static int foofoo = 0;


	if (_firstTime) {
		_firstTime = false;
	} else if ( _fakeLen == 0 && !_eof ) {
		memmove(_b,_b+_len,_windowSize);
		_len = 0;
	}

  // Because of Context Confirmation and in general of proper hashing
  // we do a one byte peek.. not the prettiest solution..

  if (_fakeLen == 0) {
    _len = r->read( _b + _windowSize,_readSize + 1 );
    if (_len <= 0) { return _len; }
    if (_len > _readSize) {
      if (r->seek(-1,IO_SEEK_CUR) < 0) { return -1; }
      _len--;
    } else {
      _eof = true;
    }

    _fakeLen = _len;
    _pos = _windowSize;
  }

  if (_fakeLen >= _blockSize) {
    localLen = _blockSize;
  } else {
    localLen = _fakeLen;
  }

  // Reset position within the sliding window 
  // and setup values for the newly read block..

  //_pos = _windowSize;
  _blockEndPos = _pos + localLen;
  lastPos = _pos;
  _initHash(_b+_pos);

  // Do not attemp matching if the read block size is
  // is below the LZ2_THRESHOLD threshold.. Just update
  // the hash table and handle all data as literals..
  // The code is actually same for both LITERAL_STRINGS
  // case and the normal 'single' literal case..

  if (localLen < LZ2_THRESHOLD) {
	//cerr << "lefovers: " << foofoo << ", len: " << localLen << endl;
    c->callback( localLen, LZ2_NO_MATCH, _b+_pos );

    // and update the hash table...

    for (int n = 0; n < localLen; n++) {
      unsigned short h = _hash(_b+_pos);
      _insert(h);
    }

    _fakeLen -= localLen;
    return localLen;
  }

  // prevLength and prevOffset are needed for lazy evaluation
  // matching to remember the previous match results..

  //int gm = lzp->goodMatch >> 1;
  int gm = lzp->goodMatch;
  int prevLength, prevOffset;
  int length, offset;

  length = 0;
  offset = 0;

  // Process the newly loaded block of data..

//  int ppos = _pos;

  while (_pos < _blockEndPos) {
    unsigned short h = _hash(_b+_pos);
    int p = _head[h].prev;

    // cache previous match results.. and
    // Initially the minimum length MUST be set to 
    // LZ2_MIN_MATCH-1.. otherwise the matching method
    // will bug..

    prevLength = length;
    prevOffset = offset;
    length = LZ2_MIN_MATCH-1;

    // Do not enter string matching if the previous match
    // was a decent match -> speeds up things some what..

    if (prevLength < gm) {
      offset = _longestMatch(p,length,_head[h].llen,lzp);
    }

    // Did we find a match??

    if ((prevLength == LZ2_MIN_MATCH && prevOffset <= lzp->minOffsetThres) ||
    	(prevLength > LZ2_MIN_MATCH && prevLength >= length)) {
      if (_pos - lastPos - 1 > 0) {
        c->callback( _pos-lastPos-1, LZ2_NO_MATCH, _b+lastPos );
      }

      c->callback(prevOffset,prevLength,_b+_pos);
      _prev[p].best = prevLength;

      // Insert remaining strings into the hash table..

      for (int n = 2; n < prevLength; n++) {
        _insert(h);
        h = _hash(_b+_pos);
      }

      lastPos = _pos+1;
      length = 0;
    }

    // insert this/last string into to hash table

    _insert(h);
  }

  if (lastPos < _pos) {
    c->callback( _pos-lastPos, LZ2_NO_MATCH, _b+lastPos );
  }

  _fakeLen -= localLen;
  return localLen;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//   This method does LZ matching with Horspool's non-greedy
//   matching with MAX_LAZY character history.
//   This is NOT an optimized version of non-greedy matching
//   algorithm (speedwise..)
//
// Parameters:
//   r [in] - ptr to Reader used for reading next block data to compress
//   c [in] - ptr to CallBack used for passing match information
//   maxNG [in] - maximum non-greedy search length
//
// Returns:
//   Number of read bytes, or -1 if error.
//
//////////////////////////////////////////////////////////////////////////////

long LZ2min::processBlockNG( const LZParams* ) {
  // NonGreedy NG[LZ2_MAX_LAZY];

  // not quite implemented..

  return -1;
}


//


void LZ2min::done() {
}


//////////////////////////////////////////////////////////////////////////////
//
//
//
//
// Parameters:
//   i - [in/out] first node in hash chain. Returns index to
//       node which had the longest match.
//   l - [in/out] match length (input is minimum).
//   m - [in] maximum hash chain length.
//
// Return:
//   match offset
//
//////////////////////////////////////////////////////////////////////////////

int LZ2min::_longestMatch( int& i, int& l, int m, const LZParams* lzp ) const {

  int chain = m < lzp->maxChain ? m : lzp->maxChain;
  int n = i;
  unsigned char* hp;
  unsigned char* cp;
  unsigned char* me;
  unsigned char* sp = _b + _pos;
  int o = 0;

  // Cache object's variables to local variables..
  // This will produce a lot better code..

//  unsigned short c = _context;
  int ws = _windowSize;
  int gm = lzp->goodMatch;
  Prev* pr = _prev;

  // Calculate search area end..

  if (_pos + lzp->maxMatch <= _blockEndPos) {
    me = sp + lzp->maxMatch;
  } else {
    me = _b + _blockEndPos;
  }  

  while (n > 0 && chain-- > 0) {
    int d = _n - n;

    if (d <= 0) { d += ws; }

    // sp - current search position
    // cp - copy of sp during string matching
    // hp - histoty search position

    hp = sp - d;

    // First check if it's worth doing a real match..

    if (/*sp[l >> 1] == hp[l >> 1] &&*/       // Gutman's halfmatch..
        sp[l] == hp[l] ) {                // must be longer than previous..

      // modify the starting position..

      cp = sp + 2;
      hp += 2;

      // Search until a mismatch or area end gets reached..

      do {
      } while ( // Must not be more than LZ2_GAP..
        *cp++ == *hp++ && *cp++ == *hp++ && 
        *cp++ == *hp++ && *cp++ == *hp++ && 
        cp <= me);

      // me - search area end..

      cp = cp > me ? me : cp-1;

      //if (cp > me) { cp = me; }
      if (cp - sp > l) {
        l = cp - sp; o = d; i = n;

        // Stop searching if
        // 1) the match is long enough i.e. >= that _goodMatchLen
        // 2) this hash chain does not contain longer matches i.e.
        //    _prev[n].best <= match

        if (l >= gm || pr[n].best <= l) { return o; }
      }
    }

    n = pr[n].prev;
  }
	return o;
}
