# Krakken-2048 Abyssal

**An SPN-ARX Hybrid Cryptographic Permutation**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![State](https://img.shields.io/badge/state-2048--bit-brightgreen)]()
[![Rounds](https://img.shields.io/badge/rounds-10-orange)]()
[![Standard](https://img.shields.io/badge/standard-Sponge%20AEAD-blue)]()
[![Tentacles](https://img.shields.io/badge/tentacles-9-00FFCC)](https://github.com/effjy/krakken)
[![DOI](https://img.shields.io/badge/DOI-10.6084/m9.figshare.32527359-blue)](https://doi.org/10.6084/m9.figshare.32527359)

<p align="center">
  <img src="https://github.com/effjy/krakken/blob/main/krakken_logo.png" alt="Krakken-2048 Abyssal Logo" width="240">
</p>

---

## Paper

**Krakken-2048 Abyssal: An SPN-ARX Hybrid Permutation and the Case for Algebraic Structure Disruption**  
Jean-François Lachance-Caumartin

[![DOI](https://img.shields.io/badge/DOI-10.6084/m9.figshare.32527359-blue)](https://doi.org/10.6084/m9.figshare.32527359)

📄 **Download the full paper:** [`paper.pdf`](paper.pdf)

The paper presents the complete specification, security analysis (differential, linear, rebound, invariant subspaces, algebraic degree), MILP bounds, and AVX2 benchmarks. All cryptanalysis scripts and verification tools are included in this repository.

---

## Overview

**Krakken-2048 Abyssal** is a 2048-bit cryptographic permutation designed for sponge-based constructions, offering exceptional security and performance for hashing and authenticated encryption (AEAD).

It employs a deliberate **SPN-ARX hybrid architecture**: a strong Substitution-Permutation Network core provides rigorous wide-trail security bounds, while an ARX mixing layer systematically destroys the algebraic invariants typical of pure SPN designs. The result is a permutation with rapid diffusion, maximal algebraic degree growth, and resistance to a broad spectrum of cryptanalytic techniques.

Originally conceived in 2016, the core design (S-box, MDS layer, and round structure) has remained stable. This repository marks its first public release, including the full specification, high-performance implementation, and extensive cryptanalytic tooling.

---

## Design Philosophy

Pure SPN constructions offer clean provable bounds but often preserve algebraic structures that can be exploited. Pure ARX designs excel at breaking symmetry through carry propagation but resist formal analysis.

**Krakken-2048 Abyssal** charts a superior third path: leverage the SPN core for strong differential and linear bounds, then deploy targeted ARX operations to annihilate the very algebraic subspaces the SPN creates.

This synergy is not decorative — it is fundamental. The accompanying invariant subspace analysis demonstrates that periodic-word symmetries surviving the linear and S-box layers are completely eliminated by the ARX *Pressure* layer and carefully chosen round constants.

To our knowledge, Krakken-2048 is the first 2048-bit permutation to achieve **full bit-level diffusion** (minimum 927 active bits, average 1024) and **maximum algebraic degree saturation** (2047) within just two rounds.

---

## Round Function (10 Rounds)

Each round consists of eight carefully ordered transformations:

| Step          | Name         | Description |
|---------------|--------------|-------------|
| 1             | **Theta**    | Column-parity diffusion |
| 2             | **Tentacle** | GF(2⁸) circulant MDS matrix (branch number 9) |
| 3             | **Rho**      | Per-word rotations (32 distinct offsets) |
| 4             | **Pi**       | Word-level permutation |
| 5             | **Chi**      | Coupled Feistel-style S-box layer |
| 6             | **Pressure** | 64-bit ARX intra-column mixing with carry propagation |
| 7             | **Beta/Iota**| Cascade XOR with SHAKE-128 round constants |
| 8             | **Ink Cloud**| Global bit-level shuffle across the full 2048-bit state |

---

## Key Features

| Feature              | Specification |
|----------------------|---------------|
| **State size**       | 2048 bits (256 bytes) |
| **Rate / Capacity**  | 1280 / 768 bits |
| **Rounds**           | 10 |
| **S-box**            | 8-bit AES-like inversion over GF(2⁸) with irreducible polynomial 0x11d |
| **S-box metrics**    | Nonlinearity 112, Differential uniformity 4, Degree 7 |
| **Diffusion**        | Whirlpool-class circulant MDS matrix (branch number 9) |
| **Nonlinearity**     | SPN S-box + 64-bit ARX carry chains |
| **Round constants**  | SHAKE-128("Krakken-2048 Abyssal v1 - Primary ") |
| **SIMD**             | AVX2-optimized (32 parallel S-boxes via `vpshufb`) |

---

## Primary Contributions

### 1. Coupled Chi Layer
A sequential Feistel-style S-box application between column pairs:
```c
A' = S(A ⊕ rotl32(B))
B' = S(B ⊕ rotl32(A'))
```
This creates deep nonlinear coupling and prevents independent column attacks.

### 2. Invariant Subspace Destruction
Comprehensive proof that the periodic-word subspaces preserved by the SPN core (V₈, V₁₆, V₃₂) are fully eliminated by the ARX *Pressure* layer and round constants.

### 3. Algebraic Degree Tracking
Symbolic propagation analysis shows:
- Degree 49 after **Chi**
- Degree **2047** (maximum) after **Pressure**
- Full saturation across all 2048 bits by the second round

---

## Security Summary

| Attack Class              | Bound / Status                  | Evidence              | Security Margin |
|---------------------------|---------------------------------|-----------------------|-----------------|
| Differential trails       | ≤ 2⁻²⁷⁰ (45 active S-boxes)   | MILP                  | 5 rounds |
| Linear trails             | ≤ 2⁻¹³⁵                        | MILP                  | 5 rounds |
| Rebound attacks           | Inbound collapse at 5 rounds   | Z3 solver             | 5 rounds |
| Algebraic degree          | 2047 in Round 1                | Symbolic model        | 9 rounds |
| Invariant subspaces       | Fully destroyed                | Eigenvector analysis  | 10 rounds |
| Rotational symmetry       | None                           | Exhaustive audit      | 10 rounds |

---

## Quick Start

```bash
# Build benchmark
gcc -O3 -mavx2 -march=skylake -pthread -o krakken_bench krakken_multi.c
./krakken_bench

# Run algebraic degree analysis
gcc -O3 -o degree_tracking degree_tracking.c
./degree_tracking
```

Additional tools include rebound search scripts, MILP optimizers, and test vector verification.

---

## Performance

- **Single-thread (AVX2)**: 119.92 MB/s  
- **4 threads (parallel streams)**: 362.86 MB/s  

*Measured on Intel Core i7-9700K. All 32 state words remain in YMM registers with zero stack spills.*

---

## Test Vector (Empty Message, 256-bit output)

```
0c513a0b7c3a875f82f4be91d8adeace4a5008498bcd54c6d8880df2dee9dbe9
```

---

## Repository Contents

- `krakken.c/h` — Clean scalar reference
- `krakken_multi.c` — High-performance AVX2 implementation
- `degree_tracking.c` — Algebraic degree propagation tool
- Multiple rebound, MILP, and verification scripts
- `SCRIPTS.md` — Complete inventory of **205 cryptanalysis scripts**
- `paper.pdf` — Full academic paper with specification and security analysis

---

## Related Projects

- **[Krakken-Disk](https://github.com/effjy/krakken-disk)** — Post-quantum encrypted disk manager
- **[vWipe Turbo](https://github.com/effjy/vwipe)** — Forensic-grade secure erasure suite

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

**🦑 Released into the Abyss — 2026**

*From the depths, with precision.*
