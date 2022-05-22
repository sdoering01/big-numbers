#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Max value of a block in BigNum
#define BN_BLOCK_MAX 0xffffffff

typedef struct BigNum {
    // Contigious block of memory that holds our 32-bit ints with little
    // endianess (the least significant int comes first in memory). The size of
    // this block will be exactly 4 * `len` bytes.
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

// Returns the block with the `offset` from the start of the BigNum data. It
// accesses memory that doesn't belong to the given BigNum when offset is out
// of bounds.
static inline uint32_t bn_get_block_unchecked(BigNum *n, size_t offset) {
    return *((uint32_t *)n->data + offset);
}

// Returns the block with the `offset` from the start of the BigNum data. It
// returns 0 if the offset is out of bounds.
static uint32_t bn_get_block(BigNum *n, size_t offset) {
    if (offset < n->len) {
        return bn_get_block_unchecked(n, offset);
    } else {
        return 0;
    }
}

// Writes the given `value` to the block with the given `offset`. This function
// asserts that `offset` is in bounds.
static inline void bn_write_block(BigNum *n, size_t offset, uint32_t value) {
    assert(offset < n->len);
    *((uint32_t *)n->data + offset) = value;
}

// Trims all leading 0-blocks of `n`. This will trim `n` at most to len 1.
static void bn_trim(BigNum *n) {
    size_t trimmed_len = n->len;
    uint32_t *block_ptr = (uint32_t *)n->data + n->len - 1;
    while (trimmed_len > 1 && *block_ptr == 0) {
        trimmed_len--;
        block_ptr--;
    }
    if (trimmed_len < n->len) {
        n->data = realloc(n->data, trimmed_len * sizeof(uint32_t));
        n->len = trimmed_len;
    }
}

// Adds the `value` to the big number `n` with the `offset`.
//
// The lower half of `value` is added to the block at `offset`. The overflow of
// this addition is then accumulated with the higher half of `value` and added
// to the next block of `n`. This continues until no overflow occurs when
// adding the `value` to the block of `n`. The caller needs to make sure that
// `n` is large enough to hold the result.
static void bn_add_block_cascading_unchecked(BigNum *n, size_t offset, uint64_t value) {
    if (value > 0) {
        uint32_t lower_half = value;
        uint32_t upper_half = value >> 32;

        uint32_t block = bn_get_block_unchecked(n, offset);
        // Cast to uint64_t necessary since both operands are uint32_t, but we
        // don't want to lose the overflow.
        uint64_t block_result_64 = (uint64_t)lower_half + block;
        uint32_t block_result_32 = block_result_64;
        bn_write_block(n, offset, block_result_32);

        uint32_t block_overflow = block_result_64 >> 32;
        uint64_t overflow = upper_half + block_overflow;
        // Recursion stops when overflow is 0
        bn_add_block_cascading_unchecked(n, offset + 1, overflow);
    }
}

// Returns a pointer to a BigNum with the given `len`.
static BigNum *bn_with_len(size_t len) {
    BigNum *bn = malloc(sizeof(BigNum));
    bn->len = len;
    bn->data = calloc(len, sizeof(uint32_t));
    return bn;
}

// Destroy `n`, freeing all its allocated heap memory and setting `*n` to NULL.
void bn_destroy(BigNum **n) {
    free((*n)->data);
    free(*n);
    *n = NULL;
}

// Returns a pointer to a BigNum representing 0.
BigNum *bn_zero() {
    BigNum *bn = malloc(sizeof(BigNum));
    bn->len = 1;
    bn->data = calloc(1, sizeof(uint32_t));
    return bn;
}

// Returns a pointer to a BigNum representing the given `n`.
BigNum *bn_from_uint32_t(uint32_t n) {
    BigNum *bn = bn_zero();
    uint32_t *data_ptr = bn->data;
    *data_ptr = n;
    return bn;
}

// This prints the BigNum in Big Endian representation to be more
// human-readable.
void bn_print_hex(BigNum *n) {
    // size_t can't be negative, this we can't use offset >= 0 as condition.
    // Because of that we count from `n->len` to 1 and use `offset - 1`.
    for (size_t offset = n->len; offset > 0; offset--) {
        printf("%08x ", *((uint32_t *)n->data + offset - 1));
    }
    printf("\n");
}

// Compares the big numbers `n1` and `n2`. Returns a positive result, if `n1`
// is greater than `n2`. Returns a negative result, if `n1` is less than `n2`.
// Returns 0, if `n1` is equal to `n2`.
int bn_compare(BigNum *n1, BigNum *n2) {
    if (n1->len > n2->len) {
        return 1;
    } else if (n1->len < n2->len) {
        return -1;
    } else {
        // Compare blocks, starting at the most significant block
        for (size_t offset = n1->len; offset > 0; offset--) {
            uint32_t n1_block = bn_get_block_unchecked(n1, offset - 1);
            uint32_t n2_block = bn_get_block_unchecked(n2, offset - 1);
            if (n1_block > n2_block) {
                return 1;
            } else if (n1_block < n2_block) {
                return -1;
            }
        }

        return 0;
    }
}

// Returns whether `n1` is greater than `n2`.
int bn_greater_than(BigNum *n1, BigNum *n2) {
    return bn_compare(n1, n2) > 0;
}

// Returns whether `n1` is less than `n2`.
int bn_less_than(BigNum *n1, BigNum *n2) {
    return bn_compare(n1, n2) < 0;
}

// Returns whether `n1` is equal to `n2`.
int bn_equal_to(BigNum *n1, BigNum *n2) {
    return bn_compare(n1, n2) == 0;
}

BigNum *bn_add(BigNum *n1, BigNum *n2) {
    size_t greater_len = n1->len > n2->len ? n1->len : n2->len;
    size_t result_len = greater_len + 1;

    BigNum *result = bn_with_len(result_len);

    int transfer = 0;
    for (size_t offset = 0; offset < result_len; offset++) {
        uint64_t block_result_64 = 0;
        block_result_64 += transfer;
        block_result_64 += bn_get_block(n1, offset);
        block_result_64 += bn_get_block(n2, offset);

        uint32_t block_result_32 = block_result_64;
        bn_write_block(result, offset, block_result_32);

        transfer = block_result_32 != block_result_64;
    }

    bn_trim(result);

    return result;
}

// Subtracts `n2` from `n1`. This function asserts that `n1` is greater than
// or equal to `n2`.
BigNum *bn_subtract(BigNum *n1, BigNum *n2) {
    assert(bn_compare(n1, n2) >= 0);

    // n1 >= n2, so the result will be at most n1->len long
    BigNum *result = bn_with_len(n1->len);

    uint32_t transfer = 0;
    for (size_t offset = 0; offset < n1->len; offset++) {
        uint32_t n1_block = bn_get_block_unchecked(n1, offset);
        uint32_t n2_block = bn_get_block(n2, offset);

        if (transfer) {
            n2_block += transfer;
            // Only reset transfer, if the addition of the transfer did not
            // cause an overflow
            if (n2_block) {
                transfer = 0;
            }
        }

        uint32_t block_diff;
        if (n1_block >= n2_block) {
            block_diff = n1_block - n2_block;
        } else {
            // n2_block is greater than n1_block, so n1_block + 1 - n2_block is
            // <= 0. Meaning that this expression will never overflow.
            block_diff = BN_BLOCK_MAX - n2_block + 1 + n1_block;
            transfer = 1;
        }
        bn_write_block(result, offset, block_diff);
    }

    bn_trim(result);

    return result;
}

BigNum *bn_multiply(BigNum *n1, BigNum *n2) {
    // New number is at most n1->len + n2->len long. We can trim the result at
    // the end as in `bn_add` (maybe we need to trim more than one block).
    size_t result_len = n1->len + n2->len;

    BigNum *result = bn_with_len(result_len);

    for (size_t n1_offset = 0; n1_offset < n1->len; n1_offset++) {
        for (size_t n2_offset = 0; n2_offset < n2->len; n2_offset++) {
            size_t result_offset = n1_offset + n2_offset;
            uint32_t n1_block = bn_get_block_unchecked(n1, n1_offset);
            uint32_t n2_block = bn_get_block_unchecked(n2, n2_offset);
            // Cast to uint64_t necessary since both operands are uint32_t, but
            // we don't want to lose the overflow.
            uint64_t block_result = (uint64_t)n1_block * n2_block;
            bn_add_block_cascading_unchecked(result, result_offset, block_result);
        }
    }

    bn_trim(result);

    return result;
}

// Returns a struct holding the quotient and the remainder of the division `n1`
// / `n2`. If you are only interested in one of the two, you may use
// `bn_divide` or `bn_mod` respectively.
bn_DivideWithRemainderResult *bn_divide_with_remainder(BigNum *n1, BigNum *n2) {
    // Catch division by zero
    assert(!(n2->len == 1 && *(uint32_t *)n2->data == 0));

    // Quotient is at most n1->len - (n2->len - 1) long. Let's asume we have
    // two big numbers n1 = 0xffffffff 0xffffffff 0xffffffff (len=3) and n2 = 1
    // 0 (len=2). Dividing n1 by n2 merely acts as a shift right operation of
    // one block on n1, even though n2's length is 2. We have to trim the
    // quotient at the end, because the example shows only the most extreme
    // case. In other cases the length of the quotient might end up smaller.
    size_t quotient_len = n1->len - (n2->len - 1);

    BigNum *quotient = bn_with_len(quotient_len);
    BigNum *remainder = bn_zero();

    BigNum *block_quotient = bn_zero();

    for (size_t _offset = n1->len; _offset > 0; _offset--) {
        size_t offset = _offset - 1;
        // Shift left remainder by one block and set least significant block of
        // remainder the block of n1 at offset
        remainder->len += 1;
        remainder->data = realloc(remainder->data, remainder->len * sizeof(uint32_t));
        memmove(
            (uint32_t *)remainder->data + 1,
            (uint32_t *)remainder->data,
            (remainder->len - 1) * sizeof(uint32_t)
        );
        *(uint32_t *)remainder->data = *((uint32_t *)n1->data + offset);
        // Necessary since left-shifting a 0 leads to a leading 0.
        bn_trim(remainder);

        // Check if remainder can be divided by n2
        if (bn_compare(remainder, n2) >= 0) {
            // Efficiently search for the block_quotient (remainder / n2).
            *(uint32_t *)block_quotient->data = 0;
            for (int bit_offset = 31; bit_offset >= 0; bit_offset--) {
                *(uint32_t *)block_quotient->data |= (1 << bit_offset);
                BigNum *product = bn_multiply(block_quotient, n2);
                // flip the bit back to 0 if the block_quotient is too large
                if (bn_greater_than(product, remainder)) {
                    *(uint32_t *)block_quotient->data ^= (1 << bit_offset);
                }
                bn_destroy(&product);
            }
            BigNum *product = bn_multiply(block_quotient, n2);

            // Could write a function that subtracts the second operand from
            // the first, and saving the result in the first operand. But this
            // should be fine to start with.
            BigNum *new_remainder = bn_subtract(remainder, product);
            bn_destroy(&remainder);
            remainder = new_remainder;
            bn_write_block(quotient, offset, *(uint32_t *)block_quotient->data);

            bn_destroy(&product);
        }
    }

    bn_destroy(&block_quotient);

    bn_trim(quotient);
    bn_trim(remainder);

    bn_DivideWithRemainderResult *result = malloc(sizeof(bn_DivideWithRemainderResult));
    result->quotient = quotient;
    result->remainder = remainder;
    return result;
}

// Returns the result of the division `n1` / `n2`, ignoring the remainder. If
// you also need the remainder, you may use `bn_divide_with_remainder`.
BigNum *bn_divide(BigNum *n1, BigNum *n2) {
    bn_DivideWithRemainderResult *result = bn_divide_with_remainder(n1, n2);
    BigNum *quotient = result->quotient;
    bn_destroy(&result->remainder);
    free(result);
    return quotient;
}

// Returns the remainder of the division `n1` / `n2`.
BigNum *bn_mod(BigNum *n1, BigNum *n2) {
    bn_DivideWithRemainderResult *result = bn_divide_with_remainder(n1, n2);
    BigNum *remainder = result->remainder;
    bn_destroy(&result->quotient);
    free(result);
    return remainder;
}

int main(void) {
    printf("Adding\n");
    BigNum *n1 = bn_from_uint32_t(0x1);
    bn_print_hex(n1);
    BigNum *n2 = bn_from_uint32_t(0x1);
    bn_print_hex(n2);
    BigNum *result = bn_add(n1, n2);
    bn_print_hex(result);

    bn_destroy(&n1);
    bn_destroy(&n2);
    bn_destroy(&result);
    printf("\n");

    printf("Adding\n");
    n1 = bn_from_uint32_t(0xffffffff);
    bn_print_hex(n1);
    n2 = bn_from_uint32_t(0xffffffff);
    bn_print_hex(n2);
    result = bn_add(n1, n2);
    bn_print_hex(result);

    printf(
        "n1 == n2: %d, n1 < n2: %d, n1 > n2: %d",
        bn_equal_to(n1, n2),
        bn_less_than(n1, n2),
        bn_greater_than(n1, n2)
    );

    bn_destroy(&result);
    bn_destroy(&n1);
    bn_destroy(&n2);
    printf("\n");

    printf("Multiplying\n");
    n1 = bn_from_uint32_t(2);
    bn_print_hex(n1);
    n2 = bn_from_uint32_t(2);
    bn_print_hex(n2);
    result = bn_multiply(n1, n2);
    bn_print_hex(result);

    printf("\n");

    n1 = bn_from_uint32_t(0xffffffff);
    // Results can be checked with the bc utility and `obase=16; ibase=16`
    bn_print_hex(n1);

    result = bn_multiply(n1, n1);
    printf("0xffffffff ^ 2: ");
    bn_print_hex(result);

    BigNum *result_2 = bn_multiply(result, result);
    printf("0xffffffff ^ 4: ");
    bn_print_hex(result_2);

    BigNum *result_3 = bn_multiply(result_2, result);
    printf("0xffffffff ^ 6: ");
    bn_print_hex(result_3);

    BigNum *result_4 = bn_multiply(result_2, result_2);
    printf("0xffffffff ^ 8: ");
    bn_print_hex(result_4);

    assert(bn_equal_to(result_4, result_4));
    assert(bn_less_than(result_3, result_4));
    assert(bn_greater_than(result_3, result_2));

    BigNum *result_diff_4_3 = bn_subtract(result_4, result_3);
    printf("0xffffffff ^ 8 - 0xffffffff ^ 6: ");
    bn_print_hex(result_diff_4_3);

    bn_destroy(&n1);
    bn_destroy(&n2);
    printf("\n");

    n1 = bn_with_len(5);
    bn_write_block(n1, 4, 0xffff);
    bn_write_block(n1, 3, 0xfedc);
    bn_write_block(n1, 0, 0xffff);
    n2 = bn_with_len(4);
    bn_write_block(n2, 3, 1);
    bn_DivideWithRemainderResult *divide_result = bn_divide_with_remainder(n1, n2);
    printf("Dividend: ");
    bn_print_hex(n1);
    printf("Divisor: ");
    bn_print_hex(n2);
    printf("Quotient: ");
    bn_print_hex(divide_result->quotient);
    printf("Remainder: ");
    bn_print_hex(divide_result->remainder);

    BigNum *quotient = bn_divide(n1, n2);
    printf("Quotient (bn_divide): ");
    bn_print_hex(quotient);
    BigNum *mod = bn_mod(n1, n2);
    printf("Remainder (bn_mod): ");
    bn_print_hex(mod);

    return 0;
}
