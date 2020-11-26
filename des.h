#ifndef __MENCI_DES_H
#define __MENCI_DES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void des_init(void);
size_t des_cbc_output_length(size_t input_length);
void des_cbc(uint64_t iv, uint64_t key, const void *input, size_t length, void *output, bool encrypt);
 
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __MENCI_DES_H
