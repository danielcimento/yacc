#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "You did not provide the correct number of arguments! (Expected 1)\n");
        return 1;
    }

    char *p = argv[1];

    // Preliminary Headers
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    
    // Read out the first character as a 10 bit number
    printf("\tmov rax, %ld\n", strtol(p, &p, 10));

    while (*p) {
        // Handle addition instructions
        if (*p == '+') {
            p++;
            // Note: we don't need p++ afterwards, as strtol will update p 
            // with the start of the string after the number
            printf("\tadd rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        // Handle subtraction instructions
        if(*p == '-') {
            p++;
            printf("\tsub rax, %ld\n", strtol(p, &p, 10));
            continue;
        }

        fprintf(stderr, "Unexpected character: '%c' (Character code: %d)\n", *p, *p);
        return 1;
    }

    printf("\tret\n");
    return 0;
}