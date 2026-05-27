# Krakken Cryptanalysis Scripts

This document lists all 205 C and Python files developed for the cryptanalysis of Krakken-2048 Abyssal. The scripts cover algebraic degree propagation, rebound attacks, MILP/SMT optimization, invariant subspace analysis, rotational symmetry testing, differential/linear trail search, boomerang analysis, and comprehensive S-box property validation.

## Full File Listing (205 files)

```
/home/user/data/krakken/github/degree_tracking.c
/home/user/data/krakken/github/krakken.c
/home/user/data/krakken/github/krakken_multi.c
/home/user/data/krakken/github/milp_active_words.py
/home/user/data/krakken/github/rebound_3round.py
/home/user/data/krakken/github/rebound_4round.py
/home/user/data/krakken/github/rebound_5round.py
/home/user/data/krakken/github/verify_krakken.c
/home/user/data/krakken/neon/neon2/krakken.c
/home/user/data/krakken/neon/neon2/krakken_multi.c
/home/user/data/krakken/neon/neon2/krakken_neon.c
/home/user/data/krakken/neon/neon2/verify_krakken.c
/home/user/data/krakken/neon/neon3/krakken.c
/home/user/data/krakken/neon/neon3/krakken_multi.c
/home/user/data/krakken/neon/neon3/krakken_neon.c
/home/user/data/krakken/neon/neon3/verify_krakken.c
/home/user/data/krakken/neon/neon/krakken.c
/home/user/data/krakken/neon/neon/krakken_multi.c
/home/user/data/krakken/neon/neon/krakken_neon.c
/home/user/data/krakken/neon/neon/verify_krakken.c
/home/user/data/krakken/research/degree/degree1/degree_growth.py
/home/user/data/krakken/research/degree/degree1/degree_propagation.py
/home/user/data/krakken/research/degree/degree1/exact_symbolic_propagation.py
/home/user/data/krakken/research/degree/degree1/krakken.c
/home/user/data/krakken/research/degree/degree1/krakken_multi.c
/home/user/data/krakken/research/degree/degree1/precise_degree_propagation.py
/home/user/data/krakken/research/degree/degree1/precise_mask_propagation.py
/home/user/data/krakken/research/degree/degree1/sbox_properties.py
/home/user/data/krakken/research/degree/degree1/test_anf.py
/home/user/data/krakken/research/degree/degree1/toy_mask_propagation.py
/home/user/data/krakken/research/degree/degree1/verify_krakken.c
/home/user/data/krakken/research/degree/degree2/deg_check.py
/home/user/data/krakken/research/degree/degree2/exact_round1_degree.py
/home/user/data/krakken/research/degree/degree2/krakken.c
/home/user/data/krakken/research/degree/degree2/krakken_multi.c
/home/user/data/krakken/research/degree/degree2/user_script.py
/home/user/data/krakken/research/degree/degree2/verify_krakken.c
/home/user/data/krakken/research/margins/research1/analyse_mds.py
/home/user/data/krakken/research/margins/research1/analyse_sbox.py
/home/user/data/krakken/research/margins/research1/check_matplotlib.py
/home/user/data/krakken/research/margins/research1/check_packages.py
/home/user/data/krakken/research/margins/research1/find_active_sboxes.py
/home/user/data/krakken/research/margins/research1/krakken.c
/home/user/data/krakken/research/margins/research1/krakken_multi.c
/home/user/data/krakken/research/margins/research1/verify_diffusion.c
/home/user/data/krakken/research/margins/research1/verify_krakken.c
/home/user/data/krakken/research/margins/research2/diffusion_test.py
/home/user/data/krakken/research/margins/research2/krakken.c
/home/user/data/krakken/research/margins/research2/krakken_multi.c
/home/user/data/krakken/research/margins/research2/test_algebraic.py
/home/user/data/krakken/research/margins/research2/test_impossible.py
/home/user/data/krakken/research/margins/research2/test_sac_bic.py
/home/user/data/krakken/research/margins/research2/verify_krakken.c
/home/user/data/krakken/research/margins/research2/verify_properties.py
/home/user/data/krakken/research/margins/research3/active_sbox_finder.py
/home/user/data/krakken/research/margins/research3/bct_analysis.c
/home/user/data/krakken/research/margins/research3/byte_margin_tracker.py
/home/user/data/krakken/research/margins/research3/degree_tracer.py
/home/user/data/krakken/research/margins/research3/krakken_2round_attack.py
/home/user/data/krakken/research/margins/research3/krakken_3round_attack.py
/home/user/data/krakken/research/margins/research3/krakken_4round_attack.py
/home/user/data/krakken/research/margins/research3/krakken.c
/home/user/data/krakken/research/margins/research3/krakken_multi.c
/home/user/data/krakken/research/margins/research3/krakken_smt_proof.py
/home/user/data/krakken/research/margins/research3/milp_active_sboxes.py
/home/user/data/krakken/research/margins/research3/test_algebraic.py
/home/user/data/krakken/research/margins/research3/test_impossible.py
/home/user/data/krakken/research/margins/research3/test_sac_bic.py
/home/user/data/krakken/research/margins/research3/user_sbox_all.py
/home/user/data/krakken/research/margins/research3/user_sbox_cumulative.py
/home/user/data/krakken/research/margins/research3/user_sbox.py
/home/user/data/krakken/research/margins/research3/user_sbox_r2.py
/home/user/data/krakken/research/margins/research3/user_sbox_r3.py
/home/user/data/krakken/research/margins/research3/verify_krakken.c
/home/user/data/krakken/research/margins/research3/verify_properties.py
/home/user/data/krakken/research/paper/paper1/active_sbox_finder.py
/home/user/data/krakken/research/paper/paper1/byte_margin_tracker.py
/home/user/data/krakken/research/paper/paper1/degree_tracer.py
/home/user/data/krakken/research/paper/paper1/krakken.c
/home/user/data/krakken/research/paper/paper1/krakken_multi.c
/home/user/data/krakken/research/paper/paper1/verify_krakken.c
/home/user/data/krakken/research/paper/paper2/dl_degree_tests.py
/home/user/data/krakken/research/paper/paper2/krakken.c
/home/user/data/krakken/research/paper/paper2/krakken_multi.c
/home/user/data/krakken/research/paper/paper2/verify_krakken.c
/home/user/data/krakken/research/paper/paper3/byte_flip_test.py
/home/user/data/krakken/research/paper/paper3/degree_test.py
/home/user/data/krakken/research/paper/paper3/empirical_search.py
/home/user/data/krakken/research/paper/paper3/generate_all_zero.py
/home/user/data/krakken/research/paper/paper3/generate_test_vector.py
/home/user/data/krakken/research/paper/paper3/krakken.c
/home/user/data/krakken/research/paper/paper3/krakken_multi.c
/home/user/data/krakken/research/paper/paper3/milp_krakken.py
/home/user/data/krakken/research/paper/paper3/test_krakken.c
/home/user/data/krakken/research/paper/paper3/verify_krakken.c
/home/user/data/krakken/research/paper/paper4/krakken.c
/home/user/data/krakken/research/paper/paper4/krakken_degree.c
/home/user/data/krakken/research/paper/paper4/krakken_multi.c
/home/user/data/krakken/research/paper/paper4/verify_krakken.c
/home/user/data/krakken/research/rebound/eigen_analysis.c
/home/user/data/krakken/research/rebound/fixed/rebound_3round.py
/home/user/data/krakken/research/rebound/fixed/rebound_4round.py
/home/user/data/krakken/research/rebound/fixed/rebound_5round.py
/home/user/data/krakken/research/rebound/invariant_test.c
/home/user/data/krakken/research/rebound/krakken.c
/home/user/data/krakken/research/rebound/krakken_multi.c
/home/user/data/krakken/research/rebound/rebound_3round.py
/home/user/data/krakken/research/rebound/rebound_4round.py
/home/user/data/krakken/research/rebound/rebound_5round.py
/home/user/data/krakken/research/rebound/rebound_core.py
/home/user/data/krakken/research/rebound/rotational_check.c
/home/user/data/krakken/research/rebound/subspace_check.c
/home/user/data/krakken/research/rebound/verify_krakken.c
/home/user/data/krakken/research/structure/research1/krakken_boomerang.py
/home/user/data/krakken/research/structure/research1/krakken.c
/home/user/data/krakken/research/structure/research1/krakken_degree_growth.py
/home/user/data/krakken/research/structure/research1/krakken_diff_search.py
/home/user/data/krakken/research/structure/research1/krakken_division_property.py
/home/user/data/krakken/research/structure/research1/krakken_exact_linear_search.py
/home/user/data/krakken/research/structure/research1/krakken_exact_sboxes.py
/home/user/data/krakken/research/structure/research1/krakken_exact_trail_search.py
/home/user/data/krakken/research/structure/research1/krakken_fast_degree.py
/home/user/data/krakken/research/structure/research1/krakken_impossible_diff_actual.py
/home/user/data/krakken/research/structure/research1/krakken_impossible_diff.py
/home/user/data/krakken/research/structure/research1/krakken_invariant_subspace.py
/home/user/data/krakken/research/structure/research1/krakken_lat_analysis.py
/home/user/data/krakken/research/structure/research1/krakken_linear_hull_search.py
/home/user/data/krakken/research/structure/research1/krakken_linear_trail_search.py
/home/user/data/krakken/research/structure/research1/krakken_multi.c
/home/user/data/krakken/research/structure/research1/krakken_quantum_estimator.py
/home/user/data/krakken/research/structure/research1/krakken_rotational_test.py
/home/user/data/krakken/research/structure/research1/krakken_sat_milp_distinguisher.py
/home/user/data/krakken/research/structure/research1/krakken_second_order_diff.py
/home/user/data/krakken/research/structure/research1/krakken_security_proof.py
/home/user/data/krakken/research/structure/research1/krakken_wide_trail.py
/home/user/data/krakken/research/structure/research1/verify_krakken_mds.c
/home/user/data/krakken/research/structure/research1/verify_krakken_sbox.c
/home/user/data/krakken/research/structure/research2/algebraic_degree.c
/home/user/data/krakken/research/structure/research2/avalanche_test.c
/home/user/data/krakken/research/structure/research2/bct_analysis.c
/home/user/data/krakken/research/structure/research2/bic_test.c
/home/user/data/krakken/research/structure/research2/branch_tester.c
/home/user/data/krakken/research/structure/research2/fixed_point_test.c
/home/user/data/krakken/research/structure/research2/impossible_diff_search.c
/home/user/data/krakken/research/structure/research2/integral_test.c
/home/user/data/krakken/research/structure/research2/integral_validation.c
/home/user/data/krakken/research/structure/research2/invariant_test.c
/home/user/data/krakken/research/structure/research2/krakken.c
/home/user/data/krakken/research/structure/research2/krakken_multi.c
/home/user/data/krakken/research/structure/research2/krakken_zero_corr_mitm_correct.py
/home/user/data/krakken/research/structure/research2/krakken_zero_corr_mitm.py
/home/user/data/krakken/research/structure/research2/krakken_zero_corr.py
/home/user/data/krakken/research/structure/research2/lane_relation_test.c
/home/user/data/krakken/research/structure/research2/milp_active_sboxes.py
/home/user/data/krakken/research/structure/research2/near_fixed_point_test.c
/home/user/data/krakken/research/structure/research2/rotational_test.c
/home/user/data/krakken/research/structure/research2/sbox_security.c
/home/user/data/krakken/research/structure/research2/verify_krakken.c
/home/user/data/krakken/research/structure/research2/wide_trail_analyzer.c
/home/user/data/krakken/research/structure/research2/wide_trail_sbox_analyzer.c
/home/user/data/krakken/research/structure/research2/xor_zero_test.c
/home/user/data/krakken/research/structure/research3/krakken_2round_attack.py
/home/user/data/krakken/research/structure/research3/krakken_3round_attack.py
/home/user/data/krakken/research/structure/research3/krakken_4round_attack.py
/home/user/data/krakken/research/structure/research3/krakken_algebraic_attack.py
/home/user/data/krakken/research/structure/research3/krakken_beam.c
/home/user/data/krakken/research/structure/research3/krakken.c
/home/user/data/krakken/research/structure/research3/krakken_diffusion.c
/home/user/data/krakken/research/structure/research3/krakken_linear_milp.c
/home/user/data/krakken/research/structure/research3/krakken_multi.c
/home/user/data/krakken/research/structure/research3/krakken_smt_proof.py
/home/user/data/krakken/research/structure/research3/krakken_test.c
/home/user/data/krakken/research/structure/research3/production_dfa_audit.c
/home/user/data/krakken/research/structure/research3/test_equivalence.c
/home/user/data/krakken/research/structure/research3/test_linear_bias.c
/home/user/data/krakken/research/structure/research3/test_sac.c
/home/user/data/krakken/research/structure/research3/test_symmetry.c
/home/user/data/krakken/research/structure/research3/test_zero.c
/home/user/data/krakken/research/structure/research3/verify_krakken.c
/home/user/data/krakken/research/trail/krakken.c
/home/user/data/krakken/research/trail/krakken_multi.c
/home/user/data/krakken/research/trail/krakken_trail_search_loop.py
/home/user/data/krakken/research/trail/krakken_trail_search.py
/home/user/data/krakken/research/trail/run_test.py
/home/user/data/krakken/research/trail/verify_krakken.c
/home/user/data/krakken/src/github/krakken.c
/home/user/data/krakken/src/github/krakken_multi.c
/home/user/data/krakken/src/github/verify_krakken.c
/home/user/data/krakken/src/krakken.c
/home/user/data/krakken/src/krakken_multi.c
/home/user/data/krakken/src/old/krakken.c
/home/user/data/krakken/src/old/krakken_multi.c
/home/user/data/krakken/src/old/verify_krakken.c
/home/user/data/krakken/src/verify_krakken.c
/home/user/data/krakken/tests/attacks/attack_demo.c
/home/user/data/krakken/tests/attacks/krakken.c
/home/user/data/krakken/tests/attacks/krakken_multi.c
/home/user/data/krakken/tests/audit/krakken.c
/home/user/data/krakken/tests/audit/krakken_multi.c
/home/user/data/krakken/tests/audit/test_security.py
/home/user/data/krakken/tests/crypto1/cryptanalysis.py
/home/user/data/krakken/tests/crypto1/krakken.c
/home/user/data/krakken/tests/crypto1/krakken_multi.c
/home/user/data/krakken/tests/crypto1/verify_inverse.py
/home/user/data/krakken/tests/crypto2/analyze.py
/home/user/data/krakken/tests/crypto2/krakken.c
/home/user/data/krakken/tests/crypto2/krakken_multi.c
/home/user/data/krakken/tests/crypto3/analysis.py
/home/user/data/krakken/tests/crypto3/check_mds.py
/home/user/data/krakken/tests/crypto3/diffusion_analysis.py
/home/user/data/krakken/tests/crypto3/krakken.c
/home/user/data/krakken/tests/crypto3/krakken_multi.c
/home/user/data/krakken/tests/crypto3/sbox_fixed_points.py
/home/user/data/krakken/tests/crypto3/z3_analysis.py
/home/user/data/krakken/tests/distinguisher/find_r2_distinguisher.c
/home/user/data/krakken/tests/distinguisher/krakken.c
/home/user/data/krakken/tests/distinguisher/krakken_multi.c
/home/user/data/krakken/tests/distinguisher/test_diff.c
/home/user/data/krakken/tests/distinguisher/test_dl.c
/home/user/data/krakken/tests/s-boxes/find_active_sboxes_fast.py
/home/user/data/krakken/tests/s-boxes/find_active_sboxes.py
/home/user/data/krakken/tests/s-boxes/krakken.c
/home/user/data/krakken/tests/s-boxes/krakken_multi.c
/home/user/data/krakken/tests/s-boxes/sbox_analysis.c
/home/user/data/krakken/tests/s-boxes/sbox_analysis.py
/home/user/data/krakken/tests/s-boxes/test_arx.py
/home/user/data/krakken/tests/s-boxes/test_z3.py
/home/user/data/krakken/tests/s-boxes/test_z3_sbox.py
/home/user/data/krakken/tests/s-boxes/user_sbox_all.py
/home/user/data/krakken/tests/s-boxes/user_sbox_cumulative.py
/home/user/data/krakken/tests/s-boxes/user_sbox.py
/home/user/data/krakken/tests/s-boxes/user_sbox_r2.py
/home/user/data/krakken/tests/s-boxes/user_sbox_r3.py
/home/user/data/krakken/tests/s-boxes/verify_arx.py
/home/user/data/krakken/tests/security/krakken.c
/home/user/data/krakken/tests/security/krakken_multi.c
/home/user/data/krakken/tests/security/security_test.py
/home/user/data/krakken/tmp/krakken.c
/home/user/data/krakken/tmp/krakken_multi.c
/home/user/data/krakken/tmp/verify_krakken.c
```

## Scripts by Category

### Core Implementation
| File | Description |
|------|-------------|
| `github/krakken.c` | AVX2 implementation |
| `github/krakken_multi.c` | Multi-threaded AVX2 version |
| `github/verify_krakken.c` | Test harness |
| `neon/*/krakken_neon.c` | ARM NEON implementations |

### Algebraic Degree Propagation
| File | Description |
|------|-------------|
| `research/degree/degree1/degree_growth.py` | Degree growth estimation |
| `research/degree/degree1/degree_propagation.py` | Symbolic degree tracker |
| `research/degree/degree1/exact_symbolic_propagation.py` | Exact ANF propagation (scaled-down) |
| `research/degree/degree1/precise_mask_propagation.py` | Mask-based degree tracking |
| `research/degree/degree2/exact_round1_degree.py` | Exact degree after Round 1 |
| `research/degree/degree2/deg_check.py` | S-box degree verification |

### Rebound Attacks (Z3)
| File | Description |
|------|-------------|
| `github/rebound_3round.py` | 3-round inbound matching |
| `github/rebound_4round.py` | 4-round inbound matching |
| `github/rebound_5round.py` | 5-round inbound matching |
| `research/rebound/rebound_core.py` | Core rebound library |

### MILP / SMT Optimization
| File | Description |
|------|-------------|
| `github/milp_active_words.py` | Word-level active S-box MILP |
| `research/margins/research3/milp_active_sboxes.py` | Byte-level MILP |
| `research/structure/research2/milp_active_sboxes.py` | Alternative MILP model |
| `research/trail/krakken_trail_search.py` | Trail search via MILP |

### Invariant Subspace & Rotational Analysis
| File | Description |
|------|-------------|
| `research/rebound/invariant_test.c` | Invariant subspace testing |
| `research/rebound/subspace_check.c` | Periodic-word subspace verification |
| `research/rebound/rotational_check.c` | Rotational symmetry audit |
| `research/structure/research1/krakken_invariant_subspace.py` | Matrix-based invariant search |
| `research/structure/research1/krakken_rotational_test.py` | Rotational distinguisher test |

### Differential / Linear Cryptanalysis
| File | Description |
|------|-------------|
| `research/structure/research1/krakken_diff_search.py` | Differential trail search |
| `research/structure/research1/krakken_linear_trail_search.py` | Linear trail search |
| `research/structure/research1/krakken_lat_analysis.py` | Linear approximation table |
| `research/structure/research2/bct_analysis.c` | Boomerang connectivity table |
| `research/structure/research2/impossible_diff_search.c` | Impossible differential search |
| `tests/distinguisher/test_dl.c` | Differential-linear distinguisher |

### Security Property Validation
| File | Description |
|------|-------------|
| `tests/s-boxes/sbox_analysis.c` | S-box cryptographic properties |
| `tests/audit/test_security.py` | Security audit suite |
| `research/structure/research2/sbox_security.c` | S-box security metrics |
| `research/structure/research2/avalanche_test.c` | SAC / BIC testing |
| `tests/crypto3/check_mds.py` | MDS branch number verification |

### Utility Scripts
| File | Description |
|------|-------------|
| `research/paper/paper3/generate_test_vector.py` | Test vector generation |
| `tests/s-boxes/test_z3_sbox.py` | Z3 S-box modeling |

## Repository Structure

The full source code, including all scripts listed above, is available at:
**https://github.com/effjy/krakken**

For questions or requests for specific scripts, please open an issue on GitHub or contact the author directly.
```
