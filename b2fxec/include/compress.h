#ifndef _compress_h_included
#define _compress_h_included

//
//
//

#include "params.h"

//
// Description:
//   General interface for the compressed that the STOC file
//   format will use.
//
// Public methods:
//
// compressSegment -- compress a segment of data and output the
//   compressed segment into the passed buffer. The called passed
//   both a ptr to output buffer and the maximum size of the
//   output buffer. The compressor can also tell the caller that
//   the output buffer contains uncompressed data i.e. in cases
//   when the compression fails.
// getNumMatches -- cumulative statistics.
// getNumLiterals -- cumulative statistics.
// getNumMatchedBytes -- cumulative statistics.
// getCompressedSize -- cumulative statistics.. compressed size of
//   of segments excluding all headers.
// getOriginalSize -- cumulative statistics.. number of loaded bytes.
// init -- initialized compressor for a new file to compress.
// isValid -- checks if the class is usable at all.
//
// Note:
//   the Compress class does not care how the data gets 'loaded'
//   for the compression.
//

class Compress {
public:
	virtual ~Compress() {}
	//                   outbuffer       size isRaw
	virtual long compressBlock( const Params*, bool&) = 0;
	virtual long getNumMatches() const = 0;
	virtual long getNumLiterals() const = 0;
	virtual long getNumMatchedBytes() const = 0;
	virtual long getCompressedSize() const = 0;
	virtual long getOriginalSize() const = 0;
	//
	virtual void init( const Params* ) = 0;
	virtual unsigned long getTypeID() const = 0;
};

#endif


