#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./yacc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$expected expected for input $input, but got $actual"
        exit 1
    fi
}

try 0 0
try 42 42
try 12 4+8
try 4 8-4
try 21 5+20-4
try 41 ' 12 + 34 - 5'
try 4 '-1 + 5'
echo OK