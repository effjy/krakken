import numpy as np
import galois
from z3 import *
import time

# ------------------------------------------------------------
# 1. Abyssal S-box and DDT over GF(2^8 / 0x11d)
# ------------------------------------------------------------

GF = galois.GF(2**8, irreducible_poly=galois.Poly.Int(0x11d))

# Hardcoded Abyssal S-box table (matches krakken_multi.c)
SBOX = np.array([
    0xA5,0xB6,0xDE,0xF7,0x18,0x37,0x8C,0xC1,0x89,0xDA,0x1E,0x85,0x31,0xF0,0x97,0x77,
    0x41,0x14,0xE8,0xC8,0x8A,0x04,0xB5,0x69,0x1D,0x2B,0x0F,0x2C,0x4E,0x19,0xCC,0x79,
    0xD7,0x4D,0x7D,0x43,0x03,0x3A,0x13,0x92,0x32,0xD9,0x75,0xDF,0xAD,0x81,0xC3,0xF1,
    0xF9,0xA7,0xE2,0x35,0x02,0xDD,0x61,0xA2,0x50,0xE1,0x09,0xC5,0xE3,0x71,0xCB,0x99,
    0x9C,0xB1,0x23,0x86,0x3B,0x93,0x24,0xE9,0xF6,0xB4,0x6A,0x66,0xFE,0x7A,0x3E,0x28,
    0x6E,0xF2,0x9B,0xF8,0x3F,0x2A,0x98,0x10,0xA1,0xFB,0x45,0x36,0x64,0x57,0x8F,0x72,
    0x8B,0x29,0x56,0xFD,0xF4,0xA4,0xED,0xA6,0x76,0xEB,0x6B,0x4A,0xC7,0x5E,0x26,0xD0,
    0x5F,0xCA,0x87,0x52,0x01,0x16,0x67,0xB9,0x74,0x4B,0xCF,0xD2,0x60,0x2F,0x49,0x6F,
    0x39,0x1C,0x5D,0x53,0xE6,0x3C,0xC6,0x7F,0xEA,0xE5,0xBE,0x00,0x65,0x88,0x83,0xE4,
    0x0C,0x38,0x2D,0x80,0xB0,0xAB,0x44,0x84,0x08,0x0D,0xB8,0x51,0x9A,0x2E,0x91,0x68,
    0x40,0x0A,0xFC,0x82,0xBA,0xCE,0x0B,0xFA,0x1A,0x5B,0x62,0x22,0xC9,0x3D,0x8D,0x06,
    0x55,0xD5,0x78,0xAE,0x27,0x9D,0x9E,0xAF,0xB7,0x4F,0xDC,0x9F,0x42,0xA3,0xBC,0x15,
    0xB2,0xDB,0x11,0xA9,0x5C,0xE7,0x7B,0xEF,0xFF,0xC2,0x25,0xEE,0x73,0xF5,0xD6,0x48,
    0x4C,0x21,0x70,0xD1,0x30,0x54,0xA0,0xB3,0x94,0x07,0x58,0xAA,0x96,0x1B,0x1F,0x0E,
    0xD8,0x17,0xE0,0xBB,0x46,0x6C,0xAC,0xA8,0x05,0x7E,0x8E,0x33,0xC4,0xD4,0x59,0xBD,
    0xBF,0xF3,0x20,0x34,0x90,0xCD,0xEC,0x63,0x47,0x95,0x12,0x6D,0xD3,0x5A,0xC0,0x7C
], dtype=int)

# Build DDT
DDT = np.zeros((256, 256), dtype=int)
for x in range(256):
    sx = SBOX[x]
    for a in range(256):
        sx2 = SBOX[x ^ a]
        b = sx ^ sx2
        DDT[a][b] += 1

DDT_support = [[b for b in range(256) if DDT[a][b] > 0] for a in range(256)]
DDT_support_inv = [[a for a in range(256) if DDT[a][b] > 0] for b in range(256)]

# ------------------------------------------------------------
# 2. MDS Matrix over GF(2^8)
# ------------------------------------------------------------

K_ints = [0x01, 0x01, 0x04, 0x01, 0x08, 0x05, 0x02, 0x09]
M_ints = np.zeros((8, 8), dtype=int)
for r in range(8):
    M_ints[r] = np.roll(K_ints, r)
M = GF(M_ints)
M_inv = np.linalg.inv(M)

# ------------------------------------------------------------
# 3. Z3 Galois Field (GF(2^8)) Modeling Helpers
# ------------------------------------------------------------

def gf28_mul_2(x):
    return If((x & 0x80) != 0, (x << 1) ^ 0x1D, x << 1)

def gf28_mul_const(x, c):
    res = BitVecVal(0, 8)
    temp = x
    for i in range(8):
        if (c >> i) & 1:
            res = res ^ temp
        temp = gf28_mul_2(temp)
    return res

# ------------------------------------------------------------
# 4. 4-Round Inbound Matching Solver
#    S1 -> MDS1 -> S2 -> MDS2 -> S3 -> MDS3 -> S4 -> MDS4
# ------------------------------------------------------------

def solve_rebound_4round(delta_in, delta_out, timeout_ms=5000):
    """
    Search for internal difference propagation across 4 rounds:
      d1[i] \\in DDT_support[delta_in[i]]
      d2 = MDS(d1)
      d2_prime[i] \\in DDT_support[d2[i]]
      d3 = MDS(d2_prime)
      d3_prime[i] \\in DDT_support[d3[i]]
      d4 = MDS(d3_prime)
      d4_prime[i] \\in DDT_support[d4[i]]
      delta_out = MDS(d4_prime)  => d4_prime = MDS^{-1}(delta_out)
    """
    # 1. Compute d4_prime from delta_out
    d4_prime_GF = M_inv @ GF(delta_out)
    d4_prime = np.array(d4_prime_GF.astype(np.int64))

    # 2. Iterate over possible d1_0 choices (at most ~128 allowed values)
    allowed_d1_0 = DDT_support[delta_in[0]]
    
    for d1_0 in allowed_d1_0:
        d1_vec = np.zeros(8, dtype=int)
        d1_vec[0] = d1_0
        
        # Compute d2
        d2_vec = np.array((M @ GF(d1_vec)).astype(np.int64))
        
        # Now solve for d2_prime, d3, d3_prime, d4 using Z3 solver
        s = Solver()
        s.set("timeout", timeout_ms)
        
        # Symbolic variables
        d2_prime_vars = [BitVec(f"d2_p_{i}", 8) for i in range(8)]
        d3_vars = [BitVec(f"d3_{i}", 8) for i in range(8)]
        d3_prime_vars = [BitVec(f"d3_p_{i}", 8) for i in range(8)]
        d4_vars = [BitVec(f"d4_{i}", 8) for i in range(8)]
        
        # Round 2 S-box constraints
        for i in range(8):
            allowed_d2_p = DDT_support[d2_vec[i]]
            s.add(Or([d2_prime_vars[i] == v for v in allowed_d2_p]))
            
        # Round 2 -> Round 3 linear MDS
        for r in range(8):
            expr = BitVecVal(0, 8)
            for c in range(8):
                expr = expr ^ gf28_mul_const(d2_prime_vars[c], int(M_ints[r, c]))
            s.add(d3_vars[r] == expr)
            
        # Round 3 S-box constraints (inbound crossing: d3 -> S3 -> d3_prime)
        # S3 input must be DDT-compatible with S3 output
        # For Z3, we can enforce: for each byte, the pair (d3, d3_prime) must represent a valid S-box difference transition.
        # This is a bit-level relation: DDT[d3][d3_prime] > 0
        # Since it is a 2D relation, we can represent it as:
        # Or([And(d3 == x, d3_prime == y) for all valid transitions (x,y)])
        # But listing all ~10,000 active DDT tuples per byte is too slow for Z3.
        # Instead, we constrain d3 and d3_prime using their individual support sets first:
        #   d3_prime[i] can only take values that are valid DDT outputs for some inputs
        # But wait! We can just check the Z3 variables directly.
        # To make it performant: we constrain d3_prime and d4 through linear relations and support boundaries:
        for r in range(8):
            expr = BitVecVal(0, 8)
            for c in range(8):
                expr = expr ^ gf28_mul_const(d3_prime_vars[c], int(M_ints[r, c]))
            s.add(d4_vars[r] == expr)
            
        # Round 4 S-box constraints: d4 -> S4 -> d4_prime (fixed)
        for i in range(8):
            allowed_d4 = DDT_support_inv[d4_prime[i]]
            s.add(Or([d4_vars[i] == v for v in allowed_d4]))
            
        # Now, the inbound link: DDT[d3[i]][d3_prime[i]] > 0
        # Since d3[i] and d3_prime[i] are linked, we can list the valid transition options.
        # To optimize: we only list the matching entries.
        # For a given byte, the allowed (d3, d3_prime) pairs.
        # We can write a fast constraint:
        # Instead of Z3, what if we use Z3 to only solve the linear dependencies, 
        # and check S-box compatibility in Python?
        # That was extremely fast for 3 rounds! Let's do that for 4 rounds:
        # Z3 will find assignments to (d2_prime, d3, d3_prime, d4) that satisfy:
        #   d2_prime[i] \\in DDT_support[d2[i]]
        #   d3 = MDS(d2_prime)
        #   d4 = MDS(d3_prime)
        #   d4[i] \\in DDT_support_inv[d4_prime[i]]
        # Then, once Z3 finds a model, we verify if d3[i] -> d3_prime[i] is valid in Python.
        # If not, we block the solution and search again (backtracking).
        # This keeps the Z3 formula extremely simple!
        
        while s.check() == sat:
            m = s.model()
            d2_p_res = np.array([m[d2_prime_vars[i]].as_long() for i in range(8)], dtype=int)
            d3_res = np.array([m[d3_vars[i]].as_long() for i in range(8)], dtype=int)
            d3_p_res = np.array([m[d3_prime_vars[i]].as_long() for i in range(8)], dtype=int)
            d4_res = np.array([m[d4_vars[i]].as_long() for i in range(8)], dtype=int)
            
            # Check Round 3 inbound S-box compatibility:
            valid = True
            for j in range(8):
                if d3_p_res[j] not in DDT_support[d3_res[j]]:
                    valid = False
                    break
            if valid:
                return {
                    "d1": d1_vec,
                    "d2": d2_vec,
                    "d2_prime": d2_p_res,
                    "d3": d3_res,
                    "d3_prime": d3_p_res,
                    "d4": d4_res,
                    "d4_prime": d4_prime
                }
            
            # Block this specific assignment to d2_prime and d3_prime to force a new search
            block = []
            for i in range(8):
                block.append(d2_prime_vars[i] != int(d2_p_res[i]))
                block.append(d3_prime_vars[i] != int(d3_p_res[i]))
            s.add(Or(block))
            
    return None

if __name__ == "__main__":
    print("====================================================")
    print("Beginning 4-round rebound matching search...")
    print("====================================================")
    
    delta_in = np.zeros(8, dtype=int)
    delta_in[0] = 0x01
    
    # We sweep delta_out[3] from 1 to 255.
    start_time = time.time()
    found_any = False
    
    for val_out in range(1, 256):
        delta_out = np.zeros(8, dtype=int)
        delta_out[3] = val_out
        
        # Solve with a fast timeout per S-box input candidate to sweep quickly
        sol = solve_rebound_4round(delta_in, delta_out, timeout_ms=100)
        if sol is not None:
            print(f"\n[FOUND 4-ROUND MATCH] for delta_out[3] = {val_out}!")
            print(f"  Δ1:       {sol['d1']}")
            print(f"  Δ2:       {sol['d2']}")
            print(f"  Δ2_prime: {sol['d2_prime']}")
            print(f"  Δ3:       {sol['d3']}")
            print(f"  Δ3_prime: {sol['d3_prime']}")
            print(f"  Δ4:       {sol['d4']}")
            print(f"  Δ4_prime: {sol['d4_prime']}")
            found_any = True
            break
            
    if not found_any:
        print("\nCompleted sweep. No valid 4-round differential patterns found.")
    print(f"Time elapsed: {time.time() - start_time:.2f} seconds")
