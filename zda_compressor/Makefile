CFLAGS	:=	-Wall -O2 -s

ifneq (,$(findstring MINGW,$(shell uname -s)))
	exeext		:= .exe
endif

ifneq (,$(findstring Linux,$(shell uname -s)))
	CFLAGS += -static
endif

cfiles	:=	$(wildcard *.c)
tools	:=$(patsubst %.c,%$(exeext),$(wildcard *.c))

all:	$(tools)

clean:
	@rm $(tools)

%$(exeext)	:: %.c	
	$(CC) $< -o $@ $(CFLAGS) -lz

install:
	cp  $(tools) $(PREFIX)
