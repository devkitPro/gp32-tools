#ifndef _fxe_h_included
#define _fxe_h_included

//

void decrypt_buffer( long, const unsigned char *, long, unsigned char * );
int checkrom( struct disassemble_info *, int );
int fixfxe( struct disassemble_info * );



#endif
