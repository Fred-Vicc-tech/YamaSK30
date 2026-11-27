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
    // --- etapp 2: poly-synth + tremolo ---
    kParamPolySel = 15,   // POLY-SYNTH: CANCEL, 1, 2, 3, MANUAL
    kParamPolyVol = 16,   // POLY-SYNTH VOLUME (-inf..+6 dB)
    kParamPolyFeet = 17,  // FEET: 4'/\, 8'⊓, BP8'/\, 8'/\, 16'⊓, 16'/\  (MANUAL)
    kParamCutoff = 18,    // CUTOFF FREQ (10 oktaver)
    kParamResonance = 19, // VCF RESONANCE (Q 0,5..10)
    kParamEgDepth = 20,   // EG DEPTH (0..10 oktaver)
    kParamAttack = 21,    // ATTACK TIME 0,003..3 s
    kParamDecay = 22,     // DECAY TIME 0,03..30 s
    kParamSustainLevel = 23, // SUSTAIN LEVEL 0..10
    kParamRelease = 24,   // RELEASE TIME 0,03..30 s
    kParamPolyTremolo = 25,  // POLY-SYNTH TREMOLO av/på
    kParamTremoloSpeed = 26, // TREMOLO SPEED: av 0,64 Hz / på 6,4 Hz
    // --- etapp 3: orgel ---
    kParamOrgSel = 27,    // ORGAN: CANCEL, 1, 2, 3, MANUAL
    kParamOrgVol = 28,    // ORGAN VOLUME (-inf..+6 dB)
    kParamOrgDecay = 29,  // DECAY-knappen av/på
    kParamOrgSustain = 30,// SUSTAIN-knappen av/på
    kParamOrgTime = 31,   // DECAY/SUSTAIN-reglaget 0,03..1,6 s
    kParamOrgBrill = 32,  // ORGAN BRILLIANCE -7..+7 dB
    kParamPercDecay = 33, // PERCUSSIVE DECAY TIME
    kParamPerc2 = 34,     // PERCUSSIVE 2nd (4')
    kParamPerc3 = 35,     // PERCUSSIVE 3rd (2 2/3')
    kParamPerc5 = 36,     // PERCUSSIVE 5th (1 3/5')
    kParamOrg16 = 37,     // fotlägen (tone levers) 16' .. 1'
    kParamOrg8 = 38,
    kParamOrg513 = 39,
    kParamOrg4 = 40,
    kParamOrg223 = 41,
    kParamOrg2 = 42,
    kParamOrg135 = 43,
    kParamOrg113 = 44,
    kParamOrg1 = 45,
    kParamOrgTremolo = 46,   // ORGAN TREMOLO av/på
    kParamOrgEnsemble = 47,  // ORGAN ENSEMBLE av/på
    kParamMaster = 48,       // MASTER VOLUME
    // --- etapp 4: solosynt, manual bass, keyboard split ---
    kParamSoloVol = 49,      // SOLO VOLUME (-inf..+6 dB)
    kParamSoloMode = 50,     // MODE: NORMAL (högsta tonen), BASS (lägsta tonen), MANUAL BASS
    kParamLfoSpeed = 51,     // LFO SPEED 0,1..100 Hz
    kParamPortamento = 52,   // PORTAMENTO 0..3 s (C3-C6)
    kParamGlide = 53,        // GLIDE DEPTH - .. + (70 ms)
    kParamSoloFeet = 54,     // VCO FEET: 4', 5 1/3', 8', 16', 32', 64'
    kParamSoloWave = 55,     // WAVE: sågtand -> fyrkant (blandbar)
    kParamSoloPw = 56,       // PW 50..85 %
    kParamPwSource = 57,     // PW-källa: LFO, MANUAL, EG
    kParamSoloCutoff = 58,   // VCF CUTOFF FREQ
    kParamSoloReso = 59,     // VCF RESONANCE
    kParamSoloMod = 60,      // VCF MODULATION (LFO, 0..3 oktaver)
    kParamSoloEgDepth = 61,  // VCF EG DEPTH (0..10 oktaver)
    kParamVcaMode = 62,      // VCA: HOLD, GATE, EG
    kParamSoloAttack = 63,   // EG ATTACK 0,003..3 s
    kParamSoloDecay = 64,    // EG DECAY 0,03..30 s
    kParamSoloSustain = 65,  // EG SUSTAIN 0..10
    kParamSoloRelease = 66,  // EG RELEASE 0,03..30 s
    kParamTouchVib = 67,     // TOUCH VIBRATO (±120 cent)
    kParamTouchBrill = 68,   // TOUCH BRILLIANCE (+5 oktaver)
    kParamTouchMod = 69,     // TOUCH MODULATION (6 oktaver)
    kParamSoloTune = 70,     // SOLO TUNE -500..+700 cent
    kParamSplit = 71,        // KEYBOARD SPLIT: OFF, POLY-SYNTH▼ORGAN, ORGAN▼POLY-SYNTH
    kParamSplitPoint = 72,   // delningspunkt (tangent)
    kParamBassPoint = 73,    // MANUAL BASS: översta basstangent
    // --- effekter (som på Fredriks inspelning: Korg SDD-3000 och Lexicon MPX500) ---
    kParamDelayTime = 74,    // DELAY TIME 1..1000 ms
    kParamDelayFeedback = 75,// DELAY FEEDBACK 0..95 %
    kParamDelayMod = 76,     // DELAY MOD (svaj på delaytiden) 0..100 %
    kParamDelayLevel = 77,   // DELAY LEVEL av..0 dB (ljudtaper: mitten -10 dB)
    kParamReverbDecay = 78,  // REVERB DECAY 0,3..8 s
    kParamReverbLevel = 79,  // REVERB LEVEL av..+8 dB (ljudtaper: mitten -2 dB)
    kParamDelaySpeed = 80,   // DELAY MOD SPEED 0,1..15 Hz (SDD-3000: VCO 0,1-15 Hz)
    kParamDelayHighCut = 81, // DELAY HIGH CUT: OFF, 8k, 4k, 2k (-6 dB/okt)
    kParamDelayLowCut = 82,  // DELAY LOW CUT: OFF, 125, 250, 500 Hz (-3 dB/okt)
    kParamReverbType = 83,   // REVERB TYPE: PLATE, HALL, CHAMBER, ROOM (som MPX500:s banker)
    kParamBoost = 84,        // BOOST: SK30:s utgångssteg (MO = 3 x mixen, VR1), +9,5 dB, på som standard
    kParamDelayMode = 85,    // DELAY: DIGI (Korg SDD-3000) eller TAPE (Roland Space Echo-stil)
    kNumParams
};

static const char* const kParamNames[kNumParams] = {
    "Volume", "Str 1", "Str 2", "Str 3", "SlowAtk", "Sustain", "SusTime",
    "Mode", "Detune", "VibDly", "VibSpd", "VibDep", "Brill", "Ensembl", "Tune",
    "PolySel", "PolyVol", "Feet", "Cutoff", "Reso", "EGDepth", "Attack", "Decay",
    "SusLvl", "Release", "PolyTrm", "TrmSpd",
    "OrgSel", "OrgVol", "OrgDcy", "OrgSus", "OrgTime", "OrgBril", "PercDcy", "Perc2", "Perc3", "Perc5",
    "Org16", "Org8", "Org513", "Org4", "Org223", "Org2", "Org135", "Org113", "Org1",
    "OrgTrm", "OrgEns", "Master",
    "SoloVol", "SMode", "LfoSpd", "Porta", "Glide", "SFeet", "SWave", "SPW", "PWSrc", "SCutoff",
    "SReso", "SMod", "SEGDep", "VCA", "SAttack", "SDecay", "SSusLvl", "SRelease", "TchVib", "TchBri",
    "TchMod", "SoloTun", "Split", "SplitPt", "BassPt",
    "DlyTime", "DlyFdbk", "DlyMod", "DlyLvl", "RevDcy", "RevLvl", "DlySpd", "DlyHiCt", "DlyLoCt", "RevType",
    "Boost", "DlyMode"
};
static const char* const kParamLabels[kNumParams] = {
    "dB", "", "", "", "", "", "s", "", "cent", "s", "Hz", "cent", "dB", "", "Hz",
    "", "dB", "", "Hz", "Q", "oct", "s", "s", "", "s", "", "",
    "", "dB", "", "", "s", "dB", "s", "", "", "",
    "", "", "", "", "", "", "", "", "",
    "", "", "dB",
    "dB", "", "Hz", "s", "", "", "", "%", "", "Hz",
    "Q", "oct", "oct", "", "s", "s", "", "s", "", "",
    "", "cent", "", "", "",
    "ms", "%", "", "dB", "s", "dB", "Hz", "", "", "",
    "", ""
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

// Poly-synth
const int kNumPolySel = 5;                       // CANCEL, 1, 2, 3, MANUAL
const int kNumFeet = 6;                          // 4'/\, 8'⊓, BP8'/\, 8'/\, 16'⊓, 16'/\  (panelens ordning)
inline int stepIndex(double n, int steps) { int i = (int)(clamp01(n) * (steps - 1) + 0.5); return i < 0 ? 0 : (i >= steps ? steps - 1 : i); }
inline double stepNorm(int i, int steps) { return steps > 1 ? (double)i / (steps - 1) : 0.0; }
inline double cutoffHz(double pos)      { return 20.0 * std::pow(2.0, 10.0 * clamp01(pos)); }   // 20 Hz..20 kHz
inline double resonanceQ(double pos)    { return 0.5 * std::pow(20.0, clamp01(pos)); }          // 0,5..10
inline double egDepthOct(double pos)    { return 10.0 * clamp01(pos); }
inline double attackSec(double pos)     { return 0.003 * std::pow(1000.0, clamp01(pos)); }
inline double decaySec(double pos)      { return 0.03 * std::pow(1000.0, clamp01(pos)); }
inline double releaseSec(double pos)    { return 0.03 * std::pow(1000.0, clamp01(pos)); }

// POLY-SYNTH 1/2/3: fasta spänningar från motståndsnäten på CPC-kortet.
// Varje rad har ett motstånd från V och 10k mot jord, alltså läge = 10k/(R+10k).
// Ordning: CUTOFF, RESONANCE, EG DEPTH, ATTACK, DECAY, SUSTAIN, RELEASE.
// Alla tre använder 16'-sågtanden (dioderna D8-D10 till FT1) och lågpass.
inline double presetPos(double rk) { return 10.0 / (rk + 10.0); }
static const double kPolyPresetR[3][7] = {
    { 18.0, 2.2, 27.0, 18.0, 15.0, 56.0, 3.3 },   // POLY-SYNTH 1 (Tr8, D11-D17)
    { 56.0, 1.2, 10.0, 12.0, 6.8, 8.2, 5.6 },     // POLY-SYNTH 2 (Tr9, D18-D24)
    { 15.0, 2.2, 22.0, 6.8, 18.0, 27.0, 4.7 },    // POLY-SYNTH 3 (Tr10, D25-D31)
};

// Orgel
const int kNumOrgSel = 5;                        // CANCEL, 1, 2, 3, MANUAL
const int kNumFootages = 9;                      // 16', 8', 5 1/3', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1'
// Fotlägenas avstånd i halvtoner från 8' (5 1/3', 2 2/3', 1 3/5', 1 1/3' lånas från andra
// tangenter, alltså liksvävande stämda, som i alla divide-down-orglar)
static const int kFootageSemis[kNumFootages] = { -12, 0, 7, 12, 19, 24, 28, 31, 36 };
// ORGAN 1/2/3 (CPB 9/9): fotlägen som blandas med lika stora motstånd (22k)
static const int kOrganPreset[3][kNumFootages] = {
    { 1, 1, 1, 0, 0, 0, 0, 0, 0 },               // ORGAN 1: 16' 8' 5 1/3'
    { 1, 1, 0, 1, 0, 1, 0, 0, 1 },               // ORGAN 2: 16' 8' 4' 2' 1'
    { 1, 1, 1, 1, 1, 1, 1, 1, 1 },               // ORGAN 3: alla
};
// Shuntmotstånden vid ingången (1k från OG1 mot 3,9k / 3,3k / 2,7k)
static const double kFootageShunt[kNumFootages] = { 1.0, 1.0, 3.9 / 4.9, 3.3 / 4.3, 2.7 / 3.7, 2.7 / 3.7, 2.7 / 3.7, 2.7 / 3.7, 2.7 / 3.7 };
inline double orgTimeSec(double n)   { return 0.03 * std::pow(1.6 / 0.03, clamp01(n)); }
inline double orgBrillDb(double n)   { return -7.0 + 14.0 * clamp01(n); }
// Slagverkets kondensator 15µF laddas ur genom 2,7k + DECAY TIME (A25k): tidskonstant 0,04..0,42 s
inline double percTau(double n)      { return 0.0405 * std::pow(0.4155 / 0.0405, clamp01(n)); }
// Tone lever (C-kurva): 0 = av, 10 = fullt. Ca 27 dB omfång, mjukt ner mot noll.
inline double leverGain(double n)
{
    n = clamp01(n);
    if (n <= 0.0) return 0.0;
    const double g = std::pow(10.0, -27.0 * (1.0 - n) / 20.0);
    return n < 0.1 ? g * n * 10.0 : g;
}

// Solosynt
const int kNumSoloModes = 3;                     // NORMAL, BASS, MANUAL BASS
const int kNumSoloFeet = 6;                      // 4', 5 1/3', 8', 16', 32', 64'
static const int kSoloFeetSemis[kNumSoloFeet] = { 12, 7, 0, -12, -24, -36 };
const int kNumPwSources = 3;                     // LFO, MANUAL, EG
const int kNumVcaModes = 3;                      // HOLD, GATE, EG
const int kNumSplit = 3;                         // OFF, POLY▼ORGAN, ORGAN▼POLY
inline double lfoSpeedHz(double n)   { return 0.1 * std::pow(1000.0, clamp01(n)); }
inline double portaSec(double n)     { n = clamp01(n); return 3.0 * n * n; }      // glidtid (RC), 0..3 s
inline double glideSemis(double n)   { return 12.0 * (2.0 * clamp01(n) - 1.0); }  // mitten = av
inline double soloPw(double n)       { return 0.50 + 0.35 * clamp01(n); }
inline double soloTuneCents(double n){ return -500.0 + 1200.0 * clamp01(n); }
inline int splitNote(double n)       { return 36 + (int)(clamp01(n) * 48.0 + 0.5); }  // MIDI 36..84
inline double normFromSplitNote(int m) { return (m - 36) / 48.0; }
inline void noteName(int m, char* out, size_t max)
{
    static const char* const nm[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    std::snprintf(out, max, "%s%d", nm[((m % 12) + 12) % 12], m / 12 - 2);   // Abletons namn: MIDI 60 = C3
}

// Effekter
inline double delayMs(double n)       { return std::pow(1000.0, clamp01(n)); }               // 1..1000 ms
inline double normFromDelayMs(double ms) { return std::log(ms) / std::log(1000.0); }
inline double delayFeedback(double n) { return 0.95 * clamp01(n); }
// DELAY LEVEL och REVERB LEVEL (0.9.1): ljudtaper, så att den användbara delen fyller skalan.
// Mitten av reglaget = 10 dB under toppen, en fjärdedel = 20 dB under. Toppen som förut.
const double kFxTaper = 10.0 / 6.0206;                 // 20*k*log10(0,5) = -10 dB
inline double fxLevelDb(double n)     { return n <= 0.0005 ? -120.0 : 20.0 * kFxTaper * std::log10(clamp01(n)); }       // av, ..0 dB
inline double normFromFxDb(double db) { return std::pow(10.0, db / (20.0 * kFxTaper)); }
// REVERB LEVEL: -36..+8 dB, så att toppen ger lika mycket klang som i 0.8.0 och tidigare
inline double revLevelDb(double n)    { return n <= 0.0005 ? -120.0 : 8.0 + 20.0 * kFxTaper * std::log10(clamp01(n)); } // av, ..+8 dB
inline double normFromRevDb(double db) { return std::pow(10.0, (db - 8.0) / (20.0 * kFxTaper)); }
// Värden sparade med den gamla linjära skalan (0.7.0-0.9.0) räknas om så att de låter likadant.
inline float convertOldDelayLevel(float n)  { return n <= 0.0f ? 0.0f : (float)normFromFxDb(-36.0 + 36.0 * clamp01(n)); }
inline float convertOldReverbLevel(float n) { return n <= 0.0f ? 0.0f : (float)normFromRevDb(-36.0 + 44.0 * clamp01(n)); }
inline void convertOldFxLevels(float* v)
{
    v[kParamDelayLevel] = convertOldDelayLevel(v[kParamDelayLevel]);
    v[kParamReverbLevel] = convertOldReverbLevel(v[kParamReverbLevel]);
}
inline double reverbSec(double n)     { return 0.3 * std::pow(8.0 / 0.3, clamp01(n)); }
// SDD-3000: MODULATION INTENSITY 0 - 2:1 (förhållandet mellan längsta och kortaste delaytid)
inline double delayModRatio(double n) { n = clamp01(n); return 1.0 + n * n; }
inline double delaySpeedHz(double n)  { return 0.1 * std::pow(150.0, clamp01(n)); }        // 0,1..15 Hz
inline double normFromDelaySpeed(double hz) { return std::log(hz / 0.1) / std::log(150.0); }
const int kNumHighCut = 4, kNumLowCut = 4, kNumReverbTypes = 10, kNumDelayModes = 6;
// REVERB TYPE: 0-3 Lexicon MPX500, 4-5 AMS RMX16, 6 fjäder, 7 EMT 140, 8 Lexicon 224, 9 EMT 250
const int kRevAmbience = 4, kRevNonLin = 5, kRevSpring = 6, kRevEmt140 = 7, kRevLex224 = 8, kRevEmt250 = 9;
// DELAY: 0 Korg SDD-3000, 1 Space Echo (ett huvud), 2 Space Echo (tre huvuden), 3 Echoplex, 4 Echorec, 5 Memory Man
const int kDlyDigi = 0, kDlyTape = 1, kDlyTape3 = 2, kDlyEchoplex = 3, kDlyEchorec = 4, kDlyMemoryMan = 5;
// Värden sparade med färre typer (äldre versioner) -> samma typ i den längre listan
inline float convertSteps(float n, int fromSteps, int toSteps) { return (float)stepNorm(stepIndex(n, fromSteps), toSteps); }
inline float convertOldReverbType(float n, int fromSteps = 4) { return convertSteps(n, fromSteps, kNumReverbTypes); }
static const double kHighCutHz[kNumHighCut] = { 0.0, 8000.0, 4000.0, 2000.0 };
static const double kLowCutHz[kNumLowCut] = { 0.0, 125.0, 250.0, 500.0 };
inline double normFromReverbSec(double s) { return std::log(s / 0.3) / std::log(8.0 / 0.3); }

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
    case kParamPolySel:     return 0.0f;                            // CANCEL
    case kParamPolyVol:     return (float)normFromVolumeDb(0.0);
    case kParamPolyFeet:    return (float)stepNorm(5, kNumFeet);    // 16' sågtand
    case kParamCutoff:      return 0.55f;
    case kParamResonance:   return 0.25f;
    case kParamEgDepth:     return 0.35f;
    case kParamAttack:      return 0.35f;
    case kParamDecay:       return 0.40f;
    case kParamSustainLevel:return 0.50f;
    case kParamRelease:     return 0.40f;
    case kParamPolyTremolo: return 0.0f;
    case kParamTremoloSpeed:return 1.0f;                            // 6,4 Hz
    case kParamOrgSel:      return 0.0f;                            // CANCEL
    case kParamOrgVol:      return (float)normFromVolumeDb(0.0);
    case kParamOrgDecay:    return 0.0f;
    case kParamOrgSustain:  return 1.0f;
    case kParamOrgTime:     return (float)(std::log(0.3 / 0.03) / std::log(1.6 / 0.03)); // 0,3 s
    case kParamOrgBrill:    return 0.5f;
    case kParamPercDecay:   return 0.5f;
    case kParamPerc2: case kParamPerc3: case kParamPerc5: return 0.0f;
    case kParamOrg16:       return 0.8f;
    case kParamOrg8:        return 0.8f;
    case kParamOrg4:        return 0.6f;
    case kParamOrg513: case kParamOrg223: case kParamOrg2: case kParamOrg135: case kParamOrg113: case kParamOrg1: return 0.0f;
    case kParamOrgTremolo:  return 0.0f;
    case kParamOrgEnsemble: return 0.0f;
    case kParamMaster:      return (float)normFromVolumeDb(0.0);
    case kParamSoloVol:     return 0.0f;                            // solosynten tyst tills den skruvas upp
    case kParamSoloMode:    return 0.0f;                            // NORMAL
    case kParamLfoSpeed:    return (float)(std::log(5.0 / 0.1) / std::log(1000.0)); // 5 Hz
    case kParamPortamento:  return 0.0f;
    case kParamGlide:       return 0.5f;                            // av
    case kParamSoloFeet:    return (float)stepNorm(2, kNumSoloFeet);// 8'
    case kParamSoloWave:    return 0.0f;                            // sågtand
    case kParamSoloPw:      return 0.3f;
    case kParamPwSource:    return (float)stepNorm(1, kNumPwSources); // MANUAL
    case kParamSoloCutoff:  return 0.55f;
    case kParamSoloReso:    return 0.3f;
    case kParamSoloMod:     return 0.0f;
    case kParamSoloEgDepth: return 0.35f;
    case kParamVcaMode:     return (float)stepNorm(2, kNumVcaModes); // EG
    case kParamSoloAttack:  return 0.15f;
    case kParamSoloDecay:   return 0.35f;
    case kParamSoloSustain: return 0.6f;
    case kParamSoloRelease: return 0.3f;
    case kParamTouchVib: case kParamTouchBrill: case kParamTouchMod: return 0.0f;
    case kParamSoloTune:    return (float)(500.0 / 1200.0);         // 0 cent
    case kParamSplit:       return 0.0f;                            // OFF
    // SK30:s tangenter C1-C6 = MIDI 36-96 (Yamahas och Abletons namn: C3 = MIDI 60).
    // Panelritningen (servicemanualen s. 4): ▼ vid F#3, MANUAL BASS-klammern upp till F#2.
    case kParamSplitPoint:  return (float)normFromSplitNote(67);    // delning mellan F#3 och G3
    case kParamBassPoint:   return (float)normFromSplitNote(54);    // MANUAL BASS upp till F#2
    case kParamDelayTime:   return (float)normFromDelayMs(380.0);
    case kParamDelayFeedback: return 0.3f;
    case kParamDelayMod:    return 0.1f;                            // svagt svaj (ca 2:1 = 1)
    case kParamDelaySpeed:  return (float)normFromDelaySpeed(0.5);
    case kParamDelayHighCut:return (float)stepNorm(1, kNumHighCut);  // 8 kHz
    case kParamDelayLowCut: return 0.0f;                            // OFF
    case kParamReverbType:  return 0.0f;                            // PLATE
    case kParamBoost:       return 1.0f;
    case kParamDelayMode:   return 0.0f;                            // DIGI (SDD-3000)                            // på (nivå i klass med andra syntar)
    case kParamDelayLevel:  return 0.0f;                            // av (torrt som originalet)
    case kParamReverbDecay: return (float)normFromReverbSec(3.0);
    case kParamReverbLevel: return 0.0f;                            // av
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
    case kParamSlowAttack: case kParamSustain: case kParamEnsemble: case kParamBoost:
        std::snprintf(out, max, "%s", isOn(n) ? "ON" : "OFF"); break;
    case kParamSustainTime: std::snprintf(out, max, "%.2f", sustainTimeSec(n)); break;
    case kParamMode:        std::snprintf(out, max, "%s", isOn(n) ? "II" : "I"); break;
    case kParamDetune:      std::snprintf(out, max, "%+.0f", detuneCents(n)); break;
    case kParamVibDelay:    std::snprintf(out, max, "%.1f", vibDelaySec(n)); break;
    case kParamVibSpeed:    std::snprintf(out, max, "%.1f", vibSpeedHz(n)); break;
    case kParamVibDepth:    std::snprintf(out, max, "%.0f", vibDepthCents(n)); break;
    case kParamBrilliance:  std::snprintf(out, max, "%+.1f", brillianceDb(n)); break;
    case kParamTune:        std::snprintf(out, max, "%.1f", tuneHz(n)); break;
    case kParamPolyVol: {
        const double db = volumeDb(n);
        if (db <= -100.0) std::snprintf(out, max, "-inf");
        else std::snprintf(out, max, "%.1f", db);
        break;
    }
    case kParamPolySel: {
        static const char* const t[kNumPolySel] = { "CANCEL", "1", "2", "3", "MANUAL" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumPolySel)]); break;
    }
    case kParamPolyFeet: {
        static const char* const t[kNumFeet] = { "4 SAW", "8 SQR", "BP8 SAW", "8 SAW", "16 SQR", "16 SAW" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumFeet)]); break;
    }
    case kParamCutoff: {
        const double f = cutoffHz(n);
        if (f >= 1000.0) std::snprintf(out, max, "%.1fk", f / 1000.0); else std::snprintf(out, max, "%.0f", f);
        break;
    }
    case kParamResonance:   std::snprintf(out, max, "%.1f", resonanceQ(n)); break;
    case kParamEgDepth:     std::snprintf(out, max, "%.1f", egDepthOct(n)); break;
    case kParamAttack:      std::snprintf(out, max, attackSec(n) < 1.0 ? "%.3f" : "%.2f", attackSec(n)); break;
    case kParamDecay:       std::snprintf(out, max, decaySec(n) < 1.0 ? "%.3f" : "%.2f", decaySec(n)); break;
    case kParamRelease:     std::snprintf(out, max, releaseSec(n) < 1.0 ? "%.3f" : "%.2f", releaseSec(n)); break;
    case kParamSustainLevel:std::snprintf(out, max, "%.1f", 10.0 * clamp01(n)); break;
    case kParamPolyTremolo: std::snprintf(out, max, "%s", isOn(n) ? "ON" : "OFF"); break;
    case kParamTremoloSpeed:std::snprintf(out, max, "%s", isOn(n) ? "6.4Hz" : "0.64Hz"); break;
    case kParamSoloMode: {
        static const char* const t[kNumSoloModes] = { "NORMAL", "BASS", "MAN BASS" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumSoloModes)]); break;
    }
    case kParamLfoSpeed:    std::snprintf(out, max, lfoSpeedHz(n) < 10.0 ? "%.2f" : "%.1f", lfoSpeedHz(n)); break;
    case kParamPortamento:  std::snprintf(out, max, "%.2f", portaSec(n)); break;
    case kParamGlide:       std::snprintf(out, max, "%+.1f", glideSemis(n)); break;
    case kParamSoloFeet: {
        static const char* const t[kNumSoloFeet] = { "4'", "5 1/3'", "8'", "16'", "32'", "64'" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumSoloFeet)]); break;
    }
    case kParamSoloWave:    std::snprintf(out, max, "%.0f%%", 100.0 * clamp01(n)); break;
    case kParamSoloPw:      std::snprintf(out, max, "%.0f", 100.0 * soloPw(n)); break;
    case kParamPwSource: {
        static const char* const t[kNumPwSources] = { "LFO", "MANUAL", "EG" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumPwSources)]); break;
    }
    case kParamSoloCutoff: {
        const double f = cutoffHz(n);
        if (f >= 1000.0) std::snprintf(out, max, "%.1fk", f / 1000.0); else std::snprintf(out, max, "%.0f", f);
        break;
    }
    case kParamSoloReso:    std::snprintf(out, max, "%.1f", resonanceQ(n)); break;
    case kParamSoloMod:     std::snprintf(out, max, "%.1f", 3.0 * clamp01(n)); break;
    case kParamSoloEgDepth: std::snprintf(out, max, "%.1f", egDepthOct(n)); break;
    case kParamVcaMode: {
        static const char* const t[kNumVcaModes] = { "HOLD", "GATE", "EG" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumVcaModes)]); break;
    }
    case kParamSoloAttack:  std::snprintf(out, max, attackSec(n) < 1.0 ? "%.3f" : "%.2f", attackSec(n)); break;
    case kParamSoloDecay:   std::snprintf(out, max, decaySec(n) < 1.0 ? "%.3f" : "%.2f", decaySec(n)); break;
    case kParamSoloRelease: std::snprintf(out, max, releaseSec(n) < 1.0 ? "%.3f" : "%.2f", releaseSec(n)); break;
    case kParamSoloSustain: std::snprintf(out, max, "%.1f", 10.0 * clamp01(n)); break;
    case kParamTouchVib: case kParamTouchBrill: case kParamTouchMod:
        std::snprintf(out, max, "%s", isOn(n) ? "ON" : "OFF"); break;
    case kParamSoloTune:    std::snprintf(out, max, "%+.0f", soloTuneCents(n)); break;
    case kParamSplit: {
        static const char* const t[kNumSplit] = { "OFF", "POLY|ORG", "ORG|POLY" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumSplit)]); break;
    }
    case kParamSplitPoint: case kParamBassPoint: noteName(splitNote(n), out, max); break;
    case kParamDelayTime:   std::snprintf(out, max, delayMs(n) < 10.0 ? "%.1f" : "%.0f", delayMs(n)); break;
    case kParamDelayFeedback: std::snprintf(out, max, "%.0f", 100.0 * delayFeedback(n)); break;
    case kParamDelayMod:    std::snprintf(out, max, "%.2f:1", delayModRatio(n)); break;
    case kParamDelaySpeed:  std::snprintf(out, max, delaySpeedHz(n) < 10.0 ? "%.2f" : "%.1f", delaySpeedHz(n)); break;
    case kParamDelayHighCut: { static const char* const t[kNumHighCut] = { "OFF", "8k", "4k", "2k" }; std::snprintf(out, max, "%s", t[stepIndex(n, kNumHighCut)]); break; }
    case kParamDelayLowCut: { static const char* const t[kNumLowCut] = { "OFF", "125", "250", "500" }; std::snprintf(out, max, "%s", t[stepIndex(n, kNumLowCut)]); break; }
    case kParamDelayMode: { static const char* const t[kNumDelayModes] = { "DIGI", "TAPE", "TAPE 3 HEADS", "ECHOPLEX", "ECHOREC", "MEMORY MAN" }; std::snprintf(out, max, "%s", t[stepIndex(n, kNumDelayModes)]); break; }
    case kParamReverbType: { static const char* const t[kNumReverbTypes] = { "PLATE", "HALL", "CHAMBER", "ROOM", "AMBIENCE", "NONLIN", "SPRING", "PLATE 140", "HALL 224", "DIGITAL 250" }; std::snprintf(out, max, "%s", t[stepIndex(n, kNumReverbTypes)]); break; }
    case kParamDelayLevel: case kParamReverbLevel: {
        const double db = index == kParamReverbLevel ? revLevelDb(n) : fxLevelDb(n);
        if (db <= -100.0) std::snprintf(out, max, "OFF"); else std::snprintf(out, max, "%.1f", db);
        break;
    }
    case kParamReverbDecay: std::snprintf(out, max, "%.1f", reverbSec(n)); break;
    case kParamOrgVol: case kParamMaster: case kParamSoloVol: {
        const double db = volumeDb(n);
        if (db <= -100.0) std::snprintf(out, max, "-inf");
        else std::snprintf(out, max, "%.1f", db);
        break;
    }
    case kParamOrgSel: {
        static const char* const t[kNumOrgSel] = { "CANCEL", "1", "2", "3", "MANUAL" };
        std::snprintf(out, max, "%s", t[stepIndex(n, kNumOrgSel)]); break;
    }
    case kParamOrgDecay: case kParamOrgSustain: case kParamOrgTremolo: case kParamOrgEnsemble:
        std::snprintf(out, max, "%s", isOn(n) ? "ON" : "OFF"); break;
    case kParamOrgTime:     std::snprintf(out, max, "%.2f", orgTimeSec(n)); break;
    case kParamOrgBrill:    std::snprintf(out, max, "%+.1f", orgBrillDb(n)); break;
    case kParamPercDecay:   std::snprintf(out, max, "%.2f", percTau(n) * 2.3); break;   // tid till -20 dB
    case kParamPerc2: case kParamPerc3: case kParamPerc5:
    case kParamOrg16: case kParamOrg8: case kParamOrg513: case kParamOrg4: case kParamOrg223:
    case kParamOrg2: case kParamOrg135: case kParamOrg113: case kParamOrg1:
        std::snprintf(out, max, "%.1f", 10.0 * clamp01(n)); break;
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

// Orgelns tonfilter (OG1, ett per fotläge):
//   in -4,7k- n1 [C1 mot jord, 1,5k + Cs mot jord] -33k- a -33k- b (= ut)
//   Cf från a till utgången, Cg från b mot jord (Sallen-Key, följare).
// Gör fyrkanterna mjukare (flöjt-liknande) i diskanten. Trapetsmetoden, 4x4.
struct OrganToneFilter
{
    double M[4][4];
    double G[4];
    double x[4] = { 0, 0, 0, 0 };
    double uPrev = 0.0;

    void set(double C1, double Cs, double Cf, double Cg, double sr)
    {
        const double R0 = 4.7e3, Rs = 1.5e3, Ra = 33e3, Rb = 33e3;
        // x = [v1, vcs, vCf (= va - vb), vb]; va = x2 + x3
        double A[4][4] = {
            { (-1.0 / R0 - 1.0 / Rs - 1.0 / Ra) / C1, (1.0 / Rs) / C1, (1.0 / Ra) / C1, (1.0 / Ra) / C1 },
            { 1.0 / (Rs * Cs), -1.0 / (Rs * Cs), 0.0, 0.0 },
            { (1.0 / Ra) / Cf, 0.0, (-1.0 / Ra - 1.0 / Rb) / Cf, (-1.0 / Ra) / Cf },
            { 0.0, 0.0, 1.0 / (Rb * Cg), 0.0 },
        };
        const double B[4] = { 1.0 / (R0 * C1), 0.0, 0.0, 0.0 };
        const double h = 0.5 / sr;
        double P[4][8];                                   // [I - A h | I]
        for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) { P[i][j] = (i == j ? 1.0 : 0.0) - A[i][j] * h; P[i][j + 4] = (i == j ? 1.0 : 0.0); }
        for (int c = 0; c < 4; ++c) {                     // Gauss-Jordan
            int piv = c;
            for (int r = c + 1; r < 4; ++r) if (std::fabs(P[r][c]) > std::fabs(P[piv][c])) piv = r;
            if (piv != c) for (int j = 0; j < 8; ++j) { const double t = P[c][j]; P[c][j] = P[piv][j]; P[piv][j] = t; }
            const double d = P[c][c];
            for (int j = 0; j < 8; ++j) P[c][j] /= d;
            for (int r = 0; r < 4; ++r) if (r != c) { const double f = P[r][c]; for (int j = 0; j < 8; ++j) P[r][j] -= f * P[c][j]; }
        }
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                double acc = 0.0;
                for (int k = 0; k < 4; ++k) acc += P[i][k + 4] * ((k == j ? 1.0 : 0.0) + A[k][j] * h);
                M[i][j] = acc;
            }
            double g = 0.0;
            for (int k = 0; k < 4; ++k) g += P[i][k + 4] * B[k] * h;
            G[i] = g;
        }
        reset();
    }
    void reset() { x[0] = x[1] = x[2] = x[3] = 0.0; uPrev = 0.0; }
    double process(double u)
    {
        const double s = u + uPrev;
        double n[4];
        for (int i = 0; i < 4; ++i) n[i] = M[i][0] * x[0] + M[i][1] * x[1] + M[i][2] * x[2] + M[i][3] * x[3] + G[i] * s;
        x[0] = n[0]; x[1] = n[1]; x[2] = n[2]; x[3] = n[3];
        uPrev = u;
        return x[3];                                       // likströmsförstärkning 1
    }
};

// Spänningsstyrt filter (IG00156 på CPC-kortet): tvåpoligt state variable-filter
// med lågpass- och bandpassutgång. TPT-struktur (stabil vid snabb modulation),
// med mjuk mättning som i ett analogt filter.
struct SVF
{
    double s1 = 0.0, s2 = 0.0;
    void reset() { s1 = s2 = 0.0; }
    // Returnerar lågpass i lp och bandpass (normerad till 0 dB topp) i bp.
    void process(double x, double g, double k, double& lp, double& bp)
    {
        x = std::tanh(x * 0.5) * 2.0;                 // ingångssteget mättar mjukt
        const double hp = (x - (k + g) * s1 - s2) / (1.0 + g * (g + k));
        const double b = g * hp + s1;
        s1 = g * hp + b;
        const double l = g * b + s2;
        s2 = g * b + l;
        lp = l;
        bp = b * k;
    }
};

// Envelope-generator (IG00151): ADSR med RC-kurvor. Attacken siktar över 1,0
// (som en kondensator mot en högre spänning) och vänder vid toppen.
struct ADSR
{
    enum Stage { Idle, Attack, Decay, Release };
    Stage stage = Idle;
    double v = 0.0;
    double aCoef = 0.0, dCoef = 0.0, rCoef = 0.0, sus = 0.5;
    static constexpr double kOvershoot = 1.3;

    void set(double attack, double decay, double sustain, double release, double sr)
    {
        aCoef = 1.0 - std::exp(-std::log(kOvershoot / (kOvershoot - 1.0)) / (attack * sr));
        dCoef = 1.0 - std::exp(-4.6 / (decay * sr));
        rCoef = 1.0 - std::exp(-4.6 / (release * sr));
        sus = sustain;
    }
    void trigger() { stage = Attack; }
    void releaseGate() { if (stage != Idle) stage = Release; }
    void reset() { stage = Idle; v = 0.0; }
    double next()
    {
        switch (stage) {
        case Attack:
            v += (kOvershoot - v) * aCoef;
            if (v >= 1.0) { v = 1.0; stage = Decay; }
            break;
        case Decay:   v += (sus - v) * dCoef; break;
        case Release: v -= v * rCoef; if (v < 1e-5) { v = 0.0; stage = Idle; } break;
        default: break;
        }
        return v;
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
    Biquad preLP1, postLP[3];
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

    // Tremololäge (ENSEMBLE av, TREMOLO på): samma klockchip ger en LFO
    // (0,64 eller 6,4 Hz) som både svajar delay-chippen och styr volymen
    // (VCA IC9). Ger en roterande högtalar-liknande effekt.
    static constexpr double kTremDelayDepthMs = 0.22;   // ca ±15 cent vid 6,4 Hz
    static constexpr double kTremAmDepth = 0.35;        // ca 3,7 dB volymsvajning
    bool tremoloMode = false;
    bool tremoloFast = true;
    double tremPhase = 0.0;
    double modeMix = 0.0;          // 0 = ensemble, 1 = tremolo (glider mjukt)
    double modeSmooth = 0.0;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        slowInc = kSlowHz / sr;
        fastInc = kFastHz / sr;
        modeSmooth = 1.0 - std::exp(-1.0 / (0.05 * sr));
        for (int i = 0; i < 3; ++i) { lfoSmooth[i].set(60.0, sr); postLP[i].lowpass(std::fmin(5400.0, sr * 0.45), 0.707, sr); }
        preLP1.lowpass(std::fmin(6600.0, sr * 0.45), 0.707, sr);
        reset();
    }
    void reset()
    {
        std::memset(buf, 0, sizeof buf);
        writePos = 0;
        noiseEnv = 0.0;
        slowPhase = 0.0; fastPhase = 0.0; tremPhase = 0.0;
        modeMix = tremoloMode ? 1.0 : 0.0;
        preLP1.reset();
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
        const double ens = kSlowDepthMs * steppedTri(slowPhase + off) + kFastDepthMs * steppedTri(fastPhase + off);
        const double trem = kTremDelayDepthMs * steppedTri(tremPhase + off);
        return ens + modeMix * (trem - ens);
    }
    double tremoloGain() const
    {
        const double l = 0.5 + 0.5 * steppedTri(tremPhase);
        return 1.0 - modeMix * kTremAmDepth * l;
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
        double in = preLP1.process(x);
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
        tremPhase += (tremoloFast ? fastInc : slowInc); if (tremPhase >= 1.0) tremPhase -= 1.0;
        modeMix += modeSmooth * ((tremoloMode ? 1.0 : 0.0) - modeMix);
        return sum * (1.0 / 3.0) * 1.41 * tremoloGain();  // tre linjer, ungefär samma upplevda nivå som torrt
    }
};

// ---------------------------------------------------------------------------
// Solosynten (CPB 7/9): en analog VCO (IG00153) med fotlägesväljare, vågform
// sågtand -> fyrkant, PW 50-85 % (LFO / MANUAL / EG), VCF (IG00156) med
// tangentföljning, VCA (HOLD / GATE / EG), ADSR, LFO 0,1-100 Hz, PORTAMENTO,
// GLIDE och TOUCH (tangenttryck: här aftertouch eller modulationshjulet).
// ---------------------------------------------------------------------------
struct SoloSynth
{
    double sr = 44100.0;
    // inställningar
    bool portaFootUsed = false, portaFoot = false;     // FOOT SW PORTAMENTO (CC65)
    double lfoHz = 5.0, portaTime = 0.0, glide = 0.0, wave = 0.0, pw = 0.6, cutoffPos = 0.55, resoPos = 0.3;
    double modOct = 0.0, egDepthPos = 0.35, tuneCents = 0.0;
    int feet = 2, pwSource = 1, vcaMode = 2;
    bool touchVib = false, touchBrill = false, touchMod = false;
    // tillstånd
    double pitch = 60.0, target = 60.0, glideOff = 0.0;
    bool hasNote = false, gate = false;
    double vcoPhase = 0.0, lfoPhase = 0.0, touch = 0.0, touchSmoothed = 0.0, vcaGate = 0.0;
    double drift = 0.0; uint32_t rnd = 0x9E3779B9u;
    ADSR eg;
    SVF vcf;
    OnePoleLP driftLP, gateLP;
    double glideCoef = 0.0, touchCoef = 0.0;
    double egKey[4] = { -1, -1, -1, -1 };
    double attackPos = 0.15, decayPos = 0.35, susPos = 0.6, releasePos = 0.3;

    void prepare(double rate)
    {
        sr = rate;
        driftLP.set(0.3, sr);
        gateLP.set(300.0, sr);                     // VCA i GATE-läge: mjuk kant (ca 0,5 ms)
        glideCoef = 1.0 - std::exp(-1.0 / (0.025 * sr));   // GLIDE: ca 70 ms
        touchCoef = 1.0 - std::exp(-1.0 / (0.02 * sr));
        egKey[0] = -1.0;
        reset();
    }
    void reset()
    {
        eg.reset(); vcf.reset(); vcoPhase = 0.0; lfoPhase = 0.0; hasNote = false; gate = false;
        glideOff = 0.0; vcaGate = 0.0; gateLP.reset(); driftLP.reset(); touchSmoothed = touch;
    }
    void updateEg()
    {
        if (egKey[0] == attackPos && egKey[1] == decayPos && egKey[2] == susPos && egKey[3] == releasePos) return;
        egKey[0] = attackPos; egKey[1] = decayPos; egKey[2] = susPos; egKey[3] = releasePos;
        eg.set(attackSec(attackPos), decaySec(decayPos), susPos, releaseSec(releasePos), sr);
    }

    // Ny ton (eller byte av ton). retrigger = ny tangent tryckt.
    void noteChange(int midi, bool retrigger)
    {
        target = midi;
        const double pt = effectivePorta();
        if (!hasNote || pt <= 0.005) pitch = target;   // första tonen hoppar direkt
        if (retrigger) {
            eg.trigger();
            glideOff = glide;                                  // GLIDE: tonen glider in från ovan/under
        }
        hasNote = true; gate = true;
    }
    void allReleased() { gate = false; eg.releaseGate(); }
    double effectivePorta() const { return (portaFootUsed && !portaFoot) ? 0.0 : portaTime; }

    double next(double tuneRatio)
    {
        updateEg();
        // PORTAMENTO: analog RC-glidning (som i Fredriks inspelning: samma tid oavsett intervall).
        // portaTime = tiden tills tonen nästan är framme (ca 95 %), max 3 s.
        const double pt = effectivePorta();
        if (pt > 0.005) {
            const double coef = 1.0 - std::exp(-3.0 / (pt * sr));
            pitch += (target - pitch) * coef;
        } else pitch = target;
        glideOff -= glideOff * glideCoef;

        // LFO (triangel)
        lfoPhase += lfoHz / sr; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const double lfo = lfoPhase < 0.5 ? 4.0 * lfoPhase - 1.0 : 3.0 - 4.0 * lfoPhase;
        touchSmoothed += touchCoef * (touch - touchSmoothed);
        const double env = eg.next();

        // Analog VCO: lite långsam drift (ca ±1,5 cent)
        rnd = rnd * 1664525u + 1013904223u;
        drift = driftLP.process(((int32_t)rnd) * (1.0 / 2147483648.0) * 40.0);
        double semis = pitch + kSoloFeetSemis[feet] + glideOff + tuneCents / 100.0 + drift * 0.015;
        if (touchVib) semis += 1.2 * touchSmoothed * lfo;    // TOUCH VIBRATO: upp till ±120 cent
        const double f = 440.0 * tuneRatio * std::pow(2.0, (semis - 69.0) / 12.0);
        double dt = f / sr; if (dt > 0.45) dt = 0.45;

        // Sågtand och puls med PolyBLEP
        vcoPhase += dt; if (vcoPhase >= 1.0) vcoPhase -= 1.0;
        double saw = 2.0 * vcoPhase - 1.0 - polyBlep(vcoPhase, dt);
        double pwv = pw;
        if (pwSource == 0) pwv = 0.5 + (pw - 0.5) * (0.5 + 0.5 * lfo);
        else if (pwSource == 2) pwv = 0.5 + (pw - 0.5) * env;
        double pulse = vcoPhase < pwv ? 1.0 : -1.0;
        pulse += polyBlep(vcoPhase, dt);
        double t2 = vcoPhase - pwv; if (t2 < 0.0) t2 += 1.0;
        pulse -= polyBlep(t2, dt);
        pulse -= 2.0 * pwv - 1.0;                              // ingen likström
        const double osc = saw * (1.0 - wave) + pulse * wave * 0.8;

        // VCF med tangentföljning (KV), EG, LFO och TOUCH
        double oct = egDepthOct(egDepthPos) * env + modOct * lfo + (pitch - 60.0) / 12.0;
        if (touchBrill) oct += 5.0 * touchSmoothed;
        if (touchMod) oct += 6.0 * touchSmoothed * lfo * 0.5;
        double fc = cutoffHz(cutoffPos) * std::pow(2.0, oct);
        const double fmax = sr * 0.45;
        if (fc > fmax) fc = fmax;
        if (fc < 10.0) fc = 10.0;
        double lp, bp;
        vcf.process(osc * 0.8, std::tan(kPi * fc / sr), 1.0 / resonanceQ(resoPos), lp, bp);

        // VCA
        double amp;
        if (vcaMode == 0) amp = 1.0;                           // HOLD: alltid öppen (drönare)
        else if (vcaMode == 1) amp = gateLP.process(gate ? 1.0 : 0.0);
        else amp = env;
        if (vcaMode != 1) gateLP.process(gate ? 1.0 : 0.0);
        if (!hasNote) amp = 0.0;                               // ingen ton spelad ännu
        return lp * amp;
    }
    bool isSounding() const { return hasNote && (gate || eg.stage != ADSR::Idle || vcaMode == 0); }
};

// ---------------------------------------------------------------------------
// DELAY efter Korg SDD-3000:s servicemanual: 0-1023 ms, effektbandbredd 17 kHz,
// LOW CUT 125/250/500 Hz (-3 dB/okt) och HIGH CUT 8/4/2 kHz (-6 dB/okt) i
// effektvägen (även återmatningen), VCO-modulation 0,1-15 Hz med INTENSITY upp
// till 2:1. Svajet går åt olika håll i vänster och höger, så ekot blir brett.
// ---------------------------------------------------------------------------
struct StereoDelay
{
    static const int kSize = 1 << 18;          // > 2 s vid 96 kHz (2:1-svaj på 1023 ms)
    float* buf = nullptr;
    int wp = 0;
    double sr = 44100.0;
    double timeSm = 0.38, lfo = 0.0;
    Biquad bandLP;                              // 17 kHz (effektens bandbredd)
    // Lägen: DIGI (SDD-3000), TAPE och TAPE 3 HEADS (Roland RE-201 Space Echo), ECHOPLEX
    // (Maestro, band), ECHOREC (Binson, magnettrumma med fyra huvuden), MEMORY MAN
    // (Electro-Harmonix, analog BBD). Alla utom DIGI har filter i återmatningen, så att
    // varje eko blir mörkare, och mättning; bandlägena svajar (wow och flutter).
    struct ModeDef { int heads; double loopLP, loopHP, drive, wow, flutter, glideSec; const double* outAmp; };
    int mode = 0;
    bool tape = false;                          // något annat än DIGI
    double wowPh = 0.0, flutPh = 0.0, wowDrift = 0.0, wowDriftT = 0.0, chorusPh = 0.0;
    uint32_t rng = 0x2F2F2F2Fu;
    OnePoleLP tapeLP, tapeLP2;
    OnePoleHP tapeHP;
    double bbdLPHz = -1.0;
    static ModeDef modeDef(int m)
    {
        static const double one[4] = { 1.0, 0, 0, 0 };
        static const double re3[4] = { 0.62, 0.52, 0.46, 0 };            // RE-201: huvud 1+2+3
        static const double rec[4] = { 0.50, 0.38, 0.44, 0.52 };         // Echorec: fyra huvuden, "swell"
        static const ModeDef d[kNumDelayModes] = {
            { 1, 0.0,    0.0,   0.0, 0.0,    0.0,    0.08, one },        // DIGI
            { 1, 3200.0, 110.0, 2.2, 0.0022, 0.0006, 0.08, one },        // TAPE
            { 3, 3200.0, 110.0, 2.2, 0.0022, 0.0006, 0.08, re3 },        // TAPE 3 HEADS
            { 1, 2500.0,  80.0, 3.0, 0.0012, 0.0004, 0.60, one },        // ECHOPLEX: varm, glidande tid
            { 4, 4200.0, 140.0, 1.6, 0.0004, 0.0008, 0.15, rec },        // ECHOREC: trumma, lite flutter
            { 1, 0.0,     60.0, 1.5, 0.0,    0.0,    0.10, one },        // MEMORY MAN: BBD (LP efter tiden)
        };
        return d[m < 0 ? 0 : (m >= kNumDelayModes ? kNumDelayModes - 1 : m)];
    }
    void setMode(int m)
    {
        if (m == mode) return;
        mode = m; tape = m != kDlyDigi;
        const ModeDef md = modeDef(m);
        if (md.loopLP > 0.0) { tapeLP.set(md.loopLP, sr); tapeLP2.set(md.loopLP * 1.6, sr); }
        if (md.loopHP > 0.0) tapeHP.set(md.loopHP, sr);
        bbdLPHz = -1.0;
    }
    OnePoleLP highCut;
    OnePoleHP lowCut;
    int highCutIdx = -1, lowCutIdx = -1;
    StereoDelay() { buf = new float[kSize]; std::memset(buf, 0, sizeof(float) * kSize); }
    ~StereoDelay() { delete[] buf; }
    StereoDelay(const StereoDelay&) = delete;
    StereoDelay& operator=(const StereoDelay&) = delete;
    void prepare(double rate)
    {
        sr = rate;
        bandLP.lowpass(std::fmin(17000.0, sr * 0.45), 0.707, sr);
        { const int m = mode; mode = -1; setMode(m); }
        highCutIdx = -1; lowCutIdx = -1;
        reset();
    }
    void reset()
    {
        std::memset(buf, 0, sizeof(float) * kSize); wp = 0; lfo = 0.0; bandLP.reset(); highCut.reset(); lowCut.reset();
        tapeLP.reset(); tapeLP2.reset(); tapeHP.reset(); wowPh = flutPh = wowDrift = wowDriftT = chorusPh = 0.0; rng = 0x2F2F2F2Fu;
    }
    void setFilters(int hi, int lo)
    {
        if (hi != highCutIdx) { highCutIdx = hi; if (hi > 0) highCut.set(kHighCutHz[hi], sr); }
        if (lo != lowCutIdx) { lowCutIdx = lo; if (lo > 0) lowCut.set(kLowCutHz[lo], sr); }
    }
    double read(double d) const
    {
        double rp = (double)wp - d;
        while (rp < 0.0) rp += kSize;
        const int i0 = (int)rp;
        const double f = rp - i0;
        return buf[i0 & (kSize - 1)] * (1.0 - f) + buf[(i0 + 1) & (kSize - 1)] * f;
    }
    // x = torrt (mono). Ut: vått vänster/höger.
    void process(double x, double timeSec, double feedback, double ratio, double speedHz, double& l, double& r)
    {
        const ModeDef md = modeDef(mode);
        // mjuk tidsändring (som SDD:ns ratt); ECHOPLEX glider långsamt så att tonhöjden sveper
        timeSm += (timeSec - timeSm) * (1.0 - std::exp(-1.0 / (md.glideSec * sr)));
        lfo += speedHz / sr; if (lfo >= 1.0) lfo -= 1.0;
        const double tri = lfo < 0.5 ? 4.0 * lfo - 1.0 : 3.0 - 4.0 * lfo;      // VCO: triangel
        // INTENSITY: tiden svajar mellan T/sqrt(r) och T*sqrt(r) (förhållande r:1)
        const double m = std::pow(ratio, 0.5 * tri);
        double d = timeSm * sr;
        if (md.wow > 0.0 || md.flutter > 0.0) {
            // wow (ca 0,55 Hz, långsamt vandrande) och flutter (ca 6,5 Hz)
            wowPh += 0.55 / sr; if (wowPh >= 1.0) { wowPh -= 1.0; rng = rng * 1664525u + 1013904223u; wowDriftT = ((rng >> 8) / 16777216.0) - 0.5; }
            flutPh += 6.5 / sr; if (flutPh >= 1.0) flutPh -= 1.0;
            wowDrift += (wowDriftT - wowDrift) * (1.0 / (0.8 * sr));
            d *= 1.0 + md.wow * std::sin(2.0 * kPi * wowPh) + 0.55 * md.wow * wowDrift + md.flutter * std::sin(2.0 * kPi * flutPh);
        }
        if (mode == kDlyMemoryMan) {
            // inbyggd chorus i BBD-linjen och bandbredd som sjunker med längre tid (lägre klocka)
            chorusPh += 0.45 / sr; if (chorusPh >= 1.0) chorusPh -= 1.0;
            d *= 1.0 + 0.0025 * std::sin(2.0 * kPi * chorusPh);
            double f = 2600.0 * std::sqrt(0.3 / std::fmax(timeSm, 0.02));
            f = f < 1200.0 ? 1200.0 : (f > 6000.0 ? 6000.0 : f);
            if (std::fabs(f - bbdLPHz) > 20.0) { bbdLPHz = f; tapeLP.set(f, sr); tapeLP2.set(f * 1.3, sr); }
        }
        auto clampD = [](double x) { return x < 1.0 ? 1.0 : (x > kSize - 4 ? (double)(kSize - 4) : x); };
        double yl = 0.0, yr = 0.0, fbSum = 0.0;
        for (int h = 0; h < md.heads; ++h) {
            // flera huvuden: jämnt fördelade upp till TIME (TIME = sista huvudet)
            const double frac = (double)(h + 1) / md.heads;
            const double hl = read(clampD(d * frac * m)), hr = read(clampD(d * frac / m));
            yl += md.outAmp[h] * hl; yr += md.outAmp[h] * hr;
            fbSum += 0.5 * (hl + hr) / md.heads;                  // återmatning: medel av huvudena (stabilt)
        }
        double fb = fbSum * feedback;
        double w;
        if (mode == kDlyMemoryMan) {
            // BBD: hela linjen är bandbegränsad (även första ekot), mättning i kompandern
            if (md.loopHP > 0.0) fb = tapeHP.process(fb);
            w = std::tanh(md.drive * (x + fb)) / md.drive;
            w = tapeLP2.process(tapeLP.process(w));
        } else if (tape) {
            fb = tapeLP2.process(tapeLP.process(fb));             // varje eko mörkare...
            if (md.loopHP > 0.0) fb = tapeHP.process(fb);         // ...och tunnare
            w = std::tanh(md.drive * (x + fb)) / md.drive;          // bandmättning
            w = bandLP.process(w);
        } else {
            fb = std::tanh(fb * 1.1) / 1.1;                         // lätt mättning vid hög återmatning
            w = bandLP.process(x + fb);
        }
        if (highCutIdx > 0) w = highCut.process(w);
        if (lowCutIdx > 0) { const double hp = lowCut.process(w); w = 0.5 * (w + hp) + 0.5 * hp; } // ca -3 dB/okt: halva vägen
        buf[wp] = (float)w;
        wp = (wp + 1) & (kSize - 1);
        l = yl; r = yr;
    }
};

// ---------------------------------------------------------------------------
// REVERB: plattreverb (Dattorro-struktur, ett klassiskt digitalt 80-talsrum i
// stil med Lexicon). Fyra diffusorer, två korskopplade tankar med svaj.
// ---------------------------------------------------------------------------
struct PlateReverb
{
    struct Line
    {
        float* b = nullptr; int n = 1, p = 0, len = 1;   // n = kapacitet, len = aktuell längd
        Line() {}
        Line(const Line&) = delete;
        Line& operator=(const Line&) = delete;
        void init(int cap) { n = cap < 2 ? 2 : cap; delete[] b; b = new float[n]; std::memset(b, 0, sizeof(float) * n); p = 0; len = n - 1; }
        void setLen(int l) { len = l < 1 ? 1 : (l > n - 1 ? n - 1 : l); }
        ~Line() { delete[] b; }
        float tap(int k) const { int i = p - k; while (i < 0) i += n; return b[i % n]; }  // k = 1 är senaste
        void push(float v) { b[p] = v; p = (p + 1) % n; }
        float last() const { return tap(len); }  // värdet för len sampel sedan
        void clear() { if (b) std::memset(b, 0, sizeof(float) * n); p = 0; }
    };
    struct AllPass
    {
        Line l; double g = 0.5;
        float process(float x) { const float d = l.last(); const float v = x + (float)g * d; l.push(v); return d - (float)g * v; }
    };
    double sr = 44100.0, scale = 1.0;
    Line pre;
    AllPass diff[4];
    // tankarna
    Line apModL, apModR;     // modulerade allpass (läses med svaj)
    Line d1L, d2L, d1R, d2R;
    AllPass ap2L, ap2R;
    double dampL = 0.0, dampR = 0.0, inLP = 0.0, fbL = 0.0, fbR = 0.0, lfo = 0.0;
    int modBaseL = 672, modBaseR = 908;
    double modExc = 16.0;
    int type = -1;
    double size = 1.0, damp = 0.35, bandwidth = 0.75, outGain = 0.6;

    // Typerna efter MPX500:s banker (manualens beskrivningar):
    //   PLATE   hög diffusion från början, ljus och färgad, ingen fördröjning
    //   HALL    större rum, ren efterklang, 25 ms fördröjning, mörkare diskant
    //   CHAMBER jämn och "dimensionslös", lite färgförändring, 12 ms
    //   ROOM    litet rum, 5 ms, mer dämpning
    struct TypeDef { double size, predelayMs, damp, bandwidth, inDiff1, inDiff2, gain, modScale, level; };
    static TypeDef typeDef(int t)
    {
        static const TypeDef d[kNumReverbTypes] = {
            { 1.00,  2.0, 0.30, 0.80, 0.75, 0.625, 0.60, 1.0, 1.0 },
            { 1.45, 25.0, 0.45, 0.65, 0.70, 0.55,  0.55, 1.0, 1.0 },
            { 1.15, 12.0, 0.38, 0.72, 0.72, 0.60,  0.58, 1.0, 1.0 },
            { 0.62,  5.0, 0.50, 0.70, 0.65, 0.55,  0.70, 1.0, 1.0 },
            { 0.42,  3.0, 0.22, 0.90, 0.78, 0.66,  0.95, 1.0, 1.0 },  // AMBIENCE (RMX16): tät tidig rymd, kort svans, ljus
            { 0.80,  0.0, 0.30, 0.85, 0.75, 0.625, 0.60, 1.0, 1.0 },  // NONLIN (RMX16): diffusorerna matar grinden nedan
            { 0.80,  0.0, 0.30, 0.85, 0.75, 0.625, 0.60, 1.0, 1.0 },  // SPRING: egen modell (fjädrar) nedan
            { 1.10,  0.0, 0.42, 0.58, 0.80, 0.72,  0.58, 0.6, 1.50 },  // PLATE 140 (EMT 140): stålplatta, mjukare och mörkare
            { 1.50, 20.0, 0.52, 0.52, 0.70, 0.58,  0.55, 2.4, 1.36 },  // HALL 224 (Lexicon 224): mörk, mjuk, svajande svans
            { 1.30,  8.0, 0.34, 0.68, 0.74, 0.62,  0.58, 1.3, 1.46 },  // DIGITAL 250 (EMT 250): lyxig svans med lite gryn
        };
        return d[t < 0 ? 0 : (t >= kNumReverbTypes ? kNumReverbTypes - 1 : t)];
    }

    // NONLIN: en tät "skur" av reflexer med jämn nivå som tar tvärt slut efter grindtiden
    // (AMS RMX16 NonLin 2). 48 tappar per kanal på fasta, slumpade platser inom grindtiden.
    static const int kNlTaps = 48;
    float* nlBuf = nullptr; int nlSize = 0, nlWp = 0;
    double nlPosL[kNlTaps], nlPosR[kNlTaps], nlAmpL[kNlTaps], nlAmpR[kNlTaps];
    int nlTapL[kNlTaps], nlTapR[kNlTaps];
    double nlGate = -1.0;
    ~PlateReverb() { delete[] nlBuf; delete[] spr[0].b; delete[] spr[1].b; }
    void setGate(double gateSec)
    {
        if (std::fabs(gateSec - nlGate) < 1e-4) return;
        nlGate = gateSec;
        const double n = gateSec * sr;
        for (int i = 0; i < kNlTaps; ++i) {
            nlTapL[i] = 1 + (int)(nlPosL[i] * n); nlTapR[i] = 1 + (int)(nlPosR[i] * n);
            if (nlTapL[i] >= nlSize) nlTapL[i] = nlSize - 1;
            if (nlTapR[i] >= nlSize) nlTapR[i] = nlSize - 1;
        }
    }
    // SPRING (fjäderreverb, Accutronics-typ): två fjädrar med olika längd. Varje fjäder är
    // en återmatad fördröjning med en kedja av allpassfilter (spridning: diskanten kommer
    // före basen, det typiska "boing"-ljudet), mörk och tunn som en riktig tank.
    static const int kSpringAp = 36;
    struct Spring { float* b = nullptr; int n = 0, p = 0, len = 1; double ap[kSpringAp] = {}; double lp = 0.0, lp2 = 0.0, hp = 0.0, hpx = 0.0, fb = 0.0; };
    Spring spr[2];
    // DIGITAL 250: lägre upplösning i utgången (EMT 250:s tidiga omvandlare) ger lite gryn
    double grain = 0.0;
    int S(double n) const { return (int)(n * scale * size + 0.5); }
    void prepare(double rate)
    {
        sr = rate; scale = sr / 29761.0;
        const double maxSize = 1.5;
        auto cap = [&](double n) { return (int)(n * scale * maxSize) + 8; };
        pre.init((int)(0.05 * sr) + 8);
        const int dl[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i) diff[i].l.init(cap(dl[i]));
        apModL.init(cap(672 + 32)); apModR.init(cap(908 + 32));
        d1L.init(cap(4453)); d2L.init(cap(3720)); d1R.init(cap(4217)); d2R.init(cap(3163));
        ap2L.l.init(cap(1800)); ap2L.g = 0.5; ap2R.l.init(cap(2656)); ap2R.g = 0.5;
        delete[] nlBuf; nlSize = 1; while (nlSize < (int)(1.3 * sr)) nlSize <<= 1;
        nlBuf = new float[nlSize]; std::memset(nlBuf, 0, sizeof(float) * nlSize); nlWp = 0;
        uint32_t r = 0x5EED1981u;
        auto rnd = [&r]() { r = r * 1664525u + 1013904223u; return (r >> 8) / 16777216.0; };
        for (int i = 0; i < kNlTaps; ++i) {
            // jämnt fördelade fack med slump inom facket, så att tätheten är jämn
            nlPosL[i] = (i + rnd()) / kNlTaps; nlPosR[i] = (i + rnd()) / kNlTaps;
            // jämn nivå med kort uppgång och tvärt slut sist (5 %)
            auto env = [](double u) { double a = u < 0.04 ? 0.6 + 10.0 * u : 1.0; if (u > 0.95) a *= (1.0 - u) / 0.05; return a; };
            const double sgnL = rnd() < 0.5 ? -1.0 : 1.0, sgnR = rnd() < 0.5 ? -1.0 : 1.0;
            nlAmpL[i] = sgnL * env(nlPosL[i]) / std::sqrt((double)kNlTaps);
            nlAmpR[i] = sgnR * env(nlPosR[i]) / std::sqrt((double)kNlTaps);
        }
        nlGate = -1.0;
        const double sl[2] = { 0.0331, 0.0397 };              // fjäderlängder (s)
        for (int k = 0; k < 2; ++k) {
            delete[] spr[k].b; spr[k].len = (int)(sl[k] * sr); spr[k].n = spr[k].len + 4;
            spr[k].b = new float[spr[k].n]; std::memset(spr[k].b, 0, sizeof(float) * spr[k].n); spr[k].p = 0;
        }
        type = -1;
        setType(0);
        reset();
    }
    void setType(int t)
    {
        if (t == type) return;
        type = t;
        const TypeDef td = typeDef(t);
        size = td.size; damp = td.damp; bandwidth = td.bandwidth; outGain = td.gain;
        pre.setLen((int)(td.predelayMs * 0.001 * sr) + 1);
        const int dl[4] = { 142, 107, 379, 277 };
        const double dg[4] = { td.inDiff1, td.inDiff1, td.inDiff2, td.inDiff2 };
        for (int i = 0; i < 4; ++i) { diff[i].l.setLen(S(dl[i])); diff[i].g = dg[i]; }
        modBaseL = S(672); modBaseR = S(908); modExc = 16.0 * scale * td.modScale;
        d1L.setLen(S(4453)); d2L.setLen(S(3720)); d1R.setLen(S(4217)); d2R.setLen(S(3163));
        ap2L.l.setLen(S(1800)); ap2R.l.setLen(S(2656));
    }
    void reset()
    {
        pre.clear(); for (int i = 0; i < 4; ++i) diff[i].l.clear();
        apModL.clear(); apModR.clear(); d1L.clear(); d2L.clear(); d1R.clear(); d2R.clear(); ap2L.l.clear(); ap2R.l.clear();
        if (nlBuf) std::memset(nlBuf, 0, sizeof(float) * nlSize);
        nlWp = 0;
        for (int k = 0; k < 2; ++k) {
            if (spr[k].b) std::memset(spr[k].b, 0, sizeof(float) * spr[k].n);
            spr[k].p = 0; for (int i = 0; i < kSpringAp; ++i) spr[k].ap[i] = 0.0;
            spr[k].lp = spr[k].lp2 = spr[k].hp = spr[k].hpx = spr[k].fb = 0.0;
        }
        dampL = dampR = inLP = fbL = fbR = lfo = 0.0;
    }
    float modAllPass(Line& l, int base, double exc, double ph, float x)
    {
        const double dly = base + exc * std::sin(ph);
        const int i = (int)dly; const double f = dly - i;
        const float d = (float)(l.tap(i) * (1.0 - f) + l.tap(i + 1) * f);
        const float g = -0.7f;                               // decay diffusion 1
        const float v = x + g * d;
        l.push(v);
        return d - g * v;
    }
    // Grindtid för NONLIN och efterklangstid för AMBIENCE ur DECAY-reglaget
    static double gateFor(double decaySec) { double g = 0.1 * decaySec; return g < 0.06 ? 0.06 : (g > 0.8 ? 0.8 : g); }
    static double ambienceDecay(double decaySec) { double d = 0.2 * decaySec; return d < 0.15 ? 0.15 : (d > 1.6 ? 1.6 : d); }
    // Nivåkompensation så att alla typer låter ungefär lika starka vid samma LEVEL
    double levelComp(double decaySec) const
    {
        if (type == kRevNonLin) return 2.55;
        if (type == kRevAmbience) return 1.1;
        if (type == kRevSpring) return kSpringLevel * std::sqrt(3.0 / springDecay(decaySec));
        return typeDef(type).level * std::sqrt(3.0 / decaySec);
    }
    static constexpr double kSpringLevel = 1.0;
    static double springDecay(double decaySec) { return decaySec > 5.0 ? 5.0 : decaySec; }   // fjädrar klingar sällan längre
    double springProcess(Spring& sp, double x, double g, double lpA, double hpA)
    {
        // allpasskedjan (första ordningen, a = -0,62): ger frekvensberoende fördröjning
        double v = x + sp.fb;
        const double a = -0.62;
        for (int i = 0; i < kSpringAp; ++i) { const double y = a * v + sp.ap[i]; sp.ap[i] = v - a * y; v = y; }
        // fördröjningen (fjäderns gångtid)
        const int rp = (sp.p - sp.len + sp.n) % sp.n;
        const double d = sp.b[rp];
        sp.b[sp.p] = (float)v; sp.p = (sp.p + 1) % sp.n;
        // mörk och tunn: två lågpass och ett högpass, sedan återmatning
        sp.lp += lpA * (d - sp.lp); sp.lp2 += lpA * (sp.lp - sp.lp2);
        sp.hp = hpA * (sp.hp + sp.lp2 - sp.hpx); sp.hpx = sp.lp2;
        sp.fb = sp.hp * g;
        return sp.hp;
    }
    void process(double x, double decaySec, double& outL, double& outR)
    {
        if (type == kRevNonLin) {
            const float in = pre.last(); pre.push((float)x);
            inLP += (in - inLP) * bandwidth;
            float v = (float)inLP;
            for (int i = 0; i < 4; ++i) v = diff[i].process(v);
            setGate(gateFor(decaySec));
            nlBuf[nlWp] = v;
            double l = 0.0, r = 0.0;
            for (int i = 0; i < kNlTaps; ++i) {
                l += nlAmpL[i] * nlBuf[(nlWp - nlTapL[i]) & (nlSize - 1)];
                r += nlAmpR[i] * nlBuf[(nlWp - nlTapR[i]) & (nlSize - 1)];
            }
            nlWp = (nlWp + 1) & (nlSize - 1);
            outL = outGain * l; outR = outGain * r;
            return;
        }
        if (type == kRevSpring) {
            const double T = springDecay(decaySec);
            const double lpA = 1.0 - std::exp(-2.0 * kPi * 4200.0 / sr), hpA = std::exp(-2.0 * kPi * 160.0 / sr);
            const double in = std::tanh(1.5 * x) / 1.5;            // tankens drivsteg mättar lite
            const double gl = std::pow(10.0, -3.0 * (spr[0].len / sr) / T), gr = std::pow(10.0, -3.0 * (spr[1].len / sr) / T);
            const double a = springProcess(spr[0], in, gl, lpA, hpA), b = springProcess(spr[1], in, gr, lpA, hpA);
            outL = 0.75 * a + 0.35 * b; outR = 0.35 * a + 0.75 * b;
            return;
        }
        if (type == kRevAmbience) decaySec = ambienceDecay(decaySec);
        // förstärkning per tankvarv för vald efterklangstid (-60 dB på decaySec)
        const double loop = (672.0 + 4453.0 + 1800.0 + 3720.0) * size / 29761.0;
        double g = std::pow(10.0, -3.0 * loop / (2.0 * decaySec));   // g används två gånger per tankhalva
        if (g > 0.98) g = 0.98;
        const float in = pre.last(); pre.push((float)x);
        inLP += (in - inLP) * bandwidth;                    // bandbredd
        float v = (float)inLP;
        for (int i = 0; i < 4; ++i) v = diff[i].process(v);
        lfo += 1.0 / sr; if (lfo >= 1.0) lfo -= 1.0;
        const double ph = 2.0 * kPi * lfo;
        // vänster tank
        float a = modAllPass(apModL, modBaseL, modExc, ph, v + (float)fbR);
        d1L.push(a);
        float t = d1L.last();
        dampL = t * (1.0 - damp) + dampL * damp;
        float b = ap2L.process((float)(dampL * g));
        d2L.push(b);
        fbL = d2L.last() * g;
        // höger tank
        float c = modAllPass(apModR, modBaseR, modExc, ph + kPi * 0.5, v + (float)fbL);
        d1R.push(c);
        float u = d1R.last();
        dampR = u * (1.0 - damp) + dampR * damp;
        float e = ap2R.process((float)(dampR * g));
        d2R.push(e);
        fbR = d2R.last() * g;
        // uttag (Dattorros tappar, skalade med rumsstorleken)
        outL = outGain * (d1R.tap(S(266)) + d1R.tap(S(2974)) - ap2R.l.tap(S(1913)) + d2R.tap(S(1996))
                          - d1L.tap(S(1990)) - ap2L.l.tap(S(187)) - d2L.tap(S(1066)));
        outR = outGain * (d1L.tap(S(353)) + d1L.tap(S(3627)) - ap2L.l.tap(S(1228)) + d2L.tap(S(2673))
                          - d1R.tap(S(2111)) - ap2R.l.tap(S(335)) - d2R.tap(S(121)));
        if (type == kRevEmt250) {
            // ca 13 bitars upplösning med lite brus som spärr: ett lätt gryn i svansen
            const double q = 1.0 / 4096.0;
            grain = grain * 0.5 + 0.5 * (((lfo * 7919.0) - std::floor(lfo * 7919.0)) - 0.5);
            outL = std::floor(outL / q + 0.5 + 0.3 * grain) * q;
            outR = std::floor(outR / q + 0.5 - 0.3 * grain) * q;
        }
    }
};

// ---------------------------------------------------------------------------
// Säkerhetslimiter sist i kedjan: TrueSymph går aldrig över -0,3 dBFS.
// 1,5 ms framförhållning (ingen överslängd), mjuk återgång på 150 ms.
// ---------------------------------------------------------------------------
struct Limiter
{
    // Framåtblickande limiter som aldrig klipper: kravet (förstärkningen som
    // behövs) hålls över hela framförhållningen och jämnas sedan ut med ett
    // glidande medelvärde lika långt. Då har förstärkningen alltid nått rätt
    // nivå precis när toppen kommer ut, och sänkningen sker som en mjuk ramp
    // (inga knäppar eller knaster som av en hård klippning).
    static const int kMaxLook = 512;
    float buf[kMaxLook];
    float bufR[kMaxLook];
    double need[kMaxLook + 1];
    double held[kMaxLook + 1];
    int pos = 0, hpos = 0, look = 88;
    double rel = 1.0, relCoef = 0.0;
    double minGain = 1.0;                       // för mätaren (lägsta förstärkning sedan senast)
    static constexpr double kCeiling = 0.966;   // -0,3 dBFS
    static constexpr double kLookSec = 0.002;   // 2 ms framförhållning (rapporteras som latens)

    void prepare(double sr)
    {
        look = (int)(kLookSec * sr + 0.5); if (look < 8) look = 8; if (look > kMaxLook) look = kMaxLook;
        relCoef = 1.0 - std::exp(-1.0 / (0.15 * sr));
        reset();
    }
    int latency() const { return look; }
    void reset()
    {
        for (int i = 0; i < kMaxLook; ++i) { buf[i] = 0.0f; bufR[i] = 0.0f; }
        for (int i = 0; i <= kMaxLook; ++i) { need[i] = 1.0; held[i] = 1.0; }
        pos = 0; hpos = 0; rel = 1.0; minGain = 1.0;
    }
    double gainFor(double a)
    {
        const int n = look + 1;
        need[hpos] = a > kCeiling ? kCeiling / a : 1.0;
        double m = 1.0;
        for (int i = 0; i < n; ++i) if (need[i] < m) m = need[i];
        rel = m < rel ? m : rel + (m - rel) * relCoef;      // sänk direkt, släpp långsamt
        held[hpos] = rel;
        double sum = 0.0;
        for (int i = 0; i < n; ++i) sum += held[i];
        hpos = hpos + 1 < n ? hpos + 1 : 0;
        const double g = sum / n;
        if (g < minGain) minGain = g;
        return g;
    }
    static double safety(double y) { return y > kCeiling ? kCeiling : (y < -kCeiling ? -kCeiling : y); }
    // Stereo: samma förstärkning i båda kanalerna (bilden flyttar sig inte)
    void processStereo(double& l, double& r)
    {
        const double g = gainFor(std::fmax(std::fabs(l), std::fabs(r)));
        const double dl = buf[pos], dr = bufR[pos];
        buf[pos] = (float)l; bufR[pos] = (float)r;
        pos = pos + 1 < look ? pos + 1 : 0;
        l = safety(dl * g); r = safety(dr * g);
    }
    double process(double x)
    {
        const double g = gainFor(std::fabs(x));
        const double d = buf[pos];
        buf[pos] = (float)x;
        pos = pos + 1 < look ? pos + 1 : 0;
        return safety(d * g);
    }
    double takeReductionDb() { const double g = minGain; minGain = 1.0; return g < 1.0 ? -20.0 * std::log10(g) : 0.0; }
};

// ---------------------------------------------------------------------------
// Motorn
// ---------------------------------------------------------------------------
class Engine
{
public:
    static const int kVoices = 7;           // SK30: 7 toner per sektion (BASS OFF)
    static const int kAllVoices = 14;       // KEYBOARD SPLIT: 7 + 7
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
        solo.prepare(sr);
        limiter.prepare(sr);
        delay.prepare(sr); reverb.prepare(sr);
        delay.setFilters(1, 0); reverb.setType(0);        // standard: HIGH CUT 8k, PLATE
        dcBlock.set(5.0, sr);
        paramSmooth = 1.0 - std::exp(-1.0 / (0.02 * sr));
        stealCoef = 1.0 - std::exp(-3.0 / (0.003 * sr));
        for (int i = 0; i < 5; ++i) coefKey[i] = -1.0;     // ny samplingsfrekvens: räkna om koefficienterna
        posSmooth = 1.0 - std::exp(-1.0 / (0.03 * sr));   // 10µF på styrledningarna: mjuka byten
        updatePolyEnv();
        // Orgelns tonfilter (OG1): C1 mot jord, Cs i serie med 1,5k, Sallen-Key Cf / Cg
        static const double kOF[kNumFootages][4] = {
            { 0.056e-6, 0.12e-6,  0.012e-6,  1800e-12 },  // 16'
            { 0.033e-6, 0.068e-6, 0.0068e-6, 820e-12 },   // 8'
            { 0.027e-6, 0.056e-6, 0.0056e-6, 680e-12 },   // 5 1/3'
            { 0.068e-6, 0.033e-6, 0.0033e-6, 390e-12 },   // 4'
            { 0.056e-6, 0.027e-6, 0.0027e-6, 330e-12 },   // 2 2/3'
            { 0.033e-6, 0.015e-6, 0.0015e-6, 220e-12 },   // 2'
            { 0.033e-6, 0.015e-6, 0.0015e-6, 220e-12 },   // 1 3/5'
            { 0.033e-6, 0.015e-6, 0.0015e-6, 220e-12 },   // 1 1/3'
            { 0.033e-6, 0.015e-6, 0.0015e-6, 220e-12 },   // 1'
        };
        for (int f = 0; f < kNumFootages; ++f) orgFilter[f].set(kOF[f][0], kOF[f][1], kOF[f][2], kOF[f][3], sr);
        orgBrill.highShelf(std::fmin(3000.0, sr * 0.4), orgBrillDbV, 0.7, sr);
        percCoef = 1.0 - std::exp(-1.0 / (percTau(0.5) * sr));
        orgAttackCoef = 1.0 - std::exp(-1.0 / (0.0007 * sr));     // klickdämpning (ca 2 ms)
        for (int i = 0; i < 3; ++i) percHp[i].set(i == 0 ? 72.0 : (i == 1 ? 106.0 : 234.0), sr);
        updateBrilliance(true);
        reset();
    }

    void reset()
    {
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) phase[g][p] = (p * 0.0731 + g * 0.37) * 64.0; // fri löpning, olika startfas
        for (int p = 0; p < 12; ++p) orgPhase[p] = (p * 0.1173 + 0.21) * 64.0;
        for (int f = 0; f < kNumFootages; ++f) { orgFilter[f].reset(); orgWeight[f] = targetFootage(f); }
        orgBrill.reset(); percEnv = 0.0;
        for (int i = 0; i < 3; ++i) { percLevel[i] = percTarget[i]; percHp[i].reset(); }
        orgVolSmoothed = targetOrgVolume(); masterSmoothed = targetMaster();
        boostSm = boostTarget; exprTarget = exprSm = 1.0; solo.portaFootUsed = false;
        orgTe = (orgEnsemble || orgTremolo) ? 1.0 : 0.0;
        for (int v = 0; v < kAllVoices; ++v) voices[v] = Voice();
        for (int m = 0; m < 128; ++m) { held[m] = false; heldOrder[m] = 0; }
        solo.reset(); soloVolSmoothed = targetSoloVolume(); soloNote = -1;
        for (int i = 0; i < 3; ++i) { hpIn[i].reset(); hpLadder[i].reset(); ladder[i].reset(); }
        hp4to8.reset(); hp8to16.reset(); hpOut4.reset();
        brill.reset(); ensemble.reset(); dcBlock.reset(); limiter.reset();
        delay.reset(); reverb.reset();
        dlyActive = 0; revActive = 0; dlyLevelSm = dlyLevelTarget; revLevelSm = revLevelTarget;
        vibPhase = 0.0; vibTime = 0.0;
        sustainPedal = false;
        ageCounter = 0;
        volSmoothed = targetVolume();
        for (int i = 0; i < 3; ++i) strLevel[i] = strTarget[i];
        ensMix = (ensembleOn || polyTremolo) ? 1.0 : 0.0;
        ensemble.tremoloMode = !(ensembleOn || orgEnsemble) && (polyTremolo || orgTremolo);
        svf.reset(); polyEnv.reset(); prevAnyGate = false;
        polyVolSmoothed = targetPolyVolume();
        curCut = polyPos(0); curReso = polyPos(1); curDepth = polyPos(2);
        polyLevel = polySel > 0 ? 1.0 : 0.0;
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

    // Poly-synth
    void setPolySelNorm(double n)     { const int v = stepIndex(n, kNumPolySel); if (v != polySel) { polySel = v; updatePolyEnv(); } }
    void setPolyVolNorm(double n)     { polyVolNorm = n; }
    void setPolyFeetNorm(double n)    { polyFeet = stepIndex(n, kNumFeet); }
    void setCutoffNorm(double n)      { manCutoff = clamp01(n); }
    void setResonanceNorm(double n)   { manReso = clamp01(n); }
    void setEgDepthNorm(double n)     { manDepth = clamp01(n); }
    void setAttackNorm(double n)      { if (n != manAttack) { manAttack = clamp01(n); updatePolyEnv(); } }
    void setDecayNorm(double n)       { if (n != manDecay) { manDecay = clamp01(n); updatePolyEnv(); } }
    void setSustainLevelNorm(double n){ if (n != manSus) { manSus = clamp01(n); updatePolyEnv(); } }
    void setReleaseNorm(double n)     { if (n != manRelease) { manRelease = clamp01(n); updatePolyEnv(); } }
    void setPolyTremoloNorm(double n) { polyTremolo = isOn(n); }
    void setTremoloSpeedNorm(double n){ ensemble.tremoloFast = isOn(n); }

    // Orgel
    void setOrgSelNorm(double n)      { orgSel = stepIndex(n, kNumOrgSel); }
    void setOrgVolNorm(double n)      { orgVolNorm = n; }
    void setOrgDecayNorm(double n)    { orgDecay = isOn(n); }
    void setOrgSustainNorm(double n)  { orgSustain = isOn(n); }
    void setOrgTimeNorm(double n)     { orgTime = orgTimeSec(n); }
    void setOrgBrillNorm(double n)    { const double d = orgBrillDb(n); if (d != orgBrillDbV) { orgBrillDbV = d; orgBrill.highShelf(std::fmin(3000.0, sr * 0.4), orgBrillDbV, 0.7, sr); } }
    void setPercDecayNorm(double n)   { percCoef = 1.0 - std::exp(-1.0 / (percTau(n) * sr)); }
    void setPercNorm(int i, double n) { if (i >= 0 && i < 3) percTarget[i] = leverGain(n); }
    void setFootageNorm(int i, double n) { if (i >= 0 && i < kNumFootages) leverTarget[i] = leverGain(n); }
    void setOrgTremoloNorm(double n)  { orgTremolo = isOn(n); }
    void setOrgEnsembleNorm(double n) { orgEnsemble = isOn(n); }
    void setMasterNorm(double n)      { masterNorm = n; }

    // Solosynt, manual bass och split
    void setSoloVolNorm(double n)     { soloVolNorm = clamp01(n); }
    void setSoloModeNorm(double n)    { const int m = stepIndex(n, kNumSoloModes); if (m != soloMode) { soloMode = m; updateSolo(false); } }
    void setLfoSpeedNorm(double n)    { solo.lfoHz = lfoSpeedHz(n); }
    void setPortamentoNorm(double n)  { solo.portaTime = portaSec(n); }
    void setGlideNorm(double n)       { solo.glide = glideSemis(n); }
    void setSoloFeetNorm(double n)    { solo.feet = stepIndex(n, kNumSoloFeet); }
    void setSoloWaveNorm(double n)    { solo.wave = clamp01(n); }
    void setSoloPwNorm(double n)      { solo.pw = soloPw(n); }
    void setPwSourceNorm(double n)    { solo.pwSource = stepIndex(n, kNumPwSources); }
    void setSoloCutoffNorm(double n)  { solo.cutoffPos = clamp01(n); }
    void setSoloResoNorm(double n)    { solo.resoPos = clamp01(n); }
    void setSoloModNorm(double n)     { solo.modOct = 3.0 * clamp01(n); }
    void setSoloEgDepthNorm(double n) { solo.egDepthPos = clamp01(n); }
    void setVcaModeNorm(double n)     { solo.vcaMode = stepIndex(n, kNumVcaModes); }
    void setSoloAttackNorm(double n)  { solo.attackPos = clamp01(n); }
    void setSoloDecayNorm(double n)   { solo.decayPos = clamp01(n); }
    void setSoloSustainNorm(double n) { solo.susPos = clamp01(n); }
    void setSoloReleaseNorm(double n) { solo.releasePos = clamp01(n); }
    void setTouchVibNorm(double n)    { solo.touchVib = isOn(n); }
    void setTouchBrillNorm(double n)  { solo.touchBrill = isOn(n); }
    void setTouchModNorm(double n)    { solo.touchMod = isOn(n); }
    void setSoloTuneNorm(double n)    { solo.tuneCents = soloTuneCents(n); }
    void setSplitNorm(double n)       { split = stepIndex(n, kNumSplit); }
    void setSplitPointNorm(double n)  { splitPoint = splitNote(n); }
    void setDelayTimeNorm(double n)   { dlyTime = delayMs(n) * 0.001; }
    void setDelayFeedbackNorm(double n){ dlyFeedback = delayFeedback(n); }
    void setDelayModNorm(double n)    { dlyMod = clamp01(n); }
    void setDelayLevelNorm(double n)  { dlyLevelTarget = fxGain(n); }
    void setReverbDecayNorm(double n) { revDecay = reverbSec(n); }
    void setReverbLevelNorm(double n) { const double db = revLevelDb(n); revLevelTarget = db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0); }
    void setDelaySpeedNorm(double n)  { dlySpeed = delaySpeedHz(n); }
    void setDelayFiltersNorm(double hi, double lo) { delay.setFilters(stepIndex(hi, kNumHighCut), stepIndex(lo, kNumLowCut)); }
    void setReverbTypeNorm(double n)  { reverb.setType(stepIndex(n, kNumReverbTypes)); }
    void setDelayModeNorm(double n)   { delay.setMode(stepIndex(n, kNumDelayModes)); }
    void setBassPointNorm(double n)   { const int b = splitNote(n); if (b != bassPoint) { bassPoint = b; updateSolo(false); } }

    // Alla parametrar på en gång (normaliserade 0..1, index enligt ParamIndex)
    void setParams(const float* v)
    {
        setVolumeNorm(v[kParamVolume]);
        setStringNorm(0, v[kParamString1]);
        setStringNorm(1, v[kParamString2]);
        setStringNorm(2, v[kParamString3]);
        setSlowAttackNorm(v[kParamSlowAttack]);
        setSustainNorm(v[kParamSustain]);
        setSustainTimeNorm(v[kParamSustainTime]);
        setModeNorm(v[kParamMode]);
        setDetuneNorm(v[kParamDetune]);
        setVibDelayNorm(v[kParamVibDelay]);
        setVibSpeedNorm(v[kParamVibSpeed]);
        setVibDepthNorm(v[kParamVibDepth]);
        setBrillianceNorm(v[kParamBrilliance]);
        setEnsembleNorm(v[kParamEnsemble]);
        setTuneNorm(v[kParamTune]);
        setPolySelNorm(v[kParamPolySel]);
        setPolyVolNorm(v[kParamPolyVol]);
        setPolyFeetNorm(v[kParamPolyFeet]);
        setCutoffNorm(v[kParamCutoff]);
        setResonanceNorm(v[kParamResonance]);
        setEgDepthNorm(v[kParamEgDepth]);
        setAttackNorm(v[kParamAttack]);
        setDecayNorm(v[kParamDecay]);
        setSustainLevelNorm(v[kParamSustainLevel]);
        setReleaseNorm(v[kParamRelease]);
        setPolyTremoloNorm(v[kParamPolyTremolo]);
        setTremoloSpeedNorm(v[kParamTremoloSpeed]);
        setOrgSelNorm(v[kParamOrgSel]);
        setOrgVolNorm(v[kParamOrgVol]);
        setOrgDecayNorm(v[kParamOrgDecay]);
        setOrgSustainNorm(v[kParamOrgSustain]);
        setOrgTimeNorm(v[kParamOrgTime]);
        setOrgBrillNorm(v[kParamOrgBrill]);
        setPercDecayNorm(v[kParamPercDecay]);
        for (int i = 0; i < 3; ++i) setPercNorm(i, v[kParamPerc2 + i]);
        for (int i = 0; i < kNumFootages; ++i) setFootageNorm(i, v[kParamOrg16 + i]);
        setOrgTremoloNorm(v[kParamOrgTremolo]);
        setOrgEnsembleNorm(v[kParamOrgEnsemble]);
        setMasterNorm(v[kParamMaster]);
        setSoloVolNorm(v[kParamSoloVol]);
        setSoloModeNorm(v[kParamSoloMode]);
        setLfoSpeedNorm(v[kParamLfoSpeed]);
        setPortamentoNorm(v[kParamPortamento]);
        setGlideNorm(v[kParamGlide]);
        setSoloFeetNorm(v[kParamSoloFeet]);
        setSoloWaveNorm(v[kParamSoloWave]);
        setSoloPwNorm(v[kParamSoloPw]);
        setPwSourceNorm(v[kParamPwSource]);
        setSoloCutoffNorm(v[kParamSoloCutoff]);
        setSoloResoNorm(v[kParamSoloReso]);
        setSoloModNorm(v[kParamSoloMod]);
        setSoloEgDepthNorm(v[kParamSoloEgDepth]);
        setVcaModeNorm(v[kParamVcaMode]);
        setSoloAttackNorm(v[kParamSoloAttack]);
        setSoloDecayNorm(v[kParamSoloDecay]);
        setSoloSustainNorm(v[kParamSoloSustain]);
        setSoloReleaseNorm(v[kParamSoloRelease]);
        setTouchVibNorm(v[kParamTouchVib]);
        setTouchBrillNorm(v[kParamTouchBrill]);
        setTouchModNorm(v[kParamTouchMod]);
        setSoloTuneNorm(v[kParamSoloTune]);
        setSplitNorm(v[kParamSplit]);
        setSplitPointNorm(v[kParamSplitPoint]);
        setBassPointNorm(v[kParamBassPoint]);
        setDelayTimeNorm(v[kParamDelayTime]);
        setDelayFeedbackNorm(v[kParamDelayFeedback]);
        setDelayModNorm(v[kParamDelayMod]);
        setDelayLevelNorm(v[kParamDelayLevel]);
        setReverbDecayNorm(v[kParamReverbDecay]);
        setReverbLevelNorm(v[kParamReverbLevel]);
        setDelaySpeedNorm(v[kParamDelaySpeed]);
        setDelayFiltersNorm(v[kParamDelayHighCut], v[kParamDelayLowCut]);
        setReverbTypeNorm(v[kParamReverbType]);
        boostTarget = isOn(v[kParamBoost]) ? kBoostGain : 1.0;
        setDelayModeNorm(v[kParamDelayMode]);
    }

    // --- MIDI --------------------------------------------------------------
    void handleMidi(uint8_t status, uint8_t d1, uint8_t d2)
    {
        const uint8_t type = status & 0xF0;
        if (type == 0x90 && d2 > 0) noteOn(d1);
        else if (type == 0x80 || (type == 0x90 && d2 == 0)) noteOff(d1);
        else if (type == 0xD0) { aftertouch = d1 / 127.0; solo.touch = std::fmax(modWheel, aftertouch); }   // TOUCH
        else if (type == 0xA0) { if (d1 == soloNote) { aftertouch = d2 / 127.0; solo.touch = std::fmax(modWheel, aftertouch); } }
        else if (type == 0xB0) {
            if (d1 == 64) { sustainPedal = d2 >= 64; if (!sustainPedal) releasePedalNotes(); }
            else if (d1 == 1) { modWheel = d2 / 127.0; solo.touch = std::fmax(modWheel, aftertouch); }
            // FOOT CONT (MIXED VOLUME): expressionpedal, CC11
            else if (d1 == 11) { const double x = d2 / 127.0; exprTarget = x * x; }
            // FOOT SW PORTAMENTO: nedtryckt = portamento på (med PORTAMENTO TIME); släppt = av.
            // Utan pedal (inget CC65 mottaget) gäller reglaget som vanligt.
            else if (d1 == 65) { solo.portaFootUsed = true; solo.portaFoot = d2 >= 64; }
            else if (d1 == 121) { exprTarget = 1.0; solo.portaFootUsed = false; }   // återställ kontroller
            else if (d1 == 120 || d1 == 123) allNotesOff(d1 == 120);
        }
    }

    // --- en sampel ---------------------------------------------------------
    // Mono (för tester och analyser): medelvärdet av vänster och höger.
    double next() { double l, r; nextStereo(l, r); return 0.5 * (l + r); }

    // Stereo: SK30 är mono; delay och reverb (om de är på) gör ljudet brett.
    void nextStereo(double& outL, double& outR)
    {
        // Parametrar som glider (klickfritt)
        volSmoothed += paramSmooth * (targetVolume() - volSmoothed);
        for (int i = 0; i < 3; ++i) { strLevel[i] += paramSmooth * (strTarget[i] - strLevel[i]); if (std::fabs(strTarget[i] - strLevel[i]) < 1e-4) strLevel[i] = strTarget[i]; }
        // TREMOLO/ENSEMBLE: ett gemensamt TE-kort. Ensemble har prioritet (ENSEMBLE priority).
        const bool teOn = ensembleOn || polyTremolo;          // stråkar + poly-synth går genom TE
        ensemble.tremoloMode = !(ensembleOn || orgEnsemble) && (polyTremolo || orgTremolo);
        ensMix += paramSmooth * ((teOn ? 1.0 : 0.0) - ensMix);
        orgTe += paramSmooth * (((orgEnsemble || orgTremolo) ? 1.0 : 0.0) - orgTe);
        orgVolSmoothed += paramSmooth * (targetOrgVolume() - orgVolSmoothed);
        masterSmoothed += paramSmooth * (targetMaster() - masterSmoothed);
        if (std::fabs(targetMaster() - masterSmoothed) < 1e-4) masterSmoothed = targetMaster();
        for (int f = 0; f < kNumFootages; ++f) { const double t = targetFootage(f); orgWeight[f] += paramSmooth * (t - orgWeight[f]); if (std::fabs(t - orgWeight[f]) < 1e-4) orgWeight[f] = t; }
        for (int i = 0; i < 3; ++i) percLevel[i] += paramSmooth * ((orgSel > 0 ? percTarget[i] : 0.0) - percLevel[i]);
        polyVolSmoothed += paramSmooth * (targetPolyVolume() - polyVolSmoothed);
        polyLevel += paramSmooth * ((polySel > 0 ? 1.0 : 0.0) - polyLevel);
        curCut += posSmooth * (polyPos(0) - curCut);
        curReso += posSmooth * (polyPos(1) - curReso);
        curDepth += posSmooth * (polyPos(2) - curDepth);

        // Vibrato (modulerar masterklockan, som på SK30)
        double vibAmt = 0.0;
        if (vibDepth > 0.0) {
            double fade = 1.0;
            // Servicemanualen s. 69: på max först ca 0,7 s utan vibrato, sedan inmatning under
            // ca 2,5 s (3,2 s totalt). Samma proportioner för kortare DELAY.
            if (vibDelay > 0.0) {
                const double wait = vibDelay * (0.7 / 3.2);
                fade = (vibTime - wait) / (vibDelay - wait);
                fade = fade < 0.0 ? 0.0 : (fade > 1.0 ? 1.0 : fade);
            }
            vibAmt = vibDepth * fade * std::sin(2.0 * kPi * vibPhase);
        }
        vibPhase += vibSpeed / sr; if (vibPhase >= 1.0) vibPhase -= 1.0;
        const bool anyG = anyPolyGate();
        if (anyG) vibTime += 1.0 / sr;

        const double clkRatio = (tune / 440.0) * std::pow(2.0, vibAmt / 1200.0);
        const double clk[2] = { kMasterClock * clkRatio, kMasterClock * clkRatio * std::pow(2.0, detune / 1200.0) };

        // Fasräknarna för de 12 tonerna i toppoktaven (fri löpning, båda generatorerna)
        double inc[2][12];
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) inc[g][p] = clk[g] / divider[p] / sr; // toppoktavsperioder per sampel

        // Orgelns tongenerator (OG1): egen masterklocka 2 MHz, ingen vibrato, ingen detune
        double orgInc[12];
        const double orgClk = kOrganClock * (tune / 440.0);
        for (int p = 0; p < 12; ++p) orgInc[p] = orgClk / orgDivider[p] / sr;
        const bool organOn = orgSel > 0 || orgVolSmoothed * orgLevelAny() > 1e-5;
        // Envelope-koefficienter räknas bara om när inställningarna ändras
        if (coefKey[0] != orgTime || coefKey[1] != (orgSustain ? 1.0 : 0.0) || coefKey[2] != (slowAttack ? 1.0 : 0.0)
            || coefKey[3] != (sustainOn ? 1.0 : 0.0) || coefKey[4] != sustainTime) {
            coefKey[0] = orgTime; coefKey[1] = orgSustain ? 1.0 : 0.0; coefKey[2] = slowAttack ? 1.0 : 0.0;
            coefKey[3] = sustainOn ? 1.0 : 0.0; coefKey[4] = sustainTime;
            cOrgDecay = 1.0 - std::exp(-4.6 / (orgTime * sr));
            cOrgRel = 1.0 - std::exp(-4.6 / ((orgSustain ? orgTime : 0.01) * sr));
            cAttack = attackCoef(); cRelease = releaseCoef();
        }
        const double orgDecayCoef = cOrgDecay, orgRelCoef = cOrgRel;

        // Summera tangenterna per fotläge (U16, U8, U4) och orgelns nio fotlägen
        double u[3] = { 0.0, 0.0, 0.0 };
        double ob[kNumFootages] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        const int gens = modeII ? 2 : 1;
        for (int v = 0; v < kAllVoices; ++v) {
            Voice& vo = voices[v];
            vo.advanceEnvelope(cAttack, cRelease, stealCoef);
            if (organOn) vo.advanceOrgan(orgAttackCoef, orgDecay ? orgDecayCoef : 0.0, orgRelCoef, stealCoef);
            else { vo.orgEnv = 0.0; vo.orgPeak = false; }
            if (vo.env < 1e-6 && vo.orgEnv < 1e-6 && !vo.gate && !vo.stealing) { vo.active = false; }
            if (!vo.active) continue;
            const int pc = vo.pc;
            if (vo.toPoly) for (int g = 0; g < gens; ++g) {
                for (int f = 0; f < 3; ++f) {
                    int k = vo.shift + f - 1;                  // 16' = shift+1, 8' = shift, 4' = shift-1
                    if (k < 0) k = 0;                          // brytning för 4' på översta tangenten
                    u[2 - f] += vo.env * square(phase[g][pc], inc[g][pc], k);
                }
            }
            if (organOn && vo.toOrg && vo.orgEnv > 1e-6) {
                for (int f = 0; f < kNumFootages; ++f) {
                    const int m = vo.note + kFootageSemis[f];
                    const int mp = m % 12;
                    int sh = (kOrganTopMidi + mp - m) / 12;    // oktaver under orgelns toppoktav
                    if (sh < 0) sh = 0;                        // brytning: högsta fotlägen upprepar toppoktaven
                    if (sh > 12) sh = 12;
                    ob[f] += vo.orgEnv * square(orgPhase[mp], orgInc[mp], sh);
                }
            }
            if (vo.stealing && vo.env < 1e-3 && vo.orgEnv < 1e-3) {
                if (vo.pendingNote) vo.startPending();
                else { vo.stealing = false; vo.active = false; vo.env = 0.0; vo.orgEnv = 0.0; }
            }
        }
        for (int g = 0; g < 2; ++g) for (int p = 0; p < 12; ++p) {
            phase[g][p] += inc[g][p];
            if (phase[g][p] >= kWrap) phase[g][p] -= kWrap;
        }
        for (int p = 0; p < 12; ++p) { orgPhase[p] += orgInc[p]; if (orgPhase[p] >= kWrap) orgPhase[p] -= kWrap; }

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
        s *= kStringGain * volSmoothed;                     // STRING VOLUME

        // POLY-SYNTH (CPC): tonväljare -> VCF (LP/BP) med envelope -> blandas med stråkarna.
        // Ingen VCA: volymen kommer från tangenternas keyer, precis som för stråkarna.
        if (prevAnyGate && !anyG) polyEnv.releaseGate();
        prevAnyGate = anyG;
        const double env = polyEnv.next();
        if (polyLevel > 1e-4) {
            const int feet = polySel == 4 ? polyFeet : 5;        // POLY-SYNTH 1/2/3: 16'-sågtand
            double src;
            switch (feet) {
            case 0:  src = saw4; break;
            case 1:  src = sq8 * 0.6; break;                     // fyrkanten är starkare än sågtanden
            case 2:  src = saw8; break;
            case 3:  src = saw8; break;
            case 4:  src = sq16 * 0.6; break;
            default: src = saw16; break;
            }
            double fc = cutoffHz(curCut) * std::pow(2.0, egDepthOct(curDepth) * env);
            const double fmax = sr * 0.45;
            if (fc > fmax) fc = fmax;
            if (fc < 10.0) fc = 10.0;
            const double g = std::tan(kPi * fc / sr);
            const double k = 1.0 / resonanceQ(curReso);
            double lp, bp;
            svf.process(src * 0.8, g, k, lp, bp);
            const double y = (feet == 2) ? bp * 1.6 : lp;
            s += y * kPolyGain * polyVolSmoothed * polyLevel;
        } else {
            svf.reset();
        }

        // BRILLIANCE (CPC)
        s = brill.process(s);

        // ORGEL (OG1 -> CPB): tonfilter per fotläge, ORGAN 1/2/3 eller tone levers,
        // slagverk (PERCUSSIVE 2nd/3rd/5th) och orgelns BRILLIANCE.
        double org = 0.0;
        if (organOn) {
            double filt[kNumFootages];
            for (int f = 0; f < kNumFootages; ++f) filt[f] = orgFilter[f].process(ob[f]) * kFootageShunt[f];
            for (int f = 0; f < kNumFootages; ++f) org += orgWeight[f] * filt[f];
            percEnv -= percEnv * percCoef;
            const double perc = percLevel[0] * percHp[0].process(filt[3]) + percLevel[1] * percHp[1].process(filt[4])
                              + percLevel[2] * percHp[2].process(filt[6]);
            org += perc * percEnv * kPercGain;
            org = orgBrill.process(org) * kOrganGain * orgVolSmoothed;
        } else {
            for (int f = 0; f < kNumFootages; ++f) orgFilter[f].reset();
            orgBrill.reset();
        }

        // ENSEMBLE / TREMOLO (TE). Körs alltid så att av/på blir klickfritt.
        // Varje sektion går antingen direkt eller genom TE-kortet (FET-grindarna på CPA).
        const double teIn = s * ensMix + org * orgTe;
        double dry = s * (1.0 - ensMix) + org * (1.0 - orgTe);

        // SOLO SYNTHESIZER: egen utgång, går inte genom TE-kortet
        soloVolSmoothed += paramSmooth * (targetSoloVolume() - soloVolSmoothed);
        if (soloVolSmoothed > 1e-6 || soloVolNorm > 0.0) dry += solo.next(tune / 440.0) * kSoloGain * soloVolSmoothed;

        double out = dry + ensemble.process(teIn);

        exprSm += paramSmooth * (exprTarget - exprSm);
        out = dcBlock.process(out) * masterSmoothed * kOutputMakeup * exprSm;

        // EFFEKTER: delay (SDD-3000-stil) -> reverb (plattreverb), som Fredriks kedja
        dlyLevelSm += paramSmooth * (dlyLevelTarget - dlyLevelSm);
        revLevelSm += paramSmooth * (revLevelTarget - revLevelSm);
        double l = out, r = out;
        if (dlyLevelSm > 1e-5 || dlyLevelTarget > 0.0 || dlyActive > 0) {
            double wl, wr;
            delay.process(out, dlyTime, dlyFeedback, delayModRatio(dlyMod), dlySpeed, wl, wr);
            l += wl * dlyLevelSm; r += wr * dlyLevelSm;
            dlyActive = (dlyLevelTarget > 0.0) ? (int)(sr * 8.0) : dlyActive - 1;   // låt ekona klinga ut innan den stängs av
        }
        if (revLevelSm > 1e-5 || revLevelTarget > 0.0 || revActive > 0) {
            double rl, rr;
            reverb.process(0.5 * (l + r), revDecay, rl, rr);
            // Nivån kalibrerad så att LEVEL på max ger ungefär lika mycket reverb som torrt
            // ljud (inte +5..7 dB som förut), och lika stark klang oavsett DECAY.
            const double rg = revLevelSm * kReverbTrim * reverb.levelComp(revDecay);
            l += rl * rg; r += rr * rg;
            revActive = (revLevelTarget > 0.0) ? (int)(sr * (revDecay + 1.0)) : revActive - 1;
        }
        // BOOST: SK30:s utgångssteg (+9,5 dB) efter effekterna, så att bara nivån ändras
        boostSm += paramSmooth * (boostTarget - boostSm);
        l *= boostSm; r *= boostSm;
        limiter.processStereo(l, r);       // går aldrig över -0,3 dBFS
        outL = l; outR = r;
    }

    // För tester / gränssnitt
    int activeVoices() const { int n = 0; for (int v = 0; v < kAllVoices; ++v) if (voices[v].active) ++n; return n; }
    int soloCurrentNote() const { return soloNote; }
    double takeLimiterReductionDb() { return limiter.takeReductionDb(); }
    int latencySamples() const { return limiter.latency(); }
    bool isSounding() const { return activeVoices() > 0 || (soloVolNorm > 0.0 && solo.isSounding()); }
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
    static constexpr double kPolyGain = 0.05;       // poly-synth ungefär lika stark som stråkarna
    static constexpr double kOrganClock = 2.0e6;    // orgelns masterklocka (OG1: 2 MHz tvåfas)
    static constexpr int kOrganTopMidi = 108;       // orgelns toppoktav C8-B8
    static constexpr double kOrganGain = 0.05;      // orgeln ungefär lika stark som stråkarna
    static constexpr double kPercGain = 1.6;        // slagverket tydligt över fotlägena
    static constexpr double kSoloGain = 0.16;       // solosynten ungefär lika stark som ett stråkackord
    static constexpr double kReverbTrim = 0.398;    // -8 dB (0.8.2)
    static constexpr double kOutputMakeup = 1.0;
    static constexpr double kBoostGain = 3.0;       // MO = 3 x UOI (servicemanualen s. 67, VR1)    // samma nivå som 0.7.0 (Fredrik föredrog det)
    static constexpr double kStringGain = 0.15;     // ger ca -24 dBFS RMS för en ton och ca -16 dBFS för ett 4-tonsackord (STRING 1+2, MODE II)

    struct Voice
    {
        bool active = false, gate = false, held = false, pedalHeld = false;
        bool stealing = false, pendingNote = false;
        int note = -1, pc = 0, shift = 0;
        int pendNote = -1, pendPc = 0, pendShift = 0;
        bool toPoly = true, toOrg = true, pendPoly = true, pendOrg = true;
        double env = 0.0;
        double orgEnv = 0.0;
        bool orgPeak = false;   // orgelns attack klar (för DECAY)
        uint32_t age = 0;

        void advanceOrgan(double aCoef, double decCoef, double rCoef, double sCoef)
        {
            if (stealing) orgEnv -= orgEnv * sCoef;
            else if (gate) {
                if (!orgPeak) { orgEnv += (1.02 - orgEnv) * aCoef; if (orgEnv >= 1.0) { orgEnv = 1.0; orgPeak = true; } }
                else if (decCoef > 0.0) orgEnv -= orgEnv * decCoef;   // DECAY: tonen klingar av medan tangenten hålls
            }
            else orgEnv -= orgEnv * rCoef;
        }

        void advanceEnvelope(double aCoef, double rCoef, double sCoef)
        {
            if (stealing) env -= env * sCoef;
            else if (gate) env += (1.0 - env) * aCoef;
            else env -= env * rCoef;
        }
        void startPending()
        {
            stealing = false; pendingNote = false;
            note = pendNote; pc = pendPc; shift = pendShift; toPoly = pendPoly; toOrg = pendOrg;
            gate = true; active = true; env = 0.0; orgEnv = 0.0; orgPeak = false;
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
            const double fo = 440.0 * std::pow(2.0, (kOrganTopMidi + p - 69) / 12.0);
            orgDivider[p] = std::floor(kOrganClock / fo + 0.5);
        }
    }

    static int shiftFor(int midi)
    {
        // Antal oktaver under toppoktaven (8'). Översta tangenten C6 (MIDI 96) spelar
        // toppoktaven på 8'; dess 4' finns inte i generatorn och upprepar 8' (brytning).
        int s = (kTopOctaveMidi + (midi % 12) - midi) / 12;
        if (s < 0) s = 0;
        if (s > 12) s = 12;
        return s;
    }

    // Fyrkant (±1) delad med 2^k ur toppoktavens fasräknare, med PolyBLEP.
    static double square(double ph, double incTop, int k)
    {
        const int64_t ip = (int64_t)ph;                     // ph >= 0 och < 2^20
        const double d = (double)(1 << k);
        double t = ((double)(ip & ((1 << k) - 1)) + (ph - (double)ip)) / d;
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

    bool anyGate() const { for (int v = 0; v < kAllVoices; ++v) if (voices[v].gate) return true; return false; }
    bool anyPolyGate() const { for (int v = 0; v < kAllVoices; ++v) if (voices[v].gate && voices[v].toPoly) return true; return false; }

    // Solosyntens ton: högsta (NORMAL) eller lägsta (BASS, MANUAL BASS) av de nedtryckta tangenterna
    void updateSolo(bool newKey)
    {
        int pick = -1;
        const bool manBass = soloMode == 2;
        if (soloMode == 0) { for (int m = 127; m >= 0; --m) if (held[m] && !(manualBassZone(m))) { pick = m; break; } }
        else { for (int m = 0; m < 128; ++m) if (held[m] && (!manBass || m <= bassPoint)) { pick = m; break; } }
        if (pick < 0) { if (soloNote >= 0) solo.allReleased(); soloNote = -1; return; }
        if (pick != soloNote) {
            // Ny tangent som blir solotonen: ny attack. Tillbaka till en hållen ton: legato.
            solo.noteChange(pick, newKey || soloNote < 0);
            soloNote = pick;
        }
    }
    bool manualBassZone(int m) const { return soloMode == 2 && m <= bassPoint; }

    void noteOn(int midi)
    {
        if (midi < 12 || midi > 120) return;
        held[midi] = true; heldOrder[midi] = ++ageCounter;
        updateSolo(true);
        if (manualBassZone(midi)) return;          // MANUAL BASS: tangenten spelar bara solosynten

        // KEYBOARD SPLIT: vilken sektion (och vilka 7 röster) tangenten styr
        bool toPoly = true, toOrg = true; int base = 0;
        if (split == 1) { if (midi < splitPoint) { toOrg = false; } else { toPoly = false; base = kVoices; } }
        else if (split == 2) { if (midi < splitPoint) { toPoly = false; base = kVoices; } else { toOrg = false; } }

        if (toPoly) {
            if (!anyPolyGate()) vibTime = 0.0;     // vibratots fördröjning startar om
            polyEnv.trigger();                     // PTR: varje ny tangent startar poly-synthens envelope
        }
        if (toOrg) percEnv = 1.0;                  // LTR: slagverkets envelope startar om
        const int pc = midi % 12, shift = shiftFor(midi);
        // Samma ton klingar redan ut: spela den igen i samma röst (som orgeln)
        for (int v = base; v < base + kVoices; ++v) {
            Voice& vo = voices[v];
            if (vo.active && !vo.stealing && vo.note == midi) {
                vo.gate = true; vo.held = true; vo.pedalHeld = false; vo.age = ageCounter;
                vo.orgPeak = false; vo.toPoly = toPoly; vo.toOrg = toOrg;
                return;
            }
        }
        int pick = -1;
        for (int v = base; v < base + kVoices; ++v) if (!voices[v].active) { pick = v; break; }
        if (pick < 0) { // äldsta som klingar ut, annars äldsta
            uint32_t best = 0xFFFFFFFFu;
            for (int v = base; v < base + kVoices; ++v) if (!voices[v].gate && voices[v].age < best) { best = voices[v].age; pick = v; }
            if (pick < 0) for (int v = base; v < base + kVoices; ++v) if (voices[v].age < best) { best = voices[v].age; pick = v; }
        }
        Voice& vo = voices[pick];
        if (vo.active && (vo.env > 1e-3 || vo.orgEnv > 1e-3)) {
            // Snabb nedtoning (3 ms) innan rösten tar den nya tonen: inget klick
            vo.stealing = true; vo.pendingNote = true;
            vo.pendNote = midi; vo.pendPc = pc; vo.pendShift = shift;
            vo.gate = false; vo.held = true; vo.pedalHeld = false; vo.age = ageCounter;
            vo.note = midi; // räknas som den nya tonen (för note off)
            vo.pendPoly = toPoly; vo.pendOrg = toOrg;
        } else {
            vo = Voice();
            vo.active = true; vo.gate = true; vo.held = true;
            vo.note = midi; vo.pc = pc; vo.shift = shift; vo.age = ageCounter;
            vo.toPoly = toPoly; vo.toOrg = toOrg;
        }
    }

    void noteOff(int midi)
    {
        if (midi < 0 || midi > 127) return;
        held[midi] = false;
        updateSolo(false);
        for (int v = 0; v < kAllVoices; ++v) {
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
        for (int v = 0; v < kAllVoices; ++v) if (voices[v].pedalHeld) { voices[v].pedalHeld = false; voices[v].gate = false; }
    }

    void allNotesOff(bool hard)
    {
        for (int m = 0; m < 128; ++m) held[m] = false;
        if (soloNote >= 0) solo.allReleased();
        soloNote = -1;
        if (hard) solo.reset();
        for (int v = 0; v < kAllVoices; ++v) {
            voices[v].gate = false; voices[v].held = false; voices[v].pedalHeld = false;
            voices[v].pendingNote = false; voices[v].stealing = false;
            if (hard) { voices[v].env = 0.0; voices[v].active = false; }
        }
    }

    // Tillstånd
    double sr = 44100.0;
    double divider[12];
    double phase[2][12];
    Voice voices[kAllVoices];
    bool held[128];
    uint32_t heldOrder[128];

    // Solosynt och split
    SoloSynth solo;
    int soloNote = -1, soloMode = 0, split = 0, splitPoint = 55, bassPoint = 42;
    double soloVolNorm = 0.0, soloVolSmoothed = 0.0;
    double targetSoloVolume() const { const double db = volumeDb(soloVolNorm); return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0); }
    uint32_t ageCounter = 0;
    bool sustainPedal = false;
    double modWheel = 0.0, aftertouch = 0.0;

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

    // Poly-synth
    int polySel = 0, polyFeet = 5;
    bool polyTremolo = false;
    double polyVolNorm = defaultParam(kParamPolyVol);
    double manCutoff = defaultParam(kParamCutoff), manReso = defaultParam(kParamResonance), manDepth = defaultParam(kParamEgDepth);
    double manAttack = defaultParam(kParamAttack), manDecay = defaultParam(kParamDecay);
    double manSus = defaultParam(kParamSustainLevel), manRelease = defaultParam(kParamRelease);
    double curCut = 0.5, curReso = 0.2, curDepth = 0.3, posSmooth = 0.001;
    double polyVolSmoothed = 1.0, polyLevel = 0.0;
    bool prevAnyGate = false;
    SVF svf;
    ADSR polyEnv;

    // Orgel
    int orgSel = 0;
    bool orgDecay = false, orgSustain = true, orgTremolo = false, orgEnsemble = false;
    double orgTime = 0.3, orgBrillDbV = 0.0, orgVolNorm = defaultParam(kParamOrgVol), masterNorm = defaultParam(kParamMaster);
    double orgDivider[12];
    double orgPhase[12];
    double leverTarget[kNumFootages] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double orgWeight[kNumFootages] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    double percTarget[3] = { 0, 0, 0 }, percLevel[3] = { 0, 0, 0 };
    double percEnv = 0.0, percCoef = 0.001, orgAttackCoef = 0.1;
    double coefKey[5] = { -1, -1, -1, -1, -1 };
    double cOrgDecay = 0.0, cOrgRel = 0.0, cAttack = 0.0, cRelease = 0.0;
    double orgVolSmoothed = 1.0, masterSmoothed = 1.0, orgTe = 0.0;
    double boostTarget = kBoostGain, boostSm = kBoostGain, exprTarget = 1.0, exprSm = 1.0;
    OrganToneFilter orgFilter[kNumFootages];
    OnePoleHP percHp[3];
    Biquad orgBrill;

    // Blandningsvikt för fotläge f: ORGAN 1/2/3 (22k, förstärkning 18k/22k) eller tone lever
    double targetFootage(int f) const
    {
        if (orgSel >= 1 && orgSel <= 3) return kOrganPreset[orgSel - 1][f] ? 18.0 / 22.0 : 0.0;
        if (orgSel == 4) return leverTarget[f] * 18.0 / 22.0;
        return 0.0;
    }
    double orgLevelAny() const { double a = 0.0; for (int f = 0; f < kNumFootages; ++f) a += orgWeight[f]; for (int i = 0; i < 3; ++i) a += percLevel[i]; return a; }
    double targetOrgVolume() const { const double db = volumeDb(orgVolNorm); return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0); }
    double targetMaster() const { const double db = volumeDb(masterNorm); return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0); }

    // Läge för styrledning i (0 CUTOFF, 1 RESONANCE, 2 EG DEPTH, 3 ATTACK, 4 DECAY, 5 SUSTAIN, 6 RELEASE)
    double polyPos(int i) const
    {
        if (polySel >= 1 && polySel <= 3) return presetPos(kPolyPresetR[polySel - 1][i]);
        switch (i) {
        case 0: return manCutoff; case 1: return manReso; case 2: return manDepth;
        case 3: return manAttack; case 4: return manDecay; case 5: return manSus; default: return manRelease;
        }
    }
    void updatePolyEnv()
    {
        polyEnv.set(attackSec(polyPos(3)), decaySec(polyPos(4)), polyPos(5), releaseSec(polyPos(6)), sr);
    }
    double targetPolyVolume() const
    {
        const double db = volumeDb(polyVolNorm);
        return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0);
    }

    // Filter
    OnePoleHP hpIn[3], hp4to8, hp8to16, hpOut4, hpLadder[3], dcBlock;
    RCLadder ladder[3];
    Biquad brill;
    Ensemble ensemble;
    Limiter limiter;
    StereoDelay delay;
    PlateReverb reverb;
    double dlySpeed = 0.5;
    double dlyTime = 0.38, dlyFeedback = 0.285, dlyMod = 0.1, dlyLevelTarget = 0.0, dlyLevelSm = 0.0;
    double revDecay = 3.0, revLevelTarget = 0.0, revLevelSm = 0.0;
    int dlyActive = 0, revActive = 0;
    static double fxGain(double n) { const double db = fxLevelDb(n); return db <= -100.0 ? 0.0 : std::pow(10.0, db / 20.0); }
};

} // namespace truesymph
