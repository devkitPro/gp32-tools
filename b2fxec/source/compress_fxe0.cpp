//////////////////////////////////////////////////////////////////////////////
//
// (c) 2002 Jouni Korhonen - mouho@iki.fi
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

#include "compress_fxe0.h"
#include "bitutils.h"

//
//
//


namespace {
  // Our comparison function for qsort()..
  // Sort into descending order..

  int litcomp( const void* a, const void* b ) {
    return reinterpret_cast<const literals*>(b)->count -
           reinterpret_cast<const literals*>(a)->count;
  }
};

//
//
//

CompressFXE0::CompressFXE0( int nblks )
  throw (std::bad_alloc, std::invalid_argument) :
  LZ2min(FXE0_SLIDING_WINDOW,FXE0_BLOCK_SIZE,nblks) {
  //_r = r;
  //_c = c;
}

//
//
//

void CompressFXE0::init( const Params* p ) {
  // these will be known _after_ the file has been compressed..

  _numMatches = 0;
  _numLiterals = 0;
  _numMatchedBytes = 0;

  _originalSize = 0;
  _compressedSize = 0;

  // joo-o

  lzinit( static_cast<const LZParams*>(p) );
}


//////////////////////////////////////////////////////////////////////////////
//
// Description:
//   This method compresses and binary encodes one block of data.
//
// Parameters:
//   b     - [in] A ptr to output buffer
//   size  - [in] size of the output buffer
//   isRaw - [out] false if compression succeeded,
//           true if if compression failed i.e. the block
//           is uncompressible.
//
// Returns:
//   Number of encoded bytes. If 0 then EOF. If < 0 then
//   there was an error..
// 
//////////////////////////////////////////////////////////////////////////////

long CompressFXE0::compressBlock( const Params* info, bool& isRaw) {
	
	const LZParams* lzp = static_cast<const LZParams*>(info);
	unsigned char* e = lzp->buf + lzp->bufSize;
	unsigned short block_header;
	unsigned char* p;
	const MatchInfo* mi;
	long r;
	int i;

  // Zero/initialize callback object storage for matches and literals..

  lzp->c->init();

  p = lzp->buf+FXEP_BLOCK_HEADER_SIZE;
  mi = *lzp->c;
  isRaw = false;
  block_header = 0;

  // LZ-parsing with lazy evaluation..

  //if ((r = processBlock(_r,_c)) <= 0) { return r; }
  if ((r = processBlockLazy( lzp )) <= 0) { return r; }

  _originalSize += r;

  //
  // Initialize bit output stuff..
  //

  _initBitOutput( p );

  //
  // Now we are ready to do the binary encoding of block.
  //

  for (i = 0; i < lzp->c->getCount(); i++) {
    unsigned long off = mi[i].offset;
    unsigned long len = mi[i].length;

    if (len == FXEP_NO_MATCH) {
      unsigned char* s = mi[i].position;
      _numLiterals += off;

      while (off-- > 0) {
        // tag: 1 + ([8])
        _putByte(p,*s++);
        _putBits(p,0x01,1);
      }

      // Check if compression failed..

      if (p >= e) {
        isRaw = true;
        break;
      }
    } else {
      _numMatches++;
      _numMatchedBytes += len;

		//
		// encode match indicator and distance (i.e. offset)
		//

		off--;
		//off = (off - 1 - len) % FXE0_SLIDING_WINDOW;

      
		unsigned long mask = 0x100;	// 1 << FXE0_STRTO
		int nbits = 0;

		while (off >= mask) {
			assert(nbits < (FXE0_STOPO-FXE0_STRTO));	// max offset is 8+5 bits..
			nbits++;
			off -= mask;
			mask <<= 1;
		}

      	mask = (mask - 1) >> 8;

		_putByte(p,(off >> nbits) & 0xff);

		if (nbits < (FXE0_STOPO-FXE0_STRTO)) {
			_putBits(p,0x01,nbits+1+1);
		} else {
			_putBits(p,0x00,nbits+1);
		}
		if (nbits > 0) {
			_putBits(p,off & mask,nbits);		
		}

		//
		// encode match length..
		//

		mask = 1 << 2;	// minimum mach of 2
		nbits = 1;
		
		while (len >= mask && nbits < 5) {
			mask <<= 1;
			nbits++;
		}

		len -= (mask >> 1);
		
		if (nbits < 5) {
			_putBits(p,(1 << nbits) | len, nbits << 1);
		} else {
			_putBits(p,0x00,4);
			while (len >= 255) {
				_putByte(p,0xff);
				len -= 255;
			}
			_putByte(p,len);
		}
    }

    // Check if compression failed..

    if (p >= e) {
      isRaw = true;
      break;
    }
  }

  // Flush remaining bits..

  _flushBitsNoUpdate(p);

  if (!isRaw) {
    block_header |= FXEP_FLAGS_COMPRESSED_MASK;
  } else {
    memcpy(lzp->buf+FXEP_BLOCK_HEADER_SIZE,peekCurrentBuffer(),r);
    p = lzp->buf + FXEP_BLOCK_HEADER_SIZE + r;
  }

  // And finally store the block header..

  r = p - lzp->buf;
  block_header |= r;
	p = lzp->buf;

  PUTW_LE(p,block_header);

  // Calculate compressed size..

  _compressedSize += r;
  return r;
}

//
//
//



