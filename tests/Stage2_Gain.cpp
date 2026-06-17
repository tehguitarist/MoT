// Stage 2 gain / HPF / polarity — validation gate Step 4c.
//
// IC_B is an INVERTING amplifier with a first-order input high-pass:
//   H(s) = −R10 / (R9 + 1/(sC7)),  passband gain −R10/R9 = −22 (26.85 dB),
//   high-pass corner f_c = 1/(2π·R9·C7) = 159 Hz (C7=100nF, R9=10k).
// Verifies: passband |gain| ≈ 22×, −3 dB corner ≈ 159 Hz, INVERTING polarity, no NaN.

#include "../src/dsp/Stage2.h"

#include <cmath>
#include <cstdio>

namespace
{
constexpr double fs = 96000.0;

double measureMagDB (monarch::Stage2& s, double freq, double amp)
{
    s.reset();
    const int N = (int) fs, settle = (int) (fs * 0.2);
    double peak = 0.0;
    for (int n = 0; n < N; ++n)
    {
        const double y = s.processSample (amp * std::sin (2.0 * M_PI * freq * (double) n / fs));
        if (n > settle)
            peak = std::max (peak, std::abs (y));
    }
    return 20.0 * std::log10 (peak / amp);
}

// Signed gain via correlation with the input sine (at a freq >> f_c, where HPF phase ≈ 0).
// Negative ⇒ inverting.
double measureSignedGain (monarch::Stage2& s, double freq, double amp)
{
    s.reset();
    const int N = (int) fs, settle = (int) (fs * 0.2);
    double xy = 0.0, xx = 0.0;
    for (int n = 0; n < N; ++n)
    {
        const double x = amp * std::sin (2.0 * M_PI * freq * (double) n / fs);
        const double y = s.processSample (x);
        if (n > settle)
        {
            xy += x * y;
            xx += x * x;
        }
    }
    return xy / xx;
}
} // namespace

int main()
{
    monarch::Stage2 stage;
    stage.prepare (fs);
    const double amp = 0.01; // tiny — linear stage, keep well clear of any rails

    const double passDB = measureMagDB (stage, 2000.0, amp); // 2 kHz ≫ 159 Hz → full passband
    const double passGain = std::pow (10.0, passDB / 20.0);
    const double at159 = measureMagDB (stage, 159.0, amp);
    const double signed5k = measureSignedGain (stage, 5000.0, amp);

    // Find the −3 dB corner (relative to passband) by scanning.
    double corner = 0.0;
    double prev = -1e9;
    for (double f = 40.0; f <= 600.0; f += 1.0)
    {
        const double g = measureMagDB (stage, f, amp);
        if (g >= passDB - 3.0 && prev < passDB - 3.0)
            corner = f;
        prev = g;
    }

    std::printf ("Stage 2 (inverting, fs=%.0f)\n", fs);
    std::printf ("  passband gain (2 kHz): %.3f dB  (%.2f×, target −22× = 26.85 dB)\n", passDB, passGain);
    std::printf ("  gain @ 159 Hz: %.3f dB  (expect passband −3 dB = %.2f dB)\n", at159, passDB - 3.0);
    std::printf ("  measured −3 dB corner: %.0f Hz  (target 159 Hz)\n", corner);
    std::printf ("  signed gain @ 5 kHz: %.2f  (negative ⇒ inverting)\n", signed5k);

    const bool gainOk = std::abs (passGain - 22.0) < 1.0;          // ±1× of 22
    const bool cornerOk = std::abs (corner - 159.0) < 15.0;        // small bilinear warp ok
    const bool invOk = signed5k < 0.0;                            // inverting
    const bool nanOk = ! (std::isnan (passDB) || std::isnan (signed5k));
    const bool pass = gainOk && cornerOk && invOk && nanOk;

    std::printf ("\n  gain≈22×: %s | corner≈159Hz: %s | inverting: %s | no NaN: %s\n",
                 gainOk ? "ok" : "FAIL", cornerOk ? "ok" : "FAIL",
                 invOk ? "ok" : "FAIL", nanOk ? "ok" : "FAIL");
    std::printf ("%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
