CFLAGS	:=	-Wall -O2 -s -I/usr/local/include

ifneq (,$(findstring MINGW,$(shell uname -s)))
	exeext		:= .exe
endif

ifneq (,$(findstring Linux,$(shell uname -s)))
	CFLAGS += -static
endif

ifneq (,$(findstring Darwin,$(shell uname -s)))
	CFLAGS += -mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc
endif

cfiles	:=	$(wildcard *.c)
tools	:=$(patsubst %.c,%$(exeext),$(wildcard *.c))

all:	$(tools)

clean:
	@rm -f $(tools)

%$(exeext)	:: %.c
	$(CC) $< -o $@ $(CFLAGS) -L/usr/local/lib -lz

install:
	cp  $(tools) $(PREFIX)
