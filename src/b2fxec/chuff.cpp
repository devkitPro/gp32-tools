/////////////////////////////////////////////////////////////////////
//
// Module: chuff.cpp
//
// Author: Jouni 'Mr.Spiv' Korhonen 
//
// Purpose: methods for class chuf. These methods and
//   member functions calculate canonical codes for
//   given alphabet & frequency distribution.
//
//   This cacnonical huffman encoder uses following nice stuff:
//   - tree balancing i.e. one can define maximum depth for the tree
//   - Moffat's inplace linear calculation of minimum redundancy codes
//
//   To be honest this encoder has never seen any form of binary tree :)
//
// Restrictions: One symbol frequency is not allowed to
//   exceed 32767 and total frequency must not exceed
//   65535! (???)
//
// Revision:
//   v0.1 -> 08-Aug-1998 JiK - initial version.
//    0.2 -> 19-Sep-1998 JiK - made to handle endianes.
//    0.3 -> 01-Aug-1999 JiK - minor naming changes & prototype
//      redefinitions.
//    0.4 -> 02-Aug-1999 JiK - added outputCodeLengthsBytes() method.
//    0.5 -> 24-Aug-1999 JiK - Fixed a sorting bug in sortFortMin..()
//      that caused calculated trees to be different depending on the
//      compiler the source was compiled.
//    0.6 -> 02-Jun-2000 JiK - Added namespaces and nothrow..
//    0.7 -> 12-Jun-2000 JiK - reworked deletes and destructor to handle
//      NULL pointers which are permitted.
//    0.8 -> 09-Sep-2000 JiK - added addCnt( int,int );
//    0.9 -> 09-Nov-2002 JiK - some fixes for gcc3.1
//    1.0 -> 09-Mar-2004 JiK - Fixed a bug in _balance_canonical_codes()
//    1.1 -> 22-Aug-2004 JiK - Fixed another bug in _balance_canonical_codes()
//    1.2 -> 4-Sep-2004 JiK - Replaced _balance_canonical_codes() with a 
//           working algorithm :)
//
/////////////////////////////////////////////////////////////////////

#include <iostream>
#include <new>	// just for std::nothrow

#include <cstdlib>
#include <cassert>

#include "chuff.h"


using namespace std;


/////////////////////////////////////////////////////////////////////
// Function: EncHuf( int s, int m );
// Parameters:
//   s -- size of the alphabet.
//   m -- maximum depth of the huffman tree (i.e. maximum code
//        lenght).
// Purpose: This is EncHuf class'es default constructor. Allocates
//   needed arrays and initialized private stuff.
// Returns:
// Changes:
/////////////////////////////////////////////////////////////////////

EncHuf::EncHuf( int s, int m ) 
  throw ( std::bad_alloc, std::invalid_argument ) {
	int n;

    //cout << "EncHuf::EncHuf(" << s << "," << m << ");" << endl;


	/*
	 * Size of the alphabet must be atleast two.
	 */
	if (s < 2) { throw std::invalid_argument(" Huffman alphabet too small"); }

	_size = s;
	_maxcodelength = m <= MAXCODELENGTH ? m : MAXCODELENGTH;

	n = s <= m ? m + 1 : s;

	m_A = new(std::nothrow) sym[s];
	m_B = new(std::nothrow) unsigned short[n];

	// funny way to handle std::bad_alloc..

if (m_A == NULL || m_B == NULL) {
		// if these pointers are NULL, delete does nothing..
		delete[] m_A;
		delete[] m_B;
		throw std::bad_alloc();
	}
}

/////////////////////////////////////////////////////////////////////
// Function: EncHuf( const EncHuf &e );
// Parameters:
//   e -- reference to class to copy.
// Purpose: This is EncHuf class'es 'copy' constructor.
// Returns:
// Changes:
/////////////////////////////////////////////////////////////////////

EncHuf::EncHuf( const EncHuf &e )
	throw ( std::bad_alloc ) {
	int n;

	_size = e._size;
	_maxcodelength = e._maxcodelength;
	_start = e._start;
	n = _size <= _maxcodelength ? _maxcodelength + 1 : _size;

	m_A = new(std::nothrow) sym[_size];
	m_B = new(std::nothrow) unsigned short[n];

	if (m_A == NULL || m_B == NULL) {
		// if these pointers are NULL, delete does nothing..
		delete[] m_A;
		delete[] m_B;
		throw std::bad_alloc();
	}

	for (n = 0; n < _size; n++) {
		m_A[n] = e.m_A[n];
	}
	for (n = 0; n <= _maxcodelength; n++) {
		m_B[n] = e.m_B[n];
	}
}

EncHuf& EncHuf::operator=( const EncHuf &e )
	throw (std::range_error) {
	int n;	

	if (_size != e._size || _maxcodelength != e._maxcodelength) {
		throw std::range_error("Huffman encoder assignment");
	}

	init();
	for (n = 0; n < _size; n++) {
		m_A[n] = e.m_A[n];
	}
	for (n = 0; n <= _maxcodelength; n++) {
		m_B[n] = e.m_B[n];
	}

	_start = e._start;

	return *this;
}


/////////////////////////////////////////////////////////////////////
// Function: ~EncHuf()
// Parameters:
// Purpose: the EncHuf class'es destructur.
// Returns:
// Changes:
/////////////////////////////////////////////////////////////////////

EncHuf::~EncHuf() {
	delete[] m_A;
	delete[] m_B;
}


/////////////////////////////////////////////////////////////////////
// Function: void init();
// Parameters:
// Purpose: (Re)Initializes EncHuf class to unused state.
// Changes: A & B array contents.
/////////////////////////////////////////////////////////////////////

void EncHuf::init() {
	int i;

	for (i = 0; i < _size; i++) {
		m_A[i].u.s.cnt = 0;
		m_A[i].u.s.alp = i;
		m_A[i].cde = 0;
	}
	for (i = 0; i <= _maxcodelength; i++) {
		m_B[i] = 0;
	}
}

/////////////////////////////////////////////////////////////////////
// Function: int balance_canonical_codes( int ovl );
// Parameters:
//   ovl -- the number of overflow leaves in the huffman tree.
// Purpose: Balances the huffman tree to 'maxcodelenght'
//   maximum depth i.e. makes sure that no leaf has greater
//   depth than 'maxcodelenght'.
// Returns: 0 if ok, > 0 if the tree can not balanced to
//   'maxcodelenght' maximum depth i.e. the 'maxcodelenghtä is too
//   small for this alphabet and symbol frequencies.
// Changes: B
/////////////////////////////////////////////////////////////////////

int EncHuf::_balance_canonical_codes( int ovl ) {
	int i;

	//
	// This algorithm is far away from an optimal
	// one.. but does its job.
	//
	// replace internal node with a leaf

	m_B[_maxcodelength]++;
	ovl--;

	// check for pathetic cases..

	if ((m_B[_maxcodelength] == 1) && (ovl > 0)) {
		m_B[_maxcodelength]++;
		ovl--;
	}

	// balance

	while (ovl > 0) {
		i = _maxcodelength - 1;
		while (!m_B[i]) { i--; }
		m_B[i]--;
		m_B[i + 1] += 2;
		if (m_B[i + 1] > (1 << (i + 1))) { return 1; }
		ovl--;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////
// Function:
// 
//
//
//
/////////////////////////////////////////////////////////////////////

void EncHuf::_calculate_canonical_codes() {
	unsigned long c = 0;
	int p = 0;
	int n;
	//
	// This algorithm assumes that the symbols
	// are in ascending order by symbol
	// depth. Symbols with same depth must be in
	// ascending order by alphabets.
	//

 	for (n = _start; n < _size; n++) {
		if (m_A[n].u.s.cnt == 0) { continue; }
		if (m_A[n].u.s.cnt > p) {
			c <<= (m_A[n].u.s.cnt - p);
			p = m_A[n].u.s.cnt;
		}
		m_A[n].cde = c;
		c++;
	}
}

/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////

int cmp::both( const void *a, const void *b ) {
#ifndef BIGENDIAN
	unsigned long aa = (((sym*)a)->u.s.cnt << 16) | ((sym*)a)->u.s.alp;
	unsigned long bb = (((sym*)b)->u.s.cnt << 16) | ((sym*)b)->u.s.alp;
	return (aa - bb);
#else
	return ( ((sym*)a)->bth - ((sym*)b)->bth );
#endif
}

int cmp::sfe( const void *a, const void *b ) {
	return ( ((sym*)(a))->u.s.alp - ((sym*)(b))->u.s.alp );
}

void EncHuf::_sort_for_minimum_redundancy() {
	int n = 0;

	qsort((void *)m_A,_size,sizeof(sym),cmp::both);
	
	while (n < _size && !m_A[n].u.s.cnt) { n++; }
	_start = n;
}

void EncHuf::_sort_for_canonical_codes() {
	qsort((void *)&m_A[_start],_size - _start,sizeof(sym),cmp::both);
}

void EncHuf::_sort_A_for_encoding() {
	qsort((void *)m_A,_size,sizeof(sym),cmp::sfe);
}

/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////

void EncHuf::_build_B_from_A_for_balancing() {
	unsigned short c;
	int n;

	for (n = 0; n <= _maxcodelength; n++) {
		m_B[n] = 0;
	}
	for (n = _start; n < _size; n++) {
		c = m_A[n].u.s.cnt;
		//if (c > _maxcodelength) { c = _maxcodelength; }
		if (c <= _maxcodelength) { m_B[c]++; }
	}
}

/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////

void EncHuf::_rebuild_A_from_B_for_encoding() {
	int m, n;

	for (n = 0; n < _size; n++) {
		m_A[0].u.s.cnt = 0;
	}
	for (n = _maxcodelength, m = _start; n >= 0; n--) {
		if (m_B[n] == 0) { continue; }
		while (m_B[n]-- > 0) { m_A[m++].u.s.cnt = n; }
	}
}

/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////

const unsigned short* EncHuf::outputCodeLengths() {
	int n;

	for (n = 0; n < _size; n++) {
		m_B[n] = m_A[n].u.s.cnt;
	}

	return m_B;
}

const unsigned char* EncHuf::outputCodeLengthsBytes() {
	// static_cast<unsigned char*>
	unsigned char* b = reinterpret_cast<unsigned char*>(m_B);
	int n;

	for (n = 0; n < _size; n++) {
		b[n] = m_A[n].u.s.cnt & 0xff;
	}
	return b;
}



/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////

int EncHuf::_calculate_minimum_redundancy() {
	int r;		/* next root node to be used */
	int l;		/* next leaf to be used */
	int n;		/* next value to be assigned */
	int a;		/* number of available nodes */
	int u;		/* number of internal nodes */
	int d;		/* current depth of leaves */
	int o = 0;	/* number of depth overflows */

	/*
	 * check for pathological cases
	 */
	if (_size == _start) { return -1; }
	if (_size - 1 == _start) {
		m_A[_start].u.s.cnt = 1;
		m_A[_start].cde = 0;
		return 0;
	}

	/* first pass, left to right, setting parent pointers */

	m_A[_start].u.s.cnt += m_A[_start + 1].u.s.cnt; r = _start; l = _start + 2;
	for (n = _start + 1; n < _size - 1; n++) {

		/* select first item for a pairing */
		if (l >= _size || m_A[r].u.s.cnt < m_A[l].u.s.cnt) {
			m_A[n].u.s.cnt = m_A[r].u.s.cnt; m_A[r++].u.s.cnt = n;
		} else {
			m_A[n].u.s.cnt = m_A[l++].u.s.cnt;
		}

		/* add on the second item */
		if (l >= _size || (r < n && m_A[r].u.s.cnt < m_A[l].u.s.cnt)) {
			m_A[n].u.s.cnt += m_A[r].u.s.cnt; m_A[r++].u.s.cnt = n;
		} else {
			m_A[n].u.s.cnt += m_A[l++].u.s.cnt;
		}
	}

        /* second pass, right to left, setting internal depths */
	m_A[_size - 2].u.s.cnt = 0;
	for (n = _size - 3; n >= _start; n--) {
		m_A[n].u.s.cnt = m_A[m_A[n].u.s.cnt].u.s.cnt + 1;
	}

        /* third pass, right to left, setting leaf depths */

	a = 1; u = d = 0; r = _size - 2; n = _size - 1;
	while (a > 0) { 
		while (r >= _start && m_A[r].u.s.cnt == d) {
			u++; r--;
		}
		while (a > u) {
			m_A[n--].u.s.cnt = d; a--;
			if (d > _maxcodelength) { o++; }
		}
		a = 2 * u; d++; u = 0;
	}
	return o;
}

/////////////////////////////////////////////////////////////////////
//
// 
//
//
//
/////////////////////////////////////////////////////////////////////


int EncHuf::buildCanonicalCodes() {
	int ovl;

	_sort_for_minimum_redundancy();
	ovl = _calculate_minimum_redundancy();

	//assert(ovl >= 0);

	if (ovl < 0) {
		//cerr << "ovl is negative: " << ovl << endl;
		//return 1;
		return 0;
	}

	if (ovl > 0) {
		_build_B_from_A_for_balancing();
		if (_balance_canonical_codes(ovl)) {
			cerr << "Code length " << _maxcodelength << " too small!" << endl;
			exit(1);
		}
		_rebuild_A_from_B_for_encoding();
	}

	_sort_for_canonical_codes();
	_calculate_canonical_codes();
	_sort_A_for_encoding();

	return 0;
}
