yacc: yacc.c

test: yacc
		./test.sh

clean:
	rm -f yacc *.o *~ tmp*