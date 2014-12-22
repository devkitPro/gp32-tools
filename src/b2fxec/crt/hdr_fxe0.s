@***************************************************
@* gp32 crt0.S for compressed FXEs v0.2 by Mr.Spiv *
@***************************************************

@ v0.1 - Original release
@ v0.2 - 18-Jan-2003 - Large file support
@ v0.3 - xx-Jan-2003 - faster decomp
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
@  1 + ([8])             -> one literal
@
@ Matches:
@
@  0 + D + C
@
@  C:
@  1    + [1]     -> 2-3
@  01   + [2]     -> 4-7
@  001  + [3]     -> 8-15
@  0001 + [4]     -> 16-31
@  0000 + (n*[8]) -> 32-2048 (if [8] == 0xff then read next [8] etc..)
@
@  D:
@  ([8]) + 1           -> 1-256       ;  9 bits
@  ([8]) + 01    + [1] -> 257-768     ; 11 bits
@  ([8]) + 001   + [2] -> 769-1792    ; 13 bits
@  ([8]) + 0001  + [3] -> 1793-3840   ; 15 bits
@  ([8]) + 00001 + [4] -> 3841-7936   ; 17 bits
@  ([8]) + 00000 + [5] -> 7937-16128  ; 18 bits
@
@
@
@ faster C:
@
@  10
@  11    -> 2-3
@
@  01+00
@  01+01
@  01+10
@  01+11 -> 4-7
@
@  00+10+00
@  00+10+01
@  00+10+10
@  00+10+11
@  00+11+00
@  00+11+01
@  00+11+10
@  00+11+11 -> 8-15
@
@  00+01+0000
@         ...
@  00+01+1111 -> 16-31
@
@  00+00 + n[8]
@
@
@ faster D:
@
@  1+[8]
@
@  0+10
@  0+11 -> 257-768
@
@  0+01+00 
@  0+01+01
@  0+01+10 
@  0+01+11 -> 768-1792 
@
@  0+00+10+00
@      +10+01
@      +10+10
@      +10+11
@      +11+00
@      +11+01
@      +11+10
@      +11+11 -> 1793-3840
@
@  0+00+01+0000
@         +0001
@         +0010
@         +0011
@         +0100
@         +0101
@         +0110
@         +0111
@         +1000
@         +1001
@         +1010
@         +1011
@         +1100
@         +1101
@         +1110
@  0+00+01+1111 -> 3841-7936
@
@  0+00+00+00000
@           ...
@  0+00+00+11111 -> 7937-16128
@


		.equ	FXE0_OFFSET_8,	256
		.equ	FXE0_OFFSET_9,	512
		.equ	FXE0_OFFSET_10,	1024
		.equ	FXE0_OFFSET_11,	2048
		.equ	FXE0_OFFSET_12,	4096
		.equ	FXE0_OFFSET_13,	8192

		.equ	FXE0_STRTO,		8
		.equ	FXE0_STOPO,		13

		.set	FXE0_SLIDING_WINDOW, FXE0_OFFSET_8+FXE0_OFFSET_9+FXE0_OFFSET_10+FXE0_OFFSET_11
		.set	FXE0_SLIDING_WINDOW, FXE0_SLIDING_WINDOW+FXE0_OFFSET_12+FXE0_OFFSET_13
		.equ	FXE0_BLOCK_SIZE, FXE0_SLIDING_WINDOW
		.equ	FXE0_OFFSET_BITS,	(FXE0_STOPO-FXE0_STRTO)
		.equ	FXE0_LENGTH_BITS,	4
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
		bic		r12,r0,#3	@ end of usable mem..

		ldr		r11,_rws	@ start of compressed data
		ldr		r9,_zis		@ end of compressed data
		sub		r11,r9,r11	@ size.. should be 4 bytes aligned..
memtransfer:
		ldr		r10,[r9,#-4]!
		subs	r11,r11,#4
		strne	r10,[r12,#-4]!
		bne		memtransfer

		@
		@ r12 = ptr to crunched data (ID already skipped)
		@ r9 = ptr to destination mem
		@

		ldr		r9,_dest
		ldr		r0,[r12],#4
		str		r0,[r9],#4
		@
blockLoop:
		ldrb	r0,[r12],#1
		ldrb	r1,[r12],#1
		and		r2,r0,r1
		add		r0,r0,r1,lsl #8
		cmp		r2,#0xff
		@ r0 = block header
		beq		EOF

		@
noEOF:	bic		r8,r0,#FXEP_COMPRESSED_MASK	@ r8 = block size
		cmp		r0,r8
		bne		decrunchFXE0Block
		@ memcpy
memcpyLoop:
		ldrb	r0,[r12],#1
		subs	r8,r8,#1
		strb	r0,[r9],#1
		bne		memcpyLoop
		b		blockLoop

		@
		@ r12 = b (r11 = _bb, r10 = _bc)
		@ r9  = d
		@ r8  = blockSize
		@
decrunchFXE0Block:
initGetB:
		add		r7,r12,r8
		ldrb	r11,[r12],#1
		mov		r10,#8
mainLoop:
		ldrb	r2,[r12],#1
		bl		getOneB
		strneb	r2,[r9],#1	@ r2 = w
		bne		endtest
decodeMatch:
		@
		@ Get offset 
		@

		mov		r3,#FXE0_OFFSET_BITS		@ r3 = n
		mov		r4,#0x100	@ r4 = pwr
		mov		r5,#1		@ r5 = plus
offsetLoop:
		bl		getOneB
		bne		offsetBreak		@ getb() == 1
		
		add		r5,r5,r4
		subs	r3,r3,#1
		add		r4,r4,r4
		bne		offsetLoop
offsetBreak:
		rsbs	r1,r3,#FXE0_OFFSET_BITS
		movne	r2,r2,lsl r1
		blne	getB
		addne	r2,r2,r0
offset255:
		@
		@ Get match length
 		@
		@add		r6,r2,r5	@ add plus @ r6 = d - w == c
		add		r5,r2,r5	@ add plus @ r6 = d - w == c
lengthLoop:
		mov		r1,#2
		bl		getB
		cmp		r0,#1
		bhi		copyLoop	@ 0+x -> length 2-3
		mov		r1,#2
		beq		length01
length00:
		bl		getB
		beq		lengthBytes	@ 00+00
		cmp		r0,#2
		movhs	r1,#2		@ 00+1x+xx
		movlo	r1,#4		@ 00+01+xxxx
length01:
		mov		r2,r0,lsl r1
		bl		getB
		orr		r0,r0,r2
		b		copyLoop
		@
lengthBytes:
		mov		r0,#32
ffLoop:	ldrb	r1,[r12],#1
		add		r0,r0,r1
		cmp		r1,#0xff
		beq		ffLoop
copyLoop:
		@ldrb	r1,[r9,-r6]
		ldrb	r1,[r9,-r5]
		subs	r0,r0,#1
		strb	r1,[r9],#1
		bne		copyLoop
		@
endtest:
		cmp		r12,r7
		blo		mainLoop	@ r9 < r7
		b		blockLoop	@ return..
		@
		@
		@

@
@ 
@ r1 = number of input bits (1-8)
@ r0 = return value
@ Trashes r0 & r1
@ Also sets flags for r0
@

getOneB:
		mov		r1,#1
getB:
		cmp		r10,r1
		ldrlsb	r0,[r12],#1
		subls	r1,r1,r10
		orrls	r11,r0,r11,lsl r10
		movls	r10,#8
enoughBits:
		mov		r11,r11,lsl r1
		sub		r10,r10,r1
		movs	r0,r11,lsr #8
		and		r11,r11,#0xff		
		mov		pc,lr

@
@
@
@
@ AppExecute()
EOF:	sub		sp,sp,#256
		mov		r0,sp
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
	

