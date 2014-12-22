@***************************************************
@* gp32 crt0.S for compressed FXEs v0.2 by Mr.Spiv *
@***************************************************

@ v0.1 -  8-Aug-2004 - Original release
@ v0.2 - 11-Aug-2004 - The first working version
@ v0.3 - 13-Aug-2004 - Added LUT for huffman decoding, which didn't work
@ v0.4 - 
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


		@.equ	SIZE_PRETABLE,	128+(8 + 16*8 + 16*4)		@ 200   0xc8
		@.equ	SIZE_LTMTABLE,	128+(8 + 16*8 + 512*4)		@ 2184  0x888
		@.equ	SIZE_HGHTABLE,	128+(8 + 16*8 + 256*4)+8	@ 1160  0x488 -> 0x490
		@.equ	SIZE_LOWTABLE,	128+(8 + 16*8 + 256*4)		@ 1160  0x488

		.equ	SIZE_PRETABLE,	(8 + 16*8 + 16*4)		@ 200   0xc8
		.equ	M_POS,			SIZE_PRETABLE
		.equ	SIZE_LTMTABLE,	16+(8 + 16*8 + 512*4)		@ 2184  0x888
		.equ	SIZE_HGHTABLE,	16+(8 + 16*8 + 256*4)+8	@ 1160  0x488 -> 0x490
		.equ	SIZE_LOWTABLE,	16+(8 + 16*8 + 256*4)		@ 1160  0x488
		.equ	M_SIZE,			16

		@.equ	SIZE_STACK,		(SIZE_PRETABLE+SIZE_LTMTABLE+SIZE_HGHTABLE+SIZE_LOWTABLE)
		@ 5224 bytes -> round to 5376 i.e. 0x1500
		@.equ	SIZE_STACK,0x1500
		.equ	SIZE_STACK,0x1300
		.equ	FOO,SIZE_PRETABLE+SIZE_LTMTABLE+SIZE_HGHTABLE-8


		.set	FXE3_SLIDING_WINDOW,	32766
		.equ	FXE3_BLOCK_SIZE, 		FXE2_SLIDING_WINDOW
		.equ	FXEP_EOF,0xffff
		.equ	FXEP_COMPRESSED_MASK,0x8000
		.equ	FXE3_NUM_SYMBOLS,		1024

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
		bic		r12,r0,#3	@ end of usable mem..

		ldr		r11,_rws	@ start of compressed data
		ldr		r9,_zis		@ end of compressed data
		sub		r11,r9,r11	@ size.. should be 4 bytes aligned..
memtransfer:
		ldr		r10,[r9,#-4]!
		subs	r11,r11,#4
		strne	r10,[r12,#-4]!
		bne		memtransfer

		ldr		r9,_dest
		ldr		r0,[r12],#4+4
		str		r0,[r9],#4	@ uncompressed size.. required by GpAppExecute()

		@ r12 - 4 = ptr to crunched data (ID already skipped)
		@ r9 = ptr to destination mem

		sub		sp,sp,#SIZE_STACK		@ space for huffman trees
		mov		r10,#16

		@
		@
		@

blockLoop:
		cmp		r10,#16
		subeq	r12,r12,#4
		subne	r12,r12,#2
		ldrh	r8,[r12],#2		@ compressed size of block
		@ r0 = compressed block size
		cmp		r8,#0
		beq		EOF
		@
		ldrh	r11,[r12],#2
		mov		r10,#16
		ldrh	r1,[r12],#2
		orr		r11,r1,r11,lsl #16

		@ r8 = (end of compressed block)+6
		@ r12 = b (r11 = _bb, r10 = _bc)
		@
		@ Build trees.. r2-r7 are free for use

decodeTrees:
		mov		r2,#8<<16
		mov		r3,#16
		mov		r4,sp
		bl		prepareDecodingStructure

		@ read in depths

		add		r6,sp,#16*8+4
0:		mov		r1,#3
		rsb		r0,r1,#32		@ moved from getB()
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

		@ init MTF table M

		mov		r0,#15
		add		r7,sp,#M_POS
1:		strb	r0,[r7,r0]
		subs	r0,r0,#1
		bpl		1b

		@ decoding structure for real decoding huffman..
		@ r7 = ptr to M

		mov		r2,#16<<16
		mov		r3,#512					@ Literals and matches
		add		r4,sp,#SIZE_PRETABLE+M_SIZE
		bl		prepareDecodingStructure
		bl		buildDecodingTree2	@ DECTREE

		mov		r3,#256					@ high offset
		add		r4,sp,#SIZE_PRETABLE+SIZE_LTMTABLE+M_SIZE
		bl		prepareDecodingStructure
		bl		buildDecodingTree2	@ DECTREE

										@ low offset
		add		r4,sp,#SIZE_PRETABLE+SIZE_LTMTABLE+SIZE_HGHTABLE+M_SIZE
		bl		prepareDecodingStructure
		bl		buildDecodingTree2	@ DECTREE

		@
		@ r5 = oldLength (PMR)      no init required
		@ r6 = oldOffset (PMR)      no init required
		@ r7 = oldOffsetLong (PMR)  no init required
		@
		@ r9  = dest
		@ r0,r1,r8 are general trash registers (functions..)
		@ r10,r11,r12 are reserved for bit input
		@ 

mainLoop:
		add		r2,sp,#SIZE_PRETABLE+M_SIZE
		bl		getSyml
		subs	r3,r2,#256
decodeLiteral:
		bmi		copyLiteral
		beq		blockLoop
		subs	r3,r3,#1
		moveq	r3,r5
		moveq	r4,r6
		beq		copyLoop
decodeMatch:
		@ r3 = match_length-1
		mov		r5,r3			@ PMR oldLength
decodeOffset:
		add		r2,sp,#SIZE_PRETABLE+SIZE_LTMTABLE+M_SIZE
		bl		getSyml
		cmp		r2,#0
		moveq	r4,r7
		beq		oldOffsetLong
		and		r4,r2,#0x7f
		cmp		r4,r2

		addne	r2,sp,#SIZE_PRETABLE+SIZE_LTMTABLE+SIZE_HGHTABLE+M_SIZE
		blne	getSyml
		orrne	r4,r2,r4,lsl #8
		movne	r7,r4			@ PMR oldOffsetLong
oldOffsetLong:
		mov		r6,r4			@ PMR oldOffset
copyLoop:
		ldrb	r2,[r9,-r4]
		subs	r3,r3,#1
copyLiteral:
		strb	r2,[r9],#1
		bpl		copyLoop
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
@  r8
@

getSyml:
		add		r8,r2,#16*8+4
0:		ldr		r1,[r2],#8		@ _tbl->cde
		cmp		r11,r1
		bhi		0b

1:		ldrh	r1,[r2,#-2]		@ cnt = _tbl->bts
		ldrh	r2,[r2,#-4]		@ _tbl->ind
		rsb		r0,r1,#32
		rsb		r2,r2,r11,lsr r0
		ldr		r2,[r8,r2,lsl #2]

@ 
@ r1 = number of input bits (1-16)
@ r0 = return value
@ r10 = bc
@ r11 = bb
@ r12 = membuffer
@
@ Trashes r0 & r1 & flags
@

getB:	@rsb		r0,r1,#32
		subs	r10,r10,r1
		mov		r0,r11,lsr r0
		mov		r11,r11,lsl r1
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
@   128 LUT ???
@   16 dectbl[n]  
@   4  0x00000000
@   m  _alp[m]
@   4  (maxcodelength+1) << 16




@
@
@
@
@
@ Parameters:
@  r2 = (_maxCodeLength+1) << 16
@  r3 = num symbols
@  r4 = dest..
@ 
@ Returns:
@  r5 = 0
@
@ Trashes:
@  r0,r1
@

prepareDecodingStructure:
		add		r1,r4,#16*8+4

		@ insert endmark to _alp..

		str		r2,[r1,r3,lsl #2]

		@ Prepare for _alp..

		mov		r5,#0
		str		r5,[r1,#-4]

		mov		pc,lr


@
@ r3 = num symbold
@ r4 = dest
@ sp = ptr to PRETREE
@
@ and do Inverse MTF at the same time..


buildDecodingTree2:
		stmdb	sp!,{r2,lr}
		mov		r5,#0
		add		r6,r4,#16*8+4
1:		add		r2,sp,#8			@ pretable.. skip r2,r4,lr in stack
		bl		getSyml

		@ inverse MTF
									@ j = M[i];
		mov		r0,r2
		ldrb	r2,[r7,r2]			@ M[i] = T[j];
		add		r7,r7,r0
2:		cmp		r0,#0				@ while (j > 0)
		ldrgtb	r1,[r7,#-1]			@   tmp = T[j - 1];
		subgt	r0,r0,#1			@   j--;
		strgtb	r1,[r7],#-1			@   T[j+1] = tmp;
		bgt		2b

		strb	r2,[r7]				@ T[0] = M[i];

		@
		
		orr		r0,r5,r2,lsl #16
		str		r0,[r6,r5,lsl #2]
		add		r5,r5,#1
		cmp		r5,r3
		blo		1b
		ldmia	sp!,{r2,lr}
		
		@ Do Inverse MTF

@
@ Parameters:
@  r3 = num symbols
@  r4 = dest..
@

buildDecodingTree:
		stmdb	sp!,{r2-r4,r7,lr}
		mov		r5,#0		@ i
		add		r2,r4,#16*8+4

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
		ldmhsia	sp!,{r2-r4,r7,pc}
		ldr		r1,[r2,r5,lsl #2]
		movs	r1,r1,lsr #16
		addeq	r5,r5,#1
		beq		2b

		@ move rest so that n will always be 0 at the beginning..
		
		mov		r6,#0
		sub		r3,r3,r5
3:		ldr		r1,[r2,r5,lsl #2]
		add		r5,r5,#1
		str		r1,[r2,r6,lsl #2]
		add		r6,r6,#1
		cmp		r6,r3
		bls		3b

calculateCanonicalCodes:

		mov		r5,#0
		mov		r6,#0
		mvn		r7,#0

		@ r2 = _alp+_start
		@ r3 = _alpsize
		@ r4 = _tlb
		@ r5 = n
		@ r6 = c = 0
		@ r7 = 0xffffffff

0:		ldrh	r0,[r2,#2]
		ldrh	r1,[r2,#6]
		subs	r1,r1,r0	@ if (r0 < r1) ??
		ble		1f

		mov		lr,r7,lsr r0
		orr		lr,lr,r6, ror r0	
		str		lr,[r4],#4		@ cde
		sub		lr,r6,r5		@ ind = (unsigned short)(c-n)
		strh	lr,[r4],#2		@ ind
		strh	r0,[r4],#2		@ bts

1:		add		r6,r6,#1
		movgt	r6,r6,lsl r1
		add		r5,r5,#1
		ldr		r0,[r2],#4
		cmp		r5,r3			@ for ( ; n < _alpsize; n++)..
		and		r0,r0,r7,lsr #16
		str		r0,[r2,#-4]
		blo		0b
		ldmia	sp!,{r2-r4,r7,pc}


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
	

