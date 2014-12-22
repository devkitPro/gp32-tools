////////////////////////////////////////////////////////////////////////
//
// Module:
//   fio.cpp
// Description:
//   This class implements FILE IO for the io.h class interfaces.
// Restrictions:
// Version:
//   v0.1  8-Aug-1999 JiK initial version. 
//   v0.2  16-Sep-1999 JiK stdio replaced with fstream
//   v0.2  5-Oct-1999 JiK stdio reintroduced due bugs in StormC's ftream
//   v0.3  9-Apr-2000 JiK Removed all StormC stuff.
//   v0.4  24-Jun-2002 JiK added some FXE encryption ;)
//
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include "fio.h"
#include <cassert>
#include <cstring>

using namespace std;

//
// General note: all functions return -1 as an indication of
//   error!
//
//
//


void FWriter::setEncryptionKey( const unsigned char* key, int keyLen ) {
	delete[] _key;  	// deleting a NULL is ok

	if (key == NULL) {
		_key = NULL;
		_keyLen = 0;
		return;
	}
	if (_key && (keyLen != _keyLen)) {
		_key = new unsigned char[keyLen];
		_keyLen = keyLen;
	}

	_keyPos = 0;
	memcpy(_key,key,keyLen);
}




FWriter::FWriter( FReader* r ) {
	_os = new ostream( r->_is->rdbuf() );
	_of = NULL;
	_key = NULL;
	_keyLen = 0;
}

FWriter::FWriter( ostream* o ) {
	_os = new ostream( o->rdbuf() );
	_of = NULL;
	_key = NULL;
	_keyLen = 0;
}

FWriter::FWriter( const char* name, int mode )  throw ( IOException ) {
  _os = NULL;
  _of = NULL;
	_key = NULL;
	_keyLen = 0;
  if (open(name,mode) < 0) {
    throw IOException();
  }
}

FWriter::FWriter() {
  _os = NULL;
  _of = NULL;
	_key = NULL;
	_keyLen = 0;
}

//

FWriter::~FWriter() {
  close();
	delete[] _key;
}

//
//
//

int FWriter::isOpen() const {
	if (_os == NULL) {
		return 0;
	} else {
		return _os->good();
	}
}

//
//
//

int FWriter::open( const char* name, int mode ) {
	ios::openmode mode_;

  switch (mode) {
  case IO_CREATE:
    mode_ = ios::out|ios::binary;
    break;
  case IO_APPEND:
    mode_ = ios::app|ios::binary;
    break;
  default:
#ifdef DEBUG_ON
    std::cerr << "Invalid open mode: " << hex << mode << endl;
#endif
    return -1;
  }
  //
  if (_of || _os) { return -1; }
  if ((_of = new ofstream( name, mode_ )) == NULL) { return -1; }
  if (!_of->is_open()) {
    delete _of;
    _of = NULL;
    return -1;
  }
  if ((_os = new ostream( _of->rdbuf() )) == NULL) {
    delete _of;
    _of = NULL;
    return -1;
  }
  if (_os->good()) {
    return 0;
  } else {
    close();
    return -1;
  }
}

//
//
//

void FWriter::close() {
  if (_os) {
    delete _os;
    _os = NULL;
  }
  if (_of) {
    _of->close();
    delete _of;
    _of = NULL;
  }
}

//
//
//

long FWriter::write( const unsigned char* buf, long len ) {
	unsigned char* b = const_cast<unsigned char*>(buf);

	// check if we need to encrypt this block..
	// This trashes the buffer by the way!!!!

	if (_key) {
		for (int n = 0; n < len; n++) {
			// This algorithm is GP32 FXE-file compliant..
			b[n] = b[n] ^ _key[_keyPos % _keyLen] ^ 0xff;
			_keyPos++;
		}
	}

	//
	_os->write( reinterpret_cast<const char*>(b), len);
	if (_os->bad()) {
		return -1;
	} else {
		return len;
	}
}

//
//
//

int FWriter::ioerr() {
  return _os->rdstate();
}

//
//
//

int FWriter::flush() {
  _os->flush();
  if (_os->bad()) {
    return -1;
  } else {
    return 0;
  }
}

//
//
//

int FWriter::putByte( int b ) {
	char buf[1];

	buf[0] = b & 0xff;

	_os->write( buf, 1);
	if (_os->bad()) {
		return -1;
	} else {
		return 1;
	}
}


//
//
//

long FWriter::seek( long pos, int how ) {
  ios::seekdir how_;
  
  switch (how) {
  case IO_SEEK_SET:
    how_ = ios::beg;
    break;
  case IO_SEEK_CUR:
    how_ = ios::cur;
    break;
  case IO_SEEK_END:
    how_ = ios::end;
    break;
  default:
    return -1;
  }
  
  //
  _os->seekp(pos,how_);
  if (_os->bad()) {
    return -1;
  } else {
    return 0;
  }
}

//
//
//

long FWriter::tell() {
  streampos pos;
  pos = _os->tellp();
  return pos;
}


//
//
//
//
//
//

FReader::FReader( FWriter* w ) {
  _is = new istream( w->_os->rdbuf() );
  _if = NULL;
}

FReader::FReader( istream* i ) {
  _is = new istream( i->rdbuf() );
  _if = NULL;
}

FReader::FReader( const char* name, int mode ) throw ( IOException ) {
  _is = NULL;
  _if = NULL;
  if (open(name,mode) < 0) {
    cerr << "freader & IOException.." << endl;
    throw IOException("foofoo");
  }
}

FReader::FReader() {
  _is = NULL;
  _if = NULL;
}

//

FReader::~FReader() {
  close();
}

//
//
//

int FReader::isOpen() const {
  if (_is == NULL) {
    return 0;
  } else {
    return _is->good();
  }
}

//
//
//

int FReader::open( const char* name, int mode ) {
	ios::openmode mode_;

  switch (mode) {
  case IO_READ:
    mode_ = ios::in|ios::binary;
    break;
  case IO_RW:
    mode_ = ios::app|ios::out|ios::binary;
    break;
  default:
    return -1;
  }
  //
  if (_if || _is) { return -1; }
  if ((_if = new ifstream( name, mode_ )) == NULL) { return -1; }
  if (!_if->is_open()) {
    cerr << "vikaa freaderissa!" << endl;
	//assert("TŠŠllŠ" == NULL);
    delete _if;
    _if = NULL;
    return -1;
  }
  if ((_is = new istream( _if->rdbuf() )) == NULL) {
    delete _if;
    _if = NULL;
    return -1;
  }
  if (_is->good()) {
    return 0;
  } else {
    close();
    return -1;
  }
}

//
//
//

void FReader::close() {
  if (_is) {
    delete _is;
    _is = NULL;
  }
  if (_if) {
    _if->close();
    delete _if;
    _if = NULL;
  }
}

//
//
//

long FReader::read( unsigned char* buf, long len ) {
  _is->read( reinterpret_cast<char*>(buf),len);
  if (_is->bad()) {
    return -1;
  } else {
    return _is->gcount();
  }
}

//
//
//

int FReader::ioerr() {
  return _is->rdstate();
}

//
//
//

int FReader::flush() {
  return 0;
}

//
//
//

int FReader::getByte() {
	unsigned char buf[1];
	_is->read( reinterpret_cast<char*>(buf),1);
	if (_is->bad()) {
		return -1;
	} else {
		return buf[0] & 0xff;
	}
}

//
//
//

long FReader::seek( long pos, int how ) {
  ios::seekdir how_;
  
  switch (how) {
  case IO_SEEK_SET:
    how_ = ios::beg;
    break;
  case IO_SEEK_CUR:
    how_ = ios::cur;
    break;
  case IO_SEEK_END:
    how_ = ios::end;
    break;
  default:
    return -1;
  }
  
  //
  _is->seekg(pos,how_);
  if (_is->bad()) {
    return -1;
  } else {
    return 0;
  }
}

//
//
//

long FReader::tell() {
  streampos pos;
  pos = _is->tellg();
  return pos;
}

//
//
//
//
//
//
