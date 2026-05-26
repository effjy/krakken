# Krakken-2048 Abyssal

**An SPN-ARX Hybrid Cryptographic Permutation**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Standard](https://img.shields.io/badge/state-2048--bit-brightgreen)]()
[![Rounds](https://img.shields.io/badge/rounds-10-orange)]()

---

## Overview

**Krakken-2048 Abyssal** is a 2048-bit wide-state cryptographic permutation designed for sponge-based hashing. It is built on a deliberate hybrid strategy: an SPN core establishes provable wide-trail bounds on differential and linear propagation, while an ARX mixing layer destroys the algebraic invariants that pure SPN structures preserve.

The permutation processes an 8×4 matrix of 64-bit words through 10 rounds, each composed of eight transformation steps. It is designed for high-assurance environments and leverages AVX2 SIMD instructions for competitive throughput on commodity hardware.

## Key Features

- **State size:** 2048 bits (rate 1280, capacity 768)
- **Rounds:** 10
- **S-box:** 8-bit AES-like inversion-based bijection (nonlinearity 112, differential uniformity 4, algebraic degree 7)
- **Diffusion:** GF(2⁸) circulant MDS matrix with branch number 9 (Whirlpool-class)
- **Architecture:** SPN core + ARX mixing layer, structured so each component addresses the cryptanalytic weakness of the other

## Three Primary Contributions

1. **Coupled S-box construction (Chi):** Column pairs are processed through a sequential Feistel-like dependency that creates asymmetric nonlinear coupling, preventing independent column analysis and doubling effective S-box depth per round.

2. **Systematic invariant subspace analysis:** We identify exact periodic-word subspaces preserved indefinitely by the linear+S-box core, and prove they are fully destroyed by the ARX Pressure layer and SHAKE-128-derived round constants — demonstrating that the ARX layer is structurally necessary, not decorative.

3. **Algebraic degree propagation analysis:** A conservative symbolic model shows the algebraic degree reaches the theoretical maximum of 2047 within the first round, with 100% saturation by Round 2.

## Security Summary

| Attack Class | Status | Margin (Rounds) |
|---|---|---|
| Differential trails | Bound ≤ 2⁻²⁷⁰ (MILP, 45 active S-boxes) | 5 |
| Linear trails | Bound ≤ 2⁻¹³⁵ (MILP) | 5 |
| Rebound attack | Inbound collapse at 5 rounds | 5 |
| Algebraic degree | 2047 reached in Round 1 | 9 |
| Invariant subspaces | Fully destroyed by ARX + constants | 10 |
| Rotational symmetry | None in full permutation | 10 |

## Performance

| Configuration | Throughput |
|---|---|
| Single-thread (AVX2) | 119.92 MB/s |
| 4 threads (parallel) | 362.86 MB/s |

Benchmarked on commodity x86-64 with AVX2 support.

## Test Vector

Empty-string digest (256-bit output, rate 160 bytes):
```
0c513a0b7c3a875f82f4be91d8adeace4a5008498bcd54c6d8880df2dee9dbe9
```

## Repository Contents

- `krakken_avx2.c` — Production AVX2 implementation of the permutation and sponge hash
- `degree_tracking.c` — Symbolic algebraic degree propagation analysis tool
- `rebound_search/` — Z3-based inbound rebound matching scripts (3, 4, and 5 rounds)
- `milp_active_words.py` — MILP optimization for minimum active S-box bound
- `test_vectors.txt` — Intermediate state test vectors for implementation verification

## Paper

A full specification and security analysis is available in the accompanying paper:

> **Krakken-2048 Abyssal: An SPN-ARX Hybrid Permutation and the Case for Algebraic Structure Disruption**
> 
> Jean-François Lachance-Caumartin, 2016
>
> [View on IACR ePrint](https://eprint.iacr.org) *(link to be added upon submission)*

## Building

```bash
# Requires GCC with AVX2 support
gcc -O3 -mavx2 -pthread -o krakken_bench krakken_avx2.c
./krakken_bench
```

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.

## Contact

Jean-François Lachance-Caumartin
[ORCID: 0009-0005-6377-1675](https://orcid.org/0009-0005-6377-1675)

---

*This repository accompanies a paper submitted for peer review. The permutation has been in private development since 2016 and is now being released for public scrutiny and independent cryptanalysis.*
```
