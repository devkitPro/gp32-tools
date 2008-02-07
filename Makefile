SUBDIRS:= `ls | egrep -v '^(CVS)$$'`
all:
	@for i in $(SUBDIRS); do if test -d $$i; then $(MAKE)  -C $$i || { exit 1;} fi; done;
clean:
	@for i in $(SUBDIRS); do if test -d $$i; then $(MAKE)  -C $$i clean || { exit 1;} fi; done;
install:
	@for i in $(SUBDIRS); do if test -d $$i; then $(MAKE)  -C $$i install || { exit 1;} fi; done;
