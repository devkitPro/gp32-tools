#ifndef _chuff_h_defined
#define _chuff_h_defined

#include <iostream>
#include <stdexcept>
#include <cstdlib>


#define MAXCODELENGTH 15

//

namespace cmp {
	int both( const void *, const void * );
	int sfe( const void *, const void * );
};

//

//namespace {

struct sym {
  union u_ {
    struct s_ {
      unsigned short cnt;
      unsigned short alp;
    } s;
    unsigned long bth;
  } u;
  unsigned long cde;
};
//};


class EncHuf {
  int _maxcodelength; // Efective maximum code length
  int _size;          // Size of the alphabet
  int _start;         // Number of zero weight leaves
  sym* m_A;            // A ptr to alphabet
  unsigned short* m_B; // A ptr to tmp array used when
  // balancing huffman tree
  void _sort_for_minimum_redundancy();
  void _sort_for_canonical_codes();
  void _sort_A_for_encoding();
  void _build_B_from_A_for_balancing();
  void _rebuild_A_from_B_for_encoding();
  int _balance_canonical_codes( int );
  void _calculate_canonical_codes();
  int _calculate_minimum_redundancy();
 public:
  EncHuf( int, int ) throw (std::bad_alloc, std::invalid_argument);
  ~EncHuf();
  
  EncHuf( const EncHuf & ) throw (std::bad_alloc);
  EncHuf& operator=( const EncHuf & ) throw (std::range_error);
  
  void init();
  void addCnt( int i ) { m_A[i].u.s.cnt++; }
  void addCnt( int i, unsigned short n ) { m_A[i].u.s.cnt += n; }
  void setCnt( int i, unsigned short c ) { m_A[i].u.s.cnt = c; }
  unsigned short getCnt( int i ) const { return m_A[i].u.s.cnt; }
  
  unsigned long getCode( int i ) const { return m_A[i].cde;}
  unsigned short getLength( int i ) const { return m_A[i].u.s.cnt; }
  const unsigned short* outputCodeLengths();
  const unsigned char* outputCodeLengthsBytes();
  int buildCanonicalCodes();
};

#endif






