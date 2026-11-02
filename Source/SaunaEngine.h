// Belgian Sauna — the signal chain. Seven original Airwindows effects in series, then our mix, gain and limiter.
#pragma once
#include "SaunaModel.h"
#include "airwindows/AirwindowsUnits.h"
#include <array>
#include <memory>
#include <vector>

namespace sauna
{
class Engine
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        units[TUBE2] = aw::makeTube2();
        units[PRESSURE4] = aw::makePressure4();
        units[AIR3] = aw::makeAir3();
        units[TOTAPE6] = aw::makeToTape6();
        units[ENSEMBLE] = aw::makeEnsemble();
        units[TAPEDELAY2] = aw::makeTapeDelay2();
        units[GALACTIC] = aw::makeGalactic();
        for (int u = 0; u < OUTPUT; ++u) units[(size_t) u]->setSampleRate (sr);
        block = std::max (32, maxBlock);
        bufL.assign ((size_t) block, 0.0); bufR.assign ((size_t) block, 0.0);
        dryL.assign ((size_t) block, 0.0); dryR.assign ((size_t) block, 0.0);
        revL.assign ((size_t) block, 0.0); revR.assign ((size_t) block, 0.0);
        agK = 1.0 - std::exp (-1.0 / (0.4 * sr)); agIn = agOut = 0; ag = 1;
        limG = 1.0; limRel = 1.0 - std::exp (-1.0 / (0.12 * sr));
        mixS = -1; gainS = -1; revS = -1;
        needsCalibration = false;
        setValues (current);
        needsCalibration = true;   // the next setValues (the real settings) calibrates the level matcher
    }

    void setValues (const Values& v)
    {
        current = v;
        for (int i = 0; i < NUM_KNOBS; ++i)
        {
            const auto& k = KNOBS[i];
            if (k.awParam >= 0 && units[(size_t) k.unit]) units[(size_t) k.unit]->setParameter (k.awParam, v.v[i]);
        }
        // Galactic runs 100% wet on a send so the dry vocal always stays at full level.
        if (units[GALACTIC]) units[GALACTIC]->setParameter (4, 1.0f);
        revT = v.on[GALACTIC] ? v.v[knobIndex ("cosmosMix")] * 1.1 : 0.0;
        if (revS < 0) revS = revT;
        const int mixI = knobIndex ("outMix"), gainI = knobIndex ("outGain");
        const bool outOn = v.on[NUM_SECTIONS - 1];
        mixT = outOn ? v.v[mixI] : 1.0;
        gainT = outOn ? v.v[gainI] * 1.25 : 1.0;
        if (mixS < 0) { mixS = mixT; gainS = gainT; }
        if (needsCalibration) { needsCalibration = false; calibrate(); }
    }

    void setBypassed (bool b) { bypassed = b; }

    // Runs a short sung test vowel through fresh copies of the colour stages so the level matcher
    // starts at the right gain instead of jumping at the first note. Called once, outside the audio thread.
    void calibrate()
    {
        std::unique_ptr<AudioEffectX> t[4] = { aw::makeTube2(), aw::makePressure4(), aw::makeAir3(), aw::makeToTape6() };
        for (auto& u : t) u->setSampleRate (sr);
        for (int i = 0; i < NUM_KNOBS; ++i)
            if (KNOBS[i].awParam >= 0 && KNOBS[i].unit <= TOTAPE6) t[KNOBS[i].unit]->setParameter (KNOBS[i].awParam, current.v[i]);
        const int n = (int) (0.6 * sr);
        std::vector<double> l ((size_t) n), r ((size_t) n);
        double ph = 0, inE = 0, outE = 0;
        for (int i = 0; i < n; ++i)
        {
            ph += 196.0 / sr; ph -= std::floor (ph);
            double s = 0;
            for (int h = 1; h <= 20; ++h) s += std::sin (6.283185307179586 * ph * h) / (h * h) * (h == 4 || h == 6 ? 2.5 : 1.0);
            l[(size_t) i] = r[(size_t) i] = 0.15 * s;
        }
        for (int i = n / 3; i < n; ++i) inE += l[(size_t) i] * l[(size_t) i];
        double* io[2] = { l.data(), r.data() };
        for (int u = 0; u < 4; ++u) if (current.on[u]) t[u]->processDoubleReplacing (io, io, n);
        for (int i = n / 3; i < n; ++i) outE += l[(size_t) i] * l[(size_t) i];
        ag = outE > 1e-12 ? std::clamp (std::sqrt (inE / outE), 0.25, 2.0) : 1.0;
    }

    struct Meters { float in = 0, out = 0, gr = 1; };
    Meters takeMeters() { Meters m { mIn, mOut, mGr }; mIn = mOut = 0; mGr = 1; return m; }

    template <typename F>
    void process (const F* inL, const F* inR, F* outL, F* outR, int n)
    {
        for (int start = 0; start < n; start += block)
        {
            const int c = std::min (block, n - start);
            double* L = bufL.data(); double* R = bufR.data();
            for (int i = 0; i < c; ++i)
            {
                double l = (double) inL[start + i], r = (double) inR[start + i];
                if (! std::isfinite (l)) l = 0;
                if (! std::isfinite (r)) r = 0;
                L[i] = dryL[(size_t) i] = l; R[i] = dryR[(size_t) i] = r;
                mIn = std::max (mIn, (float) std::max (std::abs (l), std::abs (r)));
            }
            if (! bypassed)
            {
                double* io[2] = { L, R };
                // Colour stages, then automatic level matching so drive and tape change the tone, not the loudness.
                for (int u = TUBE2; u <= TOTAPE6; ++u)
                    if (current.on[u]) units[(size_t) u]->processDoubleReplacing (io, io, c);
                for (int i = 0; i < c; ++i)
                {
                    const double di = (dryL[(size_t) i] * dryL[(size_t) i] + dryR[(size_t) i] * dryR[(size_t) i]) * 0.5;
                    const double dout = (L[i] * L[i] + R[i] * R[i]) * 0.5;
                    agIn += (di - agIn) * agK; agOut += (dout - agOut) * agK;
                    if (agIn > 1e-6 && agOut > 1e-9)
                    {
                        const double want = std::clamp (std::sqrt (agIn / agOut), 0.25, 2.0);
                        ag += (want - ag) * (want < ag ? agK * 12.0 : agK * 2.0);
                    }
                    L[i] *= ag; R[i] *= ag;
                }
                for (int u = ENSEMBLE; u <= TAPEDELAY2; ++u)
                    if (current.on[u]) units[(size_t) u]->processDoubleReplacing (io, io, c);
                if (current.on[GALACTIC] || revS > 1e-4)
                {
                    std::copy (L, L + c, revL.begin()); std::copy (R, R + c, revR.begin());
                    double* rv[2] = { revL.data(), revR.data() };
                    units[GALACTIC]->processDoubleReplacing (rv, rv, c);
                    const double k = 1.0 - std::exp (-1.0 / (0.03 * sr));
                    for (int i = 0; i < c; ++i)
                    {
                        revS += (revT - revS) * k;
                        L[i] += revL[(size_t) i] * revS; R[i] += revR[(size_t) i] * revS;
                    }
                }
            }
            const double k = 1.0 - std::exp (-1.0 / (0.02 * sr));
            for (int i = 0; i < c; ++i)
            {
                mixS += ((bypassed ? 0.0 : mixT) - mixS) * k;
                gainS += ((bypassed ? 1.0 : gainT) - gainS) * k;
                double l = (dryL[(size_t) i] + (L[i] - dryL[(size_t) i]) * mixS) * gainS;
                double r = (dryR[(size_t) i] + (R[i] - dryR[(size_t) i]) * mixS) * gainS;
                if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; }
                // Output protection: instant gain reduction above -1 dBFS, smooth release, then a hard ceiling.
                const double pk = std::max (std::abs (l), std::abs (r)), ceil = 0.891;
                const double want = pk > ceil ? ceil / pk : 1.0;
                limG = want < limG ? want : limG + (1.0 - limG) * limRel;
                l = std::clamp (l * limG, -ceil, ceil); r = std::clamp (r * limG, -ceil, ceil);
                outL[start + i] = (F) l; outR[start + i] = (F) r;
                mOut = std::max (mOut, (float) std::max (std::abs (l), std::abs (r)));
                mGr = std::min (mGr, (float) limG);
            }
        }
    }

private:
    double sr = 48000;
    int block = 512;
    std::array<std::unique_ptr<AudioEffectX>, OUTPUT> units;
    std::vector<double> bufL, bufR, dryL, dryR, revL, revR;
    double agK = 0.001, agIn = 0, agOut = 0, ag = 1, revT = 0, revS = -1;
    Values current = defaults();
    bool bypassed = false, needsCalibration = true;
    double mixT = 1, gainT = 1, mixS = -1, gainS = -1, limG = 1, limRel = 0.001;
    float mIn = 0, mOut = 0, mGr = 1;
};
} // namespace sauna
