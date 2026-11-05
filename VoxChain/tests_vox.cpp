// Test hors-ligne du moteur VoxChain
#include "Source/VoxDsp.h"
#include <cstdio>
#include <random>

using namespace vx;

static double rms (const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / (double) (b - a));
}

int main()
{
    int fails = 0;
    const double sr = 48000;

    // 1) Tous les modules actifs sur un signal "voix" (bruit filtre + salves de sibilance) : pas de NaN, niveau raisonnable
    {
        Engine e; e.prepare (sr, 512);
        Params p; p.inGain = 0; p.lowCut = 100; p.eqLow = 2; p.eqBody = -2; p.eqPres = 3; p.eqAir = 4; p.deess = 0.6f;
        p.compThr = -24; p.compRatio = 4.5f; p.compAtk = 8; p.compRel = 90; p.compMakeup = 6; p.compMix = 1; p.sat = 0.25f;
        p.dbl = 0.25f; p.dlyTime = 190; p.dlyFb = 0.3f; p.dlyMix = 0.15f; p.revSize = 0.5f; p.revMix = 0.15f;
        e.setParams (p);
        const size_t N = (size_t) (sr * 10);
        std::vector<float> L (N), R (N);
        std::mt19937 rng (3); std::normal_distribution<float> nd (0.f, 1.f);
        double a = 0, hpS = 0;
        for (size_t i = 0; i < N; ++i)
        {
            const double t = (double) i / sr;
            const double w = nd (rng);
            a += 0.05 * (w - a);                                     // corps (passe-bas)
            const double hi = w - hpS; hpS += 0.6 * (w - hpS);       // haut du spectre
            const double sib = (std::fmod (t, 1.0) > 0.5 && std::fmod (t, 1.0) < 0.6) ? 0.25 : 0.0; // "s"
            const double env = 0.5 + 0.5 * std::sin (2 * 3.14159265 * 1.7 * t);
            L[i] = R[i] = (float) (0.2 * env * a * 5 + sib * hi * 0.5);
        }
        double peakIn = 0; for (auto v : L) peakIn = std::max (peakIn, (double) std::abs (v));
        for (size_t k = 0; k < N; k += 512) e.process (L.data() + k, R.data() + k, (int) std::min<size_t> (512, N - k));
        double peak = 0; bool bad = false;
        for (size_t i = 0; i < N; ++i) { peak = std::max (peak, (double) std::max (std::abs (L[i]), std::abs (R[i]))); if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) bad = true; }
        std::printf ("voix: peakIn=%.2f peakOut=%.2f GR=%.1f dB nan=%d\n", peakIn, peak, e.grDb.load(), (int) bad);
        if (bad || peak > 3.5) ++fails;
    }

    // 2) Delay : une impulsion revient apres dlyTime
    {
        Engine e; e.prepare (sr, 512);
        Params p; p.lowCut = 20; p.deess = 0; p.compRatio = 1; p.compMakeup = 0; p.sat = 0; p.dlyTime = 100; p.dlyFb = 0; p.dlyMix = 1; p.revMix = 0; p.dbl = 0; p.ping = false;
        e.setParams (p);
        std::vector<float> L (24000, 0.f), R (24000, 0.f); L[1000] = R[1000] = 0.5f;
        for (size_t k = 0; k < L.size(); k += 512) e.process (L.data() + k, R.data() + k, (int) std::min<size_t> (512, L.size() - k));
        size_t at = 0; double best = 0;
        for (size_t i = 2000; i < L.size(); ++i) if (std::abs (L[i]) > best) { best = std::abs (L[i]); at = i; }
        std::printf ("delay: echo a l'index %zu (attendu ~%d), amplitude %.3f\n", at, 1000 + 4800, best);
        if (std::abs ((int) at - 5800) > 8 || best < 0.2) ++fails;
    }

    // 3) Compresseur : un signal fort est reduit (GR < 0), un signal faible ne l'est pas
    {
        Engine e; e.prepare (sr, 512);
        Params p; p.lowCut = 20; p.deess = 0; p.compThr = -20; p.compRatio = 6; p.compAtk = 2; p.compRel = 100; p.compMakeup = 0; p.sat = 0; p.dlyMix = 0; p.revMix = 0; p.dbl = 0;
        e.setParams (p);
        std::vector<float> L (48000), R;
        for (size_t i = 0; i < L.size(); ++i) L[i] = (float) (0.7 * std::sin (2 * 3.14159265 * 220.0 * (double) i / sr));
        R = L;
        for (size_t k = 0; k < L.size(); k += 512) e.process (L.data() + k, R.data() + k, (int) std::min<size_t> (512, L.size() - k));
        const double rIn = 0.7 / std::sqrt (2.0), rOut = rms (L, 24000, 48000);
        std::printf ("comp: entree %.3f -> sortie %.3f (GR %.1f dB)\n", rIn, rOut, e.grDb.load());
        if (! (rOut < rIn * 0.7) || e.grDb.load() > -3.0f) ++fails;
    }

    // 4) Reverb : de l'energie reste apres l'arret du signal
    {
        Engine e; e.prepare (sr, 512);
        Params p; p.lowCut = 20; p.deess = 0; p.compRatio = 1; p.compMakeup = 0; p.sat = 0; p.dlyMix = 0; p.revMix = 1; p.revSize = 0.7f; p.dbl = 0;
        e.setParams (p);
        std::vector<float> L (48000, 0.f), R (48000, 0.f);
        for (size_t i = 0; i < 4800; ++i) L[i] = R[i] = (float) (0.3 * std::sin (2 * 3.14159265 * 440.0 * (double) i / sr));
        for (size_t k = 0; k < L.size(); k += 512) e.process (L.data() + k, R.data() + k, (int) std::min<size_t> (512, L.size() - k));
        const double tail = rms (L, 9600, 14400);
        std::printf ("reverb: queue rms=%.4f\n", tail);
        if (tail < 1e-3) ++fails;
    }

    // 5) Blocs de taille variable (plus grands que le maximum annonce)
    {
        Engine e; e.prepare (sr, 256);
        Params p; e.setParams (p);
        std::vector<float> L (5000, 0.1f), R (5000, 0.1f);
        e.process (L.data(), R.data(), 5000);
        bool ok = true; for (auto v : L) if (! std::isfinite (v)) ok = false;
        std::printf ("gros bloc: %s\n", ok ? "ok" : "ECHEC");
        if (! ok) ++fails;
    }

    std::printf (fails ? "ECHEC (%d)\n" : "TOUS LES TESTS OK\n", fails);
    return fails;
}
