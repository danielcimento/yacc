CFLAGS=-Wall -std=c11 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

yacc: $(OBJS)
	gcc -o yacc $(CFLAGS) $(OBJS)

$(OBJS): yacc.h

test: yacc
	./yacc -test
	./test.sh

clean:
	rm -f yacc *.o *~ tmp* *.out