#include <iostream>
#include "arithmetic.h"

int main() {
    std::cout << "This is C++" << std::endl;
    BigNum *n1 = bn_one();
    BigNum *n2 = bn_one();
    BigNum *result = bn_add(n1, n2);
    std::cout << "  ";
    bn_print_hex(n1);
    std::cout << "+ ";
    bn_print_hex(n2);
    std::cout << "----------" << std::endl;
    std::cout << "= ";
    bn_print_hex(result);
    return 0;
}
