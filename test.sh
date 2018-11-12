#!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./yacc "$input" > tmp.s
    gcc -o tmp tmp.s || exit 1
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$expected expected for input $input, but got $actual"
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

echo "OK"