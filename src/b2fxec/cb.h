#ifndef _cb_h_included
#define _cb_h_included

//
//
//

#include "stdexcept"
#include "string"


//
//

struct MatchInfo {
	long offset;
	long length;
	unsigned char *position;
	long symbol;
};

//
//
//

class CallBack {
  MatchInfo* _info;
  int _count;
  int _size;
public:
	virtual ~CallBack() { delete[] _info; }
	CallBack( int ); // throw (std::bad_alloc);
  //CallBack( int, xxx ) throw(std::bad_alloc);
	bool callback( long, long, void* = NULL ) throw (std::out_of_range);
  void init() { _count = 0; }  
  int getSize() const { return _size; }
  int getCount() const { return _count; }

  //

  operator const MatchInfo*() const { return _info; }
  operator MatchInfo*() const { return _info; }
  operator int() const { return _count; }
  const MatchInfo& operator[]( int i ) const { return _info[i]; }
};

#endif
