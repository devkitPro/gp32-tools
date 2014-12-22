/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  This module contains all commands that the interactive disassembler
//  understands. Also the command line parsing and command function
//  discovery in this module.
//
// Version:
//  v0.1 - 0.3       JiK - Initial versions..
//  v0.4 22-Nov-2001 JiK - Fixed layout..
//
// Supported Commands:
//  d      - disassemble a block of code
//  dt     - disassemble a block of thumb code
//  di     - disassemble a block of code using indirect address
//  dit    - disassemble a block of thumb code using indirect address
//  mode   - change between raw & fxe modes
//  si     - save fxe file's icon
//  m      - hexdump a block of code
//  h      - help page
//  help   - help page
//  info   - display rom information if the rom is a NGP rom
//  :      - poke bytes/shorts/longs into memory (big endian)
//  ;      - poke bytes/shorts/longs into memory (little endian)
//  w      - save a block of code to the disk
//  r      - read a block of code from the disk
//  x      - exit
//  D      - re-disassemble previous disassembly
//  <CRLF> - repeat previous command like 'd' and 'm'
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#include "dis-asm.h"
#include "disasm.h"
#include "bitutils.h"
#include "fxe.h"
//
//
//

static int memdump( struct disassemble_info *, char * );
static int disassem( struct disassemble_info *, char * );
static int disassemT( struct disassemble_info *, char * );
static int disassemI( struct disassemble_info *, char * );
static int disassemIT( struct disassemble_info *, char * );
static int disassemD( struct disassemble_info *, char * );
static int dexit( struct disassemble_info *, char * );
static int rominfo( struct disassemble_info *, char * );
static int help( struct disassemble_info *, char * );
static int poke( struct disassemble_info *, char * );
static int pokeLE( struct disassemble_info *, char * );
static int save( struct disassemble_info *, char * );
static int read( struct disassemble_info *, char * );
static int fxefixer( struct disassemble_info *, char * );

static int compare( const void *, const void * );
static uint32_t getptr( const char * );


//static char cmd[MAX_CMD];
static bfd_vma prev_pos = 0;
static int (*prev_cmd)( struct disassemble_info *, char * ) = NULL;

// Globals.. lines and current position

bfd_vma g_pos;
int g_lines;



/////////////////////////////////////////////////////////////////////////////
//
// A list of implemented commands. Commands MUST be is alphabetical order
// by the command name. In case of mixing strings, chars and special
// marks check the ASCII table for corrent alphabetical ordering.
//
// About commands:
//
// All commands have the same function prototype. The first passed parameters
// is always a ptr to struct tlcs900d. The second passed parameter is always
// a ptr to char string that contains the 'typed' command line without
// the first parameter (i.e. the command like 'd') or NULL. Note that the
// passed string is already parsed with strtok() so the function
// can continue parsing with strtok(NULL, "....");
//
/////////////////////////////////////////////////////////////////////////////

struct _command {
  char *name;                               // name of the command
  int (*cmd)( struct disassemble_info *, char * );  // function ptr to command
  char *help;                               // description of the command
} commands[] = {
  // MUST be in alphabetical order!
  {"",    NULL,      "<CRLF>              -> repeat some previous "
   "commands without parameters"},
  {":",   poke,      ": address bytes ..  -> poke bytes into memory (big endian)"},
  {";",   pokeLE,      "; address bytes ..  -> poke bytes into memory (litte endian)"},
  {"D",   disassemD, "D                   -> disasseble starting from "
   "previous address"},
  {"d",   disassem,  "d [address]         -> disasseble starting from "
   "address"},
  {"di",   disassemI,  "di [address]        -> disasseble starting from "
   "indirect address"},
  {"dit",   disassemIT,  "dit [address]       -> disasseble starting from "
   "indirect address"},
  {"dt",   disassemT,  "dT [address]        -> disasseble thumb starting from "
   "address"},
  {"fxe", fxefixer,    "fxe                 -> Fix memory and decrypt FXE"},
  {"h",   help,      "h                   -> print short command manual"},
  {"help",help,      "help                -> print short command manual"},
  {"info", rominfo, "info                -> print file info"},
  {"m",   memdump,   "m [address]         -> dump memory starting from "
   "address"},
  {"r",   read,      "r name start [lwn]  -> read file into rom starting from start"},
  {"w",   save,      "w name start end    -> save rom between start and "
   "end"},
  {"x",   dexit,     "x                   -> exit"}
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(struct _command))

static int compare( const void *a, const void *b ) {
  return strcmp((const char *)a,((struct _command *)b)->name);
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  This function parses the 'typed' command line. The parsing is done with
//  strtok(). The list of all commands is searched with binary search (which
//  requires that the list of commands is in alphabetical order). When the
//  desired command is found then corresponding function gets called..
//
// Parameters:
//  cmd - a ptr to char string containing the 'typed' command line.
//  dd  - a ptr to disassemble_info structure.
//
// Returns:
//  DPARSE_OK if ok,
//  DPARSE_UNKNOWN_ERR if the command was not found,
//  DPARSE_SYNTAX_ERR if the command line was not valid,
//  DPARSE_BOUNDS_ERR if there was invalid numbers or number ranges,
//  DPARSE_EXIT if the disassembler should exit.
//
/////////////////////////////////////////////////////////////////////////////

int parse_command( char *cmd, struct disassemble_info *dd ) {
  struct _command *cc;
  char *p;

  if ((p = strtok(cmd," \t\r\n"))) {
    if ((cc = (struct _command *)bsearch(p,commands,NUM_COMMANDS,
                                        sizeof(struct _command),compare)) ) {
      prev_cmd = NULL;
      return cc->cmd(dd,strtok(NULL," \t\r\n"));
    } else {
      return DPARSE_UNKNOWN_ERR;
    }
  } else if (prev_cmd) {
      return prev_cmd(dd,NULL);
  }

  return DPARSE_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Dump rom as hex and ascii.
//
// Parameters:
//  dd - a ptr to disassemble_info struct.
//  b  - a ptr to remaining command line.
//
// Returns:
//  DPARSE_OK in any case.
//
// Note:
//  
//
/////////////////////////////////////////////////////////////////////////////

#define CH(p) ( isprint(*(p)) ? *(p) : '.' )

static int memdump( struct disassemble_info *dd, char *b ) {
	uint8_t *p;
	int n;

	if (b == NULL) {
  	} else {
		g_pos = getptr(b);
	}
	if (get_mem_ptr(g_pos,dd) == NULL) {
      printf("Out of file bounds.\n");
      return DPARSE_OK;
    }

	for (n = 0; n < g_lines && (p = get_mem_ptr(g_pos,dd)); n++) {
		printf("%08X: ",g_pos);
		printf("%02X%02X%02X%02X %02X%02X%02X%02X "
			"%02X%02X%02X%02X %02X%02X%02X%02X  ",
			*(p+0),*(p+1),*(p+2),*(p+3),*(p+4),*(p+5),*(p+6),*(p+7),
			*(p+8),*(p+9),*(p+10),*(p+11),*(p+12),*(p+13),*(p+14),*(p+15));

		p = get_mem_ptr(g_pos,dd);
		printf("'%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c'\n",
			CH(p+0),CH(p+1),CH(p+2),CH(p+3),CH(p+4),CH(p+5),CH(p+6),CH(p+7),
			CH(p+8),CH(p+9),CH(p+10),CH(p+11),
			CH(p+12),CH(p+13),CH(p+14),CH(p+15));
		g_pos += 16;
	}

	// remember this for prev_cmd..

	prev_cmd = memdump;

	//

	return DPARSE_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Disassemble the previous page again.
//
// Parameters:
//
// Returns:
//
/////////////////////////////////////////////////////////////////////////////

static int disassemD( struct disassemble_info *dd, char *b ) {
	char ptr[10];
	sprintf(ptr,"%x",prev_pos);
	return disassem(dd,ptr);
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Disassemble n lines of code and output it to stdout.
//
// Parameters:
//
// Returns:
//
/////////////////////////////////////////////////////////////////////////////

static int disassemI( struct disassemble_info *dd, char *b ) {
	uint8_t *p;
	char ptr[10];
	bfd_vma pos;

	if (b) { g_pos = getptr(b);	}
	if ((p = get_mem_ptr(g_pos,dd)) == NULL) {
		printf("Out of file bounds.\n");
		return DPARSE_OK;
	}

	//

	GETL_LE(p,pos);
	sprintf(ptr,"%x",pos);
	return disassem(dd,ptr);
}

static int disassem( struct disassemble_info *dd, char *b ) {
//	uint8_t *m;
	int n, l;

	if (b) { g_pos = getptr(b);	}
	if (get_mem_ptr(g_pos,dd) == NULL) {
		printf("Out of file bounds.\n");
		return DPARSE_OK;
	}

	// record previous position

	prev_pos = g_pos;

	//

	for (n = 0; n < g_lines && get_mem_ptr(g_pos,dd); n++) {
		l = print_insn(g_pos,dd,0);
		g_pos += l;
	}

	// remember this for prev_cmd..

	 prev_cmd = disassem;

	//

	return DPARSE_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Disassemble n lines of thumb code and output it to stdout.
//
// Parameters:
//
// Returns:
//
/////////////////////////////////////////////////////////////////////////////

static int disassemIT( struct disassemble_info *dd, char *b ) {
	uint8_t *p;
	char ptr[10];
	bfd_vma pos;

	if (b) { g_pos = getptr(b);	}
	if ((p = get_mem_ptr(g_pos,dd)) == NULL) {
		printf("Out of file bounds.\n");
		return DPARSE_OK;
	}

	//

	GETL_LE(p,pos);
	sprintf(ptr,"%x",pos);
	return disassemT(dd,ptr);
}

static int disassemT( struct disassemble_info *dd, char *b ) {
//	uint8_t *m;
	int n, l;


	if (b) { g_pos = getptr(b); }
	if (get_mem_ptr(g_pos,dd) == NULL) {
		printf("Out of file bounds.\n");
		return DPARSE_OK;
	}
	// record previous position

	prev_pos = g_pos;

	//

	for (n = 0; n < g_lines && get_mem_ptr(g_pos,dd); n++) {
		l = print_insn(g_pos,dd,1);
		g_pos += l;
	}

	// remember this for prev_cmd..

	 prev_cmd = disassemT;

	//

	return DPARSE_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Exit the disassembler
//
// Parameters:
//
// Returns:
//  DPARSE_EXIT which causes the disassembler to exit.
//
/////////////////////////////////////////////////////////////////////////////

static int dexit( struct disassemble_info *dd, char *b ) {
  return DPARSE_EXIT;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Check and print information about NGP's rom header chunk. It is expected
//  that the header chunk is at the beginning of the loaded rom file.
//
// Parameters:
//
// Returns:
//  DPARSE_OK in any case.
//
/////////////////////////////////////////////////////////////////////////////

static int rominfo( struct disassemble_info *dd, char *b ) {
  checkrom(dd,1);
  return DPARSE_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Print a short disassembler command description to stdout.
//
// Parameters:
//
// Returns:
//  DPARSE_OK in any case.
//
/////////////////////////////////////////////////////////////////////////////

static int help( struct disassemble_info *dd, char *b ) {
  int n;

  printf("\n Quick Command Reference\n\n");

  for (n = 0; n < NUM_COMMANDS; n++) {
    printf("%s\n",commands[n].help);
  }
  return DPARSE_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  
//
// Parameters:
//
// Returns:
//  DPARSE_OK in any case.
//
/////////////////////////////////////////////////////////////////////////////

static int fxefixer( struct disassemble_info *dd, char *b ) {
	fixfxe(dd);
	return DPARSE_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description: (big endian)
//  Poke a number of bytes/shorts/longs into rom file. The poked data size
//  is automatically selected e.g. numbers greater than 0xff are stored as
//  shorts etc. Numbers and address default to hexadecimals but is the number
//  begins with underscore then it will be handled as a decimal i.e. _20
//  means 20 not 14H.
//
// Parameters:
//
// Returns:
//  DPARSE_OK if ok,
//  DPARSE_SYNTAX_ERROR if address is missing.
//
/////////////////////////////////////////////////////////////////////////////

static int poke( struct disassemble_info *dd, char *b ) {
	uint8_t *p;
	bfd_vma i;
	uint32_t x;

	if (b == NULL) {
		return DPARSE_SYNTAX_ERR;
	}

	i = getptr(b);

	if (!get_mem_ptr(i,dd)) {
		printf("End of ROM has been reached.\n");
		return DPARSE_OK;
	}

	while ((b = strtok(NULL," \t\r\n")) && (p = get_mem_ptr(i,dd))) {
		x = getptr(b);

		if (x < 0x100) {
			*p++ = x;
			i++;
		} else if (x < 0x10000) {
			*p++ = x >> 8;
			*p++ = x;
			i += 2;
		} else if (x < 0x1000000) {
			*p++ = x >> 16;
			*p++ = x >> 8;
			*p++ = x;
			i += 3;
		} else {
			*p++ = x >> 24;
			*p++ = x >> 16;
			*p++ = x >> 8;
			*p++ = x;
			i += 4;
		}
	}

	return DPARSE_OK;
}

static int pokeLE( struct disassemble_info *dd, char *b ) {
	uint8_t *p;
	bfd_vma i;
	uint32_t x;

	if (b == NULL) {
		return DPARSE_SYNTAX_ERR;
	}

	i = getptr(b);

	if (!get_mem_ptr(i,dd)) {
		printf("End of ROM has been reached.\n");
		return DPARSE_OK;
	}

	while ((b = strtok(NULL," \t\r\n")) && (p = get_mem_ptr(i,dd))) {
		x = getptr(b);

		if (x < 0x100) {
			*p++ = x;
			i++;
		} else if (x < 0x10000) {
			*p++ = x;
			*p++ = x >> 8;
			i += 2;
		} else if (x < 0x1000000) {
			*p++ = x;
			*p++ = x >> 8;
			*p++ = x >> 16;
			i += 3;
		} else {
			*p++ = x;
			*p++ = x >> 8;
			*p++ = x >> 16;
			*p++ = x >> 24;
			i += 4;
		}
	}

	return DPARSE_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Save a selected range of rom to a file.
//
// Parameters:
//
// Returns:
//  DPARSE_OK if ok,
//  DPARSE_SYNTAX_ERR if not enough parameters are passed,
//  DPARSE_BOUNDS_ERR if save endaddress <= startaddress.
//  In case of disk io error DPARSE_OK is returned but an error message
//  gets printed to stdout.
//
/////////////////////////////////////////////////////////////////////////////

static int save( struct disassemble_info *dd, char *b ) {
  FILE *fh;
  char *name;
  bfd_vma s;
  bfd_vma e;

	if (b == NULL) {
		return DPARSE_SYNTAX_ERR;
	} else {
		name = b;
	}
	if ((b = strtok(NULL, " \t\r\n"))) {
		s = getptr(b);

		if ((b = strtok(NULL, " \t\r\n"))) {
			e = getptr(b);

			if (s >= e) { return DPARSE_BOUNDS_ERR; }

			if ((fh = fopen(name,"w+b"))) {
				if (fwrite(get_mem_ptr(s,dd),1,e-s+1,fh) != (e-s+1)) {
					printf("Writing file '%s' failed\n",name);
				} else {
					printf("Wrote %d bytes to file '%s'\n",e-s+1,name);
				}

				fclose(fh);
			} else {
				printf("Failed to open file '%s' for writing\n",name);
			}

			return DPARSE_OK;
		}
	}

	return DPARSE_SYNTAX_ERR;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Read a selected file into rom.
//
// Parameters:
//
// Returns:
//  DPARSE_OK if ok,
//  DPARSE_SYNTAX_ERR if not enough parameters are passed,
//  DPARSE_BOUNDS_ERR if save endaddress <= startaddress.
//  In case of disk io error DPARSE_OK is returned but an error message
//  gets printed to stdout.
//
/////////////////////////////////////////////////////////////////////////////

static int read( struct disassemble_info *dd, char *b ) {
  FILE *fh;
  char *name;
  bfd_vma s;
  int l;
	bfd_byte *ptr;


	if (b == NULL) {
		return DPARSE_SYNTAX_ERR;
	} else {
		name = b;
	}
	if ((b = strtok(NULL, " \t\r\n"))) {
		s = getptr(b);

		if ((b = strtok(NULL, " \t\r\n"))) {
			l = getptr(b);
		} else {
			l = dd->buffer_length;
		}
		if ((ptr = get_mem_ptr(s,dd)) == NULL) {
			return DPARSE_BOUNDS_ERR;
		}
		if ((ptr+l) > (dd->buffer+dd->buffer_length)) {
			l -= ((ptr+l) - (dd->buffer+dd->buffer_length));
		}
		if ((fh = fopen(name,"r+b"))) {
			if ((l = fread(ptr,1,l,fh)) < 0) {
				printf("Reading file '%s' failed\n",name);
			} else {
				printf("Read %d bytes from file '%s'\n",l,name);
			}
			fclose(fh);
			
		} else {
			printf("Failed to open file '%s' for reading\n",name);
		}
		return DPARSE_OK;
	} else {
		return DPARSE_SYNTAX_ERR;
	}

	return DPARSE_SYNTAX_ERR;
}


/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Search for a string or bytes.
//
// Parameters:
//
// Returns:
//  DPARSE_OK if ok,
//  DPARSE_SYNTAX_ERR if not enough parameters are passed,
//  DPARSE_BOUNDS_ERR if save endaddress <= startaddress.
//  In case of disk io error DPARSE_OK is returned but an error message
//  gets printed to stdout.
//
/////////////////////////////////////////////////////////////////////////////
/*
static int search( struct disassemble_info *dd, uint8_t *s ) {

  while (s && *s != '\0') {
    




  if (*s == '\'') {
    // handle as a string..
  } else {
    // handle as bytes..

  }

  }
  return DPARSE_OK;
}
*/



/////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Convert an ascii string to integer. If the string begins witn an
//  underscore, the number will be handled as a decimal. Normally conversion
//  handles numbers as hexadecimals.
//
// Parameters:
//  s - a ptr to char string containing one or more whitespace separated
//      ascii string representing decimal or hexadecimal numbers.
//
// Returns:
//  0 in case of error (also prints an error message to stdout), otherwise
//  the converted number.
//
/////////////////////////////////////////////////////////////////////////////

static uint32_t getptr( const char *s ) {
  int radix = 16;
  uint32_t r;
  char *b;

  if (*s == '_') {
    radix = 10;
    s++;
  }

  r = strtoul(s,&b,radix);

  if ((r == 0 || r == LONG_MAX)  && errno == ERANGE) {
    printf("Error: %s is not a valid number\n",s);
    r = 0;
  } 

  return r;
}
