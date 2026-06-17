#pragma once

#include <algorithm>
#include <cmath>

#include <chowdsp_wdf/chowdsp_wdf.h>

namespace monarch
{
namespace wdft = chowdsp::wdft;

/**
 * Stage 1 — IC_A, non-inverting gain stage (linear WDF).
 *
 * Topology (circuit.md Section 6, matsumin primary):
 *   - Input network: Vin → C3(10n) → pin3(+); R4(1M) pin3(+)→BIAS. Modelled as a separate
 *     first-order high-pass sub-filter producing V(pin3+) — pin3(+) is the op-amp's
 *     high-impedance non-inverting input, so it is decoupled from the feedback network.
 *     (R5 1M input pulldown sits across the ideal source → no effect; omitted.)
 *   - Gain stage: ideal op-amp (VCVS) closing a root R-type adaptor with 5 ports:
 *       port a = input source at pin3(+)         (ResistiveVoltageSource; Ra unused in
 *                                                  the ideal-op-amp limit)
 *       port b = Z_lower Branch1: R7(33k)+C5(10n) series, NodeF→GND
 *       port c = Z_lower Branch2: R8(27k)+C6(10n) series, NodeF→GND
 *       port d = Z_upper cap C4(100p),            NodeF→NodeG
 *       port e = Z_upper resistor R6(10k)+DRIVE,  NodeF→NodeG
 *   - Output = V(NodeG) = op-amp output, reconstructed from V(pin3+) and the Z_upper drop.
 *
 * Non-inverting → NO PolarityInverterT. Av(s) = 1 + Z_upper(s)/Z_lower(s); DC gain = 1.
 *
 * The R-type scattering matrix was derived with tools/r_solver_sympy.py from
 * tools/netlists/stage1.txt, with the ideal-op-amp limit (A→∞, Ri→∞, Ro→0) applied.
 */
class Stage1
{
public:
    static constexpr double R6_floor = 10.0e3;   // DRIVE floor resistor
    static constexpr double DRIVE_max = 100.0e3; // 100kB linear

    Stage1() { setDrive (0.5); }

    void prepare (double sampleRate)
    {
        const auto fs = sampleRate;
        // Each cap's prepare() propagates the impedance change up to the root R-type adaptor,
        // recomputing its scattering matrix. Standard bilinear — accurate at base rate
        // (residual warping ≈ −74 Hz @ 48k on the ~3.8 kHz gain peak).
        c3.prepare (fs);
        c5.prepare (fs);
        c6.prepare (fs);
        c4.prepare (fs);
        reset();
    }

    void reset()
    {
        c3.reset();
        c5.reset();
        c6.reset();
        c4.reset();
    }

    /** DRIVE in [0,1], linear (100kB taper applied here). */
    void setDrive (double drive01)
    {
        const auto rDrive = DRIVE_max * std::min (1.0, std::max (0.0, drive01));
        driveR.setResistanceValue (R6_floor + rDrive); // propagates → R-type recomputes S
    }

    /** Process one sample (Volts in → Volts out at NodeG). Stage 1 is linear. */
    inline double processSample (double x) noexcept
    {
        // 1. Input high-pass sub-filter → V(pin3+).
        vin.setVoltage (x);
        vin.incident (inputInv.reflected());
        inputInv.incident (vin.reflected());
        const double vPlus = chowdsp::wdft::voltage<double> (r4);

        // 2. Drive the gain-stage non-inverting input and scatter the root R-type.
        vPlusPort.setVoltage (vPlus);
        rtype.compute();

        // 3. Output = V(NodeG). Reconstruct from two PASSIVE port voltages so V(NodeF)
        //    cancels exactly: branch1 spans NodeF→GND (= V(NodeF)); driveR spans
        //    NodeF→NodeG (= V(NodeF) − V(NodeG)). Both carry the same-frame V(NodeF), so
        //    V(NodeG) = voltage(branch1) − voltage(driveR). (Reading the SOURCE port instead
        //    averages Vs[n] and Vs[n−1] → a spurious 2-point low-pass / HF droop.)
        return chowdsp::wdft::voltage<double> (branch1) - chowdsp::wdft::voltage<double> (driveR);
    }

private:
    // ---- Input high-pass sub-filter: Vin → C3 → [R4 → BIAS], read V across R4 ----
    wdft::CapacitorT<double> c3 { 10.0e-9 };
    wdft::ResistorT<double> r4 { 1.0e6 };
    wdft::WDFSeriesT<double, decltype (c3), decltype (r4)> inputSeries { c3, r4 };
    wdft::PolarityInverterT<double, decltype (inputSeries)> inputInv { inputSeries };
    wdft::IdealVoltageSourceT<double, decltype (inputInv)> vin { inputInv };

    // ---- Gain-stage R-type ports (order must match tools/netlists/stage1.txt: a,b,c,d,e) ----
    wdft::ResistiveVoltageSourceT<double> vPlusPort { 1.0e3 }; // port a (Ra unused in ideal limit)
    wdft::ResistorT<double> r7 { 33.0e3 };
    wdft::CapacitorT<double> c5 { 10.0e-9 };
    wdft::WDFSeriesT<double, decltype (r7), decltype (c5)> branch1 { r7, c5 }; // port b
    wdft::ResistorT<double> r8 { 27.0e3 };
    wdft::CapacitorT<double> c6 { 10.0e-9 };
    wdft::WDFSeriesT<double, decltype (r8), decltype (c6)> branch2 { r8, c6 }; // port c
    wdft::CapacitorT<double> c4 { 100.0e-12 };                                 // port d
    wdft::ResistorT<double> driveR { R6_floor + 0.5 * DRIVE_max };             // port e (R6+DRIVE)

    struct ImpedanceCalc
    {
        template <typename RType>
        static void calcImpedance (RType& R)
        {
            const auto imps = R.getPortImpedances();
            // imps[0] = Ra (port a) is unused: in the ideal-op-amp limit pin3(+) is decoupled.
            const double Rb = imps[1];
            const double Rc = imps[2];
            const double Rd = imps[3];
            const double Re = imps[4];

            const double S30 = (-2.0 * Rb * Rd * Re - 2.0 * Rc * Rd * Re) / (Rb * Rc * Rd + Rb * Rc * Re);
            const double S31 = 2.0 * Rd * Re / (Rb * Rd + Rb * Re);
            const double S32 = 2.0 * Rd * Re / (Rc * Rd + Rc * Re);
            const double S33 = (-Rd + Re) / (Rd + Re);
            const double S34 = 2.0 * Rd / (Rd + Re);
            const double S43 = 2.0 * Re / (Rd + Re);
            const double S44 = (Rd - Re) / (Rd + Re);

            // Derived by tools/r_solver_sympy.py (ideal-op-amp limit) from tools/netlists/stage1.txt.
            R.setSMatrixData ({ { 1.0, 0.0, 0.0, 0.0, 0.0 },
                                { 2.0, -1.0, 0.0, 0.0, 0.0 },
                                { 2.0, 0.0, -1.0, 0.0, 0.0 },
                                { S30, S31, S32, S33, S34 },
                                { S30, S31, S32, S43, S44 } });
        }
    };

    using RType = wdft::RootRtypeAdaptor<double, ImpedanceCalc,
                                         decltype (vPlusPort), decltype (branch1),
                                         decltype (branch2), decltype (c4), decltype (driveR)>;
    RType rtype { vPlusPort, branch1, branch2, c4, driveR };
};

} // namespace monarch
