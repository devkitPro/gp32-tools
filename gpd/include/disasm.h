#ifndef _disasm_h_included
#define _disasm_h_included

//

#include <stdarg.h>
#include "dis-asm.h"

//

extern bfd_vma g_pos;
extern int g_lines;

//

#define GPD_VERSION "0.4"

//

#define OPS_LEN 80
#define MEM_LEN 80
#define MAX_CMD 80

//
// Return codes..
//

#define DPARSE_EXIT         -1
#define DPARSE_OK           0
#define DPARSE_SYNTAX_ERR   1
#define DPARSE_UNKNOWN_ERR  2
#define DPARSE_BOUNDS_ERR   3

//

int parse_command( char *, struct disassemble_info * );

//


//
//
//



#endif
