CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

yacc: $(OBJS)
	gcc -o yacc $(CFLAGS) $(OBJS)

$(OBJS): yacc.h

test: yacc
	./test.sh

clean:
	rm -f yacc *.o *~ tmp* *.out