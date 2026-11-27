// YamaSK30Tests.cpp - automatiska tester av ljudmotorn (körs vid varje bygge).
#include "YamaSK30DSP.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace yamask30;

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
    double num = 0.0, den = 0.0;
    for (int k = 1; k < 200; ++k) {
        const double f = k * 50.0;
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < n; ++i) { const double ph = 2.0 * kPi * f * i / sr; re += v[a + i] * std::cos(ph); im += v[a + i] * std::sin(ph); }
        const double m = std::sqrt(re * re + im * im);
        num += f * m; den += m;
    }
    (void)sr;
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

    // 6. Normal nivå: ett 4-tonsackord ligger mellan -24 och -10 dBFS RMS
    {
        Engine e; e.prepare(sr);
        const int ch[4] = { 57, 60, 64, 69 };
        for (int n : ch) e.handleMidi(0x90, n, 100);
        std::vector<double> v = render(e, 2.0, sr);
        const double db = 20.0 * std::log10(rms(v, (size_t)(0.5 * sr), v.size()));
        check(db > -24.0 && db < -10.0, "standardljud: rimlig niva for ett ackord");
        check(peak(v) < 0.7, "standardljud: topp under -3 dBFS");
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

    std::printf("\n%s (%d fel)\n", failures ? "TESTER MISSLYCKADES" : "ALLA TESTER OK", failures);
    return failures ? 1 : 0;
}
