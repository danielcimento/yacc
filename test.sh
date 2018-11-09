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

echo OK