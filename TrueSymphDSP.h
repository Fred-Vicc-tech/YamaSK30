// TrueSymphDSP.h - ljudmotorn för TrueSymph (etapp 1: stråkdelen + ensemble).
//
// Byggd efter Yamaha SK30:s servicemanual (KEC-90407/90452):
//   PG-kortet:  två tongeneratorer (YM70400) som delar ner en masterklocka på
//               1 MHz. Varje tangent ger fyrkantvågor i 16', 8' och 4'. Alla
//               toner är fasläsade mot varandra (divide-down).
//               Sågtanden görs av fyrkanter i trappsteg:
//                 /\8  = 0,69 * fyrkant 8'  + 0,47 * fyrkant 4'   (68k / 100k mot 47k)
//                 /\16 = 0,69 * fyrkant 16' + 0,47 * /\8
//                 /\4  = fyrkant 4'
//               Stråkfiltren är passiva RC-stegar (12k/27n + 12k/8,2k/27n för 16',
//               12k/18n + 12k/5,6k/18n för 8' och 4'). STRING 1/2/3 = 16'/8'/4'.
//   CPA/CPC:    stråkar + poly-synth går genom BRILLIANCE (±12 dB vid 5 kHz).
//   TE-kortet:  ensemblen = tre MN3009 (analoga delay-chip) parallellt.
//               Varje chip svajas av en långsam (0,64 Hz) och en snabb (6,4 Hz)
//               trappstegs-triangel, förskjutna 120° mellan chippen (YM63300).
//               Lågpassfilter före (ca 6,6 kHz) och efter (ca 5,4 kHz) chippen.
//
// Ingen allokering i ljudtråden. Allt är deterministiskt (testbart).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

namespace truesymph {

const double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Parametrar (index ändras aldrig: nya läggs till sist)
// ---------------------------------------------------------------------------
enum ParamIndex
{
    kParamVolume = 0,     // STRING VOLUME (-inf..+6 dB)
    kParamString1 = 1,    // STRING 1 (16') av/på
    kParamString2 = 2,    // STRING 2 (8')  av/på
    kParamString3 = 3,    // STRING 3 (4')  av/på
    kParamSlowAttack = 4, // SLOW ATTACK: av 3 ms, på 80 ms
    kParamSustain = 5,    // SUSTAIN-knappen av/på
    kParamSustainTime = 6,// SUSTAIN-reglaget 0,03..1,6 s
    kParamMode = 7,       // MODE I (en generator) / II (båda)
    kParamDetune = 8,     // POLY-SYNTH DETUNE -20..+27 cent (generator II)
    kParamVibDelay = 9,   // VIBRATO DELAY 0..3,2 s
    kParamVibSpeed = 10,  // VIBRATO SPEED 5..7 Hz
    kParamVibDepth = 11,  // VIBRATO DEPTH 0..30 cent
    kParamBrilliance = 12,// BRILLIANCE -12..+12 dB
    kParamEnsemble = 13,  // TREMOLO/ENSEMBLE: ENSEMBLE av/på
    kParamTune = 14,      // TUNE: A4 415..466 Hz
    kNumParams
};

static const char* const kParamNames[kNumParams] = {
    "Volume", "Str 1", "Str 2", "Str 3", "SlowAtk", "Sustain", "SusTime",
    "Mode", "Detune", "VibDly", "VibSpd", "VibDep", "Brill", "Ensembl", "Tune"
};
static const char* const kParamLabels[kNumParams] = {
    "dB", "", "", "", "", "", "s", "", "cent", "s", "Hz", "cent", "dB", "", "Hz"
};

inline double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
inline bool isOn(double norm) { return norm >= 0.5; }

// Normaliserat värde (0..1) -> verkligt värde
inline double volumeDb(double n)      { return n <= 0.0 ? -120.0 : -48.0 + 54.0 * n; } // -48..+6 dB, 0 = tyst
inline double sustainTimeSec(double n){ return 0.03 * std::pow(1.6 / 0.03, clamp01(n)); }
inline double detuneCents(double n)   { return -20.0 + 47.0 * clamp01(n); }
inline double vibDelaySec(double n)   { return 3.2 * clamp01(n); }
inline double vibSpeedHz(double n)    { return 5.0 + 2.0 * clamp01(n); }
inline double vibDepthCents(double n) { return 30.0 * clamp01(n); }
inline double brillianceDb(double n)  { return -12.0 + 24.0 * clamp01(n); }
inline double tuneHz(double n)        { return 415.0 + 51.0 * clamp01(n); }

inline double normFromVolumeDb(double db) { return (db + 48.0) / 54.0; }
inline double normFromTune(double hz)     { return (hz - 415.0) / 51.0; }
inline double normFromDetune(double c)    { return (c + 20.0) / 47.0; }

inline float defaultParam(int index)
{
    switch (index) {
    case kParamVolume:      return (float)normFromVolumeDb(0.0);
    case kParamString1:     return 1.0f;
    case kParamString2:     return 1.0f;
    case kParamString3:     return 0.0f;
    case kParamSlowAttack:  return 1.0f;
    case kParamSustain:     return 1.0f;
    case kParamSustainTime: return (float)(std::log(0.9 / 0.03) / std::log(1.6 / 0.03)); // 0,9 s
    case kParamMode:        return 1.0f;                            // II
    case kParamDetune:      return (float)normFromDetune(7.0);
    case kParamVibDelay:    return 0.25f;                           // 0,8 s
    case kParamVibSpeed:    return 0.25f;                           // 5,5 Hz
    case kParamVibDepth:    return 0.2f;                            // 6 cent
    case kParamBrilliance:  return 0.5f;                            // 0 dB
    case kParamEnsemble:    return 1.0f;
    case kParamTune:        return (float)normFromTune(440.0);
    default:                return 0.0f;
    }
}

inline void paramDisplayText(int index, double n, char* out, size_t max)
{
    switch (index) {
    case kParamVolume: {
        const double db = volumeDb(n);
        if (db <= -100.0) std::snprintf(out, max, "-inf");
        else std::snprintf(out, max, "%.1f", db);
        break;
    }
    case kParamString1: case kParamString2: case kParamString3:
    case kParamSlowAttack: case kParamSustain: case kParamEnsemble:
        std::snprintf(out, max, "%s", isOn(n) ? "ON" : "OFF"); break;
    case kParamSustainTime: std::snprintf(out, max, "%.2f", sustainTimeSec(n)); break;
    case kParamMode:        std::snprintf(out, max, "%s", isOn(n) ? "II" : "I"); break;
    case kParamDetune:      std::snprintf(out, max, "%+.0f", detuneCents(n)); break;
    case kParamVibDelay:    std::snprintf(out, max, "%.1f", vibDelaySec(n)); break;
    case kParamVibSpeed:    std::snprintf(out, max, "%.1f", vibSpeedHz(n)); break;
    case kParamVibDepth:    std::snprintf(out, max, "%.0f", vibDepthCents(n)); break;
    case kParamBrilliance:  std::snprintf(out, max, "%+.1f", brillianceDb(n)); break;
    case kParamTune:        std::snprintf(out, max, "%.1f", tuneHz(n)); break;
    default:                std::snprintf(out, max, "%.2f", n); break;
    }
}

// ---------------------------------------------------------------------------
// Små byggstenar
// ---------------------------------------------------------------------------

// PolyBLEP: tar bort vikning (aliasing) vid fyrkantens hopp.
inline double polyBlep(double t, double dt)
{
    if (dt <= 0.0) return 0.0;
    if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

// Enpoligt högpass (kopplingskondensator).
struct OnePoleHP
{
    double a = 0.0, x1 = 0.0, y1 = 0.0;
    void set(double fc, double sr) { a = std::exp(-2.0 * kPi * fc / sr); }
    void reset() { x1 = y1 = 0.0; }
    double process(double x) { const double y = a * (y1 + x - x1); x1 = x; y1 = y; return y; }
};

// Enpoligt lågpass (utjämning).
struct OnePoleLP
{
    double a = 0.0, y = 0.0;
    void set(double fc, double sr) { a = 1.0 - std::exp(-2.0 * kPi * fc / sr); }
    void reset(double v = 0.0) { y = v; }
    double process(double x) { y += a * (x - y); return y; }
};

// Biquad (RBJ), direktform I i double.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void reset() { x1 = x2 = y1 = y2 = 0.0; }
    double process(double x)
    {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
    void lowpass(double fc, double q, double sr)
    {
        const double w = 2.0 * kPi * fc / sr, c = std::cos(w), s = std::sin(w), al = s / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = (1.0 - c) * 0.5 / a0; b1 = (1.0 - c) / a0; b2 = b0;
        a1 = -2.0 * c / a0; a2 = (1.0 - al) / a0;
    }
    void highShelf(double fc, double gainDb, double slope, double sr)
    {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * fc / sr, c = std::cos(w), s = std::sin(w);
        const double al = s / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double sq = 2.0 * std::sqrt(A) * al;
        const double a0 = (A + 1) - (A - 1) * c + sq;
        b0 = A * ((A + 1) + (A - 1) * c + sq) / a0;
        b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0;
        b2 = A * ((A + 1) + (A - 1) * c - sq) / a0;
        a1 = 2 * ((A - 1) - (A + 1) * c) / a0;
        a2 = ((A + 1) - (A - 1) * c - sq) / a0;
    }
};

// Passiv RC-stege i två steg, exakt som SK30:s stråkfilter:
//   in -R1- n1 (C1 mot jord) -R2- n2 (C2 och R3 mot jord) = ut
// Diskretiserad med trapetsmetoden (bilinjär), alltså samma frekvensgång
// som den analoga kretsen upp till ca halva samplingsfrekvensen.
struct RCLadder
{
    double m[2][2] = { {0, 0}, {0, 0} }; // (I - A T/2)^-1 (I + A T/2)
    double g[2] = { 0, 0 };              // (I - A T/2)^-1 B T/2
    double x[2] = { 0, 0 };
    double uPrev = 0.0;
    double dcGain = 1.0;

    void set(double R1, double C1, double R2, double R3, double C2, double sr)
    {
        // dv1/dt = -(1/R1+1/R2)/C1 v1 + 1/(R2 C1) v2 + 1/(R1 C1) u
        // dv2/dt =  1/(R2 C2) v1 - (1/R2+1/R3)/C2 v2
        const double A00 = -(1.0 / R1 + 1.0 / R2) / C1, A01 = 1.0 / (R2 * C1);
        const double A10 = 1.0 / (R2 * C2),             A11 = -(1.0 / R2 + 1.0 / R3) / C2;
        const double B0 = 1.0 / (R1 * C1);
        const double h = 0.5 / sr;
        // P = I - A h, Q = I + A h
        const double P00 = 1 - A00 * h, P01 = -A01 * h, P10 = -A10 * h, P11 = 1 - A11 * h;
        const double Q00 = 1 + A00 * h, Q01 = A01 * h, Q10 = A10 * h, Q11 = 1 + A11 * h;
        const double det = P00 * P11 - P01 * P10;
        const double i00 = P11 / det, i01 = -P01 / det, i10 = -P10 / det, i11 = P00 / det;
        m[0][0] = i00 * Q00 + i01 * Q10; m[0][1] = i00 * Q01 + i01 * Q11;
        m[1][0] = i10 * Q00 + i11 * Q10; m[1][1] = i10 * Q01 + i11 * Q11;
        g[0] = i00 * B0 * h; g[1] = i10 * B0 * h;
        dcGain = R3 / (R1 + R2 + R3);
    }
    void reset() { x[0] = x[1] = 0.0; uPrev = 0.0; }
    double process(double u)
    {
        const double s = u + uPrev;
        const double n0 = m[0][0] * x[0] + m[0][1] * x[1] + g[0] * s;
        const double n1 = m[1][0] * x[0] + m[1][1] * x[1] + g[1] * s;
        x[0] = n0; x[1] = n1; uPrev = u;
        return n1 / dcGain; // normerat: 0 dB vid likström
    }
};

// ---------------------------------------------------------------------------
// Ensemblen: tre BBD-linjer (MN3009, 256 steg) enligt TE-kortet.
// ---------------------------------------------------------------------------
struct Ensemble
{
    static const int kBufSize = 4096;           // > 40 ms vid 96 kHz
    float buf[kBufSize];
    int writePos = 0;
    double sr = 44100.0;

    // Klockchipets LFO:er (YM63300): trappstegs-trianglar.
    double slowPhase = 0.0, fastPhase = 0.0;
    double slowInc = 0.0, fastInc = 0.0;
    OnePoleLP lfoSmooth[3];                     // RC-utjämning av trappstegen
    Biquad preLP1, preLP2, postLP[3];
    uint32_t noise = 0x12345678u;
    double noiseEnv = 0.0;      // bruset följer signalen (tyst när inget spelas)

    // Delay-värden (ms). Mitten och djup valda så att tonhöjdssvajningen
    // blir ca 6 cent (långsam) och ca 5 cent (snabb) per linje.
    static constexpr double kCenterMs = 5.0;
    static constexpr double kSlowDepthMs = 0.90;
    static constexpr double kFastDepthMs = 0.075;
    static constexpr double kSlowHz = 0.64;
    static constexpr double kFastHz = 6.4;
    static constexpr int kSteps = 16;           // trappsteg per halvperiod

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        slowInc = kSlowHz / sr;
        fastInc = kFastHz / sr;
        for (int i = 0; i < 3; ++i) { lfoSmooth[i].set(60.0, sr); postLP[i].lowpass(std::fmin(5400.0, sr * 0.45), 0.707, sr); }
        preLP1.lowpass(std::fmin(6600.0, sr * 0.45), 0.707, sr);
        preLP2.lowpass(std::fmin(9000.0, sr * 0.45), 0.6, sr);
        reset();
    }
    void reset()
    {
        std::memset(buf, 0, sizeof buf);
        writePos = 0;
        noiseEnv = 0.0;
        slowPhase = 0.0; fastPhase = 0.0;
        preLP1.reset(); preLP2.reset();
        for (int i = 0; i < 3; ++i) { postLP[i].reset(); lfoSmooth[i].reset(lfoValue(i)); }
    }

    // Trappstegs-triangel (-1..1).
    static double steppedTri(double ph)
    {
        ph -= std::floor(ph);
        double t = ph < 0.5 ? ph * 2.0 : 2.0 - ph * 2.0;   // 0..1..0
        t = std::floor(t * kSteps + 0.5) / kSteps;
        return t * 2.0 - 1.0;
    }
    double lfoValue(int line) const
    {
        const double off = line / 3.0;                     // 0°, 120°, 240°
        return kSlowDepthMs * steppedTri(slowPhase + off) + kFastDepthMs * steppedTri(fastPhase + off);
    }

    double readInterp(double delaySamples) const
    {
        double rp = (double)writePos - delaySamples;
        while (rp < 0) rp += kBufSize;
        const int i1 = (int)rp;
        const double f = rp - i1;
        const int i0 = (i1 - 1 + kBufSize) & (kBufSize - 1);
        const int i2 = (i1 + 1) & (kBufSize - 1);
        const int i3 = (i1 + 2) & (kBufSize - 1);
        const double y0 = buf[i0], y1 = buf[i1 & (kBufSize - 1)], y2 = buf[i2], y3 = buf[i3];
        // Hermite (4 punkter)
        const double c1 = 0.5 * (y2 - y0);
        const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
        const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }

    double process(double x)
    {
        // Filtret före chippen (anti-vikning)
        double in = preLP2.process(preLP1.process(x));
        // Chippen tål ca ±5 V: mjuk mättning långt över normal nivå
        in = in / (1.0 + std::fabs(in) * 0.08);
        buf[writePos] = (float)in;
        const double ax = std::fabs(in);
        noiseEnv += (ax > noiseEnv ? 0.01 : 0.0002) * (ax - noiseEnv);

        double sum = 0.0;
        for (int i = 0; i < 3; ++i) {
            const double ms = kCenterMs + lfoSmooth[i].process(lfoValue(i));
            double d = ms * 0.001 * sr;
            if (d < 2.0) d = 2.0;
            if (d > kBufSize - 4) d = kBufSize - 4;
            // Lite brus från chippen (ca -60 dB under signalen)
            noise = noise * 1664525u + 1013904223u;
            const double n = ((int32_t)noise) * (1.0 / 2147483648.0) * 1e-3 * noiseEnv;
            sum += postLP[i].process(readInterp(d) + n);
        }
        writePos = (writePos + 1) & (kBufSize - 1);
        slowPhase += slowInc; if (slowPhase >= 1.0) slowPhase -= 1.0;
        fastPhase += fastInc; if (fastPhase >= 1.0) fastPhase -= 1.0;
        return sum * (1.0 / 3.0) * 1.41;  // tre linjer, ungefär samma upplevda nivå som torrt
    }
};

// ---------------------------------------------------------------------------
// Motorn
// ---------------------------------------------------------------------------
class Engine
{
public:
    static const int kVoices = 7;           // SK30: 7 toner (BASS OFF)
    static const int kTopOctaveMidi = 96;   // tongeneratorns översta oktav: C7 (4' för SK30:s översta tangent C6)
    static const int kWrap = 1 << 20;       // fasräknaren räknar toppoktavsperioder modulo 2^20

    Engine() { computeDividers(); prepare(44100.0); }

    void prepare(double sampleRate)
    {
        sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
        for (int i = 0; i < 3; ++i) { hpIn[i].set(34.0, sr); }
        hp4to8.set(23.0, sr); hp8to16.set(23.0, sr); hpOut4.set(0.7, sr);
        for (int i = 0; i < 3; ++i) hpLadder[i].set(6.0, sr);
        // Stråkfiltren. R3 = shuntmotståndet || 47k-biasen || grindens 4,7k/6,8k (+ ca 0,5k på-resistans)
        ladder[0].set(12e3, 27e-9, 12e3, par3(8.2e3, 47e3, 5.2e3), 27e-9, sr);   // /\16
        ladder[1].set(12e3, 18e-9, 12e3, par3(5.6e3, 47e3, 7.3e3), 18e-9, sr);   // /\8
        ladder[2].set(12e3, 18e-9, 12e3, par3(5.6e3, 47e3, 7.3e3), 18e-9, sr);   // /\4
        ensemble.prepare(sr);
        dcBlock.set(5.0, sr);
        paramSmooth = 1.0 - std::exp(-1.0 / (0.02 * sr));
        stealCoef = 1.0 - std::exp(-3.0 / (0.003 * sr));
        updateBrilliance(true);
        reset();
    }

    void reset()
    {
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) phase[g][p] = (p * 0.0731 + g * 0.37) * 64.0; // fri löpning, olika startfas
        for (int v = 0; v < kVoices; ++v) voices[v] = Voice();
        for (int i = 0; i < 3; ++i) { hpIn[i].reset(); hpLadder[i].reset(); ladder[i].reset(); }
        hp4to8.reset(); hp8to16.reset(); hpOut4.reset();
        brill.reset(); ensemble.reset(); dcBlock.reset();
        vibPhase = 0.0; vibTime = 0.0;
        sustainPedal = false;
        ageCounter = 0;
        volSmoothed = targetVolume();
        for (int i = 0; i < 3; ++i) strLevel[i] = strTarget[i];
        ensMix = ensembleOn ? 1.0 : 0.0;
    }

    // --- parametrar (normaliserade 0..1) ---------------------------------
    void setVolumeNorm(double n)      { volNorm = n; }
    void setStringNorm(int i, double n){ if (i >= 0 && i < 3) strTarget[i] = isOn(n) ? 1.0 : 0.0; }
    void setSlowAttackNorm(double n)  { slowAttack = isOn(n); }
    void setSustainNorm(double n)     { sustainOn = isOn(n); }
    void setSustainTimeNorm(double n) { sustainTime = sustainTimeSec(n); }
    void setModeNorm(double n)        { modeII = isOn(n); }
    void setDetuneNorm(double n)      { detune = detuneCents(n); }
    void setVibDelayNorm(double n)    { vibDelay = vibDelaySec(n); }
    void setVibSpeedNorm(double n)    { vibSpeed = vibSpeedHz(n); }
    void setVibDepthNorm(double n)    { vibDepth = vibDepthCents(n); }
    void setBrillianceNorm(double n)  { const double d = brillianceDb(n); if (d != brillDb) { brillDb = d; updateBrilliance(false); } }
    void setEnsembleNorm(double n)    { ensembleOn = isOn(n); }
    void setTuneNorm(double n)        { tune = tuneHz(n); }

    // --- MIDI --------------------------------------------------------------
    void handleMidi(uint8_t status, uint8_t d1, uint8_t d2)
    {
        const uint8_t type = status & 0xF0;
        if (type == 0x90 && d2 > 0) noteOn(d1);
        else if (type == 0x80 || (type == 0x90 && d2 == 0)) noteOff(d1);
        else if (type == 0xB0) {
            if (d1 == 64) { sustainPedal = d2 >= 64; if (!sustainPedal) releasePedalNotes(); }
            else if (d1 == 120 || d1 == 123) allNotesOff(d1 == 120);
        }
    }

    // --- en sampel (mono) -------------------------------------------------
    double next()
    {
        // Parametrar som glider (klickfritt)
        volSmoothed += paramSmooth * (targetVolume() - volSmoothed);
        for (int i = 0; i < 3; ++i) strLevel[i] += paramSmooth * (strTarget[i] - strLevel[i]);
        ensMix += paramSmooth * ((ensembleOn ? 1.0 : 0.0) - ensMix);

        // Vibrato (modulerar masterklockan, som på SK30)
        double vibAmt = 0.0;
        if (vibDepth > 0.0) {
            double fade = 1.0;
            if (vibDelay > 0.0) { fade = vibTime / vibDelay; fade = fade > 1.0 ? 1.0 : fade; fade = fade * fade * (3.0 - 2.0 * fade); }
            vibAmt = vibDepth * fade * std::sin(2.0 * kPi * vibPhase);
        }
        vibPhase += vibSpeed / sr; if (vibPhase >= 1.0) vibPhase -= 1.0;
        if (anyGate()) vibTime += 1.0 / sr;

        const double clkRatio = (tune / 440.0) * std::pow(2.0, vibAmt / 1200.0);
        const double clk[2] = { kMasterClock * clkRatio, kMasterClock * clkRatio * std::pow(2.0, detune / 1200.0) };

        // Fasräknarna för de 12 tonerna i toppoktaven (fri löpning, båda generatorerna)
        double inc[2][12];
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) inc[g][p] = clk[g] / divider[p] / sr; // toppoktavsperioder per sampel

        // Summera tangenterna per fotläge (U16, U8, U4)
        double u[3] = { 0.0, 0.0, 0.0 };
        const int gens = modeII ? 2 : 1;
        for (int v = 0; v < kVoices; ++v) {
            Voice& vo = voices[v];
            vo.advanceEnvelope(attackCoef(), releaseCoef(), stealCoef);
            if (vo.env < 1e-6 && !vo.gate && !vo.stealing) { vo.active = false; }
            if (!vo.active) continue;
            const int pc = vo.pc;
            for (int g = 0; g < gens; ++g) {
                for (int f = 0; f < 3; ++f) {
                    const int k = vo.shift + f - 1;            // 16' = shift+1, 8' = shift, 4' = shift-1
                    u[2 - f] += vo.env * square(phase[g][pc], inc[g][pc], k);
                }
            }
            if (vo.stealing && vo.env < 1e-3) {
                if (vo.pendingNote) vo.startPending();
                else { vo.stealing = false; vo.active = false; vo.env = 0.0; }
            }
        }
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) {
            phase[g][p] += inc[g][p];
            if (phase[g][p] >= kWrap) phase[g][p] -= kWrap;
        }

        // Trappstegs-sågtand (PG: IC20) och kopplingskondensatorer
        const double sq16 = hpIn[0].process(u[0]);
        const double sq8 = hpIn[1].process(u[1]);
        const double sq4 = hpIn[2].process(u[2]);
        const double saw4 = hpOut4.process(u[2]);
        const double saw8 = 0.69 * sq8 + 0.47 * hp4to8.process(sq4);
        const double saw16 = 0.69 * sq16 + 0.47 * hp8to16.process(saw8);

        // Stråkfilter + STRING-väljaren. 16' går in via 4,7k, 8'/4' via 6,8k.
        double s = 0.0;
        s += strLevel[0] * (1.0 / 4.7) * ladder[0].process(hpLadder[0].process(saw16));
        s += strLevel[1] * (1.0 / 6.8) * ladder[1].process(hpLadder[1].process(saw8));
        s += strLevel[2] * (1.0 / 6.8) * ladder[2].process(hpLadder[2].process(saw4));
        s *= kStringGain;

        // BRILLIANCE (CPC)
        s = brill.process(s);

        // ENSEMBLE (TE). Ensemblen körs alltid så att av/på blir klickfritt.
        const double wet = ensemble.process(s);
        double out = s + ensMix * (wet - s);

        out = dcBlock.process(out) * volSmoothed;
        // Säkerhet: mjuk begränsning nära 0 dBFS
        if (out > 0.9) out = 0.9 + 0.1 * std::tanh((out - 0.9) * 10.0);
        else if (out < -0.9) out = -0.9 + 0.1 * std::tanh((out + 0.9) * 10.0);
        return out;
    }

    // För tester / gränssnitt
    int activeVoices() const { int n = 0; for (int v = 0; v < kVoices; ++v) if (voices[v].active) ++n; return n; }
    bool isSounding() const { return activeVoices() > 0; }
    double dividerFor(int pc) const { return divider[pc]; }
    // Frekvensen tangenten (8') får utan vibrato (för tester).
    double noteFrequency(int midi, int gen = 0) const
    {
        const int pc = midi % 12;
        const int shift = shiftFor(midi);
        double clk = kMasterClock * (tune / 440.0);
        if (gen == 1) clk *= std::pow(2.0, detune / 1200.0);
        return clk / divider[pc] / (double)(1 << shift);
    }

private:
    static constexpr double kMasterClock = 1.0e6;   // 1 MHz (ΦUL1/ΦUL2)
    static constexpr double kStringGain = 0.15;     // ger ca -24 dBFS RMS för en ton och ca -16 dBFS för ett 4-tonsackord (STRING 1+2, MODE II)

    struct Voice
    {
        bool active = false, gate = false, held = false, pedalHeld = false;
        bool stealing = false, pendingNote = false;
        int note = -1, pc = 0, shift = 0;
        int pendNote = -1, pendPc = 0, pendShift = 0;
        double env = 0.0;
        uint32_t age = 0;

        void advanceEnvelope(double aCoef, double rCoef, double sCoef)
        {
            if (stealing) env -= env * sCoef;
            else if (gate) env += (1.0 - env) * aCoef;
            else env -= env * rCoef;
        }
        void startPending()
        {
            stealing = false; pendingNote = false;
            note = pendNote; pc = pendPc; shift = pendShift;
            gate = true; active = true; env = 0.0;
        }
    };

    static double par3(double a, double b, double c) { return 1.0 / (1.0 / a + 1.0 / b + 1.0 / c); }

    // Delare för toppoktaven: heltal, som i ett riktigt divide-down-chip.
    // Avrundningen ger de små (under 2 cent) avvikelserna från ren liksvävande stämning.
    void computeDividers()
    {
        for (int p = 0; p < 12; ++p) {
            const double f = 440.0 * std::pow(2.0, (kTopOctaveMidi + p - 69) / 12.0);
            divider[p] = std::floor(kMasterClock / f + 0.5);
        }
    }

    static int shiftFor(int midi)
    {
        // Antal oktaver under toppoktaven (8'). Minst 1 så att 4' finns.
        int s = (kTopOctaveMidi + (midi % 12) - midi) / 12;
        if (s < 1) s = 1;
        if (s > 12) s = 12;
        return s;
    }

    // Fyrkant (±1) delad med 2^k ur toppoktavens fasräknare, med PolyBLEP.
    static double square(double ph, double incTop, int k)
    {
        const double d = (double)(1 << k);
        double t = std::fmod(ph, d) / d;
        const double dt = incTop / d;
        double v = t < 0.5 ? 1.0 : -1.0;
        v += polyBlep(t, dt);
        double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
        v -= polyBlep(t2, dt);
        return v;
    }

    double attackCoef() const
    {
        const double t = slowAttack ? 0.080 : 0.003;      // tid till ca 95 %
        return 1.0 - std::exp(-3.0 / (t * sr));
    }
    double releaseCoef() const
    {
        const double t = sustainOn ? sustainTime : 0.015;  // tid till ca -40 dB
        return 1.0 - std::exp(-4.6 / (t * sr));
    }

    double targetVolume() const
    {
        const double db = volumeDb(volNorm);
        return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0);
    }

    void updateBrilliance(bool force)
    {
        (void)force;
        brill.highShelf(std::fmin(3000.0, sr * 0.4), brillDb, 0.7, sr);
    }

    bool anyGate() const { for (int v = 0; v < kVoices; ++v) if (voices[v].gate) return true; return false; }

    void noteOn(int midi)
    {
        if (midi < 12 || midi > 120) return;
        if (!anyGate()) vibTime = 0.0;             // vibratots fördröjning startar om
        const int pc = midi % 12, shift = shiftFor(midi);
        ++ageCounter;
        // Samma ton klingar redan ut: spela den igen i samma röst (som orgeln)
        for (int v = 0; v < kVoices; ++v) {
            Voice& vo = voices[v];
            if (vo.active && !vo.stealing && vo.note == midi) {
                vo.gate = true; vo.held = true; vo.pedalHeld = false; vo.age = ageCounter;
                return;
            }
        }
        int pick = -1;
        for (int v = 0; v < kVoices; ++v) if (!voices[v].active) { pick = v; break; }
        if (pick < 0) { // äldsta som klingar ut, annars äldsta
            uint32_t best = 0xFFFFFFFFu;
            for (int v = 0; v < kVoices; ++v) if (!voices[v].gate && voices[v].age < best) { best = voices[v].age; pick = v; }
            if (pick < 0) for (int v = 0; v < kVoices; ++v) if (voices[v].age < best) { best = voices[v].age; pick = v; }
        }
        Voice& vo = voices[pick];
        if (vo.active && vo.env > 1e-3) {
            // Snabb nedtoning (3 ms) innan rösten tar den nya tonen: inget klick
            vo.stealing = true; vo.pendingNote = true;
            vo.pendNote = midi; vo.pendPc = pc; vo.pendShift = shift;
            vo.gate = false; vo.held = true; vo.pedalHeld = false; vo.age = ageCounter;
            vo.note = midi; // räknas som den nya tonen (för note off)
        } else {
            vo = Voice();
            vo.active = true; vo.gate = true; vo.held = true;
            vo.note = midi; vo.pc = pc; vo.shift = shift; vo.age = ageCounter;
        }
    }

    void noteOff(int midi)
    {
        for (int v = 0; v < kVoices; ++v) {
            Voice& vo = voices[v];
            if (!vo.active || vo.note != midi || !vo.held) continue;
            vo.held = false;
            if (sustainPedal) { vo.pedalHeld = true; continue; }
            if (vo.stealing) vo.pendingNote = false; // tonen hann aldrig börja: rösten blir ledig
            vo.gate = false;
        }
    }

    void releasePedalNotes()
    {
        for (int v = 0; v < kVoices; ++v) if (voices[v].pedalHeld) { voices[v].pedalHeld = false; voices[v].gate = false; }
    }

    void allNotesOff(bool hard)
    {
        for (int v = 0; v < kVoices; ++v) {
            voices[v].gate = false; voices[v].held = false; voices[v].pedalHeld = false;
            voices[v].pendingNote = false; voices[v].stealing = false;
            if (hard) { voices[v].env = 0.0; voices[v].active = false; }
        }
    }

    // Tillstånd
    double sr = 44100.0;
    double divider[12];
    double phase[2][12];
    Voice voices[kVoices];
    uint32_t ageCounter = 0;
    bool sustainPedal = false;

    // Parametrar
    double volNorm = defaultParam(kParamVolume);
    double strTarget[3] = { 1.0, 1.0, 0.0 };
    double strLevel[3] = { 1.0, 1.0, 0.0 };
    bool slowAttack = true, sustainOn = true, modeII = true, ensembleOn = true;
    double sustainTime = 0.9, detune = 7.0, vibDelay = 0.8, vibSpeed = 5.5, vibDepth = 6.0;
    double brillDb = 0.0, tune = 440.0;
    double volSmoothed = 1.0, ensMix = 1.0, paramSmooth = 0.001;
    double stealCoef = 0.0;
    double vibPhase = 0.0, vibTime = 0.0;

    // Filter
    OnePoleHP hpIn[3], hp4to8, hp8to16, hpOut4, hpLadder[3], dcBlock;
    RCLadder ladder[3];
    Biquad brill;
    Ensemble ensemble;
};

} // namespace truesymph
