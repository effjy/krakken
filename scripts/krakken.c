#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "krakken.h"

/* The number of permutation rounds to execute in the default Krakken permute function. */
#define KRAKKEN_ROUNDS 10

/**
 * @brief Volatile wrapper to prevent compiler from optimizing away memory clearing operations.
 */
static void krakken_memset_wrap(void *p, int c, size_t n) { memset(p, c, n); }
static void (* volatile krakken_memset_volatile)(void *, int, size_t) = krakken_memset_wrap;

/**
 * @brief Overwrites memory with zeros to securely erase sensitive cryptographic data.
 *
 * @param ptr Pointer to the memory buffer to clear.
 * @param n Size of the buffer in bytes.
 */
static void krakken_secure_zero(void *ptr, size_t n) {
    krakken_memset_volatile(ptr, 0, n);
}

/**
 * @brief Circular left shift (rotation) of a 64-bit word.
 *
 * @param x The 64-bit word to rotate.
 * @param n The number of bits to shift.
 * @return The rotated 64-bit word.
 */
static inline uint64_t rotl64(uint64_t x, int n) {
    n &= 63;
    if (n == 0) return x;
    return (x << n) | (x >> (64 - n));
}

/**
 * @brief Circular right shift (rotation) of a 64-bit word.
 *
 * @param x The 64-bit word to rotate.
 * @param n The number of bits to shift.
 * @return The rotated 64-bit word.
 */
static inline uint64_t rotr64(uint64_t x, int n) {
    n &= 63;
    if (n == 0) return x;
    return (x >> n) | (x << (64 - n));
}

/**
 * @brief Applies the Abyssal S-box to each of the 8 bytes within a 64-bit word in constant time.
 *
 * This function uses a SWAR (SIMD within a Register) strategy to compare each byte of
 * the input 64-bit word against all possible byte values [0..255] in parallel, and
 * conditionally accumulates the corresponding S-box entries using bitwise masks.
 * This guarantees constant execution time independent of the values of the bytes in w,
 * preventing cache-timing attacks on the S-box lookups.
 *
 * @param w The input 64-bit word containing 8 individual bytes.
 * @return The 64-bit word with each byte replaced by its S-box value.
 */
static inline uint64_t custom_sbox8_64(uint64_t w) {
    uint64_t res = 0;
    for (int i = 0; i < 256; i++) {
        uint64_t entry = ABYSSAL_SBOX[i];
        // Broadcast the current S-box index byte i to all 8 bytes of a 64-bit word
        uint64_t val_i = i * 0x0101010101010101ULL;
        // Compute the difference between each byte in w and the index byte
        uint64_t diff = w ^ val_i;
        // Clear the MSB of each byte in the diff, and add 0x7F.
        // If a byte was non-zero, this addition causes a carry to the MSB,
        // or the MSB was already set in the diff.
        uint64_t temp = (diff & 0x7F7F7F7F7F7F7F7FULL) + 0x7F7F7F7F7F7F7F7FULL;
        // Identify which bytes in diff are exactly zero (i.e., matched index i)
        uint64_t zero_msb = ~(temp | diff) & 0x8080808080808080ULL;
        // Convert the MSB match into a mask of 0x01 (if matched) or 0x00 (otherwise)
        uint64_t mask = zero_msb >> 7;
        // Multiply by S-box entry value to place it in the matching byte slots
        res |= mask * entry;
    }
    return res;
}

static const int rho[32] = {
    32,  1, 62, 28, 36, 44, 15, 61,
     6, 19, 24, 55,  3, 10, 43, 17,
    25, 39, 41, 59, 47,  8, 56, 14,
    18, 35, 21, 33,  2, 49, 22, 51
};

/* Keccak round constants for the 24 rounds of Keccak-f[1600]. */
#define KECCAK_ROUNDS 24
static const uint64_t keccak_rc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

/* Keccak rotation offsets for each of the 24 lanes. */
static const int keccak_rho[24] = {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14,
    27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
};

/* Keccak permutation indices for the Pi step. */
static const int keccak_pi[24] = {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4,
    15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
};

/**
 * @brief Standard implementation of Keccak-f[1600] permutation.
 *
 * Operates on a state of 25 64-bit words in-place through 24 rounds.
 * Each round consists of the Theta, Rho, Pi, Chi, and Iota steps.
 * Used internally for generating Krakken round constants.
 *
 * @param st The Keccak state (25 uint64_t elements).
 */
static void keccakf1600(uint64_t st[25]) {
    for (int r = 0; r < KECCAK_ROUNDS; r++) {
        uint64_t bc[5], t;
        // Theta step: Compute parity of columns and XOR it into state lanes
        for (int i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i+5] ^ st[i+10] ^ st[i+15] ^ st[i+20];
        for (int i = 0; i < 5; i++) {
            t = bc[(i+4)%5] ^ rotl64(bc[(i+1)%5], 1);
            for (int j = 0; j < 25; j += 5) st[j+i] ^= t;
        }
        // Rho and Pi steps: Rotate lane bits and permute lane positions
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = keccak_pi[i];
            bc[0] = st[j];
            st[j] = rotl64(t, keccak_rho[i]);
            t = bc[0];
        }
        // Chi step: Non-linear combination of lanes along the rows
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++) bc[i] = st[j+i];
            for (int i = 0; i < 5; i++)
                st[j+i] ^= (~bc[(i+1)%5]) & bc[(i+2)%5];
        }
        // Iota step: XOR round constant into st[0]
        st[0] ^= keccak_rc[r];
    }
}

/**
 * @brief Implements SHAKE128 extendable-output function (XOF) to squeeze pseudorandom bytes.
 *
 * Squeezes an arbitrary number of output bytes from a domain-separated string.
 * Used here exclusively for generating round constants in a deterministic, secure manner.
 *
 * @param domain Null-terminated domain separation string.
 * @param out Output buffer to store squeezed bytes.
 * @param outlen Size of output buffer in bytes.
 */
static void shake128_squeeze(const char *domain, uint8_t *out, size_t outlen) {
    uint64_t st[25] = {0};
    size_t rate = 168; // SHAKE128 rate in bytes (1600 - 256) / 8 = 168
    uint8_t *st8 = (uint8_t *)st;

    size_t dlen = strlen(domain);
    if (dlen >= rate - 2) {
        fprintf(stderr, "FATAL: Domain string too long in shake128_squeeze\n");
        abort();
    }
    // Absorb the domain separation string into the state
    for (size_t i = 0; i < dlen; i++) st8[i] ^= (uint8_t)domain[i];
    // Apply SHAKE128 padding suffix (0x1F) and boundary indicator (0x80)
    st8[dlen]     ^= 0x1F;
    st8[rate - 1] ^= 0x80;
    keccakf1600(st);

    // Squeeze the requested amount of output bytes
    size_t done = 0;
    while (done < outlen) {
        size_t take = outlen - done < rate ? outlen - done : rate;
        memcpy(out + done, st8, take);
        done += take;
        if (done < outlen) keccakf1600(st);
    }
}

/* The array storing the 32 uint64_t round constants for each of the 10 permutation rounds. */
static uint64_t rc[KRAKKEN_ROUNDS][32];
/* Control block for thread-safe one-time initialization of the round constants. */
static pthread_once_t rc_once = PTHREAD_ONCE_INIT;

/**
 * @brief Internal implementation to populate the round constants using SHAKE128.
 *
 * Squeezes a block of pseudo-random bytes and parses them into uint64_t values.
 * Replaces any zero values with a default non-zero constant (0xDEADBEEFCAFEBABE) to
 * maintain cryptographic robustness (preventing identity element bypass in XOR steps).
 */
static void _rc_init_impl(void) {
    uint8_t buf[KRAKKEN_ROUNDS * 32 * 8];
    // Squeeze enough bytes to populate the round constant array (10 rounds * 32 words * 8 bytes = 2560 bytes)
    shake128_squeeze("Krakken-2048 Abyssal v1 - Primary ", buf, sizeof(buf));
    for (int ir = 0; ir < KRAKKEN_ROUNDS; ir++) {
        for (int i = 0; i < 32; i++) {
            const uint8_t *p = buf + (ir * 32 + i) * 8;
            uint64_t v = (uint64_t)p[0]
                       | ((uint64_t)p[1] <<  8)
                       | ((uint64_t)p[2] << 16)
                       | ((uint64_t)p[3] << 24)
                       | ((uint64_t)p[4] << 32)
                       | ((uint64_t)p[5] << 40)
                       | ((uint64_t)p[6] << 48)
                       | ((uint64_t)p[7] << 56);
            rc[ir][i] = v ? v : 0xDEADBEEFCAFEBABEULL;
        }
    }
}

void init_rc_vectors(void) {
    pthread_once(&rc_once, _rc_init_impl);
}

const uint64_t *rc_get(int round) { return rc[round]; }

/**
 * @brief Theta step of the Krakken permutation: linear diffusion across state columns.
 *
 * The 32 words of the state can be viewed as an 8x4 matrix. This step computes the parity
 * of each of the 8 columns (each containing 4 words), and mixes the adjacent column parities
 * back into the state words.
 *
 * @param state The 2048-bit permutation state.
 */
void theta_scalar(uint64_t state[32]) {
    uint64_t parity[8];
    // Compute parity of each of the 8 columns (XOR sum of the 4 rows)
    for (int c = 0; c < 8; c++)
        parity[c] = state[4*c] ^ state[4*c+1] ^ state[4*c+2] ^ state[4*c+3];
    // For each column c, mix in the parity of column c-1 (rotated by 1) and column c+1
    for (int c = 0; c < 8; c++) {
        uint64_t d = rotr64(parity[(c + 7) & 7], 1) ^ parity[(c + 1) & 7];
        for (int y = 0; y < 4; y++)
            state[4*c + y] ^= d;
    }
}

/**
 * @brief Multiplies a word by x (doubling) in the finite field GF(2^8) for each of its 8 bytes.
 *
 * Uses the generator polynomial x^8 + x^4 + x^3 + x^2 + 1 (represented as 0x1D after discarding
 * the x^8 term).
 *
 * @param w The input 64-bit word containing 8 field elements.
 * @return The 64-bit word after doubling each byte in GF(2^8).
 */
static inline uint64_t gf28_double_word(uint64_t w) {
    uint64_t shifted = (w << 1) & 0xFEFEFEFEFEFEFEFEULL;
    // Extract the MSB of each byte to determine if reduction is needed
    uint64_t msb_mask = (w & 0x8080808080808080ULL) >> 7;
    // Apply polynomial reduction (0x1D) if the MSB was 1
    uint64_t reduction = msb_mask * 0x1DULL;
    return shifted ^ reduction;
}

/**
 * @brief Multiplies each byte of a 64-bit word by a scalar constant k in GF(2^8).
 *
 * Optimized for specific MDS matrix coefficients: 0x01, 0x02, 0x04, 0x05, 0x08, 0x09.
 *
 * @param w The input 64-bit word.
 * @param k The scalar multiplier in GF(2^8).
 * @return The result of multiplying each byte of w by k.
 */
static inline uint64_t gf28_mul_word_fast(uint64_t w, uint8_t k) {
    switch (k) {
        case 0x01: return w;
        case 0x02: return gf28_double_word(w);
        case 0x04: return gf28_double_word(gf28_double_word(w));
        case 0x05: {
            uint64_t w2 = gf28_double_word(w);
            return gf28_double_word(w2) ^ w;
        }
        case 0x08: return gf28_double_word(gf28_double_word(gf28_double_word(w)));
        case 0x09: {
            uint64_t w2 = gf28_double_word(w);
            uint64_t w4 = gf28_double_word(w2);
            return gf28_double_word(w4) ^ w;
        }
        default:
            return w;
    }
}

/* Coefficients for the circulant MDS (Maximum Distance Separable) matrix multiplication. */
static const uint8_t mds_coeffs[8] = { 0x01, 0x01, 0x04, 0x01, 0x08, 0x05, 0x02, 0x09 };

/**
 * @brief Tentacle MDS step: applies a circulant MDS matrix multiplication on state columns.
 *
 * For each row index y in the state matrix (0 to 3), the row of 8 elements is extracted,
 * and multiplied by the MDS matrix to achieve robust diffusion across the columns.
 *
 * @param state The 2048-bit permutation state.
 */
void tentacle_mds_scalar(uint64_t state[32]) {
    for (int y = 0; y < 4; y++) {
        uint64_t row[8];
        for (int c = 0; c < 8; c++) row[c] = state[c*4 + y];
        for (int c = 0; c < 8; c++) {
            uint64_t sum = 0;
            // Circulant matrix multiplication: index offsets are wrapped modulo 8
            for (int i = 0; i < 8; i++) {
                sum ^= gf28_mul_word_fast(row[(c + i) & 7], mds_coeffs[i]);
            }
            state[c*4 + y] = sum;
        }
    }
}

/**
 * @brief Rho step: rotates the bits of each 64-bit word in the state.
 *
 * Each word is rotated left by a specific constant index defined in the `rho` array.
 *
 * @param state The 2048-bit permutation state.
 */
void rho_scalar(uint64_t state[32]) {
    for (int i = 0; i < 32; i++)
        state[i] = rotl64(state[i], rho[i]);
}

/**
 * @brief Pi step: permutes the layout position of words in the 8x4 state grid.
 *
 * Maps word index `(x, y)` to a new column index `(x + 3 * y) mod 8` while keeping the row `y` the same.
 *
 * @param state The 2048-bit permutation state.
 */
void pi_scalar(uint64_t state[32]) {
    uint64_t temp[32];
    for (int i = 0; i < 32; i++) {
        int x = i / 4, y = i % 4;
        int new_x = (x + 3 * y) & 7;
        temp[new_x * 4 + y] = state[i];
    }
    memcpy(state, temp, 32 * sizeof(uint64_t));
}

/**
 * @brief Chi step: non-linear mixing layer using custom_sbox8_64.
 *
 * Operates on columns in pairs. Mixes them using a Feistel-like construction:
 *   a_new = SBox(a ^ rotl(b, 32))
 *   b_new = SBox(b ^ rotl(a_new, 32))
 * This introduces non-linearity across the state words.
 *
 * @param state The 2048-bit permutation state.
 */
void chi_scalar(uint64_t state[32]) {
    for (int y = 0; y < 4; y++) {
        for (int p = 0; p < 4; p++) {
            int ca = (p * 2)     * 4 + y;
            int cb = (p * 2 + 1) * 4 + y;
            uint64_t a = state[ca], b = state[cb];
            uint64_t ap = custom_sbox8_64(a ^ rotl64(b,  32));
            uint64_t bp = custom_sbox8_64(b ^ rotl64(ap, 32));
            state[ca] = ap;
            state[cb] = bp;
        }
    }
}

/**
 * @brief Pressure ARX step: applies an addition-rotation-xor mixer on each column.
 *
 * Adds local row-to-row mixing within each column, providing further diffusion and non-linearity.
 *
 * @param state The 2048-bit permutation state.
 */
void pressure_arx_scalar(uint64_t state[32]) {
    for (int c = 0; c < 8; c++) {
        uint64_t a = state[4*c];
        uint64_t b = state[4*c+1];
        uint64_t cc = state[4*c+2];
        uint64_t d = state[4*c+3];

        a += (cc ^ (cc >> 17));
        b += (d ^ (d >> 17));
        cc += (a ^ (a << 31));
        d += (b ^ (b << 31));

        state[4*c]   = a;
        state[4*c+1] = rotl64(b, 7);
        state[4*c+2] = cc;
        state[4*c+3] = rotl64(d, 19);
    }
}

/**
 * @brief Beta-Iota step: XORs the round constants into the state.
 *
 * Breaks symmetry between different rounds to prevent slide attacks.
 *
 * @param state The 2048-bit permutation state.
 * @param round The current round index.
 */
void beta_iota_scalar(uint64_t state[32], int round) {
    for (int i = 0; i < 32; i++)
        state[i] ^= rc[round][i];
}

/**
 * @brief Ink Cloud Shuffle step: performs a linear shuffle of state word indices.
 *
 * Re-arranges the indices using the mapping `new_index = (old_index * 7) mod 32`, and
 * shifts each word by 11 bits to disrupt alignment.
 *
 * @param state The 2048-bit permutation state.
 */
void ink_cloud_shuffle(uint64_t state[32]) {
    uint64_t temp[32];
    for (int i = 0; i < 32; i++) {
        temp[(i * 7) & 31] = rotl64(state[i], 11);
    }
    memcpy(state, temp, 32 * sizeof(uint64_t));
}

void krakken_permute_scalar_rounds(uint64_t state[32], int rounds) {
    // Dynamically initialize round constants if not already initialized
    init_rc_vectors();

    if (!state) {
        fprintf(stderr, "FATAL: krakken_permute_scalar_rounds() called with NULL state\n");
        abort();
    }
    if (rounds <= 0) return;
    if (rounds > KRAKKEN_ROUNDS) rounds = KRAKKEN_ROUNDS;

    // Run the specified number of rounds of Krakken-2048 scalar permutation steps
    for (int ir = 0; ir < rounds; ir++) {
        theta_scalar(state);
        tentacle_mds_scalar(state);
        rho_scalar(state);
        pi_scalar(state);
        chi_scalar(state);
        pressure_arx_scalar(state);
        beta_iota_scalar(state, ir);
        ink_cloud_shuffle(state);
    }
}

void krakken_permute_scalar(uint64_t state[32]) {
    krakken_permute_scalar_rounds(state, KRAKKEN_ROUNDS);
}

void krakken_hash_scalar(uint8_t *out, size_t outlen,
                         const uint8_t *in, size_t inlen) {
    if (outlen != 0 && out == NULL) {
        fprintf(stderr, "FATAL: krakken_hash_scalar() called with NULL out\n");
        abort();
    }
    if (inlen != 0 && in == NULL) {
        fprintf(stderr, "FATAL: krakken_hash_scalar() called with NULL in\n");
        abort();
    }
    if (outlen == 0) return;

    // The 2048-bit sponge state: 32 words of 64 bits (256 bytes total)
    // Aligned to 32 bytes for potential SIMD memory operations
    union {
        uint64_t w[32];
        uint8_t  b[256];
    } state __attribute__((aligned(32)));
    memset(state.b, 0, 256);

    const uint8_t *msg = in;
    size_t rem = inlen;

    // Absorbing phase: XOR 160-byte blocks (the sponge rate) into the state,
    // then permute the state using the scalar permutation.
    while (rem >= 160) {
        for (int i = 0; i < 160; i++) state.b[i] ^= msg[i];
        krakken_permute_scalar(state.w);
        msg += 160; rem -= 160;
    }

    // Absorb trailing bytes with custom padding rules
    uint8_t block[160] __attribute__((aligned(32)));
    memset(block, 0, 160);
    if (rem > 0) memcpy(block, msg, rem);

    // Padding construction:
    // If remaining message size < 159, append 0x06 byte after message and 0x80 at block end (like Keccak/SHA-3).
    // Uses constant-time masking to prevent timing side channels on message length.
    uint8_t mask = (uint8_t)(-(rem < 159));
    block[rem]  = (uint8_t)((block[rem] & ~mask) | (0x06 & mask));
    block[159]  = (uint8_t)((mask & 0x80) | ((~mask) & 0x86));
    for (int i = 0; i < 160; i++) state.b[i] ^= block[i];
    krakken_permute_scalar(state.w);

    // Squeezing phase: extract hash digest bytes from the first 160 bytes of the sponge state
    while (outlen > 0) {
        size_t take = outlen < 160 ? outlen : 160;
        memcpy(out, state.b, take);
        out += take; outlen -= take;
        if (outlen > 0) krakken_permute_scalar(state.w);
    }

    // Zero-out sensitive data buffers on the stack before returning
    krakken_secure_zero(state.b, 256);
    krakken_secure_zero(block,   160);
}

#ifdef KRAKKEN_MAIN
#include <sys/time.h>

static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

static void benchmark(void) {
    const size_t data_size = 1 * 1024 * 1024;
    const size_t hash_size = 32;
    const int    iterations = 10;
    uint8_t *data = malloc(data_size);
    uint8_t *hash = malloc(hash_size);
    if (!data || !hash) { fprintf(stderr, "malloc failed\n"); return; }

    for (size_t i = 0; i < data_size; i++)
        data[i] = (uint8_t)(i ^ (i >> 8) ^ (i >> 16) ^ (i >> 24));

    printf("Krakken-2048 Scalar Benchmark (Abyssal, %d rounds)\n", KRAKKEN_ROUNDS);
    printf("==================================================\n");
    printf("Data: %zu MB   Output: %zu bytes   Iterations: %d\n\n",
           data_size >> 20, hash_size, iterations);

    krakken_hash_scalar(hash, hash_size, data, data_size);

    double total = 0.0;
    for (int i = 0; i < iterations; i++) {
        double t0 = get_time();
        krakken_hash_scalar(hash, hash_size, data, data_size);
        total += get_time() - t0;
    }

    double avg  = total / iterations;
    double mbps = (data_size / (1024.0 * 1024.0)) / avg;
    printf("Average time : %.3f s\n", avg);
    printf("Throughput   : %.2f MB/s  (%.3f GB/s)\n", mbps, mbps / 1024.0);

    uint8_t empty[32];
    krakken_hash_scalar(empty, 32, (const uint8_t *)"", 0);
    printf("Empty hash   : ");
    for (int i = 0; i < 32; i++) printf("%02x", empty[i]);
    printf("\n");

    free(data); free(hash);
}

int main(void) {
    benchmark();
    return 0;
}
#endif
