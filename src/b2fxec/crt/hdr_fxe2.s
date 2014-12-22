@***************************************************
@* gp32 crt0.S for compressed FXEs v0.2 by Mr.Spiv *
@***************************************************

@ v0.1 - 4-Feb-2004 - Original release
@
@
@ This file is released into the public domain for commercial
@ or non-commercial usage with no restrictions placed upon it.

		.TEXT

@
@ We do not have to ue any "official" startup code
@ because the program we are "launching" will do it
@ anyway..
@
@
@ *** Encodings ***
@
@
@ Literals:
@
@
@
@
@
@
@
@
@
@
@
@
@
@


		.equ	SIZE_PRETABLE,	(12 + 8*8 + 17*4)
		.equ	SIZE_DECTABLE,	(12 + 16*8 + 378*4)
		.equ	SIZE_STACK,		(SIZE_PRETABLE+SIZE_DCTABLE)
		@

		.set	FXE2_SLIDING_WINDOW,	32766
		.equ	FXE2_BLOCK_SIZE, 		FXE2_SLIDING_WINDOW
		.equ	FXEP_EOF,0xffff
		.equ	FXEP_COMPRESSED_MASK,0x8000


		.GLOBAL     _start

		@
		@
		@
		@
		@
		@
		@

_start:
_fxed_start:


        .ALIGN
        .CODE 32

    @ Start Vector

        b       _fxed_init

		.word   _fxed_start    	@ Start of text (Read Only) section
_roe:   .word   _fxed_end		@ End "
_rws:   .word   _data_start    	@ Start of data (Read/Write) section
        .word   0       		@ End of bss (this is the way sdt does it for some reason)
_zis:   .word   0     			@ Start of bss (Zero Initialized) section
_zie:   .word   0       		@ End "

		.word	0x44450011
		.word	0x44450011
		.word	0,0,0,0
_dest:	.word	0
_clk:	.word	0
_fac:	.word	0
_fxed_init:
		@
		@ Set clock speed
		@

		ldr		r0,_clk
		ldr		r1,_fac
		mov		r2,#3
		stmdb	sp!,{r0-r2}
		mov		r0,sp
		cmp		r1,#0
		swine	#0x0d

		@
		@ Move data into upper memory..
		@

		mov		r0,#5
		swi		0xb
		bic		r8,r0,#3	@ end of usable mem..

		ldr		r11,_rws	@ start of compressed data
		ldr		r9,_zis		@ end of compressed data
		sub		r11,r9,r11	@ size.. should be 4 bytes aligned..
memtransfer:
		ldr		r10,[r9,#-4]!
		subs	r11,r11,#4
		strne	r10,[r8,#-4]!
		bne		memtransfer

		ldr		r9,_dest
		ldr		r0,[r8],#4+4
		str		r0,[r9],#4	@ uncompressed size.. required by GpAppExecute()

		@ r8-4 -> r12 = ptr to crunched data (ID already skipped)
		@ r9 = ptr to destination mem

		sub		sp,sp,#2048-128		@ STACK_SIZE

blockLoop:
		sub		r12,r8,#4
		ldrh	r8,[r12],#2		@ compressed size of block
		add		r1,r8,#1
		@ r0 = compressed block size
		cmp		r1,#0x10000
		beq		EOF
		@
		ldrh	r11,[r12],#2
		ldrh	r1,[r12],#2
		mov		r10,#16
		orr		r11,r1,r11,lsl #16
		add		r8,r12,r8

		@ r8 = (end of compressed block)+6
		@ r12 = b (r11 = _bb, r10 = _bc)
		@
		@ Build trees.. r2-r7 are free for use

decodeTrees:
		mov		r2,#3
		mov		r3,#16
		mov		r4,sp
		bl		prepareDecodingStructure

		@ read in depths

		mov		r5,#0
		ldr		r6,[r4]
0:		mov		r1,#3
		bl		getB
		orr		r1,r5,r0,lsl #16
		str		r1,[r6],#4
		add		r5,r5,#1
		cmp		r5,r3
		blo		0b

		@ build tree..
		@  r3 = num symbols = 16
		@  r4 = ptr to decoding structure..

		bl		buildDecodingTree	@ PRETREE

		@ decoding structure for real decoding huffman..

		mov		r2,#4
		mov		r3,#376				@ actually 377
		add		r3,r3,#1
		add		r4,sp,#SIZE_PRETABLE
		bl		prepareDecodingStructure

		@ decode actual decoding huffman depths.. using pretree

		mov		r5,#0
		ldr		r6,[r4]
1:		mov		r2,sp
		bl		getSyml
		orr		r0,r5,r2,lsl #16
		str		r0,[r6],#4
		add		r5,r5,#1
		cmp		r5,r3
		blo		1b

		@ build tree..
		@  r3 = num symbols = 377
		@  r4 = ptr to decoding structure..

		bl		buildDecodingTree	@ DECTREE

		@ r2  = decoding structure ptr..
		@ r8  = (end of compressed block)+6
		@ r9  = dest
		@ r0,r1,r7 are general trash registers (functions..)
		@ r10,r11,r12 are reserved for bit input
		@ 

		sub		r5,r3,#120		@ r5 -> 257 i.e. base for match huffman codes
mainLoop:
		add		r2,sp,#SIZE_PRETABLE
		bl		getSyml
		cmp		r2,#256
		beq		blockLoop
decodeLiteral:
		strlob	r2,[r9],#1
		blo		mainLoop
decodeMatch:
 		sub		r2,r2,r5
		ands	r1,r2,#7		@ r1 = length bits..
		mov		r3,#1
		add		r3,r3,r3,lsl r1
		blne	getB			@ get low length bits.. 
		addne	r3,r3,r0

		movs	r1,r2,lsr #3	@ r4 = offset bits..
		mov		r4,#1
		movne	r4,r4,lsl r1
		blne	getB			@ get low length bits.. 
		addne	r4,r4,r0

copyLoop:
		ldrb	r1,[r9,-r4]
		subs	r3,r3,#1
		strb	r1,[r9],#1
		bne		copyLoop
		b		mainLoop

@
@ Parameters:
@  r11 = cde
@  r1 = ??
@  r2 = ptr to tables..
@
@ Returns:
@  r0 = bits encoding the symbol..
@  r1 = cnt (num bits needed for symbol..)
@  r2 = symbol
@
@ Trashes:
@  r7
@

getSyml:
		ldr		r7,[r2],#4
0:		ldr		r1,[r2],#8
		cmp		r11,r1
		bhi		0b

		ldrh	r1,[r2,#-2]	@ cnt = _tbl->bts
		ldrh	r2,[r2,#-4]	@ _tbl->ind
		rsb		r0,r1,#32
		mov		r0,r11,lsr r0
		add		r0,r0,r2
		mov		r0,r0,lsl #16
		ldr		r2,[r7,r0,lsr #14]

@ 
@ r1 = number of input bits (1-16)
@ r0 = return value
@ r10 = bc
@ r11 = bb
@ r12 = membuffer
@
@ Trashes r0 & r1 & flags
@

getB:	rsb		r0,r1,#32
		subs	r10,r10,r1
		mov		r0,r11,lsr r0
		mov		r11,r11,lsl r1
		@movgt	pc,lr
		@
		ldrleh	r1,[r12],#2
		rsble	r10,r10,#0
		orrle	r11,r11,r1,lsl r10
		rsbles	r10,r10,#16
		mov		pc,lr

@
@
@ Parameters:
@  r2 = numbits
@  r3 = num codes
@  r4 = dest..
@
@ Returns:
@  r0 = ptr to tables..
@


@ Add space for dectbl.. which is maximum 
@ 8 << numbits of bytes..
@
@ struct dectbl {
@   unsigned long cde;
@   unsigned short ind;
@   unsigned short bts;
@ };
@
@ Memory layout is as follows:
@
@   4  ptr_to_alp
@   n  dectbl[n]  
@   4  0x00000000
@   m  _alp[m]
@   4  (maxcodelength+1) << 16
@
@
@ Parameters:
@  r2 = numbits
@  r3 = num symbols
@  r4 = dest..
@

prepareDecodingStructure:
		stmdb	sp!,{r2-r4,lr}
		mov		r1,r4
		add		r4,r4,#4
  		mov		r0,#8
		add		r4,r4,r0,lsl r2

		@ Prepare for _alp..

		mov		r5,#0
		str		r5,[r4],#4
		str		r4,[r1]

		@ insert endmark to _alp..

		mov		r0,#1
		add		r0,r0,r0,lsl r2
		mov		r0,r0,lsl #16
		str		r0,[r4,r3,lsl #2]
		ldmia	sp!,{r2-r4,pc}


@
@ Parameters:
@  r3 = num symbols
@  r4 = dest..
@

buildDecodingTree:
		stmdb	sp!,{r4,lr}
		mov		r5,#0		@ i
		ldr		r2,[r4],#4

inplaceShellsort:
		ldr		r0,[r2,r5,lsl #2]	@ r0 = (i | (depth << 16)
		add		r6,r2,r5,lsl #2
0:		ldr		r1,[r6,#-4]!			@ r1 = (i | (depth << 16)
		cmp		r1,r0
		strhi	r1,[r6,#4]
		bhi		0b
		str		r0,[r6,#4]
		add		r5,r5,#1
		cmp		r5,r3
		blo		inplaceShellsort

		@ Find _start.. i.e. find the first symbol with
		@ nonzero weight.
		@
		@ r2 = start of symbols

		mov		r5,#0
2:		cmp		r5,r3
		bhs		calculateCanonicalCodes
		ldr		r1,[r2,r5,lsl #2]		@ could do lsl #2 :)
		movs	r1,r1,lsr #16
		addeq	r5,r5,#1
		beq		2b

calculateCanonicalCodes:

		add		r6,r2,r5,lsl #2
		mov		r2,#0
		mvn		r7,#0

		@ r4 = _tlb
		@ r3 = _alpsize
		@ r2 = c = 0
		@ r5 = n
		@ r6 = _alp+_start
		@ r7 = 0xffffffff

0:		ldrh	r0,[r6,#2]
		ldrh	r1,[r6,#6]
		subs	r1,r1,r0	@ if (r0 < r1) ??
		ble		1f

		mov		lr,r7,lsr r0
		orr		lr,lr,r2, ror r0	
		str		lr,[r4],#4	@ cde
		sub		lr,r5,r2	@ ind = (unsigned short)(n-c)
		strh	lr,[r4],#2	@ ind
		strh	r0,[r4],#2	@ bts

1:		add		r2,r2,#1
		movgt	r2,r2,lsl r1
		add		r5,r5,#1
		ldr		r0,[r6],#4
		cmp		r5,r3			@ for ( ; n < _alpsize; n++)..
		and		r0,r0,r7,lsr #16
		str		r0,[r6,#-4]
		blo		0b
		ldmia	sp!,{r4,pc}


@
@
@
@
@ AppExecute()
EOF:	mov		r0,sp
		mov		r4,#1
		swi		0x0f		@ GpAppPathGet()
		ldr		r1,[sp]
		mov		r2,sp
.pathloop:
		ldrb	r3,[r0],#1
		subs	r1,r1,#1
		strb		r3,[r2],#1
		bpl		.pathloop

		mov		r1,sp
		ldr 	r0,_dest
		swi		0x05		@ GpAppExecute()

_fxed_end:
		@.data

_data_start:
_compressed_start:
_compressed_end:
		@ encruption key must be at the end of decompressed data
		@ note that the key must be XORed with 0xff
_data_end:
		@
		@
		@
		@

		.ALIGN
		@.BSS


_decompressed_start:
		@.ds		data_size
_decompressed_end:
	.END
	

