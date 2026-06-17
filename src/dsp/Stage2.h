#pragma once

#include <chowdsp_wdf/chowdsp_wdf.h>

namespace monarch
{
namespace wdft = chowdsp::wdft;

/**
 * Stage 2 — IC_B, inverting amplifier (linear WDF, stock / no clipping).
 *
 * Topology (circuit.md Section 8, matsumin primary):
 *   - Inverting op-amp. Signal in via C7(100nF) + R9(10k) series into pin6(−); pin5(+) = BIAS.
 *     R10(220k) feedback (pin6− ↔ pin7). The op-amp holds pin6(−) at virtual ground (BIAS=0).
 *   - First-order HIGH-PASS with passband gain −R10/R9 = −22:
 *         H(s) = −R10 / (R9 + 1/(sC7))  →  corner f_c = 1/(2π·R9·C7) = 159 Hz, gain −22.
 *
 * **Inverting** — the inversion is carried by the op-amp VCVS terminal assignment in the
 * netlist (in+ = BIAS, in− = pin6−, so V(out) = −A·V(pin6−)) and the output sign below; no
 * separate PolarityInverterT is used in this VCVS-root-R-type approach (cf. Stage 1).
 *
 * Root R-type adaptor (ideal op-amp VCVS closes the loop), 2 ports:
 *     port a = input source path: Vin + R9 + C7 (ResistiveVoltageSource in series with C7)
 *     port b = R10 feedback resistor
 * Output = V(pin7) = −voltage(R10): pin6(−) is virtual ground (0 V), and R10 spans
 * pin6(−)→pin7, so V(pin7) = V(pin6−) − voltage(R10) = −voltage(R10). Read off a PASSIVE
 * port (never the source port — that averages Vs[n]/Vs[n−1] → HF droop; see dsp.md).
 *
 * Scattering matrix from tools/r_solver_sympy.py / tools/netlists/stage2.txt (ideal-op-amp
 * limit): S = [[-1, 0], [2·Rb/Ra, 1]], Ra = port-a impedance (R9 + C7), Rb = R10.
 *
 * SW-1 soft-clip diodes attach to the feedback (R10 ∥ [R11 + diode network]) in a later step.
 */
class Stage2
{
public:
    static constexpr double R9 = 10.0e3;     // input resistor
    static constexpr double R10 = 220.0e3;   // feedback resistor
    static constexpr double C7 = 100.0e-9;   // input coupling cap (HPF with R9 → 159 Hz)

    Stage2() = default;

    void prepare (double sampleRate)
    {
        c7.prepare (sampleRate); // propagates impedance → root R-type recomputes S-matrix
        reset();
    }

    void reset() { c7.reset(); }

    /** Process one sample (Volts in → Volts out at pin7). Linear, inverting (×−22). */
    inline double processSample (double x) noexcept
    {
        resVin.setVoltage (x);
        rtype.compute();
        // V(pin7) = voltage(R10) read off the passive feedback port (pin6− = virtual ground,
        // so the R10 port voltage IS V(pin7), already carrying the inverting sign from the
        // op-amp VCVS). Passive-port reconstruction — never read the source port (HF droop).
        return chowdsp::wdft::voltage<double> (r10);
    }

private:
    // ---- Port a: input source path  Vin → C7 → R9 → pin6(−) ----
    wdft::ResistiveVoltageSourceT<double> resVin { R9 }; // ideal source + series R9
    wdft::CapacitorT<double> c7 { C7 };
    wdft::WDFSeriesT<double, decltype (c7), decltype (resVin)> inputPort { c7, resVin };

    // ---- Port b: R10 feedback ----
    wdft::ResistorT<double> r10 { R10 };

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance (RType& R)
        {
            const auto imps = R.getPortImpedances();
            const double Ra = imps[0]; // port a = R9 + C7 wave impedance
            const double Rb = imps[1]; // port b = R10
            // Derived by tools/r_solver_sympy.py (ideal-op-amp limit) from netlists/stage2.txt.
            R.setSMatrixData ({ { -1.0, 0.0 },
                                { 2.0 * Rb / Ra, 1.0 } });
        }
    };

    using RType = wdft::RootRtypeAdaptor<double, ImpedanceCalc, decltype (inputPort), decltype (r10)>;
    RType rtype { inputPort, r10 };
};

} // namespace monarch
