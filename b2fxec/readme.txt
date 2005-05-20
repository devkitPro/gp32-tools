
b2fxec v0.6a-pre2 (c) 2002-2004 Jouni 'Mr.Spiv' Korhonen


What is b2fxec?

        b2fxec is a tool to create crunched (compressed) FXEs and GXBs
        from normal GXBs, AXFs and FXEs. The usage of this tool is almost
        identical to famous b2fxe tool by Jeff F.
	
        The nice thing about b2fxec is that the outputted file also contains
        a short and fast decruncher (decompressor) program that can
        actually run the crunched file. So produced FXEs and GXBs are
        completely self contained. I find it functionally equivalent
        to UPX and all other exe packers.
	
        To find out program options type:
		b2fxec -h

        b2fxec now handles the path correctly. The decompressed
        program "inherits" the environment that the compressed
        program has.

        b2fxec can compress and decompress as large files as the
        GP32 can load. Theretically the upper limit for a file is
        somewhere over 7MB.
		
Terms for commercial use:
        Give me an original copy of your commercial program (with
        credits inside) and you are free to use b2fxec in your
        commercial project.

Version history:
        - v0.6a-pre2 Added:
          - Fixed huffman tree balancing code - finally
        - v0.6a-pre Added:
          - Why -pre? It's mainly targetted to native Windows
            compatibility.
          - Fixed (read rewrote) the Huffman tree balancing algorithm,
            that has been giving me grey hair for a long time already.
          - Aligned windows/Linux/OSX sources so that the very same
            source compiles for all platforms.
        - v0.6beta Added:
          1) FXE to FXE compression. Good for compressing older not
             compressed FXEs.
          2) Added a new algorith, (use -A 3 option), which compresses
             significantly better. The decompression is slightly slower.
          3) Added -F option, which allows overclocking the decruncher.
             I admid this option is dangerous so use it carefully.
          4) Bugfixes (again) in Huffman tree balancing code.
          5) File framing slightly modified.
        - v0.5d Windows version without Cygwin.
        - v0.5c Fixed a bug in Huffman tree balancing code. Thanks
         to JyCet for sending me a file that triggered this bug.
        - v0.5b fixes:
          1) File truncation was also applied to data files, which
             is of course not what we want to do.
          2) Buffer overrun bug with the default algorithm 2.
             Thanks to Mithris for sending me a file that triggered
             this bug.
        - v0.5 adds:
          1) a workaround for GpAppExecute() BIOS bug when
             code & data sections are not continuous.
          2) automatic file truncation feature for files that are
             longer than stated in GXB header (ADS feature).
          3) a new -f switch to set 132MHz clockspeed for the
             decompressor
          4) large file support (again)
        - v0.4a release version.
        - v0.4 adds a new algorithm (use -A 2 option), which has
         considerably better compression. Also old decrompression
         routines have been optimized. Beta version.
        - v0.3c adds large file support i.e. files up 7MB
        - v0.3b re-fixes application path setting in decruncher code
        - v0.3 fixes application path setting in decruncher code
        - v0.2f adds possibility to load a raw bin file as an icon.
        - v0.2e adds data save option.
        - v0.2d fixes a BMP loading problem.
        - v0.2e adds better compatability to Pacrom v0.31b.

Technical information:

        Everything in b2fxec has been written from scratch in C++ and all
        algorithms have been developed by the author. No single line
        of code has been borrowed or copied from other sources - with the
        exception of "inplace minimum redundancy code generation" function
        (c) Alistair Moffat.

        Similarities with existing algorithms and implementations are
        inevitable as if you spend time with these algoritms and study
        existing literature you somehow end up to similar solutions
        that others have done before.

        Current b2fxec v0.6a-pre2 implements three algorithms:

        FXP0 - byte aligned LZSS with a 16KB sliding window. The data
        is encoded in a way that allows very fast decompression.

        FXP2 - LZSS with a Huffman backend and a 32KB sliding window.
        Algorithm resembles to LZX but is somewhat simpler. Backend
        Huffman codes are calculated per 32KB block basis and these
        Huffman codes are then further compressed with a secondary
        Huffman coding.

        FXP3 - LZSS with a Huffman backend and a 32KB sliding window. The
        algorithm uses three Huffman trees to represent literals, match
        lengths and offsets. Everything is coded with Huffman. The algorithm
        also utilizes one PMR (previous match reference) and offset caching.
        Backend Huffman codes are calculated per 32KB block basis and these
        Huffman codes are then further transformed using MTF (move to front)
        and finally compressed with a secondary Huffman coding. During
        decompression the peak stack usage is 0x1300 bytes. Apart from the
        stack no additional work memory is needed.
        
        All three algorithms - or actually the runtime decompression routines
        are implementes so that they allow in place decompression.
        
        All three algorithms use the same block based framing (not suitable
        for streamed data) and the LZ string matching algorithm. The
        LZ string matching implements 2 bytes direct hashing, context
        verification and a ring link lists with self learning
        searching cutoff mechanism. LZ string matching also uses lazy
        evaluation in order to find better matches.

        Decompression routines have been programmed in handwritten ARM
        assembler and should be pretty heavily optimized for both speed
        and code size. Code compactness has been favored over speed.

Why own algorithms:

        B2fxec is a "for fun" project. I see no challenge & joy porting
        some existing algoritm or source code. Basically it is a matter of 
        reinventing everything and sometimes the temptation of peeking into 
        other open source compression programs is very high... This approach
        of development also explains why b2fxec does not compete with the
        high end compression programs available these days.

Todo:

        - FXP1 algorithm, which is an enhanced version of FXP2
          including more fine grained offset encoding and PMR
          (Previous Match Reference) encoding. -> Turned out to be FXP3.
        - Add 2 level hashing -> up to 15x speed improvement.
        - Non-greedy matching up to 8 forward steps.
        - Rangecoder instead of Huffman. -> Would be too slow :(

Mambo Jambo:
	
	Have fun. Comments, bug reports etc:
		mouho@iki.fi or meet me in #gp32dev at EfNet.
	
	Check my GP32 & NGPC & GBA & WSC & GC development pages:
		http://www.deadcoderssociety.tk
	
	
	/Jouni aka Mr.Spiv
	
	..in memorian of StoneCracker :)
	
	
	
	And finally some greetings to people at #gp32dev:
	
		_mp_, flavor, groepaz, clemmy, ^yado^, don_migue, rlyeh, aj0,
		trapflag, CMOTD, kojote, giffel, mithris, zardozj, ratboi, trans,
		darkfader, RobBrown, NigelBro, Scoopex,
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		and costis of cos! :)
	
	
