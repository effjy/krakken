import pulp

def solve_krakken_milp(rounds=10):
    prob = pulp.LpProblem("Krakken_Active_Words", pulp.LpMinimize)
    
    # x[r][c][d] : active word in round r, column c, row d (0..7, 0..3)
    x = {}
    x_mds = {}
    y = {}
    
    for r in range(rounds + 1):
        for c in range(8):
            for d in range(4):
                x[r, c, d] = pulp.LpVariable(f"x_{r}_{c}_{d}", cat="Binary")
                if r < rounds:
                    x_mds[r, c, d] = pulp.LpVariable(f"x_mds_{r}_{c}_{d}", cat="Binary")
    
    for r in range(rounds):
        for d in range(4):
            y[r, d] = pulp.LpVariable(f"y_{r}_{d}", cat="Binary")
    
    # Objective: minimize total active words over all rounds
    prob += pulp.lpSum(x[r, c, d] for r in range(rounds) for c in range(8) for d in range(4))
    
    # Non‑zero input difference
    prob += pulp.lpSum(x[0, c, d] for c in range(8) for d in range(4)) >= 1
    
    for r in range(rounds):
        # MDS Tentacle layer applied to each row (d) across 8 columns
        for d in range(4):
            # y[r][d] is 1 if any word in row d is active before or after MDS
            for c in range(8):
                prob += y[r, d] >= x[r, c, d]
                prob += y[r, d] >= x_mds[r, c, d]
            # Branch number 9: active words before + after >= 9 * y
            prob += (pulp.lpSum(x[r, c, d] for c in range(8)) +
                     pulp.lpSum(x_mds[r, c, d] for c in range(8)) >= 9 * y[r, d])
        
        # Pi + Ink Cloud word permutation
        # Maps each (c,d) at MDS output to a new (c',d') in the next round
        for c in range(8):
            for d in range(4):
                # Pi: (c', d) = ((c + 3*d) mod 8, d)
                pi_c = (c + 3 * d) % 8
                pi_d = d
                # Ink Cloud: src index i = pi_c*4 + pi_d -> dest index j = (i*7) % 32
                src_idx = pi_c * 4 + pi_d
                dest_idx = (src_idx * 7) % 32
                dest_c = dest_idx // 4
                dest_d = dest_idx % 4
                prob += x[r+1, dest_c, dest_d] == x_mds[r, c, d]
    
    prob.solve(pulp.PULP_CBC_CMD(msg=False))
    print(f"Status: {pulp.LpStatus[prob.status]}")
    print(f"Minimum active words (10 rounds): {pulp.value(prob.objective)}")
    return pulp.value(prob.objective)

if __name__ == "__main__":
    solve_krakken_milp(10)
