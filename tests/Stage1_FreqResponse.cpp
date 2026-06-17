// Stage 1 frequency response — validation gate Step 4a.
//
// Verifies the non-inverting gain stage Av(s) = 1 + Z_upper(s)/Z_lower(s):
//   - low-frequency gain ≈ 0 dB (DC gain = 1, the op-amp DC servo),
//   - a gain peak in the few-kHz region (CCRMA Fig.6, mid-DRIVE),
//   - no NaN / instability.
// Measures the actual peak frequency/gain from the implemented WDF model (not assumed).

#include "../src/dsp/Stage1.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
constexpr double fs = 96000.0;

double measureGainDB (monarch::Stage1& stage, double freq, double amp)
{
    stage.reset();
    const int numSamples = (int) fs; // 1 second
    const int settle = (int) (fs * 0.2);
    double peak = 0.0;
    for (int n = 0; n < numSamples; ++n)
    {
        const double x = amp * std::sin (2.0 * M_PI * freq * (double) n / fs);
        const double y = stage.processSample (x);
        if (n > settle)
            peak = std::max (peak, std::abs (y));
    }
    return 20.0 * std::log10 (peak / amp);
}
} // namespace

int main()
{
    monarch::Stage1 stage;
    stage.prepare (fs);
    stage.setDrive (0.5); // mid DRIVE

    const double amp = 0.1; // small-signal, linear stage
    const std::vector<double> freqs = { 20, 50, 100, 200, 500, 1000, 2000, 3000,
                                        4000, 5000, 6000, 8000, 10000, 14000, 20000 };

    std::printf ("Stage 1 frequency response (mid DRIVE, fs=%.0f)\n", fs);
    std::printf ("  %8s  %8s\n", "freq[Hz]", "gain[dB]");

    double peakGain = -1e9, peakFreq = 0.0;
    bool nanSeen = false;
    for (double f : freqs)
    {
        const double g = measureGainDB (stage, f, amp);
        if (std::isnan (g) || std::isinf (g))
            nanSeen = true;
        std::printf ("  %8.0f  %8.3f\n", f, g);
        if (g > peakGain)
        {
            peakGain = g;
            peakFreq = f;
        }
    }

    // Refine the peak frequency with a fine sweep around the coarse argmax (the coarse grid
    // alone reports a round-number artifact, not the true peak).
    {
        double fineG = peakGain, fineF = peakFreq;
        for (double f = peakFreq - 1500.0; f <= peakFreq + 1500.0; f += 20.0)
        {
            if (f < 100.0)
                continue;
            const double g = measureGainDB (stage, f, amp);
            if (g > fineG)
            {
                fineG = g;
                fineF = f;
            }
        }
        peakGain = fineG;
        peakFreq = fineF;
    }

    // Unity-shelf probe: the gain-stage DC gain is 1 (op-amp DC servo). Above the C3 input
    // high-pass (≈16 Hz) but below the gain-stage rise (branch corners ≈482/590 Hz) there is
    // a ~0 dB shelf. Probe it at 40 Hz; the overall Stage 1 (incl. C3 HP) rolls below that.
    const double shelfGain = measureGainDB (stage, 40.0, amp);

    std::printf ("\n  unity-shelf gain (40 Hz): %.2f dB (expect ~0 dB — gain-stage DC gain = 1)\n", shelfGain);
    // Peak FREQ carries the small, expected bilinear warp (~−76 Hz @ 48k, less at higher fs)
    // vs the 3803 Hz analog peak (matsumin values, drive=0.5). CCRMA cite 4194 Hz for their
    // reference values + drive setting (the peak is drive-dependent).
    std::printf ("  peak gain: %.2f dB @ %.0f Hz  (analog 3803 Hz; bilinear-warped per fs)\n", peakGain, peakFreq);

    const bool shelfOk = std::abs (shelfGain) < 1.5;               // ~unity DC-servo shelf
    const bool peakOk = peakGain > 8.0 && peakGain < 20.0;          // mid-DRIVE Av ≈ 5× ≈ 14 dB
    const bool peakFreqOk = peakFreq >= 1000.0 && peakFreq <= 12000.0; // few-kHz region
    const bool pass = shelfOk && peakOk && peakFreqOk && ! nanSeen;

    // DRIVE direction: gain must INCREASE with DRIVE (corrected topology — DRIVE in Z_upper).
    std::printf ("\n  DRIVE sweep (gain @ 3 kHz):\n");
    double prevG = -1e9;
    bool driveMonotonic = true;
    for (double d : { 0.0, 0.25, 0.5, 0.75, 1.0 })
    {
        stage.setDrive (d);
        const double g = measureGainDB (stage, 3000.0, amp);
        std::printf ("    drive=%.2f : %6.2f dB\n", d, g);
        if (g < prevG - 0.05)
            driveMonotonic = false;
        prevG = g;
    }
    stage.setDrive (0.5);

    const bool pass2 = pass && driveMonotonic;
    std::printf ("\n  shelf~0dB: %s | peak 8-20dB: %s | peak 1-12kHz: %s | no NaN: %s | DRIVE↑gain↑: %s\n",
                 shelfOk ? "ok" : "FAIL", peakOk ? "ok" : "FAIL",
                 peakFreqOk ? "ok" : "FAIL", nanSeen ? "FAIL" : "ok",
                 driveMonotonic ? "ok" : "FAIL");
    std::printf ("%s\n", pass2 ? "PASS" : "FAIL");
    return pass2 ? 0 : 1;
}
