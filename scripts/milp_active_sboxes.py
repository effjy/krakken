# milp_active_sboxes.py
#
# Formal MILP lower-bound search for active S-boxes
#
# This is the first REAL formal cryptanalysis step.
#
# It computes:
#   - minimum active S-boxes
#   - exact lower bounds per round
#   - provable differential trail constraints
#
# Uses:
#   pip install pulp
#
# Run:
#   python3 milp_active_sboxes.py
#
# --------------------------------------------------

from pulp import *

# ==================================================
# CONFIGURATION
# ==================================================

ROUNDS = 10
STATE_BYTES = 256
BRANCH_NUMBER = 9

# ==================================================
# MODEL
# ==================================================

model = LpProblem("WideTrailMILP", LpMinimize)

# S-box activity variables: x[r, i] = 1 if byte i of round r is active
x = {}
for r in range(ROUNDS + 1):
    for i in range(STATE_BYTES):
        x[r, i] = LpVariable(
            f"x_{r}_{i}",
            lowBound=0,
            upBound=1,
            cat=LpBinary
        )

# Column activity indicator variables: y[r, g] = 1 if column g in round r is active
y = {}
for r in range(1, ROUNDS + 1):
    for g in range(32):
        y[r, g] = LpVariable(
            f"y_{r}_{g}",
            lowBound=0,
            upBound=1,
            cat=LpBinary
        )

# ==================================================
# OBJECTIVE
# ==================================================
# Minimize total active S-boxes in the first ROUNDS rounds
model += lpSum(
    x[r, i]
    for r in range(ROUNDS)
    for i in range(STATE_BYTES)
)

# ==================================================
# NONZERO INPUT CONSTRAINT
# ==================================================
model += lpSum(x[0, i] for i in range(STATE_BYTES)) >= 1

# ==================================================
# BRANCH NUMBER CONSTRAINTS
# ==================================================
# Ink cloud mapping: maps S-box byte i of round r-1 to MDS input column index j of round r
def ink_cloud(i):
    return (29 * i + 71) % 256

def inv_ink_cloud(j):
    return (53 * j + 77) % 256

# Pi mapping: maps MDS output column index j of round r to S-box byte k of round r
def pi(j):
    return (37 * j + 17) % 256

# 32 columns of 8 bytes each
C = [[c * 32 + g for c in range(8)] for g in range(32)]

for r in range(1, ROUNDS + 1):
    for g in range(32):
        active_in = []
        active_out = []
        
        for j in C[g]:
            i = inv_ink_cloud(j)
            k = pi(j)
            
            active_in.append(x[r - 1, i])
            active_out.append(x[r, k])
            
            # Indicator constraints: if S-box active, column g of round r is active
            model += y[r, g] >= x[r - 1, i]
            model += y[r, g] >= x[r, k]
            
        # MDS Branch Number constraint: active_in + active_out >= 9 * y
        model += lpSum(active_in) + lpSum(active_out) >= BRANCH_NUMBER * y[r, g]

# ==================================================
# SOLVE
# ==================================================

print("========================================")
print("MILP ACTIVE S-BOX SEARCH")
print("========================================")

solver = PULP_CBC_CMD(msg=False)
model.solve(solver)

print("\nStatus:", LpStatus[model.status])

if model.status != 1:
    print("No feasible solution.")
    exit()

# ==================================================
# RESULTS
# ==================================================

total_active = 0
print("\nPer-round active S-boxes:\n")

for r in range(ROUNDS):
    active = 0
    for i in range(STATE_BYTES):
        if value(x[r, i]) > 0.5:
            active += 1
    total_active += active
    print(f"Round {r+1:2d} : {active}")

print("\n========================================")
print(f"MINIMUM ACTIVE S-BOXES = {total_active}")
print("========================================")

# ==================================================
# SHOW TRAIL
# ==================================================

print("\nExample minimal activity trail:\n")
for r in range(ROUNDS + 1):
    print(f"Round {r}:")
    for i in range(STATE_BYTES):
        v = int(value(x[r, i]) + 0.5)
        print(v, end=" ")
    print()

print("\n========================================")
