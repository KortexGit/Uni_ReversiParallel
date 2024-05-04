
SUFFIXES = .c .o .obj .lo .a

MY_DEPS =  multiprocessor.o mailbox.o paro64bit.o

OPT=-O0 -g

all-local:  reversi$(EXEEXT)

reversi$(EXEEXT): $(MY_DEPS)
	gcc $(MY_DEPS) -o $@ -lpthread

sequential-reversi$(EXEEXT):
	gcc -DSEQUENTIAL -o $@

install-exec-local:  reversi$(EXEEXT)
	install -m 755 reversi$(EXEEXT) $(DESTDIR)$(prefix)/bin

force:
