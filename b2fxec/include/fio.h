#ifndef _fio_h_included
#define _fio_h_included


#include <iostream>
#include <fstream>
#include "b2fxec_io.h"

using namespace std;


class FReader;

class FWriter : virtual public Writer {
	ostream* _os;
	ofstream* _of;
	unsigned char* _key;
	int _keyLen;
	int _keyPos;
public:
	friend class FReader;
	//
	FWriter( FReader* );
	FWriter( ostream* );
	FWriter( const char*, int ) throw ( IOException );
	FWriter();
	//
	~FWriter();
	//
	int isOpen() const;
	//
	int open( const char*, int );
	long write( const unsigned char*, long );
	long seek( long, int );
	int ioerr();
	void close();
	int flush();
	long tell();
	int putByte( int );
	//
	int exists( char* );
	//
	void setEncryptionKey( const unsigned char* = NULL, int=0 );
};

class FReader : virtual public Reader {
	istream* _is;
	ifstream* _if;
public:
	friend class FWriter;
	//
	FReader( FWriter* );
	FReader( istream* );
	FReader( const char*, int ) throw ( IOException );
	FReader();
	//
	~FReader();
	//
	int isOpen() const;
	//
	int open( const char*, int );
	long read( unsigned char*, long );
	long seek( long, int );
	int ioerr();
	void close();
	int flush();
	long tell();
	int getByte();
};

#endif
