/////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
//
//
//
//
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include "port.h"

#include "fio.h"
#include "compress_fxe0.h"
#include "compress_fxe2.h"
#include "compress_fxe3.h"
#include "headers.h"
#include "unfxe.h"

using namespace Headers;
//
//
//

#define NBLOCKS 16
#define BLOCK_BUFFER 32768+16384
#define FXEP_BLOCK_SIZE FXE0_BLOCK_SIZE

// We'll take the larger buffer..
// And also reserve same extra space to avoid
// writing to disk too often..

// static stuff..

namespace {
	// Some local functions...

	void usage( char ** );

	// static constants..

	enum FXEPalgorithm { fxe0=0, fxe1, fxe2, fxe3, fxe4 };
	static bool FXE = true;
	FXEPalgorithm algo = fxe3;
	static bool RAW = false;
	char tmpstr[] = "./gxb.XXXXXXXX";

	// Our write buffers..

	unsigned char buf[BLOCK_BUFFER];

	//
	// Define compression parameters..
	//

	struct _compressionParams {
		unsigned char* buffer;
		long bufferSize;
		int goodMatch;
		int maxMatch;
		int minMatchThres;
		int minOffsetThres;
		int maxChain;
		int numLazy;
		int numDistBits;
		int windowSize;
		int (*saveHeader)( int, FWriter*, char*, char*, char*, char*, char* );
		int (*fixHeader)( int, FWriter*, long );
		int (*saveDecompressor)( FWriter* );
		int (*fixDecompressor)( FWriter*, long, const fixInfo* );
		int (*postFix)( FWriter*, long );
	} compressionParams[] = {
		// FXE0
		{ 	buf, BLOCK_BUFFER, 31+254, FXEP_BLOCK_SIZE, 2, 1792, 256, 1, 5+8, FXEP_BLOCK_SIZE,
			saveFXEHeader,fixFXEHeader, saveFXE0Header, fixFXE0Header, NULL },
		// FXE1
		{ 	NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			NULL,NULL, NULL, NULL, NULL },
		// FXE2
		{ 	buf, BLOCK_BUFFER, FXE2_GOOD_MATCH, FXE2_MAX_MATCH, 2, 63, 512, 1, 6+8, FXE2_BLOCK_SIZE,
			saveFXEHeader,fixFXEHeader, saveFXE2Header, fixFXE2Header, NULL },
		// FXE3
		{ 	buf, BLOCK_BUFFER, FXE3_GOOD_MATCH, FXE3_MAX_MATCH, 2, 32, 4096, 1, 6+8, FXE3_BLOCK_SIZE,
			saveFXEHeader,fixFXEHeader, saveFXE3Header, fixFXE3Header, NULL }
	};

	// misc functions..
	//
	//

	void usage( char **argv ) {
		cerr << "Usage: " << argv[0] << " [-<options>] infile outfile" << endl;
		cerr << "Options:" << endl;
		cerr << "  -t string Set game title (32 chars max, default=infile)" << endl;
		cerr << "  -a string Set game author info (32 chars max, default='')" << endl;
		cerr << "  -b file   Use uncompressed .bmp file for icon (default=GPlogo)" << endl;
		cerr << "  -B file   Use raw .bin file for icon (default=GPlogo)" << endl;
		cerr << "  -g        Save outfile as GXB (default=FXE)" << endl;
		cerr << "  -A n      Select algorithm (0=FXE0, 2=FXE2, 3=FXE3), default=3" << endl;
		cerr << "  -h        Show help" << endl;
		cerr << "  -d        Save file as raw data (no GXB or FXE headers)" << endl;
		cerr << "  -f        Fast decompression (sets 132MHz clockspeed)" << endl;
		cerr << "  -F MHz    Set decompression clockspeed (MHz=132,133,156,166,180)" << endl;
		
		exit(1);
	}
  
  	//
  	//
  	//
  
  	#define FIXHDRSIZE 28
  
  	int fixGXBLength( const char* s, bool& isFXE ) {
		unsigned char hdr[FIXHDRSIZE];
		unsigned char* p;
		int rostrt,rwstrt,rostop,rwstop,csize,osize;
		FReader rfile(s,IO_READ); 
		int gap;

		if (!rfile.isOpen()) { return -1; }
		if (rfile.read(hdr,FIXHDRSIZE) != FIXHDRSIZE) {
			rfile.close();
			return -1;
		}

		// check for FXE

		if (b2fxec_strncasecmp(reinterpret_cast<const char*>(hdr),"fxe ",4)) {
			isFXE = false;
		} else {
			rfile.close();
			isFXE = true;
			return 0;
		}

		//

		rfile.seek(0,IO_SEEK_END);
		osize = rfile.tell();
		rfile.seek(0,IO_SEEK_SET);
		rfile.close();

		//

		p = hdr+4;
		GETL_LE(p,rostrt);
		GETL_LE(p,rostop);
		GETL_LE(p,rwstrt);
		p += 4;
		GETL_LE(p,rwstop);

		gap = 0;
		csize = (rostop - rostrt) + (rwstop - rwstrt);

		// check for gaps..

		if (rwstrt > rostop) {
			gap = rwstrt;
			cout << "++ Warning - code and data sections are not continuous -> check you compiler options.." << endl;
			cout << "             Gap is " << (rwstrt-rostop) << " bytes" << endl;
		}
		if (gap + csize > 0x0c780000) {
			cout << "++ Warning - due gap decompresion MAY fail.." << endl;
		}
		if (osize > csize) {
			cout << "++ Warning - file is longer than stated in the GXB header -> truncating.." << endl;
			if (b2fxec_truncate(s,csize) < 0) { return -1; }
		}
		return gap;
  	}  

	//
	//
	//
	
	char* unFXEToTemp( const char* s, UnFXE& fxe ) {
		unsigned char* buf;
		unsigned char* ret;
		char* tmpfile;
		FReader rfile(s,IO_READ); 
		int fsize;

		if (!rfile.isOpen()) {
			cerr << "** Error: Failed to open the FXE file.." << endl;
			return NULL;
		}

		rfile.seek(0,IO_SEEK_END);
		fsize = rfile.tell();
		rfile.seek(0,IO_SEEK_SET);

		buf = new unsigned char[fsize];

		if (rfile.read(buf,fsize) != fsize) {
			cerr << "** Error: Reading the FXE failed.." << endl;
			rfile.close();
			delete[] buf;
			return NULL;
		}

		if (!(ret = fxe.unfxe(buf,fsize))) {
			cerr << "** Error: Unpacking the FXE failed due an internal error.." << endl;
			rfile.close();
			delete[] buf;
			return NULL;
		}
		if (!(tmpfile = b2fxec_mktemp(tmpstr))) {
			cerr << "** Error: Creating a temp file failed.." << endl;
			rfile.close();
			delete[] buf;
			return NULL;
		}
		// and now dump the file..

		FWriter wfile(tmpfile,IO_CREATE);

		if (!wfile.isOpen()) {
			cerr << "** Error: Failed to open temp file.." << endl;
			rfile.close();
			delete[] buf;
			return NULL;
		}
		if (wfile.write(ret,fsize) != fsize) {
			cerr << "** Error: Writing the unpacked FXE failed.." << endl;
			//unlink(tmpfile);
			rfile.close();
			wfile.close();
			delete[] buf;
			return NULL;
		}

		// Done..
		
		rfile.close();
		wfile.close();
		delete[] buf;
		return tmpfile;
	}

	//
	// Get clockspeeds
	//

	int getClockSpeed( fixInfo& fix, int mhz ) {
		switch (mhz) {
		case 132:
			fix.mhz = 132000000;
			fix.fac = 0x24001;
			break;
		case 133:
			fix.mhz = 133000000;
			fix.fac = 0x7d041;
			break;
		case 156:
			fix.mhz = 156000000;
			fix.fac = 0x2c001;
			break;
		case 166:
			fix.mhz = 166000000;
			fix.fac = 0x4b011;
			break;
		case 180:
			fix.mhz = 180000000;
			fix.fac = 0x34001;
			break;
		default:
			return -1;
		}
		return 0;
	}
};

//
//
//


int main( int argc, char** argv ) {
	Compress* compr;
	unsigned long ID;
	bool dummy = true;
	unsigned char* tb;
	LZParams lzp;
	FReader rfile;
	FWriter wfile;

	int ch;
	char *title = NULL;
	char *author = NULL;
	char *bmp = NULL;
	char *raw = NULL;
	char* icon = NULL;

	fixInfo fix;
	char* infile;
	bool infileWasFXE;
	UnFXE fxe;

	//

	cout << endl << "b2fxeC v0.6a-pre2 - (c) 2002-4 Jouni 'Mr.Spiv' Korhonen" << endl << endl;


  	// Check commandline & options..

	while ((ch = getopt(argc,argv,"t:a:A:b:B:ghdfF:")) != -1) {
		switch (ch) {
		case 't':	// title
			title = optarg;
			break;
		case 'a':	// Open icon file..
			author = optarg;
			break;
		case 'A':
			algo = static_cast<FXEPalgorithm>(atoi(optarg));
			break;
		case 'B':
			raw = optarg;
			break;
		case 'b':
			bmp = optarg;
			break;
		case 'g':	// Open afx/gxb file..
			FXE = false;
			break;
		case 'd':	//
			RAW = true;
			break;
		case 'f':
			fix.mhz = 132000000; 	// 132MHz
			fix.fac = 0x24001;
			break;
		case 'F':
			if (getClockSpeed( fix, atoi(optarg) ) < 0) {
				cerr << "** Error: not support MHz.." << endl;
				exit(0);
			}
			break;
		case '?':
		case 'h':
		default:
			usage(argv);
		}
	}

	if (optind >= argc) {
		usage(argv);
		return 0;
	}

	// default name..
	
	if (title == NULL) { title = argv[optind]; }


	// Get the compressor..

	switch (algo) {
	case fxe0:
    	compr = new CompressFXE0(NBLOCKS);
    	ID = FXEP_TYPE0_ID;
		break;
	case fxe1:
		cerr << "Sorry.. No bonus!" << endl;
		exit(0);
		//cout << "compr = new CompressFXE1(NBLOCKS);" << endl;
		//compr = new CompressFXE1(NBLOCKS);
    	//ID = FXEP_TYPE1_ID;
		break;
	case fxe2:
		//cout << "compr = new CompressFXE2(NBLOCKS);" << endl;
		compr = new CompressFXE2(NBLOCKS);
    	ID = FXEP_TYPE2_ID;
		break;
	case fxe3:
		//cout << "compr = new CompressFXE2(NBLOCKS);" << endl;
		compr = new CompressFXE3(NBLOCKS);
    	ID = FXEP_TYPE3_ID;
		break;
	default:
		cerr << "** Error: unknown compression algorithm.." << endl;
		return 0;
		break;
	}

	//
	// Setup rest of LZ parameters..
	//

	CallBack cback(compressionParams[algo].windowSize);
	

	lzp.buf 			= compressionParams[algo].buffer;
	lzp.bufSize 		= compressionParams[algo].bufferSize;
	lzp.goodMatch 		= compressionParams[algo].goodMatch;
	lzp.maxMatch 		= compressionParams[algo].maxMatch;
	lzp.minMatchThres 	= compressionParams[algo].minMatchThres;
	lzp.minOffsetThres 	= compressionParams[algo].minOffsetThres;
	lzp.maxChain 		= compressionParams[algo].maxChain;
	lzp.numLazy 		= compressionParams[algo].numLazy;
	lzp.numDistBits 	= compressionParams[algo].numDistBits;
	lzp.windowSize 		= compressionParams[algo].windowSize;
	lzp.r 				= &rfile;
	lzp.c 				= &cback;

	//

	//cout << "maxmatch: " << lzp.maxMatch << endl;


	//
	// Do the size fix for incorrect files..
	// Also check for code & data section gaps
	//
	//

	int gap = -1;
	infile = argv[optind];
	infileWasFXE = false;

	if (!RAW) {
		gap = fixGXBLength(infile, infileWasFXE );

		if (gap < 0) {
			cerr << "** Error: failed to check/fix GXB length correctness.." << endl;
			return 0;
		}
	}


	// 
	// check if the "infile" is a FXE..
 	//

	if (!RAW && infileWasFXE) {
		cout << "Unpacking a FXE.." << endl;

		if (!(infile = unFXEToTemp(infile,fxe))) {
			cerr << "Unpacking the FXE failed.." << endl;
			return 0;
		}
		infileWasFXE = true;

		// Override parameters..

		title  = const_cast<char*>(fxe.getGameName());
		author = const_cast<char*>(fxe.getAuthorName());
		icon   = const_cast<char*>(fxe.getIcon());
		bmp = NULL;
		raw = NULL;
	}
	
	//
	//
	//

	compr->init(&lzp);

	// Open files..

	rfile.open(infile,IO_READ);
	wfile.open(argv[optind+1],IO_CREATE);

	if (!rfile.isOpen()) {
		delete compr;
		cerr << "** Error: failed to open file '" << infile << "'.." << endl;
		return 0;
	}
	if (!wfile.isOpen()) {
		delete compr;
		rfile.close();
		cerr << "** Error: failed to open file '" << argv[optind+1] << "'.." << endl;
		return 0;
	}
	if (infileWasFXE == true) {
		if (b2fxec_unlink(infile) < 0) {

			cout << "++ Warning: failed to delete temporary file '" << infile << "'" << endl;

		}
	}

	//
	//
	//
	// Save headers..
	//

	long origiFileSize = 0;
	long totalFileSize = 0;
	long comprFileSize = 0;
	long decomprOffset = 0;
	long n;

	rfile.seek(0,IO_SEEK_END);
	origiFileSize = rfile.tell();
	rfile.seek(0,IO_SEEK_SET);

	if (!RAW && FXE && (decomprOffset = compressionParams[algo].saveHeader(
		0,&wfile,title,author,bmp,raw,icon)) < 0) {
		wfile.close();
		rfile.close();
		delete compr;
		return 0;
	}
	if (!RAW && (totalFileSize = compressionParams[algo].saveDecompressor(&wfile)) < 0) {
		wfile.close();
		rfile.close();
		delete compr;
		return 0;
	}

	// Now we are ready to go.. first prepare the compressed
	// file header information..

	tb = buf;
	PUTL(tb,ID);
	PUTL_LE(tb,origiFileSize);
	wfile.write(buf,8);

	totalFileSize += 8;
	comprFileSize = 8;

	cout << "Crunching (0/" << origiFileSize << ")";
	cout.flush();

	//
	//
	//

	ch = 10;

	while ((n = compr->compressBlock(&lzp,dummy)) > 0) {
		unsigned short bl;
    	tb = buf;
 		GETW_LE(tb,bl);

		// Fix block size NOT to include FXEP_BLOCK_HEADER


		bl -= FXEP_BLOCK_HEADER_SIZE;
		tb = buf;
		PUTW_LE(tb,bl);

		//cerr.setf(ios::hex);
		//cerr << "Block starts: " << totalFileSize << ", " << bl <<endl;


		if (wfile.write(buf,n) != n) {
			n = -1;
			break;
		}
		comprFileSize += n;
		totalFileSize += n;

		if (--ch == 0) {
			cout << "\rCrunching (" << compr->getOriginalSize() << "/" << origiFileSize << ")";
			cout.flush();
			ch = 10;
		}
	}
	
	//
	// Finish the output file.. add EOF
	//

	if (n == 0) {
		tb = buf;
		
		// check for a proper EOF mark
		
		switch (compr->getTypeID()) {
		case FXEP_TYPE0_ID:
		case FXEP_TYPE2_ID:
		default:
			PUTW_LE(tb,FXEP_EOF);
			break;
		case FXEP_TYPE3_ID:
			PUTW_LE(tb,FXEP_EOF_NEW);
			break;
		}
		
		if (wfile.write(buf,2) != 2) {
			cerr << "** Error writing file (1).." << endl;
			rfile.close();
			wfile.close();
			delete compr;
			return 0;
		}
		comprFileSize += 2;
		totalFileSize += 2;
		
	}
	if (n < 0) {
		cerr << "** Error writing file (2).." << endl;
		rfile.close();
		wfile.close();
		delete compr;
		return 0;
	}

	// Check alignment.. must be 4
	
	if (comprFileSize & 0x03) {
		unsigned long pad = 0;
		long l = 4-(comprFileSize & 0x03);
		wfile.write(reinterpret_cast<unsigned char*>(&pad),l);
		comprFileSize += l;
		totalFileSize += l;
	}

	//
	// Fix headers..
	//

	fix.osize = origiFileSize;
	fix.csize = comprFileSize;
	fix.gap = gap;

	if (!RAW && FXE && compressionParams[algo].fixHeader(0,&wfile,totalFileSize) < 0) {
		cerr << "** Error writing file (3).." << endl;
		rfile.close();
		wfile.close();
		delete compr;
		return 0;
	}
	if (!RAW && compressionParams[algo].fixDecompressor(&wfile,decomprOffset,&fix) < 0) {
		cerr << "** Error writing file (4).." << endl;
		rfile.close();
		wfile.close();
		delete compr;
		return 0;
	}

	//
	// Calculate stuff..
	//

	double gain = 100.0 - static_cast<double>(totalFileSize)
                / static_cast<double>(origiFileSize) * 100.0;

	cout.precision(3);
	cout << "\rCrunched " << gain << "% - total " << totalFileSize << " bytes" << endl;

	//
	//
	//

	rfile.close();
	wfile.close();
	delete compr;

	//

  return 0;
}


