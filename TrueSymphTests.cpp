// TrueSymphTests.cpp - automatiska tester av ljudmotorn (körs vid varje bygge).
#include "TrueSymphDSP.h"
#include "Presets.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace truesymph;

static int failures = 0;
static void check(bool ok, const char* name)
{
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}

static double cents(double a, double b) { return 1200.0 * std::log2(a / b); }

// Ren motor: inga effekter som stör mätningar
static void plain(Engine& e)
{
    e.setEnsembleNorm(0.0);
    e.setVibDepthNorm(0.0);
    e.setModeNorm(0.0);
    e.setSlowAttackNorm(0.0);
    e.setSustainNorm(0.0);
    e.setStringNorm(0, 0.0); e.setStringNorm(1, 1.0); e.setStringNorm(2, 0.0);
    e.setBrillianceNorm(0.5);
    e.setTuneNorm(normFromTune(440.0));
}

static std::vector<double> render(Engine& e, double sec, double sr)
{
    std::vector<double> v((size_t)(sec * sr));
    for (size_t i = 0; i < v.size(); ++i) v[i] = e.next();
    return v;
}

static double rms(const std::vector<double>& v, size_t a, size_t b)
{
    double s = 0.0; if (b > v.size()) b = v.size();
    for (size_t i = a; i < b; ++i) s += v[i] * v[i];
    return b > a ? std::sqrt(s / (double)(b - a)) : 0.0;
}

static double peak(const std::vector<double>& v)
{
    double p = 0.0; for (double x : v) p = std::fmax(p, std::fabs(x)); return p;
}

static bool finite(const std::vector<double>& v) { for (double x : v) if (!std::isfinite(x)) return false; return true; }

// Grundtonens frekvens: DFT-sökning runt en förväntad frekvens (fint steg).
static double measurePitch(const std::vector<double>& v, size_t a, size_t n, double sr, double guess)
{
    double best = guess, bestMag = -1.0;
    for (double c = -30.0; c <= 30.0; c += 0.25) {
        const double f = guess * std::pow(2.0, c / 1200.0);
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) {
            const double w = 0.5 - 0.5 * std::cos(2.0 * kPi * i / (n - 1)); // Hann
            const double ph = 2.0 * kPi * f * i / sr;
            re += w * v[a + i] * std::cos(ph); im += w * v[a + i] * std::sin(ph);
        }
        const double m = re * re + im * im;
        if (m > bestMag) { bestMag = m; best = f; }
    }
    return best;
}

// Spektral tyngdpunkt (grov ljushetsmätning)
static double centroid(const std::vector<double>& v, size_t a, size_t n, double sr)
{
    // Spektralt tyngdpunkt (Hann-fönster, FFT-upplösning sr/n, upp till 10 kHz)
    double num = 0.0, den = 0.0;
    const int kmax = (int)(10000.0 * n / sr);
    std::vector<double> w(n);
    for (size_t i = 0; i < n; ++i) w[i] = (0.5 - 0.5 * std::cos(2.0 * kPi * i / n)) * v[a + i];
    for (int k = 2; k < kmax; ++k) {
        const double f = k * sr / n;
        double re = 0.0, im = 0.0, c = 1.0, s = 0.0;
        const double dc = std::cos(2.0 * kPi * k / n), ds = std::sin(2.0 * kPi * k / n);
        for (size_t i = 0; i < n; ++i) { re += w[i] * c; im += w[i] * s; const double c2 = c * dc - s * ds; s = s * dc + c * ds; c = c2; }
        const double m = std::sqrt(re * re + im * im);
        num += f * m; den += m;
    }
    return den > 0 ? num / den : 0.0;
}

int main()
{
    const double sr = 48000.0;

    // 1. Delarna i toppoktaven ger högst ±2 cent avvikelse (som ett riktigt divide-down-chip)
    {
        Engine e; e.prepare(sr); plain(e);
        double worst = 0.0;
        for (int n = 60; n < 72; ++n) {
            const double ideal = 440.0 * std::pow(2.0, (n - 69) / 12.0);
            worst = std::fmax(worst, std::fabs(cents(e.noteFrequency(n), ideal)));
        }
        check(worst < 2.0, "stamning: alla toner inom 2 cent fran liksvavande (heltalsdelare, som chippet)");
        std::printf("      (storsta avvikelse %.2f cent)\n", worst);
        check(std::fabs(cents(e.noteFrequency(69), 440.0)) < 4.0, "A4 = 440 Hz (inom 4 cent)");
        check(e.noteFrequency(69) == 2.0 * e.noteFrequency(57), "oktaver ar exakt fasläsade (divide-down)");
    }

    // 2. Tyst utan toner
    {
        Engine e; e.prepare(sr);
        std::vector<double> v = render(e, 0.5, sr);
        check(peak(v) < 1e-6, "tyst utan toner");
    }

    // 3. Uppmätt tonhöjd stämmer
    {
        Engine e; e.prepare(sr); plain(e);
        e.handleMidi(0x90, 69, 100);
        std::vector<double> v = render(e, 1.0, sr);
        const double f = measurePitch(v, (size_t)(0.3 * sr), 16384, sr, 440.0);
        check(std::fabs(cents(f, e.noteFrequency(69))) < 1.0, "A4 later pa ratt tonhojd");
        check(finite(v), "inga ogiltiga tal");
    }

    // 4. DETUNE flyttar generator II, TUNE flyttar allt
    {
        Engine e; e.prepare(sr); plain(e);
        e.setDetuneNorm(normFromDetune(15.0));
        check(std::fabs(cents(e.noteFrequency(69, 1), e.noteFrequency(69, 0)) - 15.0) < 0.01, "DETUNE +15 cent pa generator II");
        e.setTuneNorm(normFromTune(428.0));
        check(std::fabs(cents(e.noteFrequency(69), 428.0)) < 4.0, "TUNE A4 = 428 Hz");
    }

    // 5. Polyfoni: högst 7 röster, ingen överstyrning
    {
        Engine e; e.prepare(sr);
        e.setStringNorm(0, 1); e.setStringNorm(1, 1); e.setStringNorm(2, 1);
        e.setBrillianceNorm(1.0);
        e.setVolumeNorm(1.0);
        for (int n = 48; n < 48 + 9; ++n) e.handleMidi(0x90, n, 127);
        std::vector<double> v = render(e, 1.5, sr);
        check(e.activeVoices() <= Engine::kVoices, "hogst 7 roster");
        check(peak(v) <= 1.0 && finite(v), "9 toner, allt pa max: ingen klippning");
    }

    // 6. Normal nivå. BOOST av (SK30 utan utgångssteg): ett 4-tonsackord -24..-10 dBFS RMS.
    //    BOOST på (standard): +9,5 dB, i nivå med andra syntar; limitern håller topparna.
    {
        Engine e; e.prepare(sr);
        float pv[kNumParams]; for (int i = 0; i < kNumParams; ++i) pv[i] = defaultParam(i);
        pv[kParamBoost] = 0.0f; e.setParams(pv); e.reset();
        const int ch[4] = { 57, 60, 64, 69 };
        for (int n : ch) e.handleMidi(0x90, n, 100);
        std::vector<double> v = render(e, 2.0, sr);
        const double db = 20.0 * std::log10(rms(v, (size_t)(0.5 * sr), v.size()));
        check(db > -24.0 && db < -10.0, "standardljud (BOOST av): rimlig niva for ett ackord");
        check(peak(v) < 0.7, "standardljud (BOOST av): topp under -3 dBFS");
        Engine b; b.prepare(sr);
        for (int n : ch) b.handleMidi(0x90, n, 100);
        std::vector<double> w = render(b, 2.0, sr);
        const double dbb = 20.0 * std::log10(rms(w, (size_t)(0.5 * sr), w.size()));
        check(dbb > db + 8.0 && dbb < db + 10.0, "BOOST: ca +9,5 dB");
        check(peak(w) <= Limiter::kCeiling + 1e-9, "BOOST: limitern haller toppen under -0,3 dBFS");
        Engine one; one.prepare(sr); one.handleMidi(0x90, 48, 100);
        std::vector<double> o = render(one, 1.5, sr);
        const double r1 = 20.0 * std::log10(rms(o, (size_t)(0.75 * sr), o.size()));
        check(r1 > -18.0 && r1 < -9.0, "BOOST: en ton i niva med andra syntar (-18..-9 dBFS RMS)");
    }

    // 7. Attack: SLOW ATTACK på/av
    {
        Engine e; e.prepare(sr); plain(e);
        e.setSlowAttackNorm(0.0);
        e.handleMidi(0x90, 69, 100);
        std::vector<double> v = render(e, 0.3, sr);
        const double early = rms(v, (size_t)(0.008 * sr), (size_t)(0.012 * sr));
        const double full = rms(v, (size_t)(0.2 * sr), (size_t)(0.3 * sr));
        check(early > 0.8 * full, "snabb attack (3 ms): full niva efter 10 ms");

        Engine s; s.prepare(sr); plain(s);
        s.setSlowAttackNorm(1.0);
        s.handleMidi(0x90, 69, 100);
        std::vector<double> w = render(s, 0.4, sr);
        const double e2 = rms(w, (size_t)(0.008 * sr), (size_t)(0.012 * sr));
        const double f2 = rms(w, (size_t)(0.3 * sr), (size_t)(0.4 * sr));
        check(e2 < 0.5 * f2, "SLOW ATTACK (80 ms): mjuk start");
    }

    // 8. Release: SUSTAIN av = kort, på = enligt reglaget
    {
        Engine e; e.prepare(sr); plain(e);
        e.handleMidi(0x90, 69, 100);
        render(e, 0.3, sr);
        e.handleMidi(0x80, 69, 0);
        std::vector<double> v = render(e, 0.3, sr);
        check(rms(v, (size_t)(0.1 * sr), (size_t)(0.15 * sr)) < 1e-4, "SUSTAIN av: tyst efter 0,1 s");
        check(!e.isSounding(), "rosten blir ledig efter release");

        Engine s; s.prepare(sr); plain(s);
        s.setSustainNorm(1.0); s.setSustainTimeNorm(1.0); // 1,6 s
        s.handleMidi(0x90, 69, 100);
        std::vector<double> a = render(s, 0.3, sr);
        s.handleMidi(0x80, 69, 0);
        std::vector<double> b = render(s, 0.6, sr);
        const double ratio = rms(b, (size_t)(0.45 * sr), (size_t)(0.55 * sr)) / rms(a, (size_t)(0.2 * sr), (size_t)(0.3 * sr));
        check(ratio > 0.1 && ratio < 0.9, "SUSTAIN 1,6 s: klingar ut langsamt");
    }

    // 9. Sustainpedal håller tonen
    {
        Engine e; e.prepare(sr); plain(e);
        e.handleMidi(0xB0, 64, 127);
        e.handleMidi(0x90, 69, 100);
        render(e, 0.2, sr);
        e.handleMidi(0x80, 69, 0);
        std::vector<double> v = render(e, 0.3, sr);
        check(rms(v, (size_t)(0.2 * sr), (size_t)(0.3 * sr)) > 0.005, "sustainpedal haller tonen");
        e.handleMidi(0xB0, 64, 0);
        std::vector<double> w = render(e, 0.3, sr);
        check(rms(w, (size_t)(0.2 * sr), (size_t)(0.3 * sr)) < 1e-4, "pedal upp: tonen slapps");
    }

    // 10. Stråkfiltren: 16' (STRING 1) är mörkare än 4' (STRING 3) i förhållande till tonen
    {
        double c[3];
        for (int s = 0; s < 3; ++s) {
            Engine e; e.prepare(sr); plain(e);
            e.setStringNorm(1, 0.0); e.setStringNorm(s, 1.0);
            e.handleMidi(0x90, 57, 100);
            std::vector<double> v = render(e, 0.6, sr);
            c[s] = centroid(v, (size_t)(0.3 * sr), 4096, sr);
        }
        check(c[0] < c[1] && c[1] < c[2], "STRING 1 < STRING 2 < STRING 3 i ljushet");
    }

    // 11. BRILLIANCE höjer diskanten
    {
        double c[2];
        for (int b = 0; b < 2; ++b) {
            Engine e; e.prepare(sr); plain(e);
            e.setBrillianceNorm(b ? 1.0 : 0.0);
            e.handleMidi(0x90, 57, 100);
            std::vector<double> v = render(e, 0.6, sr);
            c[b] = centroid(v, (size_t)(0.3 * sr), 4096, sr);
        }
        check(c[1] > c[0] * 1.2, "BRILLIANCE +12 dB ger ljusare ljud");
    }

    // 12. Ensemble: ändrar ljudet, nivån ungefär densamma, inga ogiltiga tal
    {
        Engine d; d.prepare(sr); plain(d);
        Engine w; w.prepare(sr); plain(w); w.setEnsembleNorm(1.0);
        d.handleMidi(0x90, 57, 100); w.handleMidi(0x90, 57, 100);
        std::vector<double> a = render(d, 3.0, sr), b = render(w, 3.0, sr);
        double diff = 0.0; for (size_t i = 0; i < a.size(); ++i) diff = std::fmax(diff, std::fabs(a[i] - b[i]));
        const double ra = rms(a, (size_t)sr, a.size()), rb = rms(b, (size_t)sr, b.size());
        check(diff > 0.01, "ENSEMBLE paverkar ljudet");
        check(std::fabs(20.0 * std::log10(rb / ra)) < 3.0, "ENSEMBLE: nivan inom 3 dB");
        check(finite(b), "ENSEMBLE: inga ogiltiga tal");
        // Långsam svajning: nivån i ett smalt band rör sig med ca 0,64 Hz
        double lo = 1e9, hi = 0.0;
        for (int k = 0; k < 20; ++k) { const double r = rms(b, (size_t)(1.0 * sr + k * 0.1 * sr), (size_t)(1.0 * sr + k * 0.1 * sr + 0.05 * sr)); lo = std::fmin(lo, r); hi = std::fmax(hi, r); }
        check(hi / lo > 1.02, "ENSEMBLE: levande (nivan svajar)");
    }

    // 13. Trappstegs-LFO: symmetrisk triangel med 16 steg per halvperiod
    {
        check(std::fabs(Ensemble::steppedTri(0.0) + 1.0) < 1e-12 && std::fabs(Ensemble::steppedTri(0.5) - 1.0) < 1e-12, "LFO: -1 vid 0, +1 vid halva perioden");
        int changes = 0; double prev = Ensemble::steppedTri(0.0);
        for (int i = 1; i < 10000; ++i) { const double v = Ensemble::steppedTri(i / 20000.0); if (v != prev) ++changes; prev = v; }
        check(changes == 16, "LFO: 16 trappsteg per halvperiod");
    }

    // 14. Röst som tas över (8:e tonen) ger inget klick
    {
        Engine e; e.prepare(sr); plain(e);
        for (int n = 60; n < 67; ++n) e.handleMidi(0x90, n, 100);
        std::vector<double> a = render(e, 0.3, sr);
        e.handleMidi(0x90, 72, 100);
        std::vector<double> b = render(e, 0.05, sr);
        double jump = 0.0; for (size_t i = 1; i < b.size(); ++i) jump = std::fmax(jump, std::fabs(b[i] - b[i - 1]));
        double normal = 0.0; for (size_t i = (size_t)(0.2 * sr) + 1; i < a.size(); ++i) normal = std::fmax(normal, std::fabs(a[i] - a[i - 1]));
        check(jump < normal * 1.5 + 1e-3, "rostbyte (8:e tonen): inget klick");
    }

    // 15. Fungerar vid 44,1, 48 och 96 kHz med samma nivå
    {
        double lv[3]; const double rates[3] = { 44100.0, 48000.0, 96000.0 };
        bool ok = true;
        for (int i = 0; i < 3; ++i) {
            Engine e; e.prepare(rates[i]);
            e.handleMidi(0x90, 60, 100);
            std::vector<double> v = render(e, 1.0, rates[i]);
            ok = ok && finite(v);
            lv[i] = rms(v, (size_t)(0.5 * rates[i]), v.size());
        }
        check(ok, "44,1/48/96 kHz: inga ogiltiga tal");
        check(std::fabs(20 * std::log10(lv[2] / lv[0])) < 1.0 && std::fabs(20 * std::log10(lv[1] / lv[0])) < 1.0, "samma niva vid alla samplingsfrekvenser");
    }

    // 16. Visade värden
    {
        char buf[32];
        paramDisplayText(kParamTune, normFromTune(440.0), buf, sizeof buf);
        check(std::string(buf) == "440.0", "TUNE visar 440.0");
        paramDisplayText(kParamMode, 1.0, buf, sizeof buf);
        check(std::string(buf) == "II", "MODE visar II");
        paramDisplayText(kParamVolume, 0.0, buf, sizeof buf);
        check(std::string(buf) == "-inf", "VOLUME 0 visar -inf");
    }

    // 17. Vibrato: fördröjning (DELAY) gör att vibratot kommer gradvis
    {
        Engine e; e.prepare(sr); plain(e);
        e.setVibDepthNorm(1.0); e.setVibDelayNorm(1.0); // 30 cent, 3,2 s
        e.handleMidi(0x90, 69, 100);
        std::vector<double> v = render(e, 4.0, sr);
        const double f1 = measurePitch(v, (size_t)(0.1 * sr), 8192, sr, 440.0);
        check(std::fabs(cents(f1, e.noteFrequency(69))) < 3.0, "VIBRATO DELAY: inget vibrato i borjan");
    }


    // ===== Etapp 2: poly-synth och tremolo =====

    // 18. POLY-SYNTH 1/2/3 läses ur motståndsnäten: läge = 10k/(R+10k)
    {
        check(std::fabs(presetPos(kPolyPresetR[0][0]) - 10.0 / 28.0) < 1e-12, "POLY-SYNTH 1: CUTOFF fran 18k");
        check(std::fabs(presetPos(kPolyPresetR[1][1]) - 10.0 / 11.2) < 1e-12, "POLY-SYNTH 2: RESONANCE fran 1,2k");
        check(std::fabs(presetPos(kPolyPresetR[2][6]) - 10.0 / 14.7) < 1e-12, "POLY-SYNTH 3: RELEASE fran 4,7k");
    }

    // 19. CANCEL: poly-synthen är tyst; 1/2/3/MANUAL låter
    {
        double lv[kNumPolySel];
        bool ok = true;
        for (int sel = 0; sel < kNumPolySel; ++sel) {
            Engine e; e.prepare(sr); plain(e);
            e.setStringNorm(1, 0.0);                       // inga stråkar
            e.setPolySelNorm(stepNorm(sel, kNumPolySel));
            e.handleMidi(0x90, 57, 100); e.handleMidi(0x90, 64, 100);
            std::vector<double> v = render(e, 1.0, sr);
            ok = ok && finite(v) && peak(v) < 0.9;
            lv[sel] = 20.0 * std::log10(rms(v, (size_t)(0.3 * sr), v.size()) + 1e-12);
        }
        check(lv[0] < -100.0, "POLY-SYNTH CANCEL: tyst");
        check(lv[1] > -40.0 && lv[2] > -40.0 && lv[3] > -40.0 && lv[4] > -40.0, "POLY-SYNTH 1/2/3/MANUAL later");
        check(ok, "POLY-SYNTH: inga ogiltiga tal, ingen klippning");
    }

    // 20. CUTOFF: högre ger ljusare
    {
        double c[2];
        for (int i = 0; i < 2; ++i) {
            Engine e; e.prepare(sr); plain(e);
            e.setStringNorm(1, 0.0);
            e.setPolySelNorm(1.0);                        // MANUAL
            e.setEgDepthNorm(0.0);
            e.setCutoffNorm(i ? 0.8 : 0.45);
            e.handleMidi(0x90, 45, 100);
            std::vector<double> v = render(e, 0.6, sr);
            c[i] = centroid(v, (size_t)(0.3 * sr), 4096, sr);
        }
        check(c[1] > c[0] * 1.3, "CUTOFF hogre ger ljusare ljud");
    }

    // 21. EG DEPTH: filtret öppnar och stänger (ljusare i början än senare)
    {
        Engine e; e.prepare(sr); plain(e);
        e.setStringNorm(1, 0.0);
        e.setPolySelNorm(1.0);
        e.setCutoffNorm(0.3); e.setEgDepthNorm(0.6);
        e.setAttackNorm(0.0); e.setDecayNorm(0.3); e.setSustainLevelNorm(0.0);
        e.handleMidi(0x90, 45, 100);
        std::vector<double> v = render(e, 1.5, sr);
        const double early = centroid(v, (size_t)(0.02 * sr), 2048, sr);
        const double late = centroid(v, (size_t)(1.2 * sr), 2048, sr);
        check(early > late * 1.3, "EG DEPTH: filtret sveper (ljust i borjan, morkare sen)");
    }

    // 22. Maximal resonans är stabil
    {
        Engine e; e.prepare(sr); plain(e);
        e.setPolySelNorm(1.0); e.setResonanceNorm(1.0); e.setEgDepthNorm(1.0);
        e.setPolyVolNorm(1.0);
        for (int n = 40; n < 47; ++n) e.handleMidi(0x90, n, 100);
        std::vector<double> v = render(e, 2.0, sr);
        check(finite(v) && peak(v) <= 1.0, "RESONANCE max (Q 10): stabilt, ingen klippning");
    }

    // 23. BP8' låter annorlunda än 8' lågpass
    {
        Engine a; a.prepare(sr); plain(a); a.setStringNorm(1, 0.0); a.setPolySelNorm(1.0); a.setPolyFeetNorm(stepNorm(2, kNumFeet));
        Engine b; b.prepare(sr); plain(b); b.setStringNorm(1, 0.0); b.setPolySelNorm(1.0); b.setPolyFeetNorm(stepNorm(3, kNumFeet));
        a.handleMidi(0x90, 45, 100); b.handleMidi(0x90, 45, 100);
        std::vector<double> va = render(a, 0.6, sr), vb = render(b, 0.6, sr);
        double diff = 0.0; for (size_t i = 0; i < va.size(); ++i) diff = std::fmax(diff, std::fabs(va[i] - vb[i]));
        check(diff > 0.01 && rms(va, (size_t)(0.3 * sr), va.size()) > 1e-3, "BP8' (bandpass) skiljer sig fran 8' lagpass och later");
    }

    // 24. TREMOLO: volymen svajar med 6,4 Hz (SPEED på) och 0,64 Hz (SPEED av)
    {
        for (int fast = 0; fast < 2; ++fast) {
            Engine e; e.prepare(sr); plain(e);
            e.setPolyTremoloNorm(1.0); e.setTremoloSpeedNorm(fast ? 1.0 : 0.0);
            e.handleMidi(0x90, 57, 100);
            std::vector<double> v = render(e, 4.0, sr);
            // nivå i korta fönster (10 ms) och dess svängning
            const double hz = fast ? 6.4 : 0.64;
            const size_t win = (size_t)(0.01 * sr);
            std::vector<double> envv;
            for (size_t i = (size_t)(0.5 * sr); i + win < v.size(); i += win) envv.push_back(rms(v, i, i + win));
            double mean = 0.0; for (double x : envv) mean += x; mean /= envv.size();
            double re = 0.0, im = 0.0, re2 = 0.0, im2 = 0.0;
            for (size_t i = 0; i < envv.size(); ++i) {
                const double t = i * 0.01;
                re += (envv[i] - mean) * std::cos(2 * kPi * hz * t); im += (envv[i] - mean) * std::sin(2 * kPi * hz * t);
                re2 += (envv[i] - mean) * std::cos(2 * kPi * hz * 1.7 * t); im2 += (envv[i] - mean) * std::sin(2 * kPi * hz * 1.7 * t);
            }
            const double at = std::sqrt(re * re + im * im), other = std::sqrt(re2 * re2 + im2 * im2);
            check(at > 3.0 * other && at / envv.size() > 0.02 * mean, fast ? "TREMOLO 6,4 Hz" : "TREMOLO 0,64 Hz");
        }
    }


    // ===== Etapp 3: orgel =====
    auto organOnly = [&](Engine& e) {
        plain(e);
        e.setStringNorm(1, 0.0);
        e.setOrgSustainNorm(0.0);
        for (int i = 0; i < kNumFootages; ++i) e.setFootageNorm(i, 0.0);
    };
    auto dft = [&](const std::vector<double>& v, size_t a, size_t n, double f) {
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos(2 * kPi * i / (n - 1)); re += w * v[a + i] * std::cos(2 * kPi * f * (a + i) / sr); im += w * v[a + i] * std::sin(2 * kPi * f * (a + i) / sr); }
        return std::sqrt(re * re + im * im) / n;
    };

    // 25. ORGAN CANCEL tyst, 1/2/3 låter, ORGAN 3 ljusast
    {
        double lv[4], cen[4];
        for (int sel = 0; sel < 4; ++sel) {
            Engine e; e.prepare(sr); organOnly(e);
            e.setOrgSelNorm(stepNorm(sel, kNumOrgSel));
            e.handleMidi(0x90, 57, 100);
            std::vector<double> v = render(e, 0.8, sr);
            lv[sel] = 20.0 * std::log10(rms(v, (size_t)(0.3 * sr), v.size()) + 1e-12);
            cen[sel] = centroid(v, (size_t)(0.3 * sr), 4096, sr);
        }
        check(lv[0] < -100.0, "ORGAN CANCEL: tyst");
        check(lv[1] > -40.0 && lv[2] > -40.0 && lv[3] > -40.0, "ORGAN 1/2/3 later");
        check(cen[3] > cen[1] * 1.3, "ORGAN 3 (alla fotlagen) ljusare an ORGAN 1");
    }

    // 26. Fotlägen: 1' = tre oktaver upp, 5 1/3' = liksvävande kvint (lånad ton)
    {
        Engine e; e.prepare(sr); organOnly(e);
        e.setOrgSelNorm(1.0); e.setFootageNorm(8, 1.0);           // bara 1'
        e.handleMidi(0x90, 57, 100);
        std::vector<double> v = render(e, 1.0, sr);
        const double f1 = measurePitch(v, (size_t)(0.3 * sr), 16384, sr, 1760.0);
        check(std::fabs(cents(f1, 1760.0)) < 2.0, "1' = tre oktaver over 8' (A3 -> 1760 Hz)");

        Engine m; m.prepare(sr); organOnly(m);
        m.setOrgSelNorm(1.0); m.setFootageNorm(2, 1.0);           // bara 5 1/3'
        m.handleMidi(0x90, 57, 100);
        std::vector<double> w = render(m, 1.0, sr);
        const double f5 = measurePitch(w, (size_t)(0.3 * sr), 16384, sr, 329.63);
        check(std::fabs(cents(f5, 329.63)) < 2.0 && std::fabs(cents(f5, 330.0)) > 0.5, "5 1/3' = liksvavande kvint (E4 329,6 Hz, inte ren 330 Hz)");
    }

    // 27. Brytning: översta tonen med 1' hamnar i toppoktaven, inga ogiltiga tal
    {
        Engine e; e.prepare(sr); organOnly(e);
        e.setOrgSelNorm(stepNorm(3, kNumOrgSel));                 // ORGAN 3
        e.handleMidi(0x90, 96, 100); e.handleMidi(0x90, 108, 100);
        std::vector<double> v = render(e, 0.5, sr);
        check(finite(v) && peak(v) < 0.9 && rms(v, (size_t)(0.2 * sr), v.size()) > 1e-4, "hoga toner med alla fotlagen (brytning) later rent");
    }

    // 28. DECAY: tonen klingar av medan tangenten hålls; SUSTAIN: lång utklingning
    {
        Engine e; e.prepare(sr); organOnly(e);
        e.setOrgSelNorm(stepNorm(1, kNumOrgSel));
        e.setOrgDecayNorm(1.0); e.setOrgTimeNorm(0.3);
        e.handleMidi(0x90, 57, 100);
        std::vector<double> v = render(e, 2.0, sr);
        check(rms(v, (size_t)(1.5 * sr), (size_t)(2.0 * sr)) < 0.3 * rms(v, (size_t)(0.01 * sr), (size_t)(0.1 * sr)), "ORGAN DECAY: tonen klingar av");

        Engine s2; s2.prepare(sr); organOnly(s2);
        s2.setOrgSelNorm(stepNorm(1, kNumOrgSel)); s2.setOrgSustainNorm(1.0); s2.setOrgTimeNorm(1.0);
        s2.handleMidi(0x90, 57, 100);
        std::vector<double> a = render(s2, 0.3, sr);
        s2.handleMidi(0x80, 57, 0);
        std::vector<double> b = render(s2, 0.5, sr);
        check(rms(b, (size_t)(0.3 * sr), (size_t)(0.4 * sr)) > 0.1 * rms(a, (size_t)(0.2 * sr), (size_t)(0.3 * sr)), "ORGAN SUSTAIN: lang utklingning");

        Engine s3; s3.prepare(sr); organOnly(s3);
        s3.setOrgSelNorm(stepNorm(1, kNumOrgSel));
        s3.handleMidi(0x90, 57, 100);
        render(s3, 0.3, sr);
        s3.handleMidi(0x80, 57, 0);
        std::vector<double> c = render(s3, 0.2, sr);
        check(rms(c, (size_t)(0.1 * sr), (size_t)(0.2 * sr)) < 1e-4, "ORGAN utan SUSTAIN: tyst direkt");
    }

    // 29. PERCUSSIVE 3rd: kort anslag på 2 2/3' som klingar av
    {
        Engine e; e.prepare(sr); organOnly(e);
        e.setOrgSelNorm(1.0);                                     // bara slagverket
        e.setPercNorm(1, 1.0); e.setPercDecayNorm(0.3);
        e.handleMidi(0x90, 57, 100);
        std::vector<double> v = render(e, 2.0, sr);
        const double early = dft(v, (size_t)(0.02 * sr), 2048, 660.0);   // 2 2/3' av A3 = E5 (ca 659 Hz)
        const double late = dft(v, (size_t)(1.5 * sr), 2048, 659.26);
        const double early2 = dft(v, (size_t)(0.02 * sr), 2048, 659.26);
        check(std::fmax(early, early2) > 5.0 * late, "PERCUSSIVE 3rd: anslag som klingar av");
    }

    // 30. ORGAN ENSEMBLE påverkar orgeln, MASTER -inf tystar allt, STRING VOLUME påverkar bara stråkarna
    {
        Engine a; a.prepare(sr); organOnly(a); a.setOrgSelNorm(stepNorm(1, kNumOrgSel));
        Engine b; b.prepare(sr); organOnly(b); b.setOrgSelNorm(stepNorm(1, kNumOrgSel)); b.setOrgEnsembleNorm(1.0);
        a.handleMidi(0x90, 57, 100); b.handleMidi(0x90, 57, 100);
        std::vector<double> va = render(a, 1.0, sr), vb = render(b, 1.0, sr);
        double diff = 0.0; for (size_t i = 0; i < va.size(); ++i) diff = std::fmax(diff, std::fabs(va[i] - vb[i]));
        check(diff > 0.01, "ORGAN ENSEMBLE paverkar orgeln");

        Engine m; m.prepare(sr); m.setMasterNorm(0.0); m.setOrgSelNorm(1.0);
        m.handleMidi(0x90, 57, 100);
        std::vector<double> mv = render(m, 0.5, sr);
        check(rms(mv, (size_t)(0.2 * sr), mv.size()) < 1e-6, "MASTER -inf: tyst");

        Engine p2; p2.prepare(sr); plain(p2); p2.setVolumeNorm(0.0); p2.setPolySelNorm(stepNorm(1, kNumPolySel));
        p2.handleMidi(0x90, 57, 100);
        check(rms(render(p2, 0.5, sr), (size_t)(0.2 * sr), (size_t)(0.5 * sr)) > 1e-3, "STRING VOLUME -inf: poly-synthen hors anda");
    }


    // ===== Etapp 4: solosynt, manual bass, split =====
    auto soloOnly = [&](Engine& e) {
        plain(e);
        e.setStringNorm(1, 0.0);
        e.setSoloVolNorm(normFromVolumeDb(0.0));
        e.setVcaModeNorm(stepNorm(1, kNumVcaModes));   // GATE
        e.setSoloCutoffNorm(0.9); e.setSoloEgDepthNorm(0.0);
    };

    // 31. Solosynten är tyst som standard
    {
        Engine e; e.prepare(sr); plain(e); e.setStringNorm(1, 0.0);
        render(e, 0.2, sr);                                  // stråkarna hinner tonas bort
        e.handleMidi(0x90, 69, 100);
        check(peak(render(e, 0.5, sr)) < 1e-6, "SOLO: tyst som standard (SOLO VOLUME -inf)");
    }

    // 32. NORMAL = högsta tonen, BASS = lägsta tonen, 16' = oktav ner
    {
        Engine e; e.prepare(sr); soloOnly(e);
        e.handleMidi(0x90, 57, 100); e.handleMidi(0x90, 69, 100);
        std::vector<double> v = render(e, 0.8, sr);
        check(std::fabs(cents(measurePitch(v, (size_t)(0.3 * sr), 16384, sr, 440.0), 440.0)) < 5.0, "SOLO NORMAL: hogsta tonen (A4)");
        check(e.soloCurrentNote() == 69, "SOLO NORMAL: tonen 69");

        Engine b; b.prepare(sr); soloOnly(b); b.setSoloModeNorm(stepNorm(1, kNumSoloModes));
        b.handleMidi(0x90, 57, 100); b.handleMidi(0x90, 69, 100);
        check(b.soloCurrentNote() == 57, "SOLO BASS: lagsta tonen");

        Engine f; f.prepare(sr); soloOnly(f); f.setSoloFeetNorm(stepNorm(3, kNumSoloFeet));
        f.handleMidi(0x90, 69, 100);
        std::vector<double> w = render(f, 0.8, sr);
        check(std::fabs(cents(measurePitch(w, (size_t)(0.3 * sr), 16384, sr, 220.0), 220.0)) < 5.0, "SOLO FEET 16': en oktav ner");
    }

    // 33. Legato tillbaka till hållen ton, tyst när alla släppts (GATE)
    {
        Engine e; e.prepare(sr); soloOnly(e);
        e.handleMidi(0x90, 60, 100); render(e, 0.1, sr);
        e.handleMidi(0x90, 67, 100); render(e, 0.1, sr);
        e.handleMidi(0x80, 67, 0);
        check(e.soloCurrentNote() == 60, "SOLO: tillbaka till hallen ton");
        e.handleMidi(0x80, 60, 0);
        std::vector<double> v = render(e, 0.2, sr);
        check(rms(v, (size_t)(0.1 * sr), v.size()) < 1e-4, "SOLO GATE: tyst nar tangenterna slapps");
    }

    // 34. VCA HOLD: fortsätter låta efter att tangenten släppts
    {
        Engine e; e.prepare(sr); soloOnly(e); e.setVcaModeNorm(0.0);
        e.handleMidi(0x90, 60, 100); render(e, 0.1, sr);
        e.handleMidi(0x80, 60, 0);
        std::vector<double> v = render(e, 0.5, sr);
        check(rms(v, (size_t)(0.3 * sr), v.size()) > 1e-3, "SOLO VCA HOLD: fortsatter lata");
    }

    // 35. PORTAMENTO: tonhöjden glider
    {
        Engine e; e.prepare(sr); soloOnly(e); e.setPortamentoNorm(1.0);   // 3 s för tre oktaver
        e.handleMidi(0x90, 48, 100); render(e, 0.2, sr);
        e.handleMidi(0x90, 72, 100);                                      // två oktaver upp: 2 s
        std::vector<double> v = render(e, 1.2, sr);
        const double f = measurePitch(v, (size_t)(0.95 * sr), 4096, sr, 261.63 * std::pow(2.0, 0.5));
        check(f > 261.63 * 1.1 && f < 523.25 * 0.9, "PORTAMENTO: tonen ar pa vag upp efter 1 s");
    }

    // 36. TOUCH VIBRATO: aftertouch ger vibrato
    {
        double spread[2];
        for (int at = 0; at < 2; ++at) {
            Engine e; e.prepare(sr); soloOnly(e); e.setTouchVibNorm(1.0); e.setLfoSpeedNorm(std::log(5.0 / 0.1) / std::log(1000.0));
            e.handleMidi(0x90, 69, 100);
            e.handleMidi(0xD0, at ? 127 : 0, 0);
            std::vector<double> v = render(e, 1.5, sr);
            double lo = 1e9, hi = 0;
            for (int k = 0; k < 8; ++k) { const double f = measurePitch(v, (size_t)(0.4 * sr + k * 0.025 * sr), 1024, sr, 440.0); lo = std::fmin(lo, f); hi = std::fmax(hi, f); }
            spread[at] = cents(hi, lo);
        }
        check(spread[0] < 10.0 && spread[1] > 40.0, "TOUCH VIBRATO: aftertouch ger vibrato");
    }

    // 37. MANUAL BASS: bastangenter spelar bara solosynten, inte stråkarna
    {
        Engine e; e.prepare(sr); plain(e);                 // stråkar på, solo tyst
        e.setSoloModeNorm(stepNorm(2, kNumSoloModes)); e.setBassPointNorm(normFromSplitNote(42));
        e.handleMidi(0x90, 36, 100);
        check(peak(render(e, 0.4, sr)) < 1e-6 && e.soloCurrentNote() == 36, "MANUAL BASS: bastangent styr bara solosynten");
        e.handleMidi(0x90, 60, 100);
        check(rms(render(e, 0.4, sr), (size_t)(0.2 * sr), (size_t)(0.4 * sr)) > 1e-3 && e.soloCurrentNote() == 36, "MANUAL BASS: ovre tangenter spelar strakarna");
    }

    // 38. KEYBOARD SPLIT POLY|ORGAN: under delningen ingen orgel, över ingen poly/stråkar; 7 + 7 röster
    {
        Engine e; e.prepare(sr); plain(e); e.setStringNorm(1, 0.0);
        e.setSplitNorm(stepNorm(1, kNumSplit)); e.setSplitPointNorm(normFromSplitNote(55));
        e.setOrgSelNorm(stepNorm(1, kNumOrgSel));            // orgel på, poly CANCEL, inga stråkar
        e.setOrgSustainNorm(0.0);
        render(e, 0.2, sr);
        e.handleMidi(0x90, 48, 100);
        check(peak(render(e, 0.3, sr)) < 1e-6, "SPLIT: lag tangent spelar inte orgeln");
        e.handleMidi(0x90, 67, 100);
        check(rms(render(e, 0.3, sr), (size_t)(0.1 * sr), (size_t)(0.3 * sr)) > 1e-3, "SPLIT: hog tangent spelar orgeln");

        Engine p2; p2.prepare(sr); plain(p2);
        p2.setSplitNorm(stepNorm(1, kNumSplit)); p2.setOrgSelNorm(stepNorm(1, kNumOrgSel));
        for (int n = 36; n < 43; ++n) p2.handleMidi(0x90, n, 100);
        for (int n = 60; n < 67; ++n) p2.handleMidi(0x90, n, 100);
        render(p2, 0.2, sr);
        check(p2.activeVoices() == 14, "SPLIT: 7 + 7 roster");
    }


    // ===== 0.6.1: nivåbalans och limiter =====
    {
        // Allt på max (+6 dB överallt), 7 toner: aldrig över -0,3 dBFS
        Engine e; e.prepare(sr);
        e.setMasterNorm(1.0); e.setVolumeNorm(1.0); e.setPolyVolNorm(1.0); e.setOrgVolNorm(1.0); e.setSoloVolNorm(1.0);
        e.setStringNorm(0, 1); e.setStringNorm(2, 1); e.setPolySelNorm(stepNorm(2, kNumPolySel)); e.setOrgSelNorm(stepNorm(3, kNumOrgSel));
        for (int n = 48; n < 55; ++n) e.handleMidi(0x90, n, 127);
        std::vector<double> v = render(e, 2.0, sr);
        check(peak(v) <= 0.9661 && finite(v), "limiter: allt pa max ger hogst -0,3 dBFS");
        check(e.takeLimiterReductionDb() > 1.0, "limiter: sanker nar det behovs");
        // Under taket: limitern rör inte ljudet
        Engine q; q.prepare(sr); plain(q);
        q.handleMidi(0x90, 60, 100);
        render(q, 1.0, sr);
        check(q.takeLimiterReductionDb() == 0.0, "limiter: orord vid normal niva");
        // Limitern klipper aldrig (inget knaster): 12 dB för mycket på en bas-sinus
        {
            Limiter lim; lim.prepare(sr);
            int flat = 0; double prev = 0.0, worstStep = 0.0;
            for (int i = 0; i < (int)sr; ++i) {
                const double x = 4.0 * std::sin(2.0 * kPi * 55.0 * i / sr) + (i % 4800 == 0 ? 3.0 : 0.0);   // plus enstaka spikar
                const double y = lim.process(x);
                if (std::fabs(y) >= Limiter::kCeiling - 1e-9 && std::fabs(prev) >= Limiter::kCeiling - 1e-9) ++flat;
                const int ph = (i - lim.latency()) % 4800;
                if (i > lim.latency() + 2 && ph > 1 && ph < 4799) worstStep = std::fmax(worstStep, std::fabs(y - prev));
                prev = y;
            }
            check(flat == 0, "limiter: ingen hard klippning (inget knaster)");
            check(worstStep < 0.05, "limiter: inga hopp i signalen");
            check(lim.latency() == (int)(0.002 * sr + 0.5), "limiter: 2 ms latens (rapporteras till Live)");
        }
        // Balans: en solo-ton och en stråkton ligger inom 4 dB vid samma inställning (0 dB)
        // Standardljudet (Dream Strings) mot solosynten med sina standardvärden
        Engine st; st.prepare(sr); st.handleMidi(0x90, 72, 100);
        Engine so; so.prepare(sr); so.setStringNorm(0, 0.0); so.setStringNorm(1, 0.0); so.setSoloVolNorm(normFromVolumeDb(0.0)); so.handleMidi(0x90, 72, 100);
        render(so, 0.2, sr);
        std::vector<double> a = render(st, 1.0, sr), b = render(so, 1.0, sr);
        const double d = 20.0 * std::log10(peak(b) / peak(a));
        std::printf("      (solo mot strakar, en ton: %+.1f dB)\n", d);
        check(std::fabs(d) < 4.0, "balans: solo och strakar lika starka vid samma installning");
    }

    // ===== Etapp 5: presetfiler =====
    {
        float v[kNumParams], w[kNumParams];
        factoryPresetValues(5, v);
        v[kParamCutoff] = 0.123f;
        const std::string text = presetToText("My Pad", v);
        std::string name;
        check(presetFromText(text, w, &name) && name == "My Pad", "preset: sparas och lases (namn)");
        bool same = true; for (int i = 0; i < kNumParams; ++i) same = same && std::fabs(v[i] - w[i]) < 1e-5f;
        check(same, "preset: alla varden tillbaka");
        check(!presetFromText("TruePan preset 8\r\nPan=0.5\r\n", w, &name), "preset: fel filtyp avvisas");
        check(presetFromText("TrueSymph preset 1\r\nCutoff=7\r\nOkand=0.5\r\n", w, &name) && w[kParamCutoff] == defaultParam(kParamCutoff), "preset: ogiltiga rader ignoreras");
        check(cleanPresetName("a=b\x01" "c") == "abc" && cleanPresetName("") == "Untitled", "preset: namn rensas");
        check(uniquePresetName("Pad", [](const std::string& n) { return n == "Pad" || n == "Pad 2"; }) == "Pad 3", "preset: unikt namn");
        bool names = true;
        for (int i = 0; i < kNumParams; ++i) for (int j = i + 1; j < kNumParams; ++j) if (std::string(kParamNames[i]) == kParamNames[j]) names = false;
        check(names, "parameternamnen ar unika (presetfilerna bygger pa dem)");
        check(kNumFactoryPresets == 36 && std::string(factoryPresetName(0)) == "Dream Strings", "36 fabriksljud");
    }


    // ===== Effekter: delay och reverb =====
    {
        auto stereo = [&](Engine& e, double sec, std::vector<double>& L, std::vector<double>& R) {
            L.resize((size_t)(sec * sr)); R.resize(L.size());
            for (size_t i = 0; i < L.size(); ++i) e.nextStereo(L[i], R[i]);
        };
        std::vector<double> L, R;
        // Av: vänster = höger (mono som SK30)
        Engine a; a.prepare(sr); a.handleMidi(0x90, 60, 100);
        stereo(a, 0.5, L, R);
        bool same = true; for (size_t i = 0; i < L.size(); ++i) same = same && L[i] == R[i];
        check(same, "effekter av: vanster = hoger");

        // DELAY 250 ms: ett eko 250 ms efter en kort ton
        Engine d; d.prepare(sr); plain(d);
        d.setDelayTimeNorm(normFromDelayMs(250.0)); d.setDelayFeedbackNorm(0.0); d.setDelayModNorm(0.0); d.setDelayLevelNorm(1.0);
        d.handleMidi(0x90, 72, 100);
        std::vector<double> v((size_t)(0.6 * sr));
        for (size_t i = 0; i < v.size(); ++i) { if (i == (size_t)(0.05 * sr)) d.handleMidi(0x80, 72, 0); double l, r; d.nextStereo(l, r); v[i] = 0.5 * (l + r); }
        const double pre = rms(v, (size_t)(0.12 * sr), (size_t)(0.24 * sr)), echo = rms(v, (size_t)(0.25 * sr), (size_t)(0.30 * sr));
        check(echo > 10.0 * pre + 1e-6, "DELAY: eko efter 250 ms");

        // DELAY med MOD: stereo (vänster och höger skiljer sig)
        Engine dm; dm.prepare(sr); plain(dm);
        dm.setDelayLevelNorm(1.0); dm.setDelayModNorm(1.0); dm.setDelayTimeNorm(normFromDelayMs(100.0));
        dm.handleMidi(0x90, 60, 100);
        stereo(dm, 1.5, L, R);
        double diff = 0.0; for (size_t i = 0; i < L.size(); ++i) diff = std::fmax(diff, std::fabs(L[i] - R[i]));
        check(diff > 1e-3 && finite(L) && finite(R), "DELAY MOD: brett i stereo");

        // REVERB: efterklang som klingar ut ungefär på vald tid
        Engine rv; rv.prepare(sr); plain(rv);
        rv.setReverbLevelNorm(1.0); rv.setReverbDecayNorm(normFromReverbSec(2.0));
        rv.handleMidi(0x90, 60, 100);
        stereo(rv, 0.3, L, R);
        rv.handleMidi(0x80, 60, 0);
        stereo(rv, 3.0, L, R);
        std::vector<double> mono(L.size()); for (size_t i = 0; i < L.size(); ++i) mono[i] = 0.5 * (L[i] + R[i]);
        const double t1 = rms(mono, (size_t)(0.2 * sr), (size_t)(0.4 * sr)), t2 = rms(mono, (size_t)(1.2 * sr), (size_t)(1.4 * sr));
        const double rate = 20.0 * std::log10(t1 / (t2 + 1e-12)) / 1.0;          // dB per sekund
        std::printf("      (reverb 2 s: %.0f dB/s, borde vara ca 30)\n", rate);
        check(t2 > 1e-5 && rate > 15.0 && rate < 60.0, "REVERB: efterklang ungefar som vald tid");
        check(finite(L) && finite(R), "REVERB: inga ogiltiga tal");

        // REVERB TYPE: alla fyra låter och skiljer sig åt
        {
            double tail[kNumReverbTypes];
            for (int ty = 0; ty < kNumReverbTypes; ++ty) {
                Engine e; e.prepare(sr); plain(e);
                e.setReverbLevelNorm(1.0); e.setReverbTypeNorm(stepNorm(ty, kNumReverbTypes)); e.setReverbDecayNorm(normFromReverbSec(2.0));
                e.handleMidi(0x90, 60, 100); stereo(e, 0.3, L, R); e.handleMidi(0x80, 60, 0); stereo(e, 1.0, L, R);
                tail[ty] = rms(L, (size_t)(0.3 * sr), (size_t)(0.5 * sr));
            }
            bool all = true; for (int ty = 0; ty < kNumReverbTypes; ++ty) all = all && tail[ty] > 1e-5;
            check(all, "REVERB TYPE: alla tio later");
            check(std::fabs(tail[0] - tail[1]) > 1e-6 && std::fabs(tail[0] - tail[3]) > 1e-6, "REVERB TYPE: typerna skiljer sig");
        }
        // REVERB LEVEL på 0 dB höjer inte hela syntens volym mycket (högst 4 dB för ett hållet ackord)
        {
            double lv[2];
            for (int k = 0; k < 2; ++k) {
                Engine e; e.prepare(sr);
                e.setReverbLevelNorm(k ? normFromRevDb(0.0) : 0.0); e.setReverbDecayNorm(normFromReverbSec(6.0));
                const int ch[4] = { 57, 60, 64, 69 }; for (int n : ch) e.handleMidi(0x90, n, 100);
                stereo(e, 4.0, L, R);
                lv[k] = 20.0 * std::log10(rms(L, (size_t)(2.0 * sr), L.size()));
            }
            check(lv[1] - lv[0] < 4.0, "REVERB LEVEL 0 dB: hela ljudet blir inte mycket starkare");
        }
        // DELAY HIGH CUT 2k ger mörkare eko än OFF
        {
            double c[2];
            for (int k = 0; k < 2; ++k) {
                Engine e; e.prepare(sr); plain(e); e.setStringNorm(2, 1.0);
                e.setDelayLevelNorm(1.0); e.setDelayFeedbackNorm(0.0); e.setDelayModNorm(0.0); e.setDelayTimeNorm(normFromDelayMs(200.0));
                e.setDelayFiltersNorm(k ? 1.0 : 0.0, 0.0);
                e.handleMidi(0x90, 72, 100);
                std::vector<double> v((size_t)(0.5 * sr));
                for (size_t i = 0; i < v.size(); ++i) { if (i == (size_t)(0.15 * sr)) e.handleMidi(0x80, 72, 0); double l, r; e.nextStereo(l, r); v[i] = l; }
                c[k] = centroid(v, (size_t)(0.25 * sr), 4096, sr);
            }
            check(c[1] < c[0] * 0.85, "DELAY HIGH CUT: morkare eko");
        }

        // Allt på max med effekter: fortfarande under -0,3 dBFS
        Engine mx; mx.prepare(sr);
        mx.setMasterNorm(1.0); mx.setVolumeNorm(1.0); mx.setDelayLevelNorm(1.0); mx.setDelayFeedbackNorm(1.0); mx.setReverbLevelNorm(1.0);
        for (int n = 48; n < 55; ++n) mx.handleMidi(0x90, n, 127);
        stereo(mx, 3.0, L, R);
        check(peak(L) <= 0.9661 && peak(R) <= 0.9661 && finite(L), "effekter pa max: under -0,3 dBFS");
    }

    // ===== 0.9.0: genomgång mot servicemanualen =====
    // Översta tangenten C6 (MIDI 96) spelar en oktav över C5 (4' bryter tillbaka)
    {
        Engine e; e.prepare(sr);
        check(std::fabs(e.noteFrequency(96) / e.noteFrequency(84) - 2.0) < 1e-9, "C6 (MIDI 96) en oktav over C5");
        Engine a; a.prepare(sr); plain(a); a.handleMidi(0x90, 84, 100);
        Engine b; b.prepare(sr); plain(b); b.handleMidi(0x90, 96, 100);
        std::vector<double> va = render(a, 0.6, sr), vb = render(b, 0.6, sr);
        const double fa = measurePitch(va, (size_t)(0.3 * sr), 4096, sr, a.noteFrequency(84));
        const double fb = measurePitch(vb, (size_t)(0.3 * sr), 4096, sr, b.noteFrequency(96));
        check(std::fabs(cents(fb, 2.0 * fa)) < 5.0, "C6 later en oktav hogre an C5");
        check(finite(vb), "C6: inga ogiltiga tal");
    }
    // Delningspunkterna som på panelritningen (s. 4): F#3|G3 och MANUAL BASS t.o.m. F#2
    {
        check(splitNote(defaultParam(kParamSplitPoint)) == 67, "KEYBOARD SPLIT: delning mellan F#3 och G3");
        check(splitNote(defaultParam(kParamBassPoint)) == 54, "MANUAL BASS: upp till F#2");
    }
    // VIBRATO DELAY: först ca 0,7 s utan vibrato, sedan inmatning (s. 69)
    {
        Engine e; e.prepare(sr); plain(e);
        e.setVibDepthNorm(1.0); e.setVibDelayNorm(1.0); e.setVibSpeedNorm(0.0);
        e.handleMidi(0x90, 69, 100);
        std::vector<double> v = render(e, 3.5, sr);
        const double f0 = e.noteFrequency(69);
        const double c1 = cents(measurePitch(v, (size_t)(0.35 * sr), 8192, sr, 440.0), f0);
        double cmax = 0.0;
        for (double t = 2.6; t < 3.3; t += 0.05) cmax = std::fmax(cmax, std::fabs(cents(measurePitch(v, (size_t)(t * sr), 2048, sr, 440.0), f0)));
        check(std::fabs(c1) < 2.0, "VIBRATO DELAY: helt stilla de forsta 0,7 s");
        check(cmax > 10.0, "VIBRATO DELAY: vibratot kommer efter vantetiden");
    }
    // FOOT CONT (CC11) sänker volymen; FOOT SW PORTAMENTO (CC65) stänger av glidningen när pedalen släpps
    {
        Engine a; a.prepare(sr); a.handleMidi(0x90, 60, 100);
        Engine b; b.prepare(sr); b.handleMidi(0xB0, 11, 64); b.handleMidi(0x90, 60, 100);
        std::vector<double> va = render(a, 1.0, sr), vb = render(b, 1.0, sr);
        const double d = 20.0 * std::log10(rms(vb, (size_t)(0.5 * sr), vb.size()) / rms(va, (size_t)(0.5 * sr), va.size()));
        check(d < -10.0 && d > -14.0, "CC11 halvvags: ca -12 dB (kvadratisk kurva som en volympedal)");
        Engine p; p.prepare(sr); soloOnly(p); p.setPortamentoNorm(1.0);
        p.handleMidi(0xB0, 65, 0);                                    // pedal ansluten, släppt
        p.handleMidi(0x90, 48, 100); render(p, 0.2, sr);
        p.handleMidi(0x90, 72, 100);
        std::vector<double> vp = render(p, 0.6, sr);
        const double f = measurePitch(vp, (size_t)(0.3 * sr), 4096, sr, 523.25);
        check(f > 500.0, "CC65 slappt: ingen portamento (tonen ar framme direkt)");
    }

    // 0.9.1: DELAY/REVERB LEVEL med ljudtaper; gamla sparade värden låter likadant
    {
        check(std::fabs(fxLevelDb(0.5) + 10.0) < 0.01, "DELAY LEVEL: mitten = -10 dB");
        check(std::fabs(fxLevelDb(1.0)) < 1e-9 && std::fabs(revLevelDb(1.0) - 8.0) < 1e-9, "LEVEL: toppen som forut (0 / +8 dB)");
        check(fxLevelDb(0.0) <= -100.0 && revLevelDb(0.0) <= -100.0, "LEVEL: 0 = av");
        check(std::fabs(fxLevelDb(convertOldDelayLevel(0.5f)) + 18.0) < 0.01, "gammal DELAY LEVEL (-18 dB) raknas om");
        check(std::fabs(revLevelDb(convertOldReverbLevel((float)((-4.0 + 36.0) / 44.0))) + 4.0) < 0.01, "gammal REVERB LEVEL (-4 dB) raknas om");
        float v[kNumParams];
        std::string nm;
        presetFromText(std::string("TrueSymph preset 1\r\nName=Old\r\nDlyLvl=0.5\r\n"), v, &nm);
        check(std::fabs(fxLevelDb(v[kParamDelayLevel]) + 18.0) < 0.01, "preset utan FxScale (gammal): raknas om");
        float w[kNumParams]; for (int i = 0; i < kNumParams; ++i) w[i] = defaultParam(i);
        w[kParamDelayLevel] = 0.5f;
        presetFromText(presetToText("New", w), v, &nm);
        check(std::fabs(v[kParamDelayLevel] - 0.5f) < 1e-5, "ny preset: samma varde tillbaka");
    }

    // 0.9.2: AMBIENCE och NONLIN (AMS RMX16-stil), TAPE-delay (Space Echo-stil)
    {
        auto stereo = [&](Engine& e, double sec, std::vector<double>& L, std::vector<double>& R) {
            L.resize((size_t)(sec * sr)); R.resize(L.size());
            for (size_t i = 0; i < L.size(); ++i) e.nextStereo(L[i], R[i]);
        };
        auto wetTail = [&](int ty, double dec, std::vector<double>& w) {
            std::vector<double> a, b, R;
            for (int k = 0; k < 2; ++k) {
                Engine e; e.prepare(sr); plain(e);
                e.setReverbLevelNorm(k ? 1.0 : 0.0); e.setReverbTypeNorm(stepNorm(ty, kNumReverbTypes)); e.setReverbDecayNorm(normFromReverbSec(dec));
                e.handleMidi(0x90, 60, 100); std::vector<double> L1; stereo(e, 0.03, L1, R); e.handleMidi(0x80, 60, 0);
                std::vector<double> L2; stereo(e, 1.2, L2, R); L1.insert(L1.end(), L2.begin(), L2.end());
                (k ? b : a) = L1;
            }
            w.resize(a.size()); for (size_t i = 0; i < a.size(); ++i) w[i] = b[i] - a[i];
        };
        std::vector<double> nl, amb, pl;
        wetTail(kRevNonLin, 3.0, nl); wetTail(kRevAmbience, 3.0, amb); wetTail(0, 3.0, pl);
        const double nlIn = rms(nl, (size_t)(0.10 * sr), (size_t)(0.25 * sr)), nlOut = rms(nl, (size_t)(0.42 * sr), (size_t)(0.55 * sr));
        check(nlIn > 1e-4 && 20.0 * std::log10(nlIn / (nlOut + 1e-12)) > 30.0, "NONLIN: jamn skur som tar tvart slut efter grindtiden (0,3 s)");
        const double ambLate = rms(amb, (size_t)(0.7 * sr), (size_t)(0.9 * sr)), plLate = rms(pl, (size_t)(0.7 * sr), (size_t)(0.9 * sr));
        check(ambLate < plLate * 0.1, "AMBIENCE: kort svans jamfort med PLATE");
        check(finite(nl) && finite(amb), "AMBIENCE/NONLIN: inga ogiltiga tal");
        check(std::fabs(convertOldReverbType((float)stepNorm(3, 4)) - (float)stepNorm(3, kNumReverbTypes)) < 1e-6, "gammal TYPE (ROOM, 4 typer) raknas om");
        check(std::fabs(convertOldReverbType((float)stepNorm(5, 6), 6) - (float)stepNorm(kRevNonLin, kNumReverbTypes)) < 1e-6, "gammal TYPE (NONLIN, 6 typer) raknas om");
        check(std::fabs(convertSteps(1.0f, 2, kNumDelayModes) - (float)stepNorm(kDlyTape, kNumDelayModes)) < 1e-6, "gammalt DELAY-lage (TAPE) raknas om");

        // TAPE: ekona blir mörkare för varje varv (jämför eko 1 och eko 3 med DIGI)
        double ratio[2];
        for (int tp = 0; tp < 2; ++tp) {
            Engine e; e.prepare(sr); plain(e);
            e.setDelayModeNorm(stepNorm(tp ? kDlyTape : kDlyDigi, kNumDelayModes));
            e.setDelayTimeNorm(normFromDelayMs(250.0)); e.setDelayFeedbackNorm(0.7); e.setDelayLevelNorm(1.0); e.setDelayModNorm(0.0);
            e.handleMidi(0x90, 72, 100); std::vector<double> L, R; stereo(e, 0.05, L, R); e.handleMidi(0x80, 72, 0);
            std::vector<double> L2; stereo(e, 1.2, L2, R); L.insert(L.end(), L2.begin(), L2.end());
            const double c1 = centroid(L, (size_t)(0.26 * sr), 2048, sr), c3 = centroid(L, (size_t)(0.76 * sr), 2048, sr);
            ratio[tp] = c3 / c1;
            check(finite(L), tp ? "TAPE: inga ogiltiga tal" : "DIGI: inga ogiltiga tal");
        }
        check(ratio[1] < ratio[0] * 0.9, "TAPE: ekona morknar for varje varv");
    }

    // 0.9.5: fler 70-talsenheter. Varje delayläge ekar på rätt ställen; varje reverb låter och är giltigt.
    {
        auto stereo = [&](Engine& e, double sec, std::vector<double>& L, std::vector<double>& R) {
            L.resize((size_t)(sec * sr)); R.resize(L.size());
            for (size_t i = 0; i < L.size(); ++i) e.nextStereo(L[i], R[i]);
        };
        // kort tonstöt, delay 300 ms, ingen återmatning: var hörs ekona?
        auto echoAt = [&](int mode, double t0, double t1) {
            Engine e; e.prepare(sr); plain(e);
            e.setDelayModeNorm(stepNorm(mode, kNumDelayModes));
            e.setDelayTimeNorm(normFromDelayMs(300.0)); e.setDelayFeedbackNorm(0.0); e.setDelayLevelNorm(1.0); e.setDelayModNorm(0.0);
            Engine dry; dry.prepare(sr); plain(dry);
            e.handleMidi(0x90, 72, 100); dry.handleMidi(0x90, 72, 100);
            std::vector<double> L, R, D, DR; stereo(e, 0.03, L, R); stereo(dry, 0.03, D, DR);
            e.handleMidi(0x80, 72, 0); dry.handleMidi(0x80, 72, 0);
            std::vector<double> L2, D2; stereo(e, 0.6, L2, R); stereo(dry, 0.6, D2, DR);
            L.insert(L.end(), L2.begin(), L2.end()); D.insert(D.end(), D2.begin(), D2.end());
            for (size_t i = 0; i < L.size(); ++i) L[i] -= D[i];              // bara ekot
            check(finite(L), "DELAY-lage: inga ogiltiga tal");
            return rms(L, (size_t)(t0 * sr), (size_t)(t1 * sr));
        };
        const double digi300 = echoAt(kDlyDigi, 0.29, 0.34), digi100 = echoAt(kDlyDigi, 0.09, 0.14);
        check(digi300 > 20.0 * digi100, "DIGI: ett eko vid TIME");
        const double t3a = echoAt(kDlyTape3, 0.09, 0.14), t3c = echoAt(kDlyTape3, 0.29, 0.34);
        check(t3a > 1e-4 && t3c > 1e-4, "TAPE 3 HEADS: eko vid 1/3 och vid TIME");
        const double era = echoAt(kDlyEchorec, 0.065, 0.11), erc = echoAt(kDlyEchorec, 0.215, 0.26);
        check(era > 1e-4 && erc > 1e-4, "ECHOREC: fyra huvuden (eko vid 1/4 och 3/4)");
        check(echoAt(kDlyEchoplex, 0.29, 0.36) > 1e-4, "ECHOPLEX: eko vid TIME");
        check(echoAt(kDlyMemoryMan, 0.29, 0.36) > 1e-4, "MEMORY MAN: eko vid TIME");
        // MEMORY MAN är mörkare än DIGI (BBD-bandbredd)
        double cen[2];
        for (int k = 0; k < 2; ++k) {
            Engine e; e.prepare(sr); plain(e);
            e.setDelayModeNorm(stepNorm(k ? kDlyMemoryMan : kDlyDigi, kNumDelayModes));
            e.setDelayTimeNorm(normFromDelayMs(300.0)); e.setDelayFeedbackNorm(0.0); e.setDelayLevelNorm(1.0); e.setDelayModNorm(0.0);
            Engine dry; dry.prepare(sr); plain(dry);
            e.handleMidi(0x90, 72, 100); dry.handleMidi(0x90, 72, 100);
            std::vector<double> L, R, D, DR; stereo(e, 0.6, L, R); stereo(dry, 0.6, D, DR);
            for (size_t i = 0; i < L.size(); ++i) L[i] -= D[i];
            cen[k] = centroid(L, (size_t)(0.4 * sr), 4096, sr);
        }
        check(cen[1] < cen[0] * 0.85, "MEMORY MAN: morkare eko an DIGI");
    }

    std::printf("\n%s (%d fel)\n", failures ? "TESTER MISSLYCKADES" : "ALLA TESTER OK", failures);
    return failures ? 1 : 0;
}
