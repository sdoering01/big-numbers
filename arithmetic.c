#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef struct BigNum {
    // Contigious block of memory that holds our 32-bit ints with little
    // endianess (the least significant int comes first in memory). The size of
    // this block will be exactly 4 * `len` bytes.
    void* data;
    // Amount of 32-bit ints
    size_t len;
} BigNum;

static uint32_t bn_get_block(BigNum *n, size_t offset) {
    if (offset < n->len) {
        uint32_t *int_blocks = n->data;
        return *(int_blocks + offset);
    } else {
        return 0;
    }
}

static void bn_write_block(BigNum *n, size_t offset, uint32_t value) {
    assert(offset < n->len);
    uint32_t *int_blocks = n->data;
    *(int_blocks + offset) = value;
}

BigNum *bn_zero() {
    BigNum *bn = malloc(sizeof(BigNum));
    bn->len = 1;
    bn->data = calloc(1, sizeof(uint32_t));
    return bn;
}

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
        uint32_t *int_blocks = n->data;
        printf("%08x ", *(int_blocks + offset - 1));
    }
    printf("\n");
}

BigNum *bn_add(BigNum *n1, BigNum *n2) {
    size_t greater_len = n1->len > n2->len ? n1->len : n2->len;
    size_t new_len = greater_len + 1;

    BigNum *result = malloc(sizeof(BigNum));
    result->len = new_len;
    result->data = calloc(new_len, sizeof(uint32_t));

    int transfer = 0;
    for (size_t offset = 0; offset < new_len; offset++) {
        uint64_t block_result_64 = 0;
        block_result_64 += transfer;
        block_result_64 += bn_get_block(n1, offset);
        block_result_64 += bn_get_block(n2, offset);

        uint32_t block_result_32 = block_result_64;
        bn_write_block(result, offset, block_result_32);

        transfer = block_result_32 != block_result_64;
    }

    // Trim most significat block if its not needed
    if (bn_get_block(result, result->len - 1) == 0) {
        result->data = realloc(result->data, (result->len - 1) * sizeof(uint32_t));
        result->len -= 1;
    }

    return result;
}

BigNum *bn_multiply(BigNum *n1, BigNum *n2) {
    // New number is at most n1->len + n2->len long. We can trim the result at
    // the end as in `bn_add` (maybe we need to trim more than one block).
}

int main(void) {
    BigNum *n1 = bn_from_uint32_t(0x1);
    bn_print_hex(n1);
    BigNum *n2 = bn_from_uint32_t(0x1);
    bn_print_hex(n2);
    BigNum *result = bn_add(n1, n2);
    bn_print_hex(result);

    free(n1);
    free(n2);
    free(result);
    printf("\n");

    n1 = bn_from_uint32_t(0xffffffff);
    bn_print_hex(n1);
    n2 = bn_from_uint32_t(0xffffffff);
    bn_print_hex(n2);
    result = bn_add(n1, n2);
    bn_print_hex(result);

    return 0;
}
