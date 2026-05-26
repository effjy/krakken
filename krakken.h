#ifndef KRAKKEN_H
#define KRAKKEN_H

/**
 * @file krakken.h
 * @brief Header file for the Krakken-2048 hashing algorithm (Abyssal variant).
 *
 * Krakken-2048 is a cryptographic permutation and hashing function operating on
 * a 2048-bit state (represented as 32 of 64-bit unsigned integers). It supports both
 * scalar and AVX2-accelerated implementations of the permutation and sponge construction.
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initializes the round constants (RC) vectors for the scalar implementation.
 *
 * This function is thread-safe and initializes the round constants using SHAKE-128
 * as an extendable output function.
 */
void init_rc_vectors(void);

/**
 * @brief Initializes the round constants (RC) vectors optimized for AVX2.
 *
 * Designed to pre-format or align round constants for vector instructions.
 */
void init_rc_vectors_avx2(void);

/**
 * @brief Retrieves a pointer to the round constants for a specific permutation round.
 *
 * @param round The round index (0 to KRAKKEN_ROUNDS - 1).
 * @return A pointer to the 32 uint64_t round constants for the given round.
 */
const uint64_t *rc_get(int round);

/**
 * @brief Performs the default 10-round Krakken permutation on a 2048-bit state (scalar).
 *
 * @param state The 2048-bit state array of 32 uint64_t elements to permute in-place.
 */
void krakken_permute_scalar(uint64_t state[32]);

/**
 * @brief Performs the default 10-round Krakken permutation on a 2048-bit state (AVX2).
 *
 * @param state The 2048-bit state array of 32 uint64_t elements to permute in-place.
 */
void krakken_permute_avx2(uint64_t state[32]);

/**
 * @brief Performs a variable number of permutation rounds on the 2048-bit state (scalar).
 *
 * @param state The 2048-bit state array of 32 uint64_t elements to permute in-place.
 * @param rounds The number of permutation rounds to execute.
 */
void krakken_permute_scalar_rounds(uint64_t state[32], int rounds);

/**
 * @brief Performs a variable number of permutation rounds on the 2048-bit state (AVX2).
 *
 * @param state The 2048-bit state array of 32 uint64_t elements to permute in-place.
 * @param rounds The number of permutation rounds to execute.
 */
void krakken_permute_avx2_rounds(uint64_t state[32], int rounds);

/**
 * @brief Hashes an input buffer using the Krakken-2048 scalar implementation.
 *
 * This function uses a sponge construction with a rate of 160 bytes and capacity of 96 bytes.
 *
 * @param out Pointer to the output buffer where the digest will be written.
 * @param outlen The length of the requested hash output in bytes.
 * @param in Pointer to the input message buffer.
 * @param inlen The length of the input message in bytes.
 */
void krakken_hash_scalar(uint8_t *out, size_t outlen,
                         const uint8_t *in, size_t inlen);

/**
 * @brief Hashes an input buffer using the Krakken-2048 AVX2 implementation.
 *
 * This function uses a sponge construction with a rate of 160 bytes and capacity of 96 bytes.
 *
 * @param out Pointer to the output buffer where the digest will be written.
 * @param outlen The length of the requested hash output in bytes.
 * @param in Pointer to the input message buffer.
 * @param inlen The length of the input message in bytes.
 */
void krakken_hash_avx2(uint8_t *out, size_t outlen,
                       const uint8_t *in, size_t inlen);

/**
 * @brief The Abyssal non-linear 8-bit S-Box lookup table.
 *
 * This S-box is a permutation of the 256 byte values used in the non-linear
 * transformation steps of the permutation function.
 */
static const uint8_t ABYSSAL_SBOX[256] = {
    0xA5, 0xB6, 0xDE, 0xF7, 0x18, 0x37, 0x8C, 0xC1, 0x89, 0xDA, 0x1E, 0x85, 0x31, 0xF0, 0x97, 0x77,
    0x41, 0x14, 0xE8, 0xC8, 0x8A, 0x04, 0xB5, 0x69, 0x1D, 0x2B, 0x0F, 0x2C, 0x4E, 0x19, 0xCC, 0x79,
    0xD7, 0x4D, 0x7D, 0x43, 0x03, 0x3A, 0x13, 0x92, 0x32, 0xD9, 0x75, 0xDF, 0xAD, 0x81, 0xC3, 0xF1,
    0xF9, 0xA7, 0xE2, 0x35, 0x02, 0xDD, 0x61, 0xA2, 0x50, 0xE1, 0x09, 0xC5, 0xE3, 0x71, 0xCB, 0x99,
    0x9C, 0xB1, 0x23, 0x86, 0x3B, 0x93, 0x24, 0xE9, 0xF6, 0xB4, 0x6A, 0x66, 0xFE, 0x7A, 0x3E, 0x28,
    0x6E, 0xF2, 0x9B, 0xF8, 0x3F, 0x2A, 0x98, 0x10, 0xA1, 0xFB, 0x45, 0x36, 0x64, 0x57, 0x8F, 0x72,
    0x8B, 0x29, 0x56, 0xFD, 0xF4, 0xA4, 0xED, 0xA6, 0x76, 0xEB, 0x6B, 0x4A, 0xC7, 0x5E, 0x26, 0xD0,
    0x5F, 0xCA, 0x87, 0x52, 0x01, 0x16, 0x67, 0xB9, 0x74, 0x4B, 0xCF, 0xD2, 0x60, 0x2F, 0x49, 0x6F,
    0x39, 0x1C, 0x5D, 0x53, 0xE6, 0x3C, 0xC6, 0x7F, 0xEA, 0xE5, 0xBE, 0x00, 0x65, 0x88, 0x83, 0xE4,
    0x0C, 0x38, 0x2D, 0x80, 0xB0, 0xAB, 0x44, 0x84, 0x08, 0x0D, 0xB8, 0x51, 0x9A, 0x2E, 0x91, 0x68,
    0x40, 0x0A, 0xFC, 0x82, 0xBA, 0xCE, 0x0B, 0xFA, 0x1A, 0x5B, 0x62, 0x22, 0xC9, 0x3D, 0x8D, 0x06,
    0x55, 0xD5, 0x78, 0xAE, 0x27, 0x9D, 0x9E, 0xAF, 0xB7, 0x4F, 0xDC, 0x9F, 0x42, 0xA3, 0xBC, 0x15,
    0xB2, 0xDB, 0x11, 0xA9, 0x5C, 0xE7, 0x7B, 0xEF, 0xFF, 0xC2, 0x25, 0xEE, 0x73, 0xF5, 0xD6, 0x48,
    0x4C, 0x21, 0x70, 0xD1, 0x30, 0x54, 0xA0, 0xB3, 0x94, 0x07, 0x58, 0xAA, 0x96, 0x1B, 0x1F, 0x0E,
    0xD8, 0x17, 0xE0, 0xBB, 0x46, 0x6C, 0xAC, 0xA8, 0x05, 0x7E, 0x8E, 0x33, 0xC4, 0xD4, 0x59, 0xBD,
    0xBF, 0xF3, 0x20, 0x34, 0x90, 0xCD, 0xEC, 0x63, 0x47, 0x95, 0x12, 0x6D, 0xD3, 0x5A, 0xC0, 0x7C
};

/**
 * @brief Computes a constant-time lookup in the Abyssal S-box.
 *
 * This function performs a linear scan over all 256 entries of the S-box,
 * utilizing bitwise operations to compute the target entry match without
 * using data-dependent branching or array indexing, mitigating cache-timing
 * side-channel attacks.
 *
 * @param byte The input byte to look up.
 * @return The corresponding S-box value.
 */
static inline uint8_t abyssal_sbox8(uint8_t byte) {
    uint8_t res = 0;
    for (int i = 0; i < 256; i++) {
        uint8_t diff = byte ^ i;
        // If diff == 0, then (diff - 1) is 0xFFFFFFFF, and shifting right by 31 yields 1.
        // If diff != 0, then (diff - 1) has the sign bit set or not depending on values, 
        // but here diff is uint8_t, and promoted to uint32_t. For diff in [1..255], 
        // (uint32_t)diff - 1 is in [0..254], so shifting right by 31 yields 0.
        uint8_t match = (uint8_t)(((uint32_t)diff - 1) >> 31);
        res |= (uint8_t)(-match & ABYSSAL_SBOX[i]);
    }
    return res;
}

#endif
