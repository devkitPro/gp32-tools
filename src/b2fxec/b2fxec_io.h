#ifndef _io_h_included
#define _io_h_included

//
//
//

#include "stdexcept"
#include "string"

//
// Physical file opening methods.
//
#define IO_CREATE  (1 << 1)
#define IO_APPEND  (1 << 2)
#define IO_READ    (1 << 3)
#define IO_RW      (1 << 4)

//
// Seek positions
//

#define IO_SEEK_SET 0
#define IO_SEEK_CUR 1
#define IO_SEEK_END 2

//
// IO exceptions
//

using namespace std;


class IOException {
private:
  string _what;
public:
  int _x;
public:
  IOException() { _what.assign("No user defined exception reason"); }
  IOException( const char* w ) {
    string t(w);
    _what = t;
    _x = 666; 
  }
  virtual const char* cwhat () const { return _what.c_str (); }
  virtual const string& what () const { return _what; }
  //virtual ~IOException() {}
  virtual ~IOException() {}
};

//namespace hh {
  ostream& operator << ( ostream&, IOException& );
//};


//
// Interfaces
//

class Reader {
public:	
	virtual ~Reader() {}
	virtual long read( unsigned char*, long ) = 0;
	virtual long seek( long, int ) = 0;
	virtual int flush() = 0;
	virtual int ioerr() = 0;
	virtual long tell() = 0;
};

class Writer {
public:
	virtual ~Writer() {}
	// returns -1 if error
	virtual long write( const unsigned char*, long ) = 0;
	// returns -1 if error
	virtual long seek( long, int ) = 0;
	// returns -1 if error
	virtual int flush() = 0;
	// returns ?? error code
	virtual int ioerr() = 0;
	// returns ?? position
	virtual long tell() = 0;
};

#endif
