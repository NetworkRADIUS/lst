#
# -O2 is necessary, in gcc or in clang up to version 13, to
# get the compiler to do the tail call optimization that
# the code is written to take advantage of (and to be as
# clearly related to the pseudocode in the "Stronger Quickheaps"
# paper as possible).
#
CFLAGS = -O2 -Wall -Werror

all: lst_tests liblst.so

lst_tests: lst_tests.c lst.c lst.h
	$(CC) $(CFLAGS) -g -o lst_tests lst_tests.c

liblst.so: lst.c lst.h
	$(CC) -c $(CFLAGS) -fpic lst.c
	$(CC) -shared -o liblst.so lst.o

clean:
	rm  -f lst_tests liblst.so lst.o
