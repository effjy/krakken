# Krakken-2048 Abyssal

**An SPN-ARX Hybrid Cryptographic Permutation**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![State](https://img.shields.io/badge/state-2048--bit-brightgreen)]()
[![Rounds](https://img.shields.io/badge/rounds-10-orange)]()
[![Standard](https://img.shields.io/badge/standard-Sponge%20(AEAD)-blue)]()

<p align="center"><img src="https://github.com/effjy/krakken/blob/main/krakken_logo.png" alt="Krakken-2048 Abyssal Logo" width="200"></p>

---

## Overview

**Krakken-2048 Abyssal** is a 2048-bit wide-state cryptographic permutation designed for sponge-based hashing and authenticated encryption. It is built on a deliberate hybrid strategy: an SPN core establishes provable wide-trail bounds on differential and linear propagation, while an ARX mixing layer destroys the algebraic invariants that pure SPN structures preserve.

The permutation processes an 8×4 matrix of 64-bit words through 10 rounds, each composed of eight transformation steps. It is designed for high-assurance environments and leverages AVX2 SIMD instructions for competitive throughput on commodity hardware.

**Historical note.** The design was originally developed in 2016. The core S-box, MDS layer, and round structure have remained unchanged since that time. This repository presents the first public disclosure of the complete specification, implementation, and cryptanalytic tooling.

---

## Design Philosophy

Pure SPN designs (AES, Whirlpool, Grøstl) offer clean wide-trail bounds but create persistent algebraic invariants — periodic-word subspaces and rotational symmetries that survive arbitrarily many rounds. Pure ARX designs (ChaCha, Sparkle) disrupt algebraic structure through carry propagation but resist formal bounding techniques.

Krakken-2048 Abyssal takes a third path: **use the SPN core to establish provable bounds, then layer ARX mixing specifically to destroy the algebraic structure the SPN core creates.** Each architectural component addresses the cryptanalytic weakness of the other. The invariant subspace analysis in the accompanying paper proves this works: the periodic-word symmetries preserved by the linear+S-box skeleton are completely eliminated by the ARX Pressure layer and round constants.

To our knowledge, Krakken-2048 Abyssal is the first 2048-bit permutation to achieve both full bit-level diffusion (minimum 927 active bits, mean 1024)
and maximal algebraic degree saturation (2047) within two rounds. This rapid convergence is a direct consequence of the SPN-ARX hybrid composition:
the GF(2⁸) MDS layer provides wide-trail bounds with branch number 9, while the ARX carry chain in the Pressure step amplifies algebraic degree to the theoretical maximum in a single round.

---

## Round Function (8 Steps × 10 Rounds)

| Step | Name | Description |
|------|------|-------------|
| 1 | **Theta** | Column-parity diffusion across neighbouring columns |
| 2 | **Tentacle** | GF(2⁸) circulant MDS matrix with branch number 9 (Whirlpool-class) |
| 3 | **Rho** | Per-word bit rotation (32 distinct offsets) |
| 4 | **Pi** | Word-level permutation across columns |
| 5 | **Chi** | Coupled S-box: sequential Feistel-like dependency prevents independent column analysis |
| 6 | **Pressure** | 64-bit ARX intra-column mixing with carry chains |
| 7 | **Beta/Iota** | Forward cascade XOR + SHAKE-128-derived round constants |
| 8 | **Ink Cloud** | Global bit-level shuffle across all 2048 bits |

---

## Key Features

| Feature | Specification |
|---|---|
| **State size** | 2048 bits (256 bytes) |
| **Rate / Capacity** | 1280 bits (160 bytes) / 768 bits (96 bytes) |
| **Rounds** | 10 |
| **S-box** | 8-bit AES-like inversion-based bijection over GF(2⁸ / 0x11d) |
| **S-box properties** | Nonlinearity 112, differential uniformity 4, algebraic degree 7, zero fixed points |
| **Diffusion** | GF(2⁸) circulant MDS matrix with branch number 9 (Whirlpool coefficients) |
| **Nonlinear layers** | SPN S-box (Chi) + 64-bit ARX (Pressure) |
| **Round constants** | SHAKE-128-derived ("Krakken-2048 Abyssal v1 - Primary ") |
| **Sponge padding** | Keccak multi-rate (0x06…0x80) |
| **SIMD** | AVX2 — 32 S-box lookups in parallel via vpshufb tree |

The domain string includes a trailing space character. This is intentional and must be preserved for reproducible derivation of round constants.

---

## Three Primary Contributions

### 1. Coupled S-box Construction (Chi)
Column pairs are processed through a sequential Feistel-like dependency:
```
A' = S(A ⊕ rotl32(B))
B' = S(B ⊕ rotl32(A'))
```
The sequential dependency (A' feeds into B') creates asymmetric nonlinear coupling that prevents independent column analysis and doubles effective S-box depth per round.

### 2. Systematic Invariant Subspace Analysis
We identify the exact periodic-word subspaces (V₈, V₁₆, V₃₂) preserved indefinitely by the linear+S-box core, and prove they are fully destroyed by the ARX Pressure layer and SHAKE-128-derived round constants. This validates the SPN-ARX design rationale: the ARX layer is not redundant; it is structurally necessary.

### 3. Algebraic Degree Propagation Analysis
A conservative symbolic degree propagation model tracks the maximum algebraic degree of each of the 2048 state bits through the round function. Results show:
- **After Chi:** degree 49 (7 × 7 from sequential S-box composition)
- **After Pressure:** degree reaches 2047 (theoretical maximum)
- **After Beta/Iota:** 93.4% of bits at maximum degree
- **By Round 2 Theta:** 100% saturation achieved

This provides strong evidence of resistance to higher-order differential and cube attacks.

---

## Security Summary

| Attack Class | Bound / Status | Evidence Level | Margin |
|---|---|---|---|
| Differential trails | ≤ 2⁻²⁷⁰ (MILP: 45 active S-boxes over 10 rounds) | Formal optimization | 5 rounds |
| Linear trails | ≤ 2⁻¹³⁵ (MILP) | Formal optimization | 5 rounds |
| Rebound attack | Inbound collapse at 5 rounds (3→4→1→0 matches) | Z3 constraint search | 5 rounds |
| Algebraic degree | 2047 reached in Round 1, 100% by Round 2 | Symbolic propagation model | 9 rounds |
| Invariant subspaces | Fully destroyed by ARX + round constants | Eigenvector analysis | 10 rounds |
| Rotational symmetry | None in full permutation | Exhaustive empirical audit | 10 rounds |
| DL distinguisher | Perfect 1-round bias collapses to random at 2 rounds | Empirical (50K trials) | 8 rounds |

---

## Quick Start

### Build & Run the Permutation Benchmark
```bash
# Requires GCC with AVX2 support
gcc -O3 -mavx2 -march=skylake -pthread -o krakken_bench krakken_multi.c
./krakken_bench
```

### Run the Degree Tracking Analysis
```bash
gcc -O3 -o degree_tracking degree_tracking.c
./degree_tracking
```

### Run the Rebound Search Scripts
```bash
# Requires: numpy, galois, z3-solver
python3 rebound_3round.py
python3 rebound_4round.py
python3 rebound_5round.py
```

### Run the MILP Active S-box Search
```bash
# Requires: pulp (pip install pulp)
python3 milp_active_words.py
```

### Verify Test Vectors
```bash
gcc -O3 -o verify_krakken verify_krakken.c
./verify_krakken
```

---

## Performance

| Configuration | Throughput |
|---|---|
| Single-thread (AVX2) | 119.92 MB/s |
| 4 threads (parallel streams) | 362.86 MB/s |

Benchmarked on commodity x86-64 (Intel Core i7-9700K) with AVX2. The permutation keeps all 32 state words in 8 YMM registers throughout the inner loop with no stack spills.

---

## Test Vector

Empty-string digest (256-bit output, sponge rate 160 bytes):
```
0c513a0b7c3a875f82f4be91d8adeace4a5008498bcd54c6d8880df2dee9dbe9
```

---

## Repository Contents

| File | Description |
|---|---|
| `krakken.c` / `krakken.h` | Reference scalar implementation of the permutation |
| `krakken_multi.c` | Production AVX2 implementation with parallel hashing |
| `degree_tracking.c` | Symbolic algebraic degree propagation analysis tool |
| `rebound_3round.py` | Z3-based inbound rebound search (3 rounds) |
| `rebound_4round.py` | Z3-based inbound rebound search (4 rounds) |
| `rebound_5round.py` | Z3-based inbound rebound search (5 rounds) |
| `milp_active_words.py` | MILP optimization for minimum active S-box bound |
| `verify_krakken.c` | Test vector verification |
| `Makefile` | Build instructions |
| `test_vectors.txt` | Intermediate state test vectors |

---

## Complete Cryptanalysis Scripts

The repository includes **205 C and Python scripts** used for cryptanalysis, covering:

- Algebraic degree propagation (exact ANF, symbolic tracking, cube tests)
- Rebound attacks (Z3-based inbound matching for 3, 4, and 5 rounds)
- MILP/SMT optimization (active S-box lower bounds, trail search)
- Invariant subspace analysis (eigenvector, periodic-word testing)
- Rotational symmetry audit (word/column/global rotations)
- Differential/linear trail search (wide-trail estimation)
- Boomerang and differential-linear cryptanalysis
- S-box property validation (nonlinearity, uniformity, BCT, fixed points)

For a **complete listing of all 205 scripts** organized by category, see:

➡️ **[`SCRIPTS.md`](SCRIPTS.md) — Full cryptanalysis script inventory**

---

## Paper

A full specification and security analysis is available in the accompanying paper:

> **Krakken-2048 Abyssal: An SPN-ARX Hybrid Permutation and the Case for Algebraic Structure Disruption**
>
> Jean-François Lachance-Caumartin
>
> *View on IACR ePrint (link to be added upon submission)*

---

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.

---

## Contact

**Jean-François Lachance-Caumartin**
- ORCID: [0009-0005-6377-1675](https://orcid.org/0009-0005-6377-1675)
- GitHub: [@effjy](https://github.com/effjy)
- Gravatar: [Profile](https://gravatar.com/luminous0816ec2f7a)

---

*This repository accompanies a paper submitted for peer review. The permutation has been in private development since 2016 and is now being released for public [![Tentacles](https://img.shields.io/badge/tentacles-9-00FFCC)](https://github.com/effjy/krakken)
