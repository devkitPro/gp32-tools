#ifndef _bitutils_h_included
#define _bitutils_h_included

//
//
//

typedef unsigned char uc;

#define PUTB(b,c) { *(b)++ = (uc)(c); }
#define PUTW(b,w) { *(b)++ = (uc)((w) >> 8); *(b)++ = (uc)(w); }
#define PUTL(b,l) { *(b)++ = (uc)((l) >> 24); *(b)++ = (uc)((l) >> 16); \
                    *(b)++ = (uc)((l) >> 8); *(b)++ = (uc)(l); }
#define PUTLL(b,l) { *(b)++ = (uc)((l) >> 56); *(b)++ = (uc)((l) >> 48); \
                     *(b)++ = (uc)((l) >> 40); *(b)++ = (uc)((l) >> 32); \
                     *(b)++ = (uc)((l) >> 24); *(b)++ = (uc)((l) >> 16); \
                     *(b)++ = (uc)((l) >> 8); *(b)++ = (uc)(l); }
#define GETW(b,w) { (w) = *(b)++; (w) <<= 8; (w) |= *(b)++; }
#define GETL(b,l) { (l) = *(b)++; (l) <<= 8; (l) |= *(b)++;\
                    (l) <<= 8; (l) |= *(b)++; (l) <<= 8; (l) |= *(b)++;}
#define GETLL(b,l) { (l) = *(b)++; (l) <<= 8; (l) |= *(b)++;\
                     (l) <<= 8; (l) |= *(b)++; (l) <<= 8; (l) |= *(b)++;\
                     (l) <<= 8; (l) |= *(b)++; (l) <<= 8; (l) |= *(b)++;\
                     (l) <<= 8; (l) |= *(b)++; (l) <<= 8; (l) |= *(b)++;}
#define MAKEBCD(x,a,b) { (x) = ((a) & 0x0f) << 4; (x) |= ((b) & 0x0f); }

// Little Endian versions..

#define PUTL_LE(b,l) { *(b)++ = (uc)(l); *(b)++ = (uc)((l) >> 8); \
                    *(b)++ = (uc)((l) >> 16); *(b)++ = (uc)((l) >> 24); }
/*#define GETL_LE(b,l) { (l) = *(b)++; (l) |= ((*(b)++) << 8);\
                       (l) |= ((*(b)++) << 16); (l) |= ((*(b)++) << 24);}
*/
#define GETL_LE(b,l) { (l) = (b)[0]; (l) |= ((b)[1] << 8);\
                       (l) |= ((b)[2] << 16); (l) |= ((b)[3] << 24); (b) += 4;}
#define PUTW_LE(b,w) { *(b)++ = (uc)(w); *(b)++ = (uc)((w) >> 8); }
//#define GETW_LE(b,w) { (w) = *(b)++; (w) |= (*(b)++) << 8; }
#define GETW_LE(b,w) { (w) = (b)[0]; (w) |= ((b)[1] << 8); (b) += 2; }

#endif
