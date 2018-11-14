#!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./yacc -l "$input" > tmp.s
    gcc -o tmp tmp.s || exit 1
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$expected expected for input $input, but got $actual"
        exit 1
    fi
}

try_file() {
    expected="$1"
    file_name="$2"

    ./yacc "$file_name" > tmp.s
    gcc -o tmp tmp.s || exit 1
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$expected expected for file $file_name, but got $actual"
        exit 1
    fi
}

# Case 1: Numbers
try 0 '0;'
try 42 '42;'

# Case 2: Addition and Subtraction
try 12 '4+8;'
try 4 '8-4;'
try 21 '5+20-4;'
try 41 ' 12 + 34 - 5 ;'
try 4 '-1 + 5 ; '

# Case 3: Multiplication and Division
try 47 "5+6*7 ; "

# Case 4: Parentheses
try 20 '(3 + 2) * 4; '
try 18 '3 * ((2*2) + 2); '
try 50 '(5 * 1) * ((2 + 4) + (2 * (1+1))); '
try 15 "5*(9-6);"
try 4 "(3+5)/2;"

# Case 5: Variables
try 12 "a = 1; b = 2; c = 4; (a + b) * c;"

# Case 6: Equality
try 1 "3 == 3;"
try 0 "3 == 2;"
try 1 "3 != 2;"
try 0 "3 != 3;"
try 1 "a = 4; b = 4; a == b;"
try 0 "a = b = 5; a != b;"
try 2 "(1 != 2) + (12 == 12);"
try 5 "a = b = 5; c = a == b; c * 5;"
try 1 "5 == 3 + 2;"
try 1 "2 + 3 == 5;"

# Case 7: Multicharacter Variables
try 12 "foo = 1; bar = 2; buzz=4;(foo+bar)*buzz;"
try 12 "foo13 = 1; bar12 = 2; bu_zz=4;(foo13+bar12)*bu_zz;"

# Case 8: Comparison Operators
try 10 "foo = 10; bar = 3; buzz = 4; (bar < buzz) * foo;"
try 12 "foo = 12; bar = 5; buzz = 4; (bar > buzz) * foo;"
try 5 "foo = 10; bar = 5; buzz = 4; (bar <= buzz) * foo + bar;"
try 1 "10 >= 10;"
try 0 "12 > 12;"
try 1 "10 <= 10;"
try 0 "10 < 10;"

# Case 9: Unary operators
try 5 "3 - -2;"
try 6 "+2 + +4;"
try 255 "~0;"
try 0 "~255;"
try 0 "foo = 5; !foo;"
try 1 "foo = 12; foo = foo - foo; !foo;"

# Case 10: Modulus
try 3 "8 % 5;"
try 0 "3 % 3;"
try 0 "6 % 3;"

# Case 11: Increment/Decrement
try 3 "i = 2; ++i;"
try 2 "i = 2; i++;"
try 3 "i = 2; i++; i;"
try 2 "i = 3; --i;"
try 3 "i = 3; i--;"
try 2 "i = 3; i--; i;"

# Case 12: Ternary conditionals
try 3 "10 ? 3: 5;"
try 5 "0 ? 3 : 5;"
try 6 "3 ? 5 ? 6 : 4 : 2;"
try 4 "3 ? 0 ? 6 : 4 : 2;"
try 2 "foo = 10 ? 3 : 5; --foo;"

# Case 13: Scopes
try 12 "a = 1; {b = 2; a = a + b;} c = 4; a * c;"
try 15 "a = 3; { b = 2; { c = 3; { (c + b) * a; } } }"

# Case 14: Control Flow
try 5 "a = 3; if(a > 2) { a++; } else { a--; } if(a > 3) { a++; } a;"
try 5 "a = 3; if(a > 2) a++; else a--; if(a > 3) a++; a;"

# Case 15: Comments
try_file 5 "test_programs/comments.yacc"

# Case 16: Hex and Octal numbers
try 15 "017;"
try 15 "0xf;"
try 15 "0b1111;"
try 248 "0370;"
try 248 "0xf8;"
try 248 "0b11111000;"

# Case 17: While loops
try_file 205 "test_programs/while_loops.yacc"
try 10 "a = 3; while(a < 10) a++; a;"
try 10 "a = 3; while(++a < 10); a;"

# Case 18: Do-while loops
try_file 205 "test_programs/do_while_loops.yacc"
try 10 "a = 3; do a++; while(a < 10); a;"
try 10 "a = 3; do; while(++a < 10); a;"

echo "OK"