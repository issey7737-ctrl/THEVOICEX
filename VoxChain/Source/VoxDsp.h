#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

// VoxChain : chaine de traitement voix originale.
// Low-cut -> EQ 4 bandes -> De-esser -> Compresseur -> Saturation -> Doubleur / Delay / Reverb -> Sortie

namespace vx
{
inline double dbToLin (double d) noexcept { return std::pow (10.0, d / 20.0); }

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    inline double process (double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() noexcept { z1 = z2 = 0; }
    void raw (double B0, double B1, double B2, double A0, double A1, double A2) noexcept
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    void peaking (double fs, double f, double q, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        const double c = std::cos (w), al = std::sin (w) / (2.0 * q);
        raw (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double fs, double f, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        const double c = std::cos (w), al = std::sin (w) / std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        raw (A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
             (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
    }
    void highShelf (double fs, double f, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        const double c = std::cos (w), al = std::sin (w) / std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        raw (A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
             (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
    }
    void highPass (double fs, double f, double q) noexcept
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        const double c = std::cos (w), al = std::sin (w) / (2.0 * q);
        raw ((1 + c) * 0.5, -(1 + c), (1 + c) * 0.5, 1 + al, -2 * c, 1 - al);
    }
};

struct Params
{
    float inGain = 0, lowCut = 80, eqLow = 0, eqBody = 0, eqPres = 0, eqAir = 0;
    float deess = 0.3f;                                    // 0..1
    float compThr = -18, compRatio = 3, compAtk = 15, compRel = 150, compMakeup = 3, compMix = 1;
    float sat = 0.1f;                                      // 0..1
    float dbl = 0;                                         // 0..1
    float dlyTime = 250, dlyFb = 0.25f, dlyTone = 5000, dlyMix = 0;
    bool ping = true;
    float revSize = 0.4f, revDamp = 0.5f, revMix = 0.1f;   // 0..1
    float outGain = 0;
};

class Engine
{
public:
    std::atomic<float> inPk { 0.f }, outPk { 0.f }, grDb { 0.f };

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        maxBlk = std::max (64, maxBlock);
        wetL.assign ((size_t) maxBlk, 0.f);
        wetR.assign ((size_t) maxBlk, 0.f);
        dlyN = (int) (sr * 1.05) + 8;
        dlyL.assign ((size_t) dlyN, 0.0);
        dlyR.assign ((size_t) dlyN, 0.0);
        dblN = (int) (sr * 0.06) + 8;
        dblM.assign ((size_t) dblN, 0.0);
        dw = bw = 0;
        reverb.reset();
        reverb.prepare ({ sr, (juce::uint32) maxBlk, 2 });
        for (int c = 0; c < 2; ++c)
        {
            hp[c].reset(); dsHP[c].reset();
            dsHP[c].highPass (sr, 5500.0, 0.707);
            for (auto& b : eq[c]) b.reset();
        }
        lastLowCut = -1;
        lastEq.fill (1e9);
        cGr = 0; dsEnv = 0; dsG = 1; lpL = lpR = 0; lfo = 0; curD = -1;
        dsRel = std::exp (-1.0 / (0.030 * sr));
        dsSm = 1.0 - std::exp (-1.0 / (0.020 * sr));
        updateFilters();
    }

    void setParams (const Params& np) { p = np; }

    void process (float* l, float* r, int n)
    {
        while (n > 0)
        {
            const int m = std::min (n, maxBlk);
            chunk (l, r, m);
            l += m; r += m; n -= m;
        }
    }

private:
    void updateFilters()
    {
        if (std::abs ((double) p.lowCut - lastLowCut) > 0.01)
        {
            lastLowCut = p.lowCut;
            for (int c = 0; c < 2; ++c) hp[c].highPass (sr, (double) p.lowCut, 0.707);
        }
        const std::array<double, 4> g { (double) p.eqLow, (double) p.eqBody, (double) p.eqPres, (double) p.eqAir };
        for (int b = 0; b < 4; ++b)
        {
            if (std::abs (g[(size_t) b] - lastEq[(size_t) b]) < 0.001) continue;
            lastEq[(size_t) b] = g[(size_t) b];
            for (int c = 0; c < 2; ++c)
            {
                auto& f = eq[c][(size_t) b];
                if (b == 0) f.lowShelf (sr, 110.0, g[0]);
                else if (b == 1) f.peaking (sr, 320.0, 0.8, g[1]);
                else if (b == 2) f.peaking (sr, 3200.0, 0.9, g[2]);
                else f.highShelf (sr, 11000.0, g[3]);
            }
        }
    }

    static inline double readInterp (const std::vector<double>& b, int wpos, double d) noexcept
    {
        const int N = (int) b.size();
        double rp = (double) wpos - d;
        while (rp < 0) rp += N;
        const int i0 = (int) rp % N, i1 = (i0 + 1) % N;
        const double f = rp - std::floor (rp);
        return b[(size_t) i0] * (1.0 - f) + b[(size_t) i1] * f;
    }

    void chunk (float* l, float* r, int n)
    {
        updateFilters();
        const double inG = dbToLin ((double) p.inGain), outG = dbToLin ((double) p.outGain);
        const double atk = std::exp (-1.0 / (std::max (0.1, (double) p.compAtk) * 0.001 * sr));
        const double rel = std::exp (-1.0 / (std::max (1.0, (double) p.compRel) * 0.001 * sr));
        const double thr = (double) p.compThr, ratio = std::max (1.0, (double) p.compRatio), knee = 6.0;
        const double mk = dbToLin ((double) p.compMakeup), cmix = (double) p.compMix;
        const double dsThr = -8.0 - 32.0 * (double) p.deess;
        const double sat = (double) p.sat, drive = 1.0 + sat * 5.0;
        double pkIn = 0, minGr = 0;

        // ---- Passe 1 : low-cut, EQ, de-esser, compresseur, saturation
        for (int i = 0; i < n; ++i)
        {
            double xl = (double) l[i] * inG, xr = (double) r[i] * inG;
            pkIn = std::max (pkIn, std::max (std::abs (xl), std::abs (xr)));

            xl = hp[0].process (xl); xr = hp[1].process (xr);
            for (int b = 0; b < 4; ++b) { xl = eq[0][(size_t) b].process (xl); xr = eq[1][(size_t) b].process (xr); }

            // De-esser (reduit seulement les hautes frequences quand elles depassent le seuil)
            const double hl = dsHP[0].process (xl), hr = dsHP[1].process (xr);
            const double lvl0 = std::max (std::abs (hl), std::abs (hr));
            dsEnv = lvl0 > dsEnv ? lvl0 : lvl0 + (dsEnv - lvl0) * dsRel;
            double gTgt = 1.0;
            if (p.deess > 0.001f)
            {
                const double over = 20.0 * std::log10 (dsEnv + 1e-9) - dsThr;
                if (over > 0) gTgt = dbToLin (-std::min (over * 0.75, 12.0));
            }
            dsG += (gTgt - dsG) * (gTgt < dsG ? 0.3 : dsSm);
            xl -= hl * (1.0 - dsG);
            xr -= hr * (1.0 - dsG);

            // Compresseur (genou doux, detection crete liee)
            const double lvl = 20.0 * std::log10 (std::max (std::abs (xl), std::abs (xr)) + 1e-9);
            const double over = lvl - thr;
            double outLvl;
            if (2.0 * over < -knee) outLvl = lvl;
            else if (2.0 * std::abs (over) <= knee) { const double t = over + knee * 0.5; outLvl = lvl + (1.0 / ratio - 1.0) * t * t / (2.0 * knee); }
            else outLvl = thr + over / ratio;
            const double tgt = outLvl - lvl;
            const double co = tgt < cGr ? atk : rel;
            cGr = co * cGr + (1.0 - co) * tgt;
            minGr = std::min (minGr, cGr);
            const double cg = dbToLin (cGr) * mk;
            double yl = xl * (1.0 - cmix) + xl * cg * cmix;
            double yr = xr * (1.0 - cmix) + xr * cg * cmix;

            // Saturation douce
            if (sat > 0.001)
            {
                yl = yl * (1.0 - sat) + sat * std::tanh (drive * yl) / drive;
                yr = yr * (1.0 - sat) + sat * std::tanh (drive * yr) / drive;
            }
            l[i] = (float) yl; r[i] = (float) yr;
            wetL[(size_t) i] = (float) yl; wetR[(size_t) i] = (float) yr;
        }

        // ---- Reverb (envoi)
        const bool revOn = p.revMix > 0.001f;
        if (revOn)
        {
            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = p.revSize; rp.damping = p.revDamp; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 1.0f; rp.freezeMode = 0.0f;
            reverb.setParameters (rp);
            float* ch[2] = { wetL.data(), wetR.data() };
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (blk));
        }

        // ---- Passe 2 : doubleur, delay, retour reverb, sortie
        const double fb = juce::jlimit (0.0, 0.92, (double) p.dlyFb);
        const double lpA = 1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * (double) p.dlyTone / sr);
        const double tgtD = juce::jlimit (2.0, sr * 1.0, (double) p.dlyTime * 0.001 * sr);
        if (curD < 0) curD = tgtD;
        const double dmix = (double) p.dlyMix, rmix = (double) p.revMix * 0.3, dbl = (double) p.dbl * 0.55;
        double pkOut = 0;

        for (int i = 0; i < n; ++i)
        {
            const double dl = l[i], dr = r[i];
            double oL = dl, oR = dr;

            // Doubleur / elargisseur (copies retardees et legerement modulees)
            if (dbl > 0.001)
            {
                dblM[(size_t) bw] = 0.5 * (dl + dr);
                lfo += 2.0 * juce::MathConstants<double>::pi * 0.35 / sr;
                if (lfo > 6.283185307179586) lfo -= 6.283185307179586;
                const double tl = readInterp (dblM, bw, 0.017 * sr + 0.0012 * sr * std::sin (lfo));
                const double tr = readInterp (dblM, bw, 0.023 * sr + 0.0012 * sr * std::sin (lfo + 1.5707963));
                oL += dbl * tl; oR += dbl * tr;
                bw = (bw + 1) % dblN;
            }

            // Delay stereo (option ping-pong), filtre passe-bas dans la boucle de retour
            curD += (tgtD - curD) * 0.0015;
            const double yL = readInterp (dlyL, dw, curD), yR = readInterp (dlyR, dw, curD);
            lpL += lpA * (yL * fb - lpL);
            lpR += lpA * (yR * fb - lpR);
            if (p.ping) { dlyL[(size_t) dw] = 0.5 * (dl + dr) + lpR; dlyR[(size_t) dw] = lpL; }
            else        { dlyL[(size_t) dw] = dl + lpL;              dlyR[(size_t) dw] = dr + lpR; }
            dw = (dw + 1) % dlyN;
            oL += dmix * yL; oR += dmix * yR;

            if (revOn) { oL += rmix * (double) wetL[(size_t) i]; oR += rmix * (double) wetR[(size_t) i]; }

            oL *= outG; oR *= outG;
            if (! std::isfinite (oL)) oL = 0;
            if (! std::isfinite (oR)) oR = 0;
            oL = juce::jlimit (-4.0, 4.0, oL); oR = juce::jlimit (-4.0, 4.0, oR);
            pkOut = std::max (pkOut, std::max (std::abs (oL), std::abs (oR)));
            l[i] = (float) oL; r[i] = (float) oR;
        }

        inPk.store (std::max (inPk.load(), (float) pkIn));
        outPk.store (std::max (outPk.load(), (float) pkOut));
        grDb.store ((float) minGr);
    }

    double sr = 44100;
    int maxBlk = 512;
    Params p;
    std::array<Biquad, 2> hp, dsHP;
    std::array<std::array<Biquad, 4>, 2> eq;
    double lastLowCut = -1;
    std::array<double, 4> lastEq {};
    double cGr = 0, dsEnv = 0, dsG = 1, dsRel = 0, dsSm = 0;
    std::vector<double> dlyL, dlyR, dblM;
    int dlyN = 1, dblN = 1, dw = 0, bw = 0;
    double curD = -1, lpL = 0, lpR = 0, lfo = 0;
    std::vector<float> wetL, wetR;
    juce::dsp::Reverb reverb;
};
} // namespace vx
