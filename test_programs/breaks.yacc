a = 4;
b = 0;

while (a < 100) {
    if(a == 20) break;
    a++;
}

b = b + a;

do {
    if(a % 2 == 0) continue;
    b++;
} while (++a < 30);

b;