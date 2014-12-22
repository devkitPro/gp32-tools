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

#include "compress_fxe2.h"
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

CompressFXE2::CompressFXE2( int nblks )
	throw (std::bad_alloc, std::invalid_argument) :
	LZ2min(FXE2_SLIDING_WINDOW,FXE2_BLOCK_SIZE,nblks) {

	_dechf = new EncHuf(FXE2_NUM_SYMBOLS,FXE2_MAX_CODELEN);
	_prehf = new EncHuf(FXE2_PRE_SYMBOLS,7);
}

//
//
//

CompressFXE2::~CompressFXE2() {
	delete _dechf;
	delete _prehf;
}

//
//
//

void CompressFXE2::init( const Params* p ) {
	// these will be known _after_ the file has been compressed..

	_numMatches = 0;
	_numLiterals = 0;
	_numMatchedBytes = 0;

	_originalSize = 0;
	_compressedSize = 0;

	// joo-o

	lzinit( static_cast<const LZParams*>(p) );
	
	// Init canonical Huffman delta coding table--

	memset(_preCodes,0,FXE2_PRE_SYMBOLS);
	memset(_blockCodes,0,FXE2_NUM_SYMBOLS);
	memset(_deltaCodes,0,FXE2_NUM_SYMBOLS);
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

long CompressFXE2::compressBlock( const Params* info, bool& isRaw) {
	
	const LZParams* lzp = static_cast<const LZParams*>(info);
//	unsigned char* e = lzp->buf + lzp->bufSize;
	unsigned short block_header;
	unsigned char* p;
	MatchInfo* mi;
	long r;
	int i, j;


	//static int cnt = 0;

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

	_dechf->init();
	_prehf->init();

	//
	// First calculate the Canonical Huffman encoding
	// tables for use in the next round..
	//

	for (i = 0; i < lzp->c->getCount(); i++) {
		int off = mi[i].offset;
		int len = mi[i].length;

		assert(len < 257);
		assert(off < 32768);
		//assert(off < 65536);

		if (len == FXEP_NO_MATCH) {
			unsigned char* s = mi[i].position;
			_numLiterals += off;

			while (off-- > 0) {
				_dechf->addCnt(*s++);
			}
		} else {
			int offset_mask = 0x2;	// 1 << FXE2_STRTO
			int length_mask = 0x2;
			int lbits = 0;
			int obits = 0;
			int sym;

			//cout << "Match: " << len << ", " << off << ", " << (_numMatchedBytes+_numLiterals) << endl;

			_numMatches++;
			_numMatchedBytes += len;
			len--;

			while (off >= offset_mask) {	// offset
				offset_mask <<= 1;
				obits++;
			}
			while (len >= length_mask) {	// length
				length_mask <<= 1;
				lbits++;
			}

			assert(lbits < 8);
			assert(obits < 15);
			//assert(obits < 16);

			sym = FXE2_HUFFBASE + (lbits | (obits << 3));
			assert(sym < FXE2_NUM_SYMBOLS);

			_dechf->addCnt(sym);

			// save sym for later use.. also store "calculated" offset..

			mi[i].symbol = sym;
		}
	}

	_dechf->addCnt(FXE2_EOB);


	// Canonical code coding..

	_dechf->buildCanonicalCodes();
	memcpy(_blockCodes,_dechf->outputCodeLengthsBytes(),FXE2_NUM_SYMBOLS);

	// Encode coded real encoding tree..

	for (i = 0; i < FXE2_NUM_SYMBOLS; i++) {
		_prehf->addCnt(_blockCodes[i]);
	}

	// Pre Tree..

	_prehf->buildCanonicalCodes();
	memcpy(_preCodes,_prehf->outputCodeLengthsBytes(),FXE2_PRE_SYMBOLS);

	// output pre huffman tree.. 3 bits for each leaf..


	for (i = 0; i < FXE2_PRE_SYMBOLS; i++) {
		assert(_preCodes[i] < 8);
		_putBits(p,_preCodes[i],3);
	}

	// Encode the real coding huffman tree..

	int bb = 0;

	for (i = j = 0; i < FXE2_NUM_SYMBOLS; i++) {
		long sym = _blockCodes[i];
		assert(_prehf->getLength(sym) > 0);
		bb += _prehf->getLength(sym);
		_putBits(p,_prehf->getCode(sym),_prehf->getLength(sym));

		assert((1 << _prehf->getLength(sym)) >  _prehf->getCode(sym) );
	}

	//exit(0);

	//cout << "header: " <<  << bb << ", " << ((bb+7) >> 3) << endl;

	//
	// Now we are ready to do the binary encoding of block.
	//

	for (i = 0; i < lzp->c->getCount(); i++) {
		unsigned long off = mi[i].offset;
		unsigned long len = mi[i].length;

		if (len == FXEP_NO_MATCH) {
			unsigned char* s = mi[i].position;

			while (off-- > 0) {
				int sym = *s++;
				_putBits(p,_dechf->getCode(sym),_dechf->getLength(sym));
			}
		} else {
			unsigned long sym = mi[i].symbol;
			int mbits = (sym - FXE2_HUFFBASE) & 0x07;
			int obits = (sym - FXE2_HUFFBASE) >> 3;

			// Encode match & length..

			_putBits(p,_dechf->getCode(sym),_dechf->getLength(sym));


	//if (cnt == 45) {
		//cerr << "sym: " << sym << endl;
		//cerr << "code: " << _dechf->getCode(sym) << endl;
		//cerr << "len: " << _dechf->getLength(sym) << endl;
		//cerr << "len: " << len << ", " << mbits << endl;
		//cerr << "off: " << off << ", " << obits << endl;
		//cerr << "_bc: " << _bc << " _bb: " << (_bb & 0xffff) << ", ";


			if (mbits > 0) {		// length
				_putBits(p,((len-1) ^ (1 << mbits)),mbits);
			}
			if (obits > 0) {		// offset > 1
				_putBits(p,(off ^ (1 << obits)),obits);
			}

		//cerr << "_bc: " << _bc << " _bb: " << (_bb & 0xffff) << endl << endl;
	//}

    	}
  	}

	// Max end of block
	
	_putBits(p,_dechf->getCode(FXE2_EOB),_dechf->getLength(FXE2_EOB));

	//cnt++;

	//cerr << "FXE2_EOB: " << _dechf->getCode(FXE2_EOB) << ", " << _dechf->getLength(FXE2_EOB) << endl;

	// Flush remaining bits.. and align to 16 bits

	_flushBits(p);

	// And finally store the block header..

	r = p - lzp->buf;
	block_header = r;
	p = lzp->buf;

	//cout << r << endl;


	PUTW_LE(p,block_header);

	// Calculate compressed size..

	_compressedSize += r;
	return r;
}

//
//
//



