//////////////////////////////////////////////////////////////////////////////
//
// (c) 2002-3 Jouni Korhonen - mouho@iki.fi
//
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "compress_fxe3.h"
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

CompressFXE3::CompressFXE3( int nblks )
	throw (std::bad_alloc, std::invalid_argument) :
	LZ2min(FXE3_SLIDING_WINDOW,FXE3_BLOCK_SIZE,nblks) {

	_prehf      = new EncHuf(FXE3_PRE_SYMBOLS,7);
	_litmatchhf = new EncHuf(FXE3_NUM_LITMATCH,FXE3_MAX_CODELEN);
	_offhghhf   = new EncHuf(FXE3_NUM_OFFHGH,FXE3_MAX_CODELEN);
	_offlowhf   = new EncHuf(FXE3_NUM_OFFLOW,FXE3_MAX_CODELEN);


}

//
//
//

CompressFXE3::~CompressFXE3() {
	//delete _dechf;
	//delete _prehf;
}

//
//
//

void CompressFXE3::init( const Params* p ) {
	// these will be known _after_ the file has been compressed..

	_numMatches = 0;
	_numLiterals = 0;
	_numMatchedBytes = 0;

	_originalSize = 0;
	_compressedSize = 0;

	// joo-o

	lzinit( static_cast<const LZParams*>(p) );
	
	// Init canonical Huffman delta coding table--

	memset(_preCodes,0,FXE3_PRE_SYMBOLS);
	memset(_blockCodes,0,FXE3_NUM_SYMBOLS);
	memset(_deltaCodes,0,FXE3_NUM_SYMBOLS);

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
// Note:
//   Huffman trees get calculated for every second compressed block only!
//   This trick increases compression efficiency due less header overhead.
// 
// 
//////////////////////////////////////////////////////////////////////////////

long CompressFXE3::compressBlock( const Params* info, bool& isRaw) {
	
	const LZParams* lzp = static_cast<const LZParams*>(info);
//	unsigned char* e = lzp->buf + lzp->bufSize;
	unsigned short block_header;
	unsigned char*p;
	unsigned char c, pending;
	MatchInfo* mi;
	long r;
	int i, j;

	long oldOffset, oldLength, oldOffsetLong;

	// Zero/initialize callback object storage for matches and literals..

	isRaw = false;

	lzp->c->init();

	p = lzp->buf+FXEP_BLOCK_HEADER_SIZE;
	mi = *lzp->c;

	// LZ-parsing with lazy evaluation..

	if ((r = processBlockLazy( lzp )) <= 0) { return r; }
	//if ((r = processBlock( lzp )) <= 0) { return r; }

	_originalSize += r;

	// Initialize bit output stuff..

	_initBitOutput( p );

	// Init huffmann stuff

	_prehf->init();
	_litmatchhf->init();
	_offhghhf->init();
	_offlowhf->init();

	//
	//
	//
	
	oldOffset = -1;
	oldLength = -1;
	oldOffsetLong = -1;	//OLDOFFSETLONG;

	//
	// First calculate the Canonical Huffman encoding
	// tables for use in the next round..
	//

	for (i = 0; i < lzp->c->getCount(); i++) {
		int off = mi[i].offset;
		int len = mi[i].length;

		assert(len <= FXE3_MAX_MATCH);
		assert(off < 32768);

		if (len == FXEP_NO_MATCH) {
			unsigned char* s = mi[i].position;
			_numLiterals += off;

			while (off-- > 0) {
				_litmatchhf->addCnt(*s++);
			}
		} else {
			int sym;

			_numMatches++;
			_numMatchedBytes += len;

			if (oldOffset == off && oldLength == len) {
				sym = FXE3_PMR;
				_litmatchhf->addCnt(sym);
			} else {

				sym = FXE3_HUFFBASE + len - 2;
				assert(sym < FXE3_NUM_LITMATCH);
				_litmatchhf->addCnt(sym);
			
				if (off < 128) {
					_offhghhf->addCnt(off);
				} else if (off == oldOffsetLong) {
					_offhghhf->addCnt(0);
				} else {
					_offhghhf->addCnt((off >> 8) | 0x80);
					_offlowhf->addCnt(off & 0xff);
					oldOffsetLong = off;
				}
				oldOffset = off;
				oldLength = len;
			}

			// save sym for later use.. also store "calculated" length..

			mi[i].symbol = sym;
		}
	}

	_litmatchhf->addCnt(FXE3_EOB);


	// Canonical code coding..

	if (_litmatchhf->buildCanonicalCodes()) {
		cerr << "_litmatchhf->buildCanonicalCodes()" << endl;
	}
	if (_offhghhf->buildCanonicalCodes()) {
		cerr << "_offhghhf->buildCanonicalCodes()" << endl;
	}
	if (_offlowhf->buildCanonicalCodes()) {
		cerr << "_offlowhf->buildCanonicalCodes()" << endl;
	}
	memcpy(_blockCodes,_litmatchhf->outputCodeLengthsBytes(),FXE3_NUM_LITMATCH);
	memcpy(_blockCodes+FXE3_NUM_LITMATCH,_offhghhf->outputCodeLengthsBytes(),FXE3_NUM_OFFHGH);
	memcpy(_blockCodes+FXE3_NUM_LITMATCH+FXE3_NUM_OFFHGH,_offlowhf->outputCodeLengthsBytes(),FXE3_NUM_OFFLOW);

	//fwrite(_blockCodes,1,1024,stderr);


	//
	// Now do a MTF transformation to node depths..
	//

	for (i = 0; i < MTFSIZE; i++) {
		T[i] = i;
	}
	for (i = 0; i < FXE3_NUM_SYMBOLS; i++) {
		c = pending = _blockCodes[i];
		j = 0;
		while (T[j] != c) {
			char swap;
			swap = T[j]; T[j++] = pending; pending = swap;
		}
		_blockCodes[i] = j;
		T[j] = pending;
	}
	
	//
	//
	//



	// Encode coded real encoding tree..

	for (i = 0; i < FXE3_NUM_SYMBOLS; i++) {
		_prehf->addCnt(_blockCodes[i]);
	}

	// Pre Tree..

	_prehf->buildCanonicalCodes();
	memcpy(_preCodes,_prehf->outputCodeLengthsBytes(),FXE3_PRE_SYMBOLS);

	// output pre huffman tree.. 3 bits for each leaf..

	int puuppa = 0;

	for (i = 0; i < FXE3_PRE_SYMBOLS; i++) {
		assert(_preCodes[i] < 8);
		_putBits(p,_preCodes[i],3);
		puuppa += 3;
	}

	// Encode the real coding huffman tree..
	//

	int bb = 0;

	for (i = j = 0; i < FXE3_NUM_SYMBOLS; i++) {
		long sym = _blockCodes[i];
		assert(_prehf->getLength(sym) > 0);
		bb += _prehf->getLength(sym);
		_putBits(p,_prehf->getCode(sym),_prehf->getLength(sym));
		puuppa += _prehf->getLength(sym);
	}
	
	//
	// Now we are ready to do the binary encoding of block.
	//

	oldOffsetLong = -1;	//OLDOFFSETLONG;

	for (i = 0; i < lzp->c->getCount(); i++) {
		/*unsigned*/ long off = mi[i].offset;
		/*unsigned*/ long len = mi[i].length;

		if (len == FXEP_NO_MATCH) {
			unsigned char* s = mi[i].position;

			while (off-- > 0) {
				int sym = *s++;
				_putBits(p,_litmatchhf->getCode(sym),_litmatchhf->getLength(sym));
			}
		} else {
			unsigned long sym = mi[i].symbol;

			// Encode match & length..

			//assert(sym >= FXE3_HUFFBASE);
			assert(sym >= FXE3_PMR);
			assert(sym < FXE3_NUM_LITMATCH);
			assert(_litmatchhf->getLength(sym) > 0);

			_putBits(p,_litmatchhf->getCode(sym),_litmatchhf->getLength(sym));

			if (sym != FXE3_PMR) {
				if (off < 128) {
					_putBits(p,_offhghhf->getCode(off),_offhghhf->getLength(off));
				} else if (off == oldOffsetLong) {
					_putBits(p,_offhghhf->getCode(0),_offhghhf->getLength(0));
				} else {
					_putBits(p,_offhghhf->getCode((off >> 8) | 0x80),_offhghhf->getLength((off >> 8) | 0x80));
					_putBits(p,_offlowhf->getCode(off & 0xff),_offlowhf->getLength(off & 0xff));
					oldOffsetLong = off;
				}
			}
    	}
  	}

	// Max end of block
	
	_putBits(p,_litmatchhf->getCode(FXE3_EOB),_litmatchhf->getLength(FXE3_EOB));

	// Flush remaining bits.. and align to 16 bits

	_flushBits(p);

	// And finally store the block header..

	r = p - lzp->buf;
	block_header = r;
	p = lzp->buf;

	assert(r > 0);

	PUTW_LE(p,block_header);

	// Calculate compressed size..

	_compressedSize += r;

	//cout << endl << r << " - " << _compressedSize << " - " << lzp->c->getCount();


	return r;
}

//
//
//



