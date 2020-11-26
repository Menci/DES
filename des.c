#include "des.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define DES_BLOCK_SIZE 64
#define DES_ITERATIONS 16

#define DES_F_ITERATIONS            8
#define DES_S_BOX_INPUT_GROUP_SIZE  6
#define DES_S_BOX_OUTPUT_GROUP_SIZE 4
#define DES_S_BOX_COUNT             DES_F_ITERATIONS
#define DES_S_BOX_ROWS              4
#define DES_S_BOX_COLUMNS           16
#define DES_PC1_HALF_KEY_LENGTH     28

typedef uint64_t des_block_t;
typedef uint32_t des_half_block_t;
typedef uint64_t des_key_t;
typedef uint32_t des_half_key_t;
typedef size_t des_transposition_table_t[DES_BLOCK_SIZE];
typedef bool des_bit_t;

static const des_transposition_table_t des_initial_transposition_table = {
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7,
    56, 48, 40, 32, 24, 16, 8, 0, 58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6
};

static des_transposition_table_t des_final_transposition_table;

static const des_transposition_table_t des_expansion_transposition_table = {
    31, 0, 1, 2, 3, 4, 3, 4, 5, 6, 7, 8, 7, 8, 9, 10,
    11, 12, 11, 12, 13, 14, 15, 16, 15, 16, 17, 18, 19, 20, 19, 20,
    21, 22, 23, 24, 23, 24, 25, 26, 27, 28, 27, 28, 29, 30, 31, 0
};

static const des_transposition_table_t des_permutation_transposition_table = {
    15, 6, 19, 20, 28, 11, 27, 16, 0, 14, 22, 25, 4, 17, 30, 9,
    1, 7, 23, 13, 31, 26, 2, 8, 18, 12, 29, 5, 21, 10, 3, 24
};

static const size_t des_substitution_boxes[DES_S_BOX_COUNT][DES_S_BOX_ROWS][DES_S_BOX_COLUMNS] = {
    {
        { 14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7 },
        { 0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8 },
        { 4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0 },
        { 15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13 }
    }, {
        { 15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10 },
        { 3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5 },
        { 0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15 },
        { 13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9 }
    }, {
        { 10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8 },
        { 13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1 },
        { 13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7 },
        { 1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12 }
    }, {
        { 7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15 },
        { 13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9 },
        { 10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4 },
        { 3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14 }
    }, {
        { 2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9 },
        { 14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6 },
        { 4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14 },
        { 11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3 }
    }, {
        { 12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11 },
        { 10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8 },
        { 9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6 },
        { 4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13 }
    }, {
        { 4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1 },
        { 13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6 },
        { 1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2 },
        { 6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12 }
    }, {
        { 13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7 },
        { 1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2 },
        { 7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8 },
        { 2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11 }
    }
};

static const des_transposition_table_t des_pc1_l_transposition_table = {
    56, 48, 40, 32, 24, 16, 8, 0, 57, 49, 41, 33, 25, 17,
    9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35
};

static const des_transposition_table_t des_pc1_r_transposition_table = {
    62, 54, 46, 38, 30, 22, 14, 6, 61, 53, 45, 37, 29, 21,
    13, 5, 60, 52, 44, 36, 28, 20, 12, 4, 27, 19, 11, 3
};

static const des_transposition_table_t des_pc2_transposition_table = {
    13, 16, 10, 23, 0, 4, 2, 27, 14, 5, 20, 9,
    22, 18, 11, 3, 25, 7, 15, 6, 26, 19, 12, 1,
    40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47,
    43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31
};

static const des_transposition_table_t des_shift_transposition_table = {
    27, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26
};

static const size_t des_shift_count[] = {
    1, 1, 2, 2, 2, 2, 2, 2,
    1, 2, 2, 2, 2, 2, 2, 1
};

void des_init(void) {
    for (size_t i = 0; i < DES_BLOCK_SIZE; i++) {
        size_t j = des_initial_transposition_table[i];
        des_final_transposition_table[j] = i;
    }
}

static des_block_t des_setbit(des_block_t block, size_t i, des_bit_t value) {
    if (value)
        return block |= ((des_block_t)1) << i;
    else
        return block &= ~(((des_block_t)1) << i);
}

static des_bit_t des_getbit(des_block_t block, size_t i) {
    return (block >> i) & 1;
}

static des_block_t des_transposition(des_block_t block, const des_transposition_table_t transposition_table) {
    des_block_t result = 0;
    for (size_t i = 0; i < DES_BLOCK_SIZE; i++) {
        size_t j = transposition_table[i];
        des_bit_t v = des_getbit(block, j);
        result = des_setbit(result, i, v);
    }
    return result;
}

static des_half_block_t des_half_l(des_block_t block) {
    return block >> (DES_BLOCK_SIZE / 2);
}

static des_half_block_t des_half_r(des_block_t block) {
    return block & ~((des_half_block_t)0);
}

static des_block_t des_merge(des_half_block_t l, des_half_block_t r) {
    return (((des_block_t)l) << (DES_BLOCK_SIZE / 2)) | r;
}

static des_half_block_t des_f(des_half_block_t r, des_key_t k) {
    des_block_t expanded_r = des_transposition(r, des_expansion_transposition_table);
    des_block_t mixed = expanded_r ^ k;

    des_half_block_t substitution_result = 0;
    for (size_t i = 0; i < DES_S_BOX_COUNT; i++) {
        size_t offset = DES_S_BOX_INPUT_GROUP_SIZE * i;
        size_t row = (((size_t)des_getbit(mixed, offset + 5)) << 1)
                   | (((size_t)des_getbit(mixed, offset + 0)) << 0);
        size_t column = (((size_t)des_getbit(mixed, offset + 4)) << 3)
                      | (((size_t)des_getbit(mixed, offset + 3)) << 2)
                      | (((size_t)des_getbit(mixed, offset + 2)) << 1)
                      | (((size_t)des_getbit(mixed, offset + 1)) << 0);
        
        size_t value = des_substitution_boxes[i][row][column];

        substitution_result <<= DES_S_BOX_INPUT_GROUP_SIZE;
        substitution_result |= value;
    }

    return des_transposition(substitution_result, des_permutation_transposition_table);
}

static des_block_t des(des_block_t block, des_key_t key, bool encrypt) {
    block = des_transposition(block, des_initial_transposition_table);

    des_half_block_t l = des_half_l(block);
    des_half_block_t r = des_half_r(block);
    des_key_t subkey[DES_ITERATIONS];

    des_half_key_t k_l = des_transposition(key, des_pc1_l_transposition_table);
    des_half_key_t k_r = des_transposition(key, des_pc1_r_transposition_table);
    for (size_t i = 0; i < DES_ITERATIONS; i++) {
        for (size_t j = 0; j < des_shift_count[i]; j++) {
            k_l = des_transposition(k_l, des_shift_transposition_table);
            k_r = des_transposition(k_r, des_shift_transposition_table);
        }

        des_key_t k = (((des_key_t)k_l) << DES_PC1_HALF_KEY_LENGTH) | k_r;
        subkey[i] = des_transposition(k, des_pc2_transposition_table);
    }

    for (size_t i = 0; i < DES_ITERATIONS; i++) {
        des_key_t k = subkey[encrypt ? i : DES_ITERATIONS - i - 1];
        des_half_block_t l_next = r;
        des_half_block_t r_next = l ^ des_f(r, k);

        l = l_next;
        r = r_next;
    }

    block = des_merge(r, l);
    block = des_transposition(block, des_final_transposition_table);

    return block;
}

static void des_cbc_process_block(des_block_t *prev, des_key_t key, des_block_t block, des_block_t *output, bool encrypt) {
    *output = encrypt ? des(block, key, true) ^ *prev
                      : des(block ^ *prev, key, false);

    *prev = encrypt ? *output : block;
}

size_t des_cbc_output_length(size_t input_length) {
    return (input_length + (sizeof(des_block_t) - 1)) / sizeof(des_block_t) * sizeof(des_block_t);
}

void des_cbc(uint64_t iv, uint64_t key, const void *input, size_t length, void *output, bool encrypt) {
    const size_t block_size = sizeof(des_block_t);

    size_t i;
    for (i = 0; i < length / block_size; i++) {
        size_t j = i * 8;
        des_cbc_process_block(
            &iv,
            key,
            *(des_block_t *)((const char *)input + j),
            (des_block_t *)output + i,
            encrypt
        );
    }

    size_t j = i * 8;
    if (j < length) {
        des_block_t padded_block = 0;
        memcpy(&padded_block, (const char *)input + j, length - j);
        des_cbc_process_block(
            &iv,
            key,
            padded_block,
            (des_block_t *)output + i,
            encrypt
        );
    }
}
