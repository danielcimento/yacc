CFLAGS=-Wall -std=c11 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

# debug: CFLAGS += -DDEBUG -g
# debug: CCFLAGS += -DDEBUG -g
debug: yacc

yacc: $(OBJS)
	gcc -o yacc $(CFLAGS) $(OBJS)

$(OBJS): yacc.h

test: yacc
	./yacc -test
	./test.sh

clean:
	rm -f yacc *.o *~ tmp* *.out