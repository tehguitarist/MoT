#!/usr/bin/env python3
"""
r_solver_sympy.py — sympy port of Jatin Chowdhury's R-Solver
(https://github.com/jatinchowdhury18/R-Solver).

Derives the symbolic R-type scattering matrix for a circuit given as a netlist,
using sympy instead of Sage so it runs with a plain `pip install sympy` (no
multi-GB Sage install). Output is a C-style 2D array ready to paste into a
chowdsp_wdf ImpedanceCalculator's `setSMatrixData(...)`.

Netlist format (same as upstream R-Solver, 1-indexed nodes):
    Ra n1 n2 Ra        # resistor: WDF port, impedance symbol = label (3rd field)
    Va n1 n2           # voltage source / WDF port (positive node first)
    E1 np nm nip nin A # VCVS (ideal op-amp): out+ out- in+ in- gain
The number of resistors must equal the number of voltage sources (each pairs to
form one WDF port). VCVS elements add extra MNA unknowns (op-amp), parameterised
by gain A and the symbols Ri (input R) / Ro (output R) — substitute large A, large
Ri, small Ro in C++ for the ideal-op-amp limit.

Usage:
    python r_solver_sympy.py NETLIST [--datum 0] [--adapt N] [--out FILE]

This is a faithful reimplementation; see upstream r_solver.py / r_solver_utils/.
"""
import argparse
import re
import sys

import sympy as sp

RES_TYPE = "Res"
VOLTAGE_TYPE = "Vs"
VCVS_TYPE = "VCVS"


class Element:
    def __init__(self, type, node1, node2, impedance=None, port=None,
                 node3=None, node4=None, gain=None):
        self.type = type
        self.node1 = node1
        self.node2 = node2
        self.impedance = impedance
        self.port = port
        self.node3 = node3
        self.node4 = node4
        self.gain = gain


def parse_netlist(lines):
    num_nodes = 0
    num_voltages = 0
    num_resistors = 0
    num_extras = 0
    elements = []
    for raw in lines:
        l = raw.strip()
        if not l:
            continue
        port = None
        if l[0] == "R":
            el_type = RES_TYPE
            port = num_resistors
            num_resistors += 1
        elif l[0] == "V":
            el_type = VOLTAGE_TYPE
            port = num_voltages
            num_voltages += 1
        elif l[0] == "E":
            el_type = VCVS_TYPE
            num_extras += 1
        else:
            continue

        parts = l.split()
        el_node1 = int(parts[1])
        el_node2 = int(parts[2])
        num_nodes = max(num_nodes, el_node1, el_node2)

        if el_type == VCVS_TYPE:
            el_node3 = int(parts[3])
            el_node4 = int(parts[4])
            elements.append(Element(
                type=el_type, node1=el_node1 - 1, node2=el_node2 - 1,
                impedance=sp.Symbol(parts[0]), port=num_extras - 1,
                node3=el_node3 - 1, node4=el_node4 - 1,
                gain=sp.Symbol(parts[5])))
        else:
            # impedance symbol is the 3rd field for R, else the label itself
            imp_label = parts[3] if (el_type == RES_TYPE and len(parts) > 3) else parts[0]
            elements.append(Element(
                type=el_type, node1=el_node1 - 1, node2=el_node2 - 1,
                impedance=sp.Symbol(imp_label), port=port))

    assert num_resistors == num_voltages, \
        f"#resistors ({num_resistors}) must equal #voltage sources ({num_voltages})"
    return elements, num_nodes, num_voltages, num_extras


def stamp_resistor(X, r):
    g = 1 / r.impedance
    X[r.node1, r.node1] += g
    X[r.node2, r.node2] += g
    X[r.node1, r.node2] -= g
    X[r.node2, r.node1] -= g


def stamp_voltage(X, v, num_nodes):
    p = num_nodes + v.port
    X[p, v.node1] += 1
    X[p, v.node2] += -1
    X[v.node1, p] += 1
    X[v.node2, p] += -1


def stamp_vcvs(X, e, num_nodes, num_ports):
    Ri = sp.Symbol("Ri")
    Ro = sp.Symbol("Ro")
    extra = num_nodes + num_ports + e.port
    X[extra, e.node1] += -e.gain
    X[extra, e.node2] += e.gain
    X[extra, e.node3] += 1
    X[extra, e.node4] += -1
    X[e.node3, extra] += 1
    X[e.node4, extra] += -1
    g = 1 / Ri
    X[e.node1, e.node1] += g
    X[e.node2, e.node2] += g
    X[e.node1, e.node2] -= g
    X[e.node2, e.node1] -= g
    X[extra, extra] += Ro


def construct_X_matrix(elements, num_nodes, num_ports, num_extras):
    n = num_nodes + num_ports + num_extras
    X = sp.zeros(n, n)
    for el in elements:
        if el.type == RES_TYPE:
            stamp_resistor(X, el)
        elif el.type == VOLTAGE_TYPE:
            stamp_voltage(X, el, num_nodes)
        elif el.type == VCVS_TYPE:
            stamp_vcvs(X, el, num_nodes, num_ports)
    return X


def remove_datum_node(X, datum):
    keep = [i for i in range(X.rows) if i != datum]
    return X[keep, keep]


def compute_S_matrix(X_inv, elements, num_ports, num_extras):
    rows = X_inv.rows
    cols = X_inv.cols
    start = rows - (num_ports + num_extras)
    # rows/cols block selecting the port unknowns (the num_ports voltage-source rows)
    vert_id = sp.zeros(rows, num_ports)
    for i in range(num_ports):
        vert_id[start + i, i] = 1
    hor_id = sp.zeros(num_ports, cols)
    for i in range(num_ports):
        hor_id[i, start + i] = 1

    Rp = sp.zeros(num_ports, num_ports)
    for el in elements:
        if el.type == RES_TYPE:
            Rp[el.port, el.port] = el.impedance

    S = sp.eye(num_ports) + 2 * Rp * hor_id * X_inv * vert_id
    return S, Rp


def adapt_port(S, Rp, port):
    if port < 0 or port >= S.rows:
        raise IndexError("Port index out of range")
    S_nn = S[port, port]
    R_n = Rp[port, port]
    sols = sp.solve(sp.Eq(S_nn, 0), R_n)
    if not sols:
        raise RuntimeError("Could not solve for adapted port impedance")
    R_n_solved = sp.simplify(sols[0])
    print("\nAdapted Port Impedance:")
    print(f"{R_n} == {R_n_solved}")
    S_adapted = S.subs(R_n, R_n_solved)
    return S_adapted, (R_n, R_n_solved)


def matrix_to_c(S, num_ports, adapt_expr, argstr):
    comments = ("// Derived by r_solver_sympy.py (sympy port of "
                "https://github.com/jatinchowdhury18/R-Solver)\n"
                f"// invoked: {argstr}\n")
    prefix = f"const auto S_matrix[{num_ports}][{num_ports}] = {{"
    rows = []
    for i in range(S.rows):
        entries = []
        for j in range(S.cols):
            expr = sp.cancel(sp.together(S[i, j]))
            entries.append(c_expr(expr))
        rows.append("{ " + ", ".join(entries) + " }")
    body = ",\n    ".join(rows)
    out = comments + prefix + "\n    " + body + "\n};"
    if adapt_expr is not None:
        out += f"\n\n// adapted port impedance:\n// {adapt_expr[0]} = {c_expr(adapt_expr[1])}"
    return out


def c_expr(expr):
    """sympy expr -> C string. Expand powers a**2 -> a*a (C has no ^)."""
    s = sp.ccode(expr)
    # ccode emits pow(x, 2); convert simple integer powers of symbols to products
    s = re.sub(r"pow\(([A-Za-z_]\w*),\s*2\)", r"(\1*\1)", s)
    s = re.sub(r"pow\(([A-Za-z_]\w*),\s*3\)", r"(\1*\1*\1)", s)
    return s


def main():
    p = argparse.ArgumentParser(description="Derive R-type scattering matrix (sympy).")
    p.add_argument("netlist")
    p.add_argument("--datum", type=int, default=0)
    p.add_argument("--adapt", type=int, default=-1)
    p.add_argument("--out", default=None)
    p.add_argument("--verbose", action="store_true")
    args = p.parse_args()

    with open(args.netlist) as f:
        elements, num_nodes, num_ports, num_extras = parse_netlist(f.readlines())

    X = construct_X_matrix(elements, num_nodes, num_ports, num_extras)
    if args.verbose:
        sp.pprint(X)
    X = remove_datum_node(X, args.datum)
    X_inv = X.inv()
    S, Rp = compute_S_matrix(X_inv, elements, num_ports, num_extras)

    adapt_expr = None
    if args.adapt >= 0:
        S, adapt_expr = adapt_port(S, Rp, args.adapt)

    S = sp.simplify(S)
    argstr = "r_solver_sympy.py " + " ".join(sys.argv[1:])
    out = matrix_to_c(S, num_ports, adapt_expr, argstr)
    print("DONE!")
    if args.out:
        with open(args.out, "w") as f:
            f.write(out)
        print(f"written to {args.out}")
    else:
        print(out)


if __name__ == "__main__":
    main()
