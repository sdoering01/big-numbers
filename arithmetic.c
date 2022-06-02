#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "arithmetic.h"

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

void bn_destroy(BigNum **n) {
    free((*n)->data);
    free(*n);
    *n = NULL;
}

BigNum *bn_copy(BigNum *orig) {
    if (!orig) {
        return NULL;
    }
    BigNum *copy = malloc(sizeof(BigNum));
    copy->len = orig->len;
    copy->data = malloc(copy->len * sizeof(uint32_t));
    memcpy(copy->data, orig->data, copy->len * sizeof(uint32_t));
    return copy;
}

BigNum *bn_zero() {
    BigNum *bn = malloc(sizeof(BigNum));
    bn->len = 1;
    bn->data = calloc(1, sizeof(uint32_t));
    return bn;
}

BigNum *bn_one() {
    BigNum *bn = malloc(sizeof(BigNum));
    bn->len = 1;
    bn->data = malloc(sizeof(uint32_t));
    *((uint32_t *)bn->data) = 1;
    return bn;
}

BigNum *bn_from_uint32_t(uint32_t n) {
    BigNum *bn = bn_zero();
    uint32_t *data_ptr = bn->data;
    *data_ptr = n;
    return bn;
}

BigNum *bn_from_hex(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t num_chars = 0;
    size_t num_hex_chars = 0;
    for (const char *c = str; *c; c++) {
        if ((*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'f') || (*c >= 'A' && *c <= 'F')) {
            num_hex_chars++;
        } else if (*c != ' ') {
            fprintf(stderr, "warning in bn_from_hex: found invalid hex char '%c' at pos %zu\n", *c, num_chars);
            return NULL;
        }
        num_chars++;
    }

    if (num_hex_chars == 0) {
        return NULL;
    }

    size_t len = (num_hex_chars + 7) / 8;
    BigNum *result = bn_with_len(len);

    uint32_t block = 0;
    int hex_chars_read = 0;
    size_t offset = 0;

    for (const char *c = str + num_chars - 1; c >= str; c--) {
        if (*c != ' ') {
            uint32_t c_val;
            if (*c >= '0' && *c <= '9') {
                c_val =  *c - '0';
            } else if (*c >= 'a' && *c <= 'f') {
                c_val =  10 + *c - 'a';
            } else {
                c_val = 10 + *c - 'A';
            }
            block += (c_val << (4 * hex_chars_read));
            hex_chars_read++;

            if (hex_chars_read == 8) {
                bn_write_block(result, offset, block);
                block = 0;
                hex_chars_read = 0;
                offset++;
            }
        }
    }

    if (hex_chars_read) {
        bn_write_block(result, offset, block);
    }

    bn_trim(result);

    return result;
}

void bn_print_hex(BigNum *n) {
    // size_t can't be negative, this we can't use offset >= 0 as condition.
    // Because of that we count from `n->len` to 1 and use `offset - 1`.
    for (size_t offset = n->len; offset > 0; offset--) {
        printf("%08x ", *((uint32_t *)n->data + offset - 1));
    }
    printf("\n");
}

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

int bn_greater_than(BigNum *n1, BigNum *n2) {
    return bn_compare(n1, n2) > 0;
}

int bn_less_than(BigNum *n1, BigNum *n2) {
    return bn_compare(n1, n2) < 0;
}

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

BigNum *bn_subtract(BigNum *n1, BigNum *n2) {
    if (bn_greater_than(n2, n1)) {
        return NULL;
    }

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

bn_DivideWithRemainderResult *bn_divide_with_remainder(BigNum *n1, BigNum *n2) {
    // Catch division by zero
    if (n2->len == 1 && *(uint32_t *)n2->data == 0) {
        return NULL;
    }

    // Quotient is at most n1->len - (n2->len - 1) long if n1->len > n2->len.
    // Let's asume we have two big numbers n1 = 0xffffffff 0xffffffff
    // 0xffffffff (len=3) and n2 = 1 0 (len=2). Dividing n1 by n2 merely acts
    // as a shift right operation of one block on n1, even though n2's length
    // is 2. We have to trim the quotient at the end, because the example shows
    // only the most extreme case. In other cases the length of the quotient
    // might end up smaller. When n1->len <= n2->len the result will be only
    // one block long.
    size_t quotient_len;
    if (n1->len > n2->len) {
        quotient_len = n1->len - (n2->len - 1);
    } else {
        quotient_len = 1;
    }

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

BigNum *bn_divide(BigNum *n1, BigNum *n2) {
    bn_DivideWithRemainderResult *result = bn_divide_with_remainder(n1, n2);
    BigNum *quotient = NULL;
    if (result) {
        quotient = result->quotient;
        bn_destroy(&result->remainder);
        free(result);
    }
    return quotient;
}

BigNum *bn_mod(BigNum *n1, BigNum *n2) {
    bn_DivideWithRemainderResult *result = bn_divide_with_remainder(n1, n2);
    BigNum *remainder = NULL;
    if (result) {
        remainder = result->remainder;
        bn_destroy(&result->quotient);
        free(result);
    }
    return remainder;
}

BigNum *bn_power_mod(BigNum *base, BigNum *exp, BigNum *mod) {
    if (mod->len == 1 && *((uint32_t *)mod->data) == 0) {
        return NULL;
    }

    BigNum *result = bn_one();

    int search_start = 1;
    for (size_t _exp_offset = exp->len; _exp_offset > 0; _exp_offset--) {
        size_t exp_offset = _exp_offset - 1;
        uint32_t exp_block = *((uint32_t *)exp->data + exp_offset);
        for (int exp_bit_offset = 31; exp_bit_offset >= 0; exp_bit_offset--) {
            int bit = exp_block & (1 << exp_bit_offset);
            if (search_start) {
                if (bit) {
                    search_start = 0;
                } else {
                    continue;
                }
            }

            BigNum *new_result = bn_multiply(result, result);
            bn_destroy(&result);
            result = new_result;

            if (bit) {
                BigNum *new_result = bn_multiply(result, base);
                bn_destroy(&result);
                result = new_result;
            }

            new_result = bn_mod(result, mod);
            bn_destroy(&result);
            result = new_result;
        }
    }

    bn_trim(result);

    return result;
}
