// Renders every preset at three STEAM settings and three sample rates and checks the chain stays sane.
#include "SaunaEngine.h"
#include <cstdio>
#include <vector>
#include <cstring>

static std::vector<float> testVoice (double sr, double seconds)
{
    std::vector<float> v ((size_t) (sr * seconds));
    double ph = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const double t = (double) i / sr, gate = std::fmod (t, 2.0) < 1.4 ? 1.0 : 0.0;
        const double f0 = 196.0 * (1 + 0.006 * std::sin (6.283185307 * 5.4 * t));
        ph += f0 / sr; ph -= std::floor (ph);
        double s = 0;
        for (int h = 1; h <= 20; ++h) s += std::sin (6.283185307 * ph * h) / (h * h) * (h == 4 || h == 6 ? 2.5 : 1.0);
        v[i] = (float) (0.2 * s * gate);
    }
    return v;
}

int main (int argc, char** argv)
{
    const bool verbose = argc > 1 && std::strcmp (argv[1], "-v") == 0;
    int failures = 0;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const auto in = testVoice (sr, 6.0);
        double inE = 0; for (float x : in) inE += (double) x * x;
        for (int p = 0; p < sauna::NUM_PRESETS; ++p)
            for (float steam : { 0.0f, 0.5f, 1.0f })
            {
                sauna::Engine e; e.prepare (sr, 512);
                e.setValues (sauna::applySteam (sauna::presetValues (p), steam));
                const size_t n = in.size() + (size_t) (sr * 6);
                std::vector<float> L (n), R (n), buf (n, 0.0f);
                std::copy (in.begin(), in.end(), buf.begin());
                for (size_t s = 0; s < n; s += 512) { const int c = (int) std::min<size_t> (512, n - s); e.process (buf.data() + s, buf.data() + s, L.data() + s, R.data() + s, c); }
                double peak = 0, energy = 0, tail = 0; bool finite = true;
                for (size_t i = 0; i < n; ++i)
                {
                    if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) finite = false;
                    peak = std::max (peak, (double) std::abs (L[i]));
                    if (i < in.size()) energy += (double) L[i] * L[i];
                    if (i > n - (size_t) sr) tail = std::max (tail, (double) std::abs (L[i]));
                }
                const double gainDb = 10 * std::log10 (energy / inE);
                const bool ok = finite && peak <= 0.892 && gainDb > -12 && gainDb < 12 && tail < 0.1;
                if (! ok) ++failures;
                if (! ok || (verbose && sr == 48000.0))
                    std::printf ("%s %6.0f Hz  %-30s steam %.1f  gain %+5.1f dB  peak %.3f  last-second %.4f\n", ok ? "  " : "FAIL", sr, sauna::PRESETS[p].name, steam, gainDb, peak, tail);
            }
    }
    std::printf (failures == 0 ? "All presets pass.\n" : "%d failures.\n", failures);
    return failures == 0 ? 0 : 1;
}
