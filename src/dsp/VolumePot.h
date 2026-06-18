#pragma once

#include <algorithm>
#include <cmath>

#include <chowdsp_wdf/chowdsp_wdf.h>

namespace monarch
{
namespace wdft = chowdsp::wdft;

/**
 * Volume + output stage (linear) — circuit.md Section 12.
 *
 *   VOL pot (100kA, AUDIO taper): top = node_T_out, bottom = BIAS, wiper → C11(1µF) → out.
 *   R14(1M) output bleed to GND. Output HPF f_c = 1/(2π·R14·C11) ≈ 0.16 Hz (DC block only).
 *
 * The VOL pot BODY load (100k top→bottom) on node_T_out is ALREADY modelled in ToneStage, so
 * this stage must NOT re-load node_T_out (dsp-validator Step-6 contract). The VOL wiper feeds
 * the high-impedance C11/R14 output, so the wiper tap is a simple voltage division of
 * node_T_out by the audio-taper fraction `pow(10, 2x−2)` ∈ [0.01, 1] — no further loading.
 * The C11/R14 output high-pass is then modelled as a small passive WDF (DC block).
 */
class VolumePot
{
public:
    static constexpr double C11 = 1.0e-6; // output coupling cap
    static constexpr double R14 = 1.0e6;  // output bleed

    VolumePot() { setVolume (0.5); }

    void prepare (double sampleRate)
    {
        c11.prepare (sampleRate);
        reset();
    }

    void reset() { c11.reset(); }

    /** VOL in [0,1], AUDIO taper. Wiper fraction = pow(10, 2x−2): x=1→1.0, x=0.5→0.1, x=0→0.01. */
    void setVolume (double vol01)
    {
        const auto x = std::min (1.0, std::max (0.0, vol01));
        volGain = std::pow (10.0, 2.0 * x - 2.0);
    }

    /** x = V(node_T_out); returns the stage output (post audio-taper tap + C11/R14 DC block). */
    inline double processSample (double x) noexcept
    {
        const double vWiper = x * volGain; // audio-taper wiper tap (feeds high-Z output → no loading)
        source.setVoltage (vWiper);
        source.incident (seriesC11.reflected());
        seriesC11.incident (source.reflected());
        // Output = V across R14 (the out node → GND), a passive read. C11/R14 = 0.16 Hz HPF.
        return chowdsp::wdft::voltage<double> (r14);
    }

private:
    double volGain { 0.1 };

    // C11/R14 output high-pass: Vwiper → C11 → out, R14 (out → GND). Read across R14.
    wdft::ResistorT<double> r14 { R14 };
    wdft::CapacitorT<double> c11 { C11 };
    wdft::WDFSeriesT<double, decltype (c11), decltype (r14)> seriesC11 { c11, r14 };
    wdft::IdealVoltageSourceT<double, decltype (seriesC11)> source { seriesC11 };
};

} // namespace monarch
