#ifndef BN_ARITHMETIC_H_
#define BN_ARITHMETIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Max value of a block in BigNum
#define BN_BLOCK_MAX 0xffffffff


// Unsigned big number. Do not mutate this directly but use the provided
// functions.
typedef struct BigNum {
    // Contiguous block of memory that holds our 32-bit ints with little
    // endianness (the least significant int comes first in memory). The size
    // of this block will be exactly 4 * `len` bytes.
    void* data;
    // Amount of 32-bit ints
    size_t len;
} BigNum;

typedef struct bn_DivideWithRemainderResult {
    // Quotient of the division
    BigNum *quotient;
    // Remainder of the division
    BigNum *remainder;
} bn_DivideWithRemainderResult;


// Destroys `n`, freeing all its allocated heap memory and setting `*n` to
// NULL.
void destroy(BigNum **n);

// Returns a pointer to a newly created big number representing 0.
BigNum *bn_zero();

// Returns a pointer to a newly created big number representing 1.
BigNum *bn_one();

// Returns a pointer to a newly created big number representing `n`.
BigNum *bn_from_uint32_t();

// Converts a hex string to a big number. The string may contain all valid hex
// chars (0-9, a-f, A-F). Spaces are ignored. This function returns a null
// pointer, if a null pointer or an invalid string is provided.
BigNum *bn_from_hex();

// Prints `n` in big Endian representation to be more human-readable.
void bn_print_hex(BigNum *n);

// Compares the big numbers `n1` and `n2`. Returns a positive result, if `n1`
// is greater than `n2`. Returns a negative result, if `n1` is less than `n2`.
// Returns 0, if `n1` is equal to `n2`.
int bn_compare(BigNum *n1, BigNum *n2);

// Returns whether `n1` is greater than `n2`.
int bn_greater_than(BigNum *n1, BigNum *n2);

// Returns whether `n1` is less than `n2`.
int bn_less_than(BigNum *n1, BigNum *n2);

// Returns whether `n1` is equal to `n2`.
int bn_equal_to(BigNum *n1, BigNum *n2);

// Returns the result of the addition `n1` + `n2` as a new big number.
BigNum *bn_add(BigNum *n1, BigNum *n2);

// Returns the result of the subtraction `n2` - `n1` as a new big number. This
// function asserts that `n1` is greater than or equal to `n2`.
BigNum *bn_subtract(BigNum *n1, BigNum *n2);

// Returns the result of the multiplication `n1` * `n2` as a new big number.
BigNum *bn_multiply(BigNum *n1, BigNum *n2);

// Returns the quotient and the remainder of the division `n1` / `n2`. If you
// are only interested in one of the two, you may use `bn_divide` or `bn_mod`
// respectively.
bn_DivideWithRemainderResult *bn_divide_with_remainder(BigNum *n1, BigNum *n2);

// Returns the quotient of the division `n1` / `n2` as a new big number,
// ignoring the remainder. If you also need the remainder, you may use
// `bn_divide_with_remainder`.
BigNum *bn_divide(BigNum *n1, BigNum *n2);

// Returns the remainder of the division `n1` / `n2` as a new big number.
BigNum *bn_mod(BigNum *n1, BigNum *n2);

// Returns the result of the modular exponentiation (`base` ^ `exp`) % `mod` as
// a new big number. This function uses the square and multiply algorithm.
BigNum *bn_power_mod(BigNum *base, BigNum *exp, BigNum *mod);

#ifdef __cplusplus
}
#endif

#endif // BN_ARITHMETIC_H_
