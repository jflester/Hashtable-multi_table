CC = gcc
CFLAGS = -ansi -pedantic-errors -Wall -Werror -g
PROGS = hashtester public00 public01 public02 \
	public03 public04 public05 testfile00 testfile01 \
	testfile03

.PHONY: all clean

all: $(PROGS)

clean:
	mv hashtester.o hashtester.ext
	mv htester htester.ext
	rm -f *.o $(PROGS)
	mv hashtester.ext hashtester.o
	mv htester.ext htester

$(PROGS): hashtable.o
hashtester: hashtester.o
public%: public%.o
testfile%: testfile%.o

hashtester.o: hashtable.h
hashtable.o: hashtable.h hashtable.c
public%.o: hashtable.h
testfile%.o: hashtable.h