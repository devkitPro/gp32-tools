CFLAGS	:=	-Wall -O2 -s -I/usr/local/include

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
	@rm -f $(tools)

%$(exeext)	:: %.c
	$(CC) $< -o $@ $(CFLAGS) -L/usr/local/lib -lz

install:
	cp  $(tools) $(PREFIX)
