#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// ============================================================================
// Algebraic Degree Tracking for Krakken-2048 Abyssal
// 
// This program tracks the maximum algebraic degree of each bit through
// the round function. We use conservative (upper-bound) degree propagation
// rules. The goal is to determine whether the degree approaches the 
// theoretical maximum (2047 for the full state) within the claimed rounds.
// ============================================================================

#define NUM_WORDS 32
#define WORD_BITS 64
#define TOTAL_BITS 2048
#define MAX_DEGREE (TOTAL_BITS - 1)  // 2047

// ============================================================================
// Degree tracking per bit
// ============================================================================
typedef struct {
    int degree;       // current maximum algebraic degree
    int is_const;     // 1 if this bit is constant (degree 0), 0 otherwise
} BitDeg;

// Full state
typedef struct {
    BitDeg bits[TOTAL_BITS];
} State;

// ============================================================================
// Helper: get/set individual bits in the state
// ============================================================================
static inline int bit_index(int word, int bit) {
    return word * WORD_BITS + bit;
}

static inline int word_from_index(int idx) {
    return idx / WORD_BITS;
}

static inline int bit_from_index(int idx) {
    return idx % WORD_BITS;
}

// ============================================================================
// Degree arithmetic: worst-case propagation rules
// ============================================================================

// XOR: max of the two degrees
static inline int deg_xor(int d1, int d2) {
    return (d1 > d2) ? d1 : d2;
}

// AND: sum of degrees (no cancellation assumed)
static inline int deg_and(int d1, int d2) {
    if (d1 == 0 || d2 == 0) return 0;
    int sum = d1 + d2;
    return (sum > MAX_DEGREE) ? MAX_DEGREE : sum;
}

// XOR of multiple values: max degree among them
static int deg_xor_n(const int *degrees, int n) {
    int max_d = 0;
    for (int i = 0; i < n; i++) {
        if (degrees[i] > max_d) max_d = degrees[i];
    }
    return max_d;
}

// ============================================================================
// State operations
// ============================================================================

static void state_copy(State *dst, const State *src) {
    memcpy(dst->bits, src->bits, sizeof(dst->bits));
}

static void state_zero(State *s) {
    for (int i = 0; i < TOTAL_BITS; i++) {
        s->bits[i].degree = 0;
        s->bits[i].is_const = 1;
    }
}

// Initialize with a single variable at a given bit position (degree 1)
static void state_init_single_var(State *s, int var_bit) {
    state_zero(s);
    s->bits[var_bit].degree = 1;
    s->bits[var_bit].is_const = 0;
}

// Initialize with the first k bits as independent variables
static void __attribute__((unused)) state_init_n_vars(State *s, int n_vars) {
    state_zero(s);
    for (int i = 0; i < n_vars && i < TOTAL_BITS; i++) {
        s->bits[i].degree = 1;
        s->bits[i].is_const = 0;
    }
}

// Initialize with all bits as independent variables (degree 1)
static void state_init_all_vars(State *s) {
    for (int i = 0; i < TOTAL_BITS; i++) {
        s->bits[i].degree = 1;
        s->bits[i].is_const = 0;
    }
}

// ============================================================================
// Degree-max tracking across the state
// ============================================================================
static void state_max_degree(const State *s, int *max_deg, int *bits_at_max) {
    *max_deg = 0;
    *bits_at_max = 0;
    for (int i = 0; i < TOTAL_BITS; i++) {
        if (s->bits[i].degree > *max_deg) {
            *max_deg = s->bits[i].degree;
            *bits_at_max = 1;
        } else if (s->bits[i].degree == *max_deg) {
            (*bits_at_max)++;
        }
    }
}

static void state_degree_histogram(const State *s, int hist[10]) {
    // bins: 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63, 64-127, 128-255, 256+
    memset(hist, 0, 10 * sizeof(int));
    for (int i = 0; i < TOTAL_BITS; i++) {
        int d = s->bits[i].degree;
        if (d == 0) hist[0]++;
        else if (d == 1) hist[1]++;
        else if (d <= 3) hist[2]++;
        else if (d <= 7) hist[3]++;
        else if (d <= 15) hist[4]++;
        else if (d <= 31) hist[5]++;
        else if (d <= 63) hist[6]++;
        else if (d <= 127) hist[7]++;
        else if (d <= 255) hist[8]++;
        else hist[9]++;
    }
}

// ============================================================================
// S-box degree propagation
// The Abyssal S-box has algebraic degree 7.
// When applied to a byte where input bits have varying degrees, the output
// degree is bounded by: max_degree_output = min(7, sum of 7 highest input degrees)
// 
// More precisely: if the S-box has degree 7, and inputs have degrees d0..d7
// (sorted descending), then output degree <= min(7, d0+d1+...+d6)
// 
// Conservative bound: output_degree = min(7, 7 * max_input_degree)
// This is the product bound for degree-7 function composition
// ============================================================================
static int sbox_output_degree(const int input_degrees[8]) {
    // Check if any input is non-constant
    int all_const = 1;
    int max_input_deg = 0;
    for (int i = 0; i < 8; i++) {
        if (input_degrees[i] > 0) all_const = 0;
        if (input_degrees[i] > max_input_deg) max_input_deg = input_degrees[i];
    }
    if (all_const) return 0;
    
    // The S-box has algebraic degree 7
    // Composition: if input bits have max degree d, output can reach up to 7*d
    int product_bound = max_input_deg * 7;
    if (product_bound > MAX_DEGREE) product_bound = MAX_DEGREE;
    
    // Also bounded by the fact that it operates on only 8 bits,
    // so cannot exceed degree 7 for fresh variables, but can be higher
    // if inputs already have elevated degree from previous rounds
    return product_bound;
}

// ============================================================================
// RHO rotation constants (same as production)
// ============================================================================
static const int rho_offsets[32] = {
    32, 1,  62, 28, 36, 44, 15, 61, 6,  19, 24, 55, 3, 10, 43, 17,
    25, 39, 41, 59, 47, 8,  56, 14, 18, 35, 21, 33, 2, 49, 22, 51
};

// ============================================================================
// Step implementations with degree tracking
// ============================================================================

// Theta: Column-parity diffusion
// P_c = s_{c,0} ^ s_{c,1} ^ s_{c,2} ^ s_{c,3}
// s_{c,r} ^= (P_{c-1} >> 1) ^ P_{c+1}
static void step_theta(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    // Column structure: word w belongs to column c = w / 4, row r = w % 4
    // For each column c (0..7), compute parity P_c bit by bit
    for (int c = 0; c < 8; c++) {
        for (int b = 0; b < WORD_BITS; b++) {
            /* XOR of all 4 words in this column, bit b
            int degs[4];
            for (int r = 0; r < 4; r++) {
                degs[r] = tmp.bits[bit_index(c * 4 + r, b)].degree;
            }
            // int Pc_b_deg = deg_xor_n(degs, 4);
            */
            
            // Compute P_{c-1} rotated right by 1 (which is bit (b+1) mod 64)
            int Pc_prev_bit = (b + 1) % WORD_BITS;
            int degs_prev[4];
            int cp = (c + 7) % 8;
            for (int r = 0; r < 4; r++) {
                degs_prev[r] = tmp.bits[bit_index(cp * 4 + r, Pc_prev_bit)].degree;
            }
            int Pcm1_deg = deg_xor_n(degs_prev, 4);
            
            // P_{c+1}, bit b
            int degs_next[4];
            int cn = (c + 1) % 8;
            for (int r = 0; r < 4; r++) {
                degs_next[r] = tmp.bits[bit_index(cn * 4 + r, b)].degree;
            }
            int Pcp1_deg = deg_xor_n(degs_next, 4);
            
            // New degree for this bit: XOR of original, P_{c-1}>>1, and P_{c+1}
            // int new_deg = deg_xor(deg_xor(tmp.bits[bit_index(c * 4 + c % 4, b)].degree, Pcm1_deg), Pcp1_deg);
            
            // Actually, s_{c,r} for ALL r gets XORed with the same column parities
            for (int r = 0; r < 4; r++) {
                int orig_deg = tmp.bits[bit_index(c * 4 + r, b)].degree;
                int out_deg = deg_xor(deg_xor(orig_deg, Pcm1_deg), Pcp1_deg);
                s->bits[bit_index(c * 4 + r, b)].degree = out_deg;
            }
        }
    }
}

// Tentacle: GF(2^8) circulant MDS
// For each row r, w_c = XOR_{i=0..7} GF_mul(v_{c+i}, K_i)
// K = {01, 01, 04, 01, 08, 05, 02, 09}
//
// GF multiplication by constant is linear over GF(2), so the degree
// of each output byte is the max of the degrees of the input bytes
// that participate in the XOR (since GF mul is just XOR of shifted inputs)
static void step_tentacle(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    // Tentacle operates row-wise: same row index across all 8 columns
    // Each 64-bit word is treated as 8 bytes
    for (int r = 0; r < 4; r++) {
        // For each output column c
        for (int c = 0; c < 8; c++) {
            // For each byte within the word
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                // This output byte is XOR of GF_mul(v_{c+i}[byte_idx], K_i) for i=0..7
                // Since GF mul by constant is linear, degree of output = max degree of inputs
                int max_deg = 0;
                for (int i = 0; i < 8; i++) {
                    int src_col = (c + i) % 8;
                    int src_word = src_col * 4 + r;
                    int src_byte_start = bit_index(src_word, byte_idx * 8);
                    // Each byte has 8 bits; take max degree across these 8 bits
                    for (int bit = 0; bit < 8; bit++) {
                        int d = tmp.bits[src_byte_start + bit].degree;
                        if (d > max_deg) max_deg = d;
                    }
                }
                // Set all 8 bits of this output byte to max_deg
                int dst_byte_start = bit_index(c * 4 + r, byte_idx * 8);
                for (int bit = 0; bit < 8; bit++) {
                    s->bits[dst_byte_start + bit].degree = max_deg;
                }
            }
        }
    }
}

// Rho: per-word bit rotation
// Simple permutation of bits - degrees just move
static void step_rho(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    for (int w = 0; w < 32; w++) {
        int rot = rho_offsets[w];
        for (int b = 0; b < WORD_BITS; b++) {
            int src_bit = (b - rot + WORD_BITS) % WORD_BITS;
            s->bits[bit_index(w, b)].degree = tmp.bits[bit_index(w, src_bit)].degree;
        }
    }
}

// Pi: word-level permutation
// s'_{(x+3y)%8, y} = s_{x,y}
static void step_pi(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 4; y++) {
            int src_word = x * 4 + y;
            int dst_col = (x + 3 * y) % 8;
            int dst_word = dst_col * 4 + y;
            // Copy all 64 bits
            for (int b = 0; b < WORD_BITS; b++) {
                s->bits[bit_index(dst_word, b)].degree = tmp.bits[bit_index(src_word, b)].degree;
            }
        }
    }
}

// Chi: coupled S-box
// Column pairs (2p, 2p+1):
//   A' = S(A ^ rotl32(B))
//   B' = S(B ^ rotl32(A'))
//
// rotl32 swaps 32-bit halves, so each byte of rotl32(X) comes from a
// different byte of X (specifically, byte i comes from byte (i+4)%8)
static void step_chi(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    for (int p = 0; p < 4; p++) {
        int col_a = 2 * p;
        int col_b = 2 * p + 1;
        
        for (int row = 0; row < 4; row++) {
            int word_a = col_a * 4 + row;
            int word_b = col_b * 4 + row;
            
            // Compute A ^ rotl32(B) byte by byte
            // rotl32: byte j comes from byte (j+4)%8 of B
            int input_deg_a[8];
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                int b_byte = (byte_idx + 4) % 8;  // rotl32 maps byte j to j+4 mod 8
                int max_deg = 0;
                for (int bit = 0; bit < 8; bit++) {
                    int da = tmp.bits[bit_index(word_a, byte_idx * 8 + bit)].degree;
                    int db = tmp.bits[bit_index(word_b, b_byte * 8 + bit)].degree;
                    int d = deg_xor(da, db);
                    if (d > max_deg) max_deg = d;
                }
                input_deg_a[byte_idx] = max_deg;
            }
            
            // S-box output degrees
            int output_deg_a[8];
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                // For S-box, each output byte depends on all 8 bits of input byte
                // We set the degree of all output bits to sbox_output_degree(input_byte_degrees)
                int input_byte_bits[8];
                for (int bit = 0; bit < 8; bit++) {
                    input_byte_bits[bit] = input_deg_a[byte_idx];  // same max degree for all bits
                }
                output_deg_a[byte_idx] = sbox_output_degree(input_byte_bits);
            }
            
            // Write A' to state
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                for (int bit = 0; bit < 8; bit++) {
                    int idx = bit_index(word_a, byte_idx * 8 + bit);
                    s->bits[idx].degree = output_deg_a[byte_idx];
                    s->bits[idx].is_const = (output_deg_a[byte_idx] == 0);
                }
            }
            
            // Now compute B ^ rotl32(A')
            int input_deg_b[8];
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                int a_byte = (byte_idx + 4) % 8;
                int max_deg = 0;
                for (int bit = 0; bit < 8; bit++) {
                    int db = tmp.bits[bit_index(word_b, byte_idx * 8 + bit)].degree;
                    int da = output_deg_a[a_byte];  // use newly computed A' degree
                    int d = deg_xor(db, da);
                    if (d > max_deg) max_deg = d;
                }
                input_deg_b[byte_idx] = max_deg;
            }
            
            // S-box for B'
            int output_deg_b[8];
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                int input_byte_bits[8];
                for (int bit = 0; bit < 8; bit++) {
                    input_byte_bits[bit] = input_deg_b[byte_idx];
                }
                output_deg_b[byte_idx] = sbox_output_degree(input_byte_bits);
            }
            
            // Write B' to state
            for (int byte_idx = 0; byte_idx < 8; byte_idx++) {
                for (int bit = 0; bit < 8; bit++) {
                    int idx = bit_index(word_b, byte_idx * 8 + bit);
                    s->bits[idx].degree = output_deg_b[byte_idx];
                    s->bits[idx].is_const = (output_deg_b[byte_idx] == 0);
                }
            }
        }
    }
}

// Pressure: ARX intra-column mixing
// Within each column (a,b,c,d):
//   a = a + (c ^ (c >> 17))
//   b = b + (d ^ (d >> 17))
//   c = c + (a ^ (a << 31))
//   d = d + (b ^ (b << 31))
//   b = rotl(b, 7), d = rotl(d, 19)
//
// This is the critical step for degree growth.
// Modular addition carries cause AND operations that multiply degrees.
static int __attribute__((unused)) degree_add_carry_chain(const State *s, int word_a, int word_b, int result_word, State *out, int start_bit, int end_bit) {
    // Track the degree of each bit of the sum a + b
    // Using the ripple-carry model: s_i = a_i ^ b_i ^ c_i
    // c_{i+1} = (a_i & b_i) ^ (c_i & (a_i ^ b_i))
    // This means: deg(c_{i+1}) <= max(deg(a_i)+deg(b_i), deg(c_i)+max(deg(a_i),deg(b_i)))
    
    int c_deg = 0;  // carry-in degree (starts at 0 for LSB)
    int max_output_deg = 0;
    
    for (int bit = start_bit; bit < end_bit; bit++) {
        int a_deg = s->bits[bit_index(word_a, bit)].degree;
        int b_deg = s->bits[bit_index(word_b, bit)].degree;
        
        // Sum bit: s_i = a_i ^ b_i ^ c_i
        int s_deg = deg_xor(deg_xor(a_deg, b_deg), c_deg);
        out->bits[bit_index(result_word, bit)].degree = s_deg;
        if (s_deg > max_output_deg) max_output_deg = s_deg;
        
        // Next carry: c_{i+1} = (a_i & b_i) ^ (c_i & (a_i ^ b_i))
        int term1 = deg_and(a_deg, b_deg);
        int term2 = deg_and(c_deg, deg_xor(a_deg, b_deg));
        c_deg = deg_xor(term1, term2);
    }
    
    return max_output_deg;
}

static void step_pressure(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    // Process each column independently
    for (int col = 0; col < 8; col++) {
        int wa = col * 4 + 0;  // word a
        int wb = col * 4 + 1;  // word b
        int wc = col * 4 + 2;  // word c
        int wd = col * 4 + 3;  // word d
        
        State temp_col;
        state_zero(&temp_col);
        
        // Step 1: a = a + (c ^ (c >> 17))
        // Compute c ^ (c >> 17) degree per bit
        int c_xor_shr_deg[WORD_BITS];
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int c_bit = bit;
            int c_shr_bit = (bit + 17) % WORD_BITS;
            c_xor_shr_deg[bit] = deg_xor(tmp.bits[bit_index(wc, c_bit)].degree,
                                          tmp.bits[bit_index(wc, c_shr_bit)].degree);
        }
        
        // Now compute a + (c ^ (c>>17)) with carry chain
        State a_new;
        state_zero(&a_new);
        int c_deg = 0;
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int a_deg = tmp.bits[bit_index(wa, bit)].degree;
            int b_deg = c_xor_shr_deg[bit];
            int s_deg = deg_xor(deg_xor(a_deg, b_deg), c_deg);
            a_new.bits[bit_index(wa, bit)].degree = s_deg;
            int term1 = deg_and(a_deg, b_deg);
            int term2 = deg_and(c_deg, deg_xor(a_deg, b_deg));
            c_deg = deg_xor(term1, term2);
        }
        
        // Step 2: b = b + (d ^ (d >> 17))
        int d_xor_shr_deg[WORD_BITS];
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int d_bit = bit;
            int d_shr_bit = (bit + 17) % WORD_BITS;
            d_xor_shr_deg[bit] = deg_xor(tmp.bits[bit_index(wd, d_bit)].degree,
                                          tmp.bits[bit_index(wd, d_shr_bit)].degree);
        }
        
        State b_new;
        state_zero(&b_new);
        c_deg = 0;
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int b_deg = tmp.bits[bit_index(wb, bit)].degree;
            int d_deg = d_xor_shr_deg[bit];
            int s_deg = deg_xor(deg_xor(b_deg, d_deg), c_deg);
            b_new.bits[bit_index(wb, bit)].degree = s_deg;
            int term1 = deg_and(b_deg, d_deg);
            int term2 = deg_and(c_deg, deg_xor(b_deg, d_deg));
            c_deg = deg_xor(term1, term2);
        }
        
        // Step 3: c = c + (a_new ^ (a_new << 31))
        int a_xor_shl_deg[WORD_BITS];
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int a_bit = bit;
            int a_shl_bit = (bit - 31 + WORD_BITS) % WORD_BITS;
            a_xor_shl_deg[bit] = deg_xor(a_new.bits[bit_index(wa, a_bit)].degree,
                                          a_new.bits[bit_index(wa, a_shl_bit)].degree);
        }
        
        State c_new;
        state_zero(&c_new);
        c_deg = 0;
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int c_bit_deg = tmp.bits[bit_index(wc, bit)].degree;
            int a_mix_deg = a_xor_shl_deg[bit];
            int s_deg = deg_xor(deg_xor(c_bit_deg, a_mix_deg), c_deg);
            c_new.bits[bit_index(wc, bit)].degree = s_deg;
            int term1 = deg_and(c_bit_deg, a_mix_deg);
            int term2 = deg_and(c_deg, deg_xor(c_bit_deg, a_mix_deg));
            c_deg = deg_xor(term1, term2);
        }
        
        // Step 4: d = d + (b_new ^ (b_new << 31))
        int b_xor_shl_deg[WORD_BITS];
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int b_bit = bit;
            int b_shl_bit = (bit - 31 + WORD_BITS) % WORD_BITS;
            b_xor_shl_deg[bit] = deg_xor(b_new.bits[bit_index(wb, b_bit)].degree,
                                          b_new.bits[bit_index(wb, b_shl_bit)].degree);
        }
        
        State d_new;
        state_zero(&d_new);
        c_deg = 0;
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int d_bit_deg = tmp.bits[bit_index(wd, bit)].degree;
            int b_mix_deg = b_xor_shl_deg[bit];
            int s_deg = deg_xor(deg_xor(d_bit_deg, b_mix_deg), c_deg);
            d_new.bits[bit_index(wd, bit)].degree = s_deg;
            int term1 = deg_and(d_bit_deg, b_mix_deg);
            int term2 = deg_and(c_deg, deg_xor(d_bit_deg, b_mix_deg));
            c_deg = deg_xor(term1, term2);
        }
        
        // Write results and apply rotations
        // a stays as-is
        for (int bit = 0; bit < WORD_BITS; bit++) {
            s->bits[bit_index(wa, bit)].degree = a_new.bits[bit_index(wa, bit)].degree;
        }
        
        // b gets rotl(7)
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int src_bit = (bit - 7 + WORD_BITS) % WORD_BITS;
            s->bits[bit_index(wb, bit)].degree = b_new.bits[bit_index(wb, src_bit)].degree;
        }
        
        // c stays as-is
        for (int bit = 0; bit < WORD_BITS; bit++) {
            s->bits[bit_index(wc, bit)].degree = c_new.bits[bit_index(wc, bit)].degree;
        }
        
        // d gets rotl(19)
        for (int bit = 0; bit < WORD_BITS; bit++) {
            int src_bit = (bit - 19 + WORD_BITS) % WORD_BITS;
            s->bits[bit_index(wd, bit)].degree = d_new.bits[bit_index(wd, src_bit)].degree;
        }
    }
}

// Beta/Iota: forward cascade XOR + round constants
// S[i] ^= S[i-1] for i=1..31 (with wraparound? No - it's a forward cascade)
// Then XOR round constants (which are constants, degree 0, so no effect on degree)
static void step_beta_iota(State *s, int round) {
    (void)round;  // constants don't affect degree
    
    // Forward cascade: S[i] ^= S[i-1] for i=1..31
    // This is serial: S[1] ^= S[0], S[2] ^= S[1] (which now includes S[0]), etc.
    State tmp;
    state_copy(&tmp, s);
    
    for (int w = 1; w < 32; w++) {
        for (int b = 0; b < WORD_BITS; b++) {
            int idx = bit_index(w, b);
            int prev_idx = bit_index(w - 1, b);
            // S[w] gets degree max(S[w], S[w-1] including accumulated dependencies)
            s->bits[idx].degree = deg_xor(s->bits[idx].degree, s->bits[prev_idx].degree);
        }
    }
}

// Ink Cloud: global bit-level shuffle
// rotl(11) on each word, then dest[(i*7)%32] = src[i]
static void step_ink_cloud(State *s) {
    State tmp;
    state_copy(&tmp, s);
    
    // Step 1: rotl(11) per word
    State rot;
    state_zero(&rot);
    for (int w = 0; w < 32; w++) {
        for (int b = 0; b < WORD_BITS; b++) {
            int src_bit = (b - 11 + WORD_BITS) % WORD_BITS;
            rot.bits[bit_index(w, b)].degree = tmp.bits[bit_index(w, src_bit)].degree;
        }
    }
    
    // Step 2: dest[(i*7)%32] = src[i]
    state_zero(s);
    for (int i = 0; i < 32; i++) {
        int dest = (i * 7) % 32;
        for (int b = 0; b < WORD_BITS; b++) {
            s->bits[bit_index(dest, b)].degree = rot.bits[bit_index(i, b)].degree;
        }
    }
}

// ============================================================================
// Full round
// ============================================================================
static void __attribute__((unused)) krakken_round(State *s, int round_num) {
    step_theta(s);
    step_tentacle(s);
    step_rho(s);
    step_pi(s);
    step_chi(s);
    step_pressure(s);
    step_beta_iota(s, round_num);
    step_ink_cloud(s);
}

// ============================================================================
// Analysis and output
// ============================================================================

static void print_degree_info(const State *s, const char *label) {
    int max_deg, bits_at_max;
    state_max_degree(s, &max_deg, &bits_at_max);
    
    int hist[10];
    state_degree_histogram(s, hist);
    
    printf("\n=== %s ===\n", label);
    printf("  Max degree: %d (theoretical max: %d)\n", max_deg, MAX_DEGREE);
    printf("  Bits at max degree: %d / %d (%.1f%%)\n", 
           bits_at_max, TOTAL_BITS, 100.0 * bits_at_max / TOTAL_BITS);
    
    printf("  Degree histogram:\n");
    const char *bins[] = {"0", "1", "2-3", "4-7", "8-15", "16-31", 
                          "32-63", "64-127", "128-255", "256+"};
    for (int i = 0; i < 10; i++) {
        if (hist[i] > 0) {
            printf("    deg %8s: %6d bits (%.1f%%)\n", 
                   bins[i], hist[i], 100.0 * hist[i] / TOTAL_BITS);
        }
    }
}

static void analyze_round(const State *s, int round_num, const char *step_name) {
    char label[128];
    snprintf(label, sizeof(label), "Round %d after %s", round_num, step_name);
    print_degree_info(s, label);
}

// ============================================================================
// Main analysis
// ============================================================================

int main(void) {
    printf("Krakken-2048 Abyssal - Algebraic Degree Propagation Analysis\n");
    printf("===========================================================\n");
    printf("State size: %d bits (theoretical max degree: %d)\n", TOTAL_BITS, MAX_DEGREE);
    printf("Tracking worst-case (conservative) degree bounds.\n");
    printf("\nInitialization: one variable (bit 0 has degree 1, all others constant)\n");
    
    State s;
    state_init_single_var(&s, 0);
    
    print_degree_info(&s, "Initial state");
    
    // Round 1
    printf("\n--- Round 1 ---\n");
    
    step_theta(&s);
    analyze_round(&s, 1, "Theta");
    
    step_tentacle(&s);
    analyze_round(&s, 1, "Tentacle");
    
    step_rho(&s);
    analyze_round(&s, 1, "Rho");
    
    step_pi(&s);
    analyze_round(&s, 1, "Pi");
    
    step_chi(&s);
    analyze_round(&s, 1, "Chi");
    
    step_pressure(&s);
    analyze_round(&s, 1, "Pressure");
    
    step_beta_iota(&s, 0);
    analyze_round(&s, 1, "Beta/Iota");
    
    step_ink_cloud(&s);
    analyze_round(&s, 1, "Ink Cloud");
    
    // Round 2
    printf("\n--- Round 2 ---\n");
    
    step_theta(&s);
    analyze_round(&s, 2, "Theta");
    
    step_tentacle(&s);
    analyze_round(&s, 2, "Tentacle");
    
    step_rho(&s);
    analyze_round(&s, 2, "Rho");
    
    step_pi(&s);
    analyze_round(&s, 2, "Pi");
    
    step_chi(&s);
    analyze_round(&s, 2, "Chi");
    
    step_pressure(&s);
    analyze_round(&s, 2, "Pressure");
    
    step_beta_iota(&s, 1);
    analyze_round(&s, 2, "Beta/Iota");
    
    step_ink_cloud(&s);
    analyze_round(&s, 2, "Ink Cloud");
    
    // Round 3
    printf("\n--- Round 3 ---\n");
    
    step_theta(&s);
    analyze_round(&s, 3, "Theta");
    
    step_tentacle(&s);
    analyze_round(&s, 3, "Tentacle");
    
    step_rho(&s);
    analyze_round(&s, 3, "Rho");
    
    step_pi(&s);
    analyze_round(&s, 3, "Pi");
    
    step_chi(&s);
    analyze_round(&s, 3, "Chi");
    
    step_pressure(&s);
    analyze_round(&s, 3, "Pressure");
    
    step_beta_iota(&s, 2);
    analyze_round(&s, 3, "Beta/Iota");
    
    step_ink_cloud(&s);
    analyze_round(&s, 3, "Ink Cloud");
    
    // Summary
    printf("\n===========================================================\n");
    printf("SUMMARY\n");
    printf("===========================================================\n");
    int max_deg, bits_at_max;
    state_max_degree(&s, &max_deg, &bits_at_max);
    printf("Final max degree after 3 rounds: %d\n", max_deg);
    printf("Percentage of theoretical maximum (2047): %.1f%%\n", 
           100.0 * max_deg / MAX_DEGREE);
    
    if (max_deg >= MAX_DEGREE) {
        printf("\nCONCLUSION: Degree reaches theoretical maximum (2047).\n");
        printf("This supports the claim of resistance to higher-order differential attacks.\n");
    } else if (max_deg >= 1000) {
        printf("\nCONCLUSION: Degree is very high (%d) but not maximal.\n", max_deg);
        printf("Higher-order differential attacks appear infeasible.\n");
    } else {
        printf("\nCONCLUSION: Degree is %d - potential vulnerability to higher-order\n", max_deg);
        printf("differential attacks of order > %d should be investigated.\n", max_deg);
    }
    
    printf("\n===========================================================\n");
    printf("ADDITIONAL TEST: All 2048 bits as independent variables\n");
    printf("===========================================================\n");

    State s2;
    state_init_all_vars(&s2);  // All bits have degree 1

    // Run just Round 1 and measure after each step
    // (This tests whether the degree bound is reached even faster
    //  when starting from a fully variable state)

    step_theta(&s2);
    analyze_round(&s2, 1, "Theta (all-vars init)");

    step_tentacle(&s2);
    analyze_round(&s2, 1, "Tentacle (all-vars init)");

    step_chi(&s2);
    analyze_round(&s2, 1, "Chi (all-vars init)");
    // Chi should show degree 7 (S-box degree on degree-1 inputs)

    step_pressure(&s2);
    analyze_round(&s2, 1, "Pressure (all-vars init)");
    // Should show rapid degree explosion from the carry chain
    
    return 0;
}
