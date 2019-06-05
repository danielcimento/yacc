a = 4;
init: b = 3;

while(b-- > 0) {
    a++;
    if(a * b == 10) goto init;
    if(a * a > 40) goto eol;
}

eol: a;