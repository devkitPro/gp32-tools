//
//
//
//
//

#include <cassert>
#include <cctype>
#include "cb.h"
#include <iostream>

using namespace std;


//
//
//

CallBack::CallBack( int size ) { //throw (std::bad_alloc) {
  _info = NULL;
  _size = size;
  _count = 0;
  _info = new MatchInfo[size];
}

//
// Returns:
//   false if ok, true if must break.


//static int total = 0;

bool CallBack::callback( long off, long len, void* pos )
  throw (std::out_of_range) {

  assert (_count < _size);
  // {
  //	std::cerr << "throw std::out_of_range(""foobar"");" << std::endl;
  //  throw std::out_of_range("foobar");
  //}

  _info[_count].length = len;
  _info[_count].offset = off;
  _info[_count].position = static_cast<unsigned char*>(pos);
  _count++;

  return false;
}


