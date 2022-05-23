#include <stdio.h>
#include "arithmetic.h"

int main(void) {
    printf("This is C\n");
    BigNum *n1 = bn_one();
    BigNum *n2 = bn_one();
    BigNum *result = bn_add(n1, n2);
    printf("  ");
    bn_print_hex(n1);
    printf("+ ");
    bn_print_hex(n2);
    printf("----------\n");
    printf("= ");
    bn_print_hex(result);
    return 0;
}
