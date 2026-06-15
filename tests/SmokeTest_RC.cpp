#include <chowdsp_wdf/chowdsp_wdf.h>

#include <cmath>
#include <cstdio>

using namespace chowdsp::wdft;

namespace
{
constexpr double fs = 44100.0;
constexpr double fc = 1000.0;
constexpr double capValue = 100.0e-9;
constexpr double resValue = 1.0 / (2.0 * M_PI * fc * capValue);

template <typename Vs, typename P1, typename C1>
double measureMagnitudeDB (double freq, Vs& vs, P1& p1, C1& c1)
{
    c1.reset();

    double magnitude = 0.0;
    const auto numSamples = (int) fs;
    for (int n = 0; n < numSamples; ++n)
    {
        const auto x = std::sin (2.0 * M_PI * freq * (double) n / fs);
        vs.setVoltage (x);

        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        const auto y = voltage<double> (c1);
        if (n > 1000)
            magnitude = std::max (magnitude, std::abs (y));
    }

    return 20.0 * std::log10 (magnitude);
}
} // namespace

int main()
{
    CapacitorT<double> c1 (capValue, fs);
    ResistorT<double> r1 (resValue);

    auto s1 = makeSeries<double> (r1, c1);
    auto p1 = makeInverter<double> (s1);
    IdealVoltageSourceT<double, decltype (p1)> vs { p1 };

    const auto magAtFc = measureMagnitudeDB (fc, vs, p1, c1);

    std::printf ("RC lowpass smoke test\n");
    std::printf ("  R = %.2f ohm, C = %.2e F\n", resValue, capValue);
    std::printf ("  Theoretical -3dB corner: %.1f Hz\n", fc);
    std::printf ("  Measured magnitude at corner: %.3f dB (expected -3.0 dB)\n", magAtFc);

    const auto pass = std::abs (magAtFc - (-3.0)) < 0.2;
    std::printf ("%s\n", pass ? "PASS" : "FAIL");

    return pass ? 0 : 1;
}
