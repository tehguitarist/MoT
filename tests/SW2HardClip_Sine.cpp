// SW-2 hard clip — validation gate Step 5b.
//
// 1S1588 true-antiparallel pair shunting node_HC to BIAS via the always-present R12(1k).
// Verifies: near-unity small-signal gain (diodes off), SYMMETRIC HARD clipping clamped at
// ≈ ±0.584 V, a HARDER knee than SW-1 soft clip (output barely rises above onset), no NaN.

#include "../src/dsp/SW2HardClip.h"

#include <cmath>
#include <cstdio>

namespace
{
constexpr double fs = 96000.0;

struct Peaks
{
    double outPos, outNeg, signedGain;
};

Peaks measure (monarch::SW2HardClip& s, double amp)
{
    s.reset();
    const double f = 1000.0;
    const int N = (int) fs, settle = (int) (fs * 0.1);
    double pos = 0.0, neg = 0.0, xy = 0.0, xx = 0.0;
    for (int n = 0; n < N; ++n)
    {
        const double x = amp * std::sin (2.0 * M_PI * f * (double) n / fs);
        const double y = s.processSample (x);
        if (n > settle)
        {
            pos = std::max (pos, y);
            neg = std::min (neg, y);
            xy += x * y;
            xx += x * x;
        }
    }
    return { pos, -neg, xy / xx };
}
} // namespace

int main()
{
    monarch::SW2HardClip stage;
    stage.prepare (fs);

    const auto small = measure (stage, 0.01); // ≪ 0.584 V → diodes off, node_HC follows input
    const double ssGain = small.signedGain;

    std::printf ("SW-2 hard clip (fs=%.0f)\n", fs);
    std::printf ("  small-signal gain: %.3f  (target ≈ +1, diodes off)\n", ssGain);
    std::printf ("\n  transfer curve (1 kHz):\n");
    std::printf ("    %8s  %9s  %9s  %7s\n", "in_pk[V]", "out+[V]", "out-[V]", "sym%%");

    bool nanSeen = false, symOk = true;
    for (double a : { 0.01, 0.1, 0.3, 0.5, 0.584, 0.8, 1.0, 3.0, 10.0 })
    {
        const auto p = measure (stage, a);
        const double outAvg = 0.5 * (p.outPos + p.outNeg);
        const double sym = 100.0 * std::abs (p.outPos - p.outNeg) / std::max (1e-9, outAvg);
        if (std::isnan (outAvg))
            nanSeen = true;
        if (a >= 0.5 && sym > 5.0)
            symOk = false;
        std::printf ("    %8.3f  %9.4f  %9.4f  %7.2f\n", a, p.outPos, p.outNeg, sym);
    }

    // Clamp level at heavy overdrive.
    const auto at1 = measure (stage, 1.0);
    const auto at10 = measure (stage, 10.0);
    const double clamp1 = 0.5 * (at1.outPos + at1.outNeg);
    const double clamp10 = 0.5 * (at10.outPos + at10.outNeg);
    // Hardness: a 10× input increase (1 V → 10 V, i.e. +20 dB) should barely raise the clamp.
    // The diode's V = n·Vt·ln(I/Is) gives ≈ n·Vt·ln(10) ≈ 0.10 V rise → ~1.2× (a TRUE clamp:
    // ~1.6 dB out for 20 dB in). A soft feedback clipper (SW-1) rises far more.
    const double hardness = clamp10 / clamp1;

    std::printf ("\n  clamp @ 1 V in: %.4f V ; @ 10 V in: %.4f V  (target ≈ ±0.584 V)\n", clamp1, clamp10);
    std::printf ("  hardness (out@10V / out@1V): %.3f  (≈1.2 = diode-log clamp ⇒ hard)\n", hardness);

    const bool gainOk = ssGain > 0.9 && ssGain < 1.02;          // near-unity, non-inverting
    const bool clampOk = std::abs (clamp1 - 0.584) < 0.12;       // ≈ Vf_1S1588
    const bool hardOk = hardness < 1.30;                        // diode-log clamp (~1.2); SW-1 soft rises more
    const bool pass = gainOk && symOk && clampOk && hardOk && ! nanSeen;

    std::printf ("\n  gain≈+1: %s | symmetric: %s | clamp≈0.584V: %s | hard knee: %s | no NaN: %s\n",
                 gainOk ? "ok" : "FAIL", symOk ? "ok" : "FAIL", clampOk ? "ok" : "FAIL",
                 hardOk ? "ok" : "FAIL", nanSeen ? "FAIL" : "ok");
    std::printf ("%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
