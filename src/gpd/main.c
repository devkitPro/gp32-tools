//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  This module contains the main entrypoint for the disassembler and 
//  the initializing of structures etc.
//
// Version:
//  v0.01 08-May-2002 JiK 
//  v0.02 18-May-2002 JiK - Fixed bug in unfxe routine..
//  v0.03
//
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include "disasm.h"
#include "fxe.h"


//
// Static data for dump mode..
//

static uint8_t *buffer = NULL;
static bfd_vma dumpstart = 0;
static bfd_vma dumpend = 0;
//static int fxe = 0;
static int space = 0;
static int len;
static FILE *fh = NULL;

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Usage..
//
// Parameters:
// Returns:
//
//////////////////////////////////////////////////////////////////////////////

static void usage( char **argv ) {
  fprintf(stderr,"Usage: %s [-<options>] rom-file\n",argv[0]);
  fprintf(stderr,"Options:\n");
  fprintf(stderr,"  -l n        List n lines\n");
  fprintf(stderr,"  -b address  File base address in hex\n");
  fprintf(stderr,"  -s n        Add n bytes of empty space\n");
  fprintf(stderr,"  -S address  Start address for dump disassembly\n");
  fprintf(stderr,"  -E address  End address for dump disassembly\n");
  fprintf(stderr,"  -r regs     Register names (raw|gcc|srd|apcs|atpcs|special-atpcs)\n");
  exit(1);
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  
// Parameters:
//  dd - A ptr to tlcs900d structure.
//
// Returns:
//  None.
//
//////////////////////////////////////////////////////////////////////////////

int loadrom_and_init( char *file, struct disassemble_info *dd ) {

  if ((fh = fopen(file,"r+b"))) {
    fseek(fh,0,SEEK_END);
    len = ftell(fh);
    fseek(fh,0,SEEK_SET);

    if ((buffer = (uint8_t *)malloc(len + space + 16))) {
      if (fread(buffer,1,len,fh) == len) {
        fclose(fh);
        fh = NULL;

        printf("Loaded %d (%Xh) bytes. Extra space is %d bytes.\n\n",
                len,len,space);


        // add some space that user requested..

        len += space;

		// setup structure.. only buffer!
		
		setup_disasm(0,NULL,buffer,len,dd);
        checkrom(dd,0);


        return 0;
      } else {
        printf("Error: fread() failed (errno: %d)\n",errno);
        fclose(fh);
        fh = NULL;
      }
    } else {
      printf("Error: malloc() failed (errno: %d)\n",errno);
    }
  } else {
    printf("Error: fopen(%s) failed (errno: %d)\n",file,errno);
  }
  return 1;
}


//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Initializes disassembler structures sets system defaults for the
//  disassembler.
//
// Parameters:
//  dd - A ptr to tlcs900d structure.
//
// Returns:
//  None.
//
//////////////////////////////////////////////////////////////////////////////

static void setdefaults( struct disassemble_info *dd ) {
	disasm_init(dd);
	g_lines = 20;
	g_pos = 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  Parse command line options and override some system defaults
//  based on options.
//
// Parameters:
//  dd - A ptr to tlcs900d structure, which contains system defaults
//       for the disassembler.
//  argc - Number of arguments.
//  argv - A list of command line arguments.
//
// Returns:
//  0 if ok. 1 if error.
//
//////////////////////////////////////////////////////////////////////////////

static int checkoptions( struct disassemble_info *dd, int argc, char **argv ) {
  char *e;
  int n;

  for (n = 1; n < argc - 1; n += 2) {
    if (!strcmp("-l",argv[n])) {        // display lines
      g_lines = atoi(argv[n+1]);
      if (g_lines < 1) { g_lines = 1; }
    } else if (!strcmp("-b",argv[n])) { // base address
    	setup_disasm(strtoul(argv[n+1],&e,16),NULL,NULL,0,dd);
		g_pos = dd->buffer_vma;
      //dd->buffer_vma = strtoul(argv[n+1],&e,16);
    } else if (!strcmp("-s",argv[n])) { // extra space
      space = atoi(argv[n+1]);
      if (space < 0) { space = 0; }
    } else if (!strcmp("-S",argv[n])) { // extra space
      dumpstart = strtoul(argv[n+1],&e,16);
    } else if (!strcmp("-E",argv[n])) { // extra space
      dumpend = strtoul(argv[n+1],&e,16);
    } else if (!strcmp("-r",argv[n])) { // register names
      setup_disasm(0,argv[n+1],NULL,0,dd);
    } else {
      fprintf(stderr,"Unknown option: %s\n",argv[n]);
      return 1;
    }
  }

  if (dumpend && dumpstart > dumpend) {
    fprintf(stderr,"Dump start can not be lower than dump end.\n");
    return 1;
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//  main()
//
// Parameters:
//  argc - 
//  argv - 
//
// Returns:
//  0 if ok, > 0 if something went wrong.
//
//////////////////////////////////////////////////////////////////////////////

int main( int argc, char** argv ) {
  char new_cmd[MAX_CMD];
  struct disassemble_info dd;
  int n=0;

  // Check command line and options..

  if (argc < 2 || argc & 1) {
    usage(argv);
    return 0;
  }

  setdefaults(&dd);

  if (checkoptions(&dd,argc,argv)) {
    usage(argv);
    return 0;
  }

  // Greetings.. :)

  printf( "\n"
          "GP32 aware ARM920T Disassembler v%s\n"
          "     (c) 2002-3 Jouni 'Mr.Spiv' Korhonen\n"
          "\n",GPD_VERSION); 

  // Load rom and init structures.. also check if the rom is
  // a NGP compliant rom..

  if (loadrom_and_init(argv[argc-1],&dd)) {
    printf("Exiting..\n");
    return 0;
  }


  //
  // There are two modes available: dump mode and interactive.
  // The dump mode just disassembles a predefined range of
  // rom to stdout. The dump mode is entered if the user
  // defines either -S or -E option..
  //
  // The interactive mode is.. hmm.. interactive :)
  //

  if (dumpstart != 0 || dumpend != 0) {
    // fix dump mode.. if dump end address is not defined then
    // dump from dump start to the end of rom.

    if (dumpend > dd.buffer_vma) { len = dumpend - dd.buffer_vma; }
    if (dumpstart == 0) { dumpstart = dd.buffer_vma; }

    // tweak the disassembly function not to care about
    // maximum number of outputted lines..

    g_lines = INT_MAX;
    sprintf(new_cmd,"d %x",dumpstart);
    parse_command(new_cmd,&dd);

  } else {
    do {
      printf("-> ");

      if (fgets(new_cmd,80,stdin)) {
        n = parse_command(new_cmd,&dd);

        switch (n) {
        case DPARSE_SYNTAX_ERR:
          printf("Syntax Error..\n");
          break;
        case DPARSE_UNKNOWN_ERR:
          printf("Unknown Command Error..\n");
          break;
        case DPARSE_BOUNDS_ERR:
          printf("Invalid numbers or range error..\n");
          break;
        default:
          break;
        }
      } else {
        puts("Error..");
        continue;
      }
    } while (n != DPARSE_EXIT);
  }

  // Exit stuff..

  if (buffer) { free(buffer); }
  if (fh) { fclose(fh); }
  return 0;
}
