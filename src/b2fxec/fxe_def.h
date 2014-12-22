#ifndef _fxp_def_h_included
#define _fxp_def_h_included

//
// FXEPack1 file format:
// ========================
//
// * General header:
//
//    31  23  15  7   0
//    +---+---+---+---+
//    |'F'|'X'|'P'|'?'|
//    +---+---+---+---+
//
//    31  23  15  7   0
//
//    +---+---+---+---+
//    | ORIGINAL_SIZE |  (Little Endian in file..)
//    +---+---+---+---+
//
// * For each compressed block (1-n times)
//
//     15 14 11   7    3    0
//      +-+--+----+----+----+
//      |C|  BLOCK_SIZE     |  (Little Endian in file..)
//      +-+--+----+----+----+
//
//      C = if 1 then the block is compressed,
//          if 0 then the block contains BLOCK_SIZE bytes of literals
//      if whole word is 0xffff then eof.
//
//      BLOCK_SIZE = size of following block excluding block header bytes.
//
//        +-===================================-+
//        | BLOCK_SIZE bytes of compressed data |
//        +-===================================-+
//
// Note that block, except for the last block, contain always
// 2048 bytes of uncompressed data. There is _no_ explicit EOF
// mark, nor length of the partial block. The decompressor must
// take advance of the feature that the binary encoding is strictly
// byte aligned. Thus when the last byte of the compressed block
// has been processed the decompressor can exit.
//
//
// FXPack1 File header defines..
//

#define FXEP_TYPE0_ID (('F'<<24)|('X'<<16)|('P'<<8)|'0')
#define FXEP_TYPE1_ID (('F'<<24)|('X'<<16)|('P'<<8)|'1')
#define FXEP_TYPE2_ID (('F'<<24)|('X'<<16)|('P'<<8)|'2')
#define FXEP_TYPE3_ID (('F'<<24)|('X'<<16)|('P'<<8)|'3')
#define FXEP_TYPE4_ID (('F'<<24)|('X'<<16)|('P'<<8)|'4')

//
// FXPx  block header defines..
//

#define FXEP_FLAGS_COMPRESSED_MASK 0x8000
#define FXEP_FLAGS_BSIZE_MASK      0x7fff
#define FXEP_BLOCK_HEADER_SIZE     2
#define FXEP_EOF                   0xffff
#define FXEP_EOF_NEW               0x0000

//
// Other NGPack specific constants for (de)compression
//

#define FXEP_MAX_MATCH             2048
#define FXEP_GOOD_MATCH           7+254
#define FXEP_DEF_CHAIN_LEN         300
#define FXEP_NO_MATCH LZ2_NO_MATCH
#define FXEP_MIN_MATCH LZ2_MIN_MATCH
#define FXEP_GAP LZ2_GAP
#define FXEP_SAFETY_LEN            512

//
// IO related defines..
//

#define FXEP_OK 0
#define FXEP_ERR_BAD_HEADER (-1)
#define FXEP_ERR_TYPE       (-4)
#define FXEP_ERR_MEMORY     (-6)
#define FXEP_ERR_NEED_MORE  (-7)

#define FXEP_ERR_IO_READ    (-8)
#define FXEP_ERR_IO_WRITE   (-9)
#define FXEP_ERR_USER_BREAK (-10)
#define FXEP_ERR_COMPRESSION_FAILED (-11)
#define FXEP_ERR_NO_IO      (-12)
#define FXEP_ERR_INIT       (-13)
#define FXEP_ERR_DECOMPRESSION_FAILED (-14)

//

struct literals {
  unsigned short count;
  unsigned short index;
};

#endif
