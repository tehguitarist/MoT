#pragma once

#include <chowdsp_wdf/chowdsp_wdf.h>

namespace monarch
{
namespace wdft = chowdsp::wdft;

/**
 * SW-2 hard clip — 1S1588 shunt clipper at node_HC (Distortion mode). Nonlinear WDF.
 *
 * Topology (circuit.md Section 10, 16):
 *   - IC_B output (pin7) → R12(1k, always present) → node_HC.
 *   - SW-2 = 1S1588 ×2 TRUE ANTIPARALLEL pair (D6 ∥ D7) shunting node_HC → BIAS (0 V).
 *   - node_HC clamps at ±Vf_1S1588 ≈ ±0.584 V → HARD clipping (sharper knee than SW-1).
 *
 * This is a classic series-R + shunt-diode clipper: the upstream signal V(pin7) drives, with
 * R12 in series, a diode pair shunting the node to ground. WDF: a `ResistiveVoltageSourceT`
 * carries V(pin7) with series R12; the `DiodePairT` (1S1588) is the nonlinear ROOT.
 *     V(pin7) ─[Vs+R12]─ node_HC ─ DiodePairT(root, shunt to 0)
 * Output = V(node_HC) = the diode-pair voltage (read across the root — not the source port).
 *
 * 1S1588 = 1N914 = 1N4148: single diode each direction (true antiparallel, NOT series
 * strings), so the ideality is n_1S1588 (passed as the DiodePairT `nDiodes` arg, which scales
 * Vt). Validated by symmetric HARD clipping with onset ≈ 0.584 V.
 */
class SW2HardClip
{
public:
    static constexpr double R12 = 1.0e3; // always-present Stage 2 output series R

    // 1S1588 (= 1N914 = 1N4148) — circuit.md Section 16.
    static constexpr double Is_1S1588 = 2.52e-9;
    static constexpr double n_1S1588 = 1.752;
    static constexpr double Vt = 25.85e-3;

    SW2HardClip() = default;

    void prepare (double /*sampleRate*/) { reset(); }

    void reset() {}

    /** Process one sample. x = IC_B output V(pin7); returns V(node_HC) (hard-clipped). */
    inline double processSample (double x) noexcept
    {
        src.setVoltage (x);
        dp.incident (src.reflected());
        src.incident (dp.reflected());
        // V(node_HC) = voltage across the shunt diode pair (the root). Not the source port.
        return chowdsp::wdft::voltage<double> (dp);
    }

private:
    wdft::ResistiveVoltageSourceT<double> src { R12 }; // V(pin7) + series R12 → node_HC
    wdft::DiodePairT<double, decltype (src), wdft::DiodeQuality::Best> dp { src, Is_1S1588, Vt, n_1S1588 };
};

} // namespace monarch
