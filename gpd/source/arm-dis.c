/* Instruction printing code for the ARM
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001
   Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)
   Modification by James G. Smith (jsmith@cygnus.co.uk)

This file is part of libopcodes. 

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option)
any later version. 

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
more details. 

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "dis-asm.h"
#include "arm-dis.h"


#ifndef streq
#define streq(a,b)	(strcmp ((a), (b)) == 0)
#endif

#ifndef strneq
#define strneq(a,b,n)	(strncmp ((a), (b), (n)) == 0)
#endif

#ifndef NUM_ELEM
#define NUM_ELEM(a)     (sizeof (a) / sizeof (a)[0])
#endif

static char * arm_conditional[] =
{"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
 "hi", "ls", "ge", "lt", "gt", "le", "", "nv"};

typedef struct
{
  const char * name;
  const char * description;
  const char * reg_names[16];
}
arm_regname;

static arm_regname regnames[] =
{
  { "raw" , "Select raw register names",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"}},
  { "gcc",  "Select register names used by GCC",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "sl",  "fp",  "ip",  "sp",  "lr",  "pc" }},
  { "std",  "Select register names used in ARM's ISA documentation",
    { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "sp",  "lr",  "pc" }},
  { "apcs", "Select register names used in the APCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "v4", "v5", "v6", "sl",  "fp",  "ip",  "sp",  "lr",  "pc" }},
  { "atpcs", "Select register names used in the ATPCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "v4", "v5", "v6", "v7",  "v8",  "IP",  "SP",  "LR",  "PC" }},
  { "special-atpcs", "Select special register names used in the ATPCS",
    { "a1", "a2", "a3", "a4", "v1", "v2", "v3", "WR", "v5", "SB", "SL",  "FP",  "IP",  "SP",  "LR",  "PC" }}
};

/* Default to GCC register name set.  */
static unsigned int regname_selected = 2;

#define NUM_ARM_REGNAMES  NUM_ELEM (regnames)
#define arm_regnames      regnames[regname_selected].reg_names

static char * arm_fp_const[] =
{"0.0", "1.0", "2.0", "3.0", "4.0", "5.0", "0.5", "10.0"};

static char * arm_shift[] = 
{"lsl", "lsr", "asr", "ror"};

/* Forward declarations.  */
static void arm_decode_shift(long, fprintf_ftype, void *);
static int  print_insn_arm(bfd_vma, struct disassemble_info *, long);
static int  print_insn_thumb(bfd_vma, struct disassemble_info *, long);

int get_arm_regname_num_options (void);
int set_arm_regname_option (int option);
int get_arm_regnames (int option, const char **setname,
		      const char **setdescription,
		      const char ***register_names);

/* Functions. */


static int local_read( bfd_vma addr, bfd_byte *dest, uint32_t len,
						 struct disassemble_info *info) {

	bfd_vma pos = addr - info->buffer_vma;

	if (pos < 0 || pos >= info->buffer_length) {
		return 1;
	}

	//

	memcpy(dest,&(info->buffer[pos]),len);
	return 0;
}


static void local_addr_print(bfd_vma addr, struct disassemble_info *info) {
	info->fprintf_func(info->stream,"%08x",addr);
}

static void local_addr_print_with_long(bfd_vma addr, struct disassemble_info *info) {
	unsigned long val;
	unsigned char b[4];
	local_read(addr,b,4,info);
	val = (b[0]) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);

	info->fprintf_func(info->stream,"%08x -> %08x",addr,val);
}

static void local_addr_print_with_short(bfd_vma addr, struct disassemble_info *info) {
	unsigned short val;
	unsigned char b[2];
	local_read(addr,b,4,info);
	val = (b[0]) | (b[1] << 8);

	info->fprintf_func(info->stream,"%08x -> %04x",addr,val);
}


uint8_t *get_mem_ptr( bfd_vma addr, struct disassemble_info *info ) {
	bfd_vma pos = addr - info->buffer_vma;

	if (pos < 0 || pos >= info->buffer_length) {
		return NULL;
	} else {
		return info->buffer + pos;
	}
}

//
// Setup this 'engine' for LittleARM920T.. and for GP32..
//
//

void disasm_init( struct disassemble_info *info ) {
	info->buffer_vma = 0x00000000;
	info->buffer = NULL;
	info->buffer_length = 0;
	regname_selected = 2;   // std

	// print function..
	
	info->fprintf_func = (fprintf_ftype)fprintf;
	info->stream = (void *)stdout;
	
	// memory copy function..

	info->read_memory_func = local_read;
	info->print_address_func = local_addr_print;
	info->print_address_func_with_long = local_addr_print_with_long;
	info->print_address_func_with_short = local_addr_print_with_short;
}


void setup_disasm( bfd_vma base, char *regs, uint8_t *buf, long blen,
  				   struct disassemble_info *info ) {
	int n;

	// virtual start of the memory...?
	
	if (base != 0) { info->buffer_vma = base; }

	if (buf) {
		info->buffer = buf;
		info->buffer_length = blen;
	}
	
	
	// find regnames..

	if (regs) {
		for (n = 0; n < get_arm_regname_num_options(); n++) {
			if (streq(regnames[n].name,regs)) {
				set_arm_regname_option(n);
				break;
			}
		}
		if (n == get_arm_regname_num_options()) {
			fprintf(stderr,"*** Unknown regname option. Defaulting to 'std'\n");
			set_arm_regname_option(2);
		}
	}
}





int
get_arm_regname_num_options (void)
{
  return NUM_ARM_REGNAMES;
}

int
set_arm_regname_option (int option)
{
  int old = regname_selected;
  regname_selected = option;
  return old;
}

int
get_arm_regnames (int option, const char **setname,
		  const char **setdescription,
                  const char ***register_names)
{
  *setname = regnames[option].name;
  *setdescription = regnames[option].description;
  *register_names = regnames[option].reg_names;
  return 16;
}

static void
arm_decode_shift (given, func, stream)
     long given;
     fprintf_ftype func;
     void * stream;
{
  func (stream, "%s", arm_regnames[given & 0xf]);
  
  if ((given & 0xff0) != 0)
    {
      if ((given & 0x10) == 0)
	{
	  int amount = (given & 0xf80) >> 7;
	  int shift = (given & 0x60) >> 5;
	  
	  if (amount == 0)
	    {
	      if (shift == 3)
		{
		  func (stream, ", rrx");
		  return;
		}
	      
	      amount = 32;
	    }
	  
	  func (stream, ", %s #%d", arm_shift[shift], amount);
	}
      else
	func (stream, ", %s %s", arm_shift[(given & 0x60) >> 5],
	      arm_regnames[(given & 0xf00) >> 8]);
    }
}

/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction (always 4 on ARM). */
static int
print_insn_arm (pc, info, given)
     bfd_vma                   pc;
     struct disassemble_info * info;
     long                      given;
{
  struct arm_opcode *  insn;
  void *               stream = info->stream;
  fprintf_ftype        func   = info->fprintf_func;

  for (insn = arm_opcodes; insn->assembler; insn++)
    {
      if ((given & insn->mask) == insn->value)
	{
	  char * c;
	  
	  for (c = insn->assembler; *c; c++)
	    {
	      if (*c == '%')
		{
		  switch (*++c)
		    {
		    case '%':
		      func (stream, "%%");
		      break;

		    case 'a':
		      if (((given & 0x000f0000) == 0x000f0000)
			  && ((given & 0x02000000) == 0))
			{
			  int offset = given & 0xfff;
			  
			  func (stream, "[pc");
 
			  if (given & 0x01000000)
			    {
			      if ((given & 0x00800000) == 0)
				offset = - offset;
			  
			      /* pre-indexed */
			      func (stream, ", #%d]", offset);

			      offset += pc + 8;

			      /* Cope with the possibility of write-back
				 being used.  Probably a very dangerous thing
				 for the programmer to do, but who are we to
				 argue ?  */
			      if (given & 0x00200000)
				func (stream, "!");
			    }
			  else
			    {
			      /* Post indexed.  */
			      func (stream, "], #%d", offset);

			      offset = pc + 8;  /* ie ignore the offset.  */
			    }
			  
			  func (stream, "\t; ");
				if ((given & 0x0c100000) == 0x04100000) {	// print exteded LDR info
					if (!(given & 0x400000)) {
					  info->print_address_func_with_long (offset, info);
					} else {
					  info->print_address_func (offset, info);
					}
				}
			}
		      else
			{
			  func (stream, "[%s", 
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, ", %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
				  func (stream, ", %s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}

			      func (stream, "]%s", 
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
			      if ((given & 0x02000000) == 0)
				{
				  int offset = given & 0xfff;
				  if (offset)
				    func (stream, "], %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else 
				    func (stream, "]");
				}
			      else
				{
				  func (stream, "], %s",
					(((given & 0x00800000) == 0) 
					 ? "-" : ""));
				  arm_decode_shift (given, func, stream);
				}
			    }
			}
		      break;

		    case 's':
                      if ((given & 0x004f0000) == 0x004f0000)
			{
                          /* PC relative with immediate offset.  */
			  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
			  
			  if ((given & 0x00800000) == 0)
			    offset = -offset;
			  
			  func (stream, "[pc, #%d]\t; ", offset);
			  
			  (*info->print_address_func)
			    (offset + pc + 8, info);
			}
		      else
			{
			  func (stream, "[%s", 
				arm_regnames[(given >> 16) & 0xf]);
			  if ((given & 0x01000000) != 0)
			    {
                              /* Pre-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, ", %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				}
			      else
				{
                                  /* Register.  */
				  func (stream, ", %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}

			      func (stream, "]%s", 
				    ((given & 0x00200000) != 0) ? "!" : "");
			    }
			  else
			    {
                              /* Post-indexed.  */
			      if ((given & 0x00400000) == 0x00400000)
				{
                                  /* Immediate.  */
                                  int offset = ((given & 0xf00) >> 4) | (given & 0xf);
				  if (offset)
				    func (stream, "], %s#%d",
					  (((given & 0x00800000) == 0)
					   ? "-" : ""), offset);
				  else 
				    func (stream, "]");
				}
			      else
				{
                                  /* Register.  */
				  func (stream, "], %s%s",
					(((given & 0x00800000) == 0)
					 ? "-" : ""),
                                        arm_regnames[given & 0xf]);
				}
			    }
			}
		      break;
			  
		    case 'b':
		      (*info->print_address_func)
			(BDISP (given) * 4 + pc + 8, info);
		      break;

		    case 'c':
		      func (stream, "%s",
			    arm_conditional [(given >> 28) & 0xf]);
		      break;

		    case 'm':
		      {
			int started = 0;
			int reg;

			func (stream, "{");
			for (reg = 0; reg < 16; reg++)
			  if ((given & (1 << reg)) != 0)
			    {
			      if (started)
				func (stream, ", ");
			      started = 1;
			      func (stream, "%s", arm_regnames[reg]);
			    }
			func (stream, "}");
		      }
		      break;

		    case 'o':
		      if ((given & 0x02000000) != 0)
			{
			  int rotate = (given & 0xf00) >> 7;
			  int immed = (given & 0xff);
			  immed = (((immed << (32 - rotate))
				    | (immed >> rotate)) & 0xffffffff);
			  func (stream, "#%d\t; 0x%x", immed, immed);
			}
		      else
			arm_decode_shift (given, func, stream);
		      break;

		    case 'p':
		      if ((given & 0x0000f000) == 0x0000f000)
			func (stream, "p");
		      break;

		    case 't':
		      if ((given & 0x01200000) == 0x00200000)
			func (stream, "t");
		      break;

		    case 'A':
		      func (stream, "[%s", arm_regnames [(given >> 16) & 0xf]);
		      if ((given & 0x01000000) != 0)
			{
			  int offset = given & 0xff;
			  if (offset)
			    func (stream, ", %s#%d]%s",
				  ((given & 0x00800000) == 0 ? "-" : ""),
				  offset * 4,
				  ((given & 0x00200000) != 0 ? "!" : ""));
			  else
			    func (stream, "]");
			}
		      else
			{
			  int offset = given & 0xff;
			  if (offset)
			    func (stream, "], %s#%d",
				  ((given & 0x00800000) == 0 ? "-" : ""),
				  offset * 4);
			  else
			    func (stream, "]");
			}
		      break;

		    case 'B':
		      /* Print ARM V5 BLX(1) address: pc+25 bits.  */
		      {
			bfd_vma address;
			bfd_vma offset = 0;
			
			if (given & 0x00800000)
			  /* Is signed, hi bits should be ones.  */
			  offset = (-1) ^ 0x00ffffff;

			/* Offset is (SignExtend(offset field)<<2).  */
			offset += given & 0x00ffffff;
			offset <<= 2;
			address = offset + pc + 8;
			
			if (given & 0x01000000)
			  /* H bit allows addressing to 2-byte boundaries.  */
			  address += 2;

		        info->print_address_func (address, info);
		      }
		      break;

		    case 'I':
		      /* Print a Cirrus/DSP shift immediate.  */
		      /* Immediates are 7bit signed ints with bits 0..3 in
			 bits 0..3 of opcode and bits 4..6 in bits 5..7
			 of opcode.  */
		      {
			int imm;

			imm = (given & 0xf) | ((given & 0xe0) >> 1);

			/* Is ``imm'' a negative number?  */
			if (imm & 0x40)
			  imm |= (-1 << 7);

			func (stream, "%d", imm);
		      }

		      break;

		    case 'C':
		      func (stream, "_");
		      if (given & 0x80000)
			func (stream, "f");
		      if (given & 0x40000)
			func (stream, "s");
		      if (given & 0x20000)
			func (stream, "x");
		      if (given & 0x10000)
			func (stream, "c");
		      break;

		    case 'F':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "4");
			  break;
			case 0x8000:
			  func (stream, "1");
			  break;
			case 0x00400000:
			  func (stream, "2");
			  break;
			default:
			  func (stream, "3");
			}
		      break;
			
		    case 'P':
		      switch (given & 0x00080080)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x80:
			  func (stream, "d");
			  break;
			case 0x00080000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, "<illegal precision>");
			  break;
			}
		      break;
		    case 'Q':
		      switch (given & 0x00408000)
			{
			case 0:
			  func (stream, "s");
			  break;
			case 0x8000:
			  func (stream, "d");
			  break;
			case 0x00400000:
			  func (stream, "e");
			  break;
			default:
			  func (stream, "p");
			  break;
			}
		      break;
		    case 'R':
		      switch (given & 0x60)
			{
			case 0:
			  break;
			case 0x20:
			  func (stream, "p");
			  break;
			case 0x40:
			  func (stream, "m");
			  break;
			default:
			  func (stream, "z");
			  break;
			}
		      break;

		    case '0': case '1': case '2': case '3': case '4': 
		    case '5': case '6': case '7': case '8': case '9':
		      {
			int bitstart = *c++ - '0';
			int bitend = 0;
			while (*c >= '0' && *c <= '9')
			  bitstart = (bitstart * 10) + *c++ - '0';

			switch (*c)
			  {
			  case '-':
			    c++;
			    
			    while (*c >= '0' && *c <= '9')
			      bitend = (bitend * 10) + *c++ - '0';
			    
			    if (!bitend)
			      abort ();
			    
			    switch (*c)
			      {
			      case 'r':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%s", arm_regnames[reg]);
				}
				break;
			      case 'd':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%d", reg);
				}
				break;
			      case 'x':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "0x%08x", reg);
				  
				  /* Some SWI instructions have special
				     meanings.  */
				  if ((given & 0x0fffffff) == 0x0FF00000)
				    func (stream, "\t; IMB");
				  else if ((given & 0x0fffffff) == 0x0FF00001)
				    func (stream, "\t; IMBRange");
				}
				break;
			      case 'X':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  func (stream, "%01x", reg & 0xf);
				}
				break;
			      case 'f':
				{
				  long reg;
				  
				  reg = given >> bitstart;
				  reg &= (2 << (bitend - bitstart)) - 1;
				  
				  if (reg > 7)
				    func (stream, "#%s",
					  arm_fp_const[reg & 7]);
				  else
				    func (stream, "f%d", reg);
				}
				break;
			      default:
				abort ();
			      }
			    break;

			  case 'y':
			  case 'z':
			    {
			      int single = *c == 'y';
			      int regno;

			      switch (bitstart)
				{
				case 4: /* Sm pair */
				  func (stream, "{");
				  /* Fall through.  */
				case 0: /* Sm, Dm */
				  regno = given & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 5) & 1;
				    }
				  break;

				case 1: /* Sd, Dd */
				  regno = (given >> 12) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 22) & 1;
				    }
				  break;

				case 2: /* Sn, Dn */
				  regno = (given >> 16) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 7) & 1;
				    }
				  break;

				case 3: /* List */
				  func (stream, "{");
				  regno = (given >> 12) & 0x0000000f;
				  if (single)
				    {
				      regno <<= 1;
				      regno += (given >> 22) & 1;
				    }
				  break;

				  
				default:
				  abort ();
				}

			      func (stream, "%c%d", single ? 's' : 'd', regno);

			      if (bitstart == 3)
				{
				  int count = given & 0xff;

				  if (single == 0)
				    count >>= 1;

				  if (--count)
				    {
				      func (stream, "-%c%d",
					    single ? 's' : 'd',
					    regno + count);
				    }

				  func (stream, "}");
				}
			      else if (bitstart == 4)
				func (stream, ", %c%d}", single ? 's' : 'd',
				      regno + 1);

			      break;
			    }

			  case '`':
			    c++;
			    if ((given & (1 << bitstart)) == 0)
			      func (stream, "%c", *c);
			    break;
			  case '\'':
			    c++;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c);
			    break;
			  case '?':
			    ++c;
			    if ((given & (1 << bitstart)) != 0)
			      func (stream, "%c", *c++);
			    else
			      func (stream, "%c", *++c);
			    break;
			  default:
			    abort ();
			  }
			break;

		      default:
			abort ();
		      }
		    }
		}
	      else
		func (stream, "%c", *c);
	    }
	  return 4;
	}
    }
  abort ();
}

/* Print one instruction from PC on INFO->STREAM.
   Return the size of the instruction. */
static int
print_insn_thumb (pc, info, given)
     bfd_vma                   pc;
     struct disassemble_info * info;
     long                      given;
{
  struct thumb_opcode * insn;
  void *                stream = info->stream;
  fprintf_ftype         func = info->fprintf_func;

  for (insn = thumb_opcodes; insn->assembler; insn++) {
      if ((given & insn->mask) == insn->value) {
          char * c = insn->assembler;

          /* Special processing for Thumb 2 instruction BL sequence:  */
          if (!*c) { /* Check for empty (not NULL) assembler string.  */
	      	long offset;
	      
			info->fprintf_func(info->stream," %04X\t",(given>>16)&0xffff);
	      	info->bytes_per_chunk = 4;
	      	info->bytes_per_line  = 4;

	      	offset = BDISP23 (given);
	      
	      	//if ((given & 0x10000000) == 0) {
	      	if ((given & 0x1000) == 0) {
			  func (stream, "blx\t");

			  /* The spec says that bit 1 of the branch's destination
			     address comes from bit 1 of the instruction's
			     address and not from the offset in the instruction.  */
			  	if (offset & 0x1) {
			    	/* func (stream, "*malformed!* "); */
			    	offset &= ~ 0x1;
				}

				offset |= ((pc & 0x2) >> 1);
			} else {
				func (stream, "bl\t");
			}

			info->print_address_func (offset * 2 + pc + 4, info);
			return 4;
		} else {

			info->fprintf_func(info->stream,"    \t");

			info->bytes_per_chunk = 2;
			info->bytes_per_line  = 4;
	  	      
			given &= 0xffff;
	      	//given >>= 16;
	      
			for (; *c; c++) {
				if (*c == '%') {
					int domaskpc = 0;
					int domasklr = 0;
		      
					switch (*++c) {
					case '%':
						func (stream, "%%");
						break;
					case 'S':
						{
							long reg;
			    
							reg = (given >> 3) & 0x7;
							if (given & (1 << 6)) { reg += 8; }
			    
							func (stream, "%s", arm_regnames[reg]);
						}
						break;
					case 'D':
						{
 							long reg;
			    
                            reg = given & 0x7;
                            if (given & (1 << 7))
                             reg += 8;
			    
                            func (stream, "%s", arm_regnames[reg]);
						}
						break;
					case 'T':
                          func (stream, "%s",
                                arm_conditional [(given >> 8) & 0xf]);
                          break;

                        case 'N':
                          if (given & (1 << 8))
                            domasklr = 1;
                          /* Fall through.  */
                        case 'O':
                          if (*c == 'O' && (given & (1 << 8)))
                            domaskpc = 1;
                          /* Fall through.  */
                        case 'M':
                          {
                            int started = 0;
                            int reg;
			    
                            func (stream, "{");
			    
                            /* It would be nice if we could spot
                               ranges, and generate the rS-rE format: */
                            for (reg = 0; (reg < 8); reg++)
                              if ((given & (1 << reg)) != 0)
                                {
                                  if (started)
                                    func (stream, ", ");
                                  started = 1;
                                  func (stream, "%s", arm_regnames[reg]);
                                }

                            if (domasklr)
                              {
                                if (started)
                                  func (stream, ", ");
                                started = 1;
                                func (stream, arm_regnames[14] /* "lr" */);
                              }

                            if (domaskpc)
                              {
                                if (started)
                                  func (stream, ", ");
                                func (stream, arm_regnames[15] /* "pc" */);
                              }

                            func (stream, "}");
                          }
                          break;


                        case '0': case '1': case '2': case '3': case '4': 
                        case '5': case '6': case '7': case '8': case '9':
                          {
                            int bitstart = *c++ - '0';
                            int bitend = 0;
			    
                            while (*c >= '0' && *c <= '9')
                              bitstart = (bitstart * 10) + *c++ - '0';

                            switch (*c)
                              {
                              case '-':
                                {
                                  long reg;
				  
                                  c++;
                                  while (*c >= '0' && *c <= '9')
                                    bitend = (bitend * 10) + *c++ - '0';
                                  if (!bitend)
                                    abort ();
                                  reg = given >> bitstart;
                                  reg &= (2 << (bitend - bitstart)) - 1;
                                  switch (*c)
                                    {
                                    case 'r':
                                      func (stream, "%s", arm_regnames[reg]);
                                      break;

                                    case 'd':
                                      func (stream, "%d", reg);
                                      break;

                                    case 'H':
                                      func (stream, "%d", reg << 1);
                                      break;

                                    case 'W':
                                      func (stream, "%d", reg << 2);
                                      break;

                                    case 'a':
				      /* PC-relative address -- the bottom two
					 bits of the address are dropped
					 before the calculation.  */
                                      info->print_address_func_with_long
					(((pc + 4) & ~3) + (reg << 2), info);
                                      break;

                                    case 'x':
                                      func (stream, "0x%04x", reg);
                                      break;

                                    case 'I':
                                      reg = ((reg ^ (1 << bitend)) - (1 << bitend));
                                      func (stream, "%d", reg);
                                      break;

                                    case 'B':
                                      reg = ((reg ^ (1 << bitend)) - (1 << bitend));
                                      (*info->print_address_func)
                                        (reg * 2 + pc + 4, info);
                                      break;

                                    default:
                                      abort ();
                                    }
                                }
                                break;

                              case '\'':
                                c++;
                                if ((given & (1 << bitstart)) != 0)
                                  func (stream, "%c", *c);
                                break;

                              case '?':
                                ++c;
                                if ((given & (1 << bitstart)) != 0)
                                  func (stream, "%c", *c++);
                                else
                                  func (stream, "%c", *++c);
                                break;

                              default:
                                 abort ();
                              }
                          }
                          break;

                        default:
                          abort ();
                        }
                    }
                  else
                    func (stream, "%c", *c);
                }
             }
          return 2;
       }
    }

  /* No match.  */

  abort ();
}


/* NOTE: There are no checks in these routines that
   the relevant number of data bytes exist.  */
int print_insn (pc, info, thumb )
     bfd_vma pc;
     struct disassemble_info * info;
     boolean thumb;
{
	unsigned char      b[4];
	uint32_t           given;
	int                status;

	//
  
	if (thumb) {
		info->bytes_per_chunk = 2;
		//pc &= ~0x01;
		if (info->read_memory_func(pc,&b[0],4,info)) {
			return 0;
		}

		// in case of BL & BLX..
		given = (b[0]) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
		//given = (b[0]) | (b[1] << 8);
		//info->fprintf_func(info->stream,"%08X: %04X    \t",pc,given & 0xffff);
		info->fprintf_func(info->stream,"%08X: %04X",pc,given & 0xffff);
		status = print_insn_thumb (pc, info, given);
	} else {
		info->bytes_per_chunk = 4;
		//pc &= ~0x03;
		info->read_memory_func(pc,&b[0],4,info);
		given = (b[0]) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);

		//printf("pc: %08x, given: %0x\n",pc,given);
		info->fprintf_func(info->stream,"%08X: %08X\t",pc,given);
 		status = print_insn_arm (pc, info, given);
 	}

	info->fprintf_func(info->stream,"\n");
	

 	return status;
}

