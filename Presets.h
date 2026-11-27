// Presets.h - TrueSymphs fabriksljud och egna presets (textfiler i
// Dokument\TrueSymph Presets\*.tsy). Gemensamt för pluginet, fönstret och testerna.
#pragma once
#include "TrueSymphDSP.h"
#include <cstdlib>
#include <string>

namespace truesymph {

// ---------------------------------------------------------------------------
// Fabriksljud (visas i Lives presetlista). Varje ljud utgår från
// standardvärdena och ändrar bara de parametrar som räknas upp.
// ---------------------------------------------------------------------------
struct PresetValue { int index; float value; };
struct FactoryPreset { const char* name; PresetValue v[20]; int count; };

inline float SUS(double s) { return (float)(std::log(s / 0.03) / std::log(1.6 / 0.03)); }
inline float VOL(double db) { return (float)normFromVolumeDb(db); }
inline float DET(double c) { return (float)normFromDetune(c); }
inline float SEL(int i) { return (float)stepNorm(i, kNumPolySel); }
inline float FEET(int i) { return (float)stepNorm(i, kNumFeet); }
inline float OSEL(int i) { return (float)stepNorm(i, kNumOrgSel); }
inline float LEV(double v) { return (float)(v / 10.0); }   // tone lever 0..10
inline float SMODE(int i) { return (float)stepNorm(i, kNumSoloModes); }
inline float SFEET(int i) { return (float)stepNorm(i, kNumSoloFeet); }
inline float VCA(int i) { return (float)stepNorm(i, kNumVcaModes); }
inline float PWS(int i) { return (float)stepNorm(i, kNumPwSources); }
inline float SPLIT(int i) { return (float)stepNorm(i, kNumSplit); }

static const FactoryPreset kPresets[] = {
    { "Dream Strings",   { }, 0 },
    { "Ensemble 8'",     { {kParamString1, 0}, {kParamSustainTime, SUS(0.6)}, {kParamMode, 0}, {kParamDetune, DET(0)},
                           {kParamVibDelay, 0}, {kParamVibDepth, 0} }, 6 },
    { "Full Strings",    { {kParamVolume, VOL(-3)}, {kParamString3, 1}, {kParamSustainTime, SUS(1.2)}, {kParamDetune, DET(10)},
                           {kParamVibDelay, 0.3f}, {kParamVibSpeed, 0.3f}, {kParamBrilliance, 0.6f} }, 7 },
    { "Cello 16'",       { {kParamString2, 0}, {kParamSustainTime, SUS(0.7)}, {kParamDetune, DET(5)}, {kParamVibDelay, 0.4f},
                           {kParamVibSpeed, 0.2f}, {kParamVibDepth, 0.25f}, {kParamBrilliance, 0.4f} }, 7 },
    { "Bright Violins",  { {kParamString1, 0}, {kParamString3, 1}, {kParamSustainTime, SUS(0.8)}, {kParamDetune, DET(8)},
                           {kParamVibSpeed, 0.35f}, {kParamVibDepth, 0.25f}, {kParamBrilliance, 0.8f} }, 7 },
    { "Dry Organ Keyer", { {kParamSlowAttack, 0}, {kParamSustain, 0}, {kParamMode, 0}, {kParamDetune, DET(0)},
                           {kParamVibDelay, 0}, {kParamVibDepth, 0}, {kParamEnsemble, 0} }, 7 },
    { "Poly-Synth 1",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(1)} }, 3 },
    { "Poly-Synth 2",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(2)} }, 3 },
    { "Poly-Synth 3",    { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(3)} }, 3 },
    { "Strings + Poly 2",{ {kParamPolySel, SEL(2)}, {kParamPolyVol, VOL(-6)} }, 2 },
    { "Soft Pad",        { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(4)}, {kParamPolyFeet, FEET(3)},
                           {kParamCutoff, 0.45f}, {kParamResonance, 0.1f}, {kParamEgDepth, 0.25f}, {kParamAttack, 0.8f},
                           {kParamDecay, 0.7f}, {kParamSustainLevel, 0.6f}, {kParamRelease, 0.6f}, {kParamSustainTime, SUS(1.4)},
                           {kParamPolyVol, VOL(6)} }, 13 },
    { "Wah Brass",       { {kParamString1, 0}, {kParamString2, 0}, {kParamPolySel, SEL(4)}, {kParamPolyFeet, FEET(5)},
                           {kParamCutoff, 0.3f}, {kParamResonance, 0.6f}, {kParamEgDepth, 0.45f}, {kParamAttack, 0.42f},
                           {kParamDecay, 0.45f}, {kParamSustainLevel, 0.45f}, {kParamRelease, 0.3f}, {kParamSlowAttack, 0},
                           {kParamSustainTime, SUS(0.3)}, {kParamEnsemble, 0} }, 14 },
    { "Tremolo Strings", { {kParamEnsemble, 0}, {kParamPolyTremolo, 1}, {kParamTremoloSpeed, 1} }, 3 },
    { "Organ 1",         { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(1)} }, 3 },
    { "Organ 2",         { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(2)} }, 3 },
    { "Organ 3 Full",    { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(3)}, {kParamOrgVol, VOL(-4)} }, 4 },
    { "Jazz Percussion", { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(8)}, {kParamOrg8, LEV(8)}, {kParamOrg4, LEV(0)}, {kParamPerc3, LEV(8)},
                           {kParamPercDecay, 0.35f}, {kParamOrgSustain, 0}, {kParamOrgTremolo, 1}, {kParamTremoloSpeed, 0} , {kParamOrgVol, VOL(4)} }, 12 },
    { "Rotor Organ",     { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(8)}, {kParamOrg8, LEV(8)}, {kParamOrg513, LEV(6)}, {kParamOrg4, LEV(5)},
                           {kParamOrg2, LEV(4)}, {kParamOrgTremolo, 1}, {kParamTremoloSpeed, 1}, {kParamOrgSustain, 0} , {kParamOrgVol, VOL(4)} }, 12 },
    { "Ensemble Organ",  { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg8, LEV(8)}, {kParamOrg4, LEV(6)}, {kParamOrg2, LEV(4)}, {kParamOrg16, LEV(5)},
                           {kParamOrgEnsemble, 1}, {kParamOrgTime, 0.75f} , {kParamOrgVol, VOL(4)} }, 10 },
    { "Organ Piano",     { {kParamString1, 0}, {kParamString2, 0}, {kParamOrgSel, OSEL(4)},
                           {kParamOrg16, LEV(6)}, {kParamOrg8, LEV(9)}, {kParamOrg4, LEV(7)}, {kParamOrg2, LEV(4)},
                           {kParamOrgDecay, 1}, {kParamOrgTime, 0.8f}, {kParamOrgSustain, 1}, {kParamOrgBrill, 0.7f} , {kParamOrgVol, VOL(6)} }, 12 },
    { "Strings + Organ", { {kParamOrgSel, OSEL(4)}, {kParamOrg16, LEV(6)}, {kParamOrg8, LEV(7)}, {kParamOrg4, LEV(4)},
                           {kParamOrgVol, VOL(-4)}, {kParamOrgEnsemble, 1} }, 6 },
    { "Solo Lead",       { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamSoloCutoff, 0.5f},
                           {kParamSoloReso, 0.35f}, {kParamSoloEgDepth, 0.4f}, {kParamPortamento, 0.25f},
                           {kParamTouchVib, 1}, {kParamTouchBrill, 1} }, 9 },
    { "Square Solo PWM", { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamSoloWave, 1},
                           {kParamSoloPw, 0.8f}, {kParamPwSource, PWS(0)}, {kParamLfoSpeed, 0.25f}, {kParamSoloCutoff, 0.6f},
                           {kParamSoloReso, 0.15f}, {kParamTouchVib, 1} }, 10 },
    { "Strings + Man Bass", { {kParamSoloVol, VOL(-4)}, {kParamSoloMode, SMODE(2)}, {kParamSoloFeet, SFEET(3)},
                           {kParamSoloCutoff, 0.42f}, {kParamSoloReso, 0.25f}, {kParamSoloEgDepth, 0.35f}, {kParamSoloDecay, 0.3f},
                           {kParamSoloSustain, 0.4f}, {kParamSoloRelease, 0.15f} }, 9 },
    { "Strings + Bass Mode", { {kParamSoloVol, VOL(-6)}, {kParamSoloMode, SMODE(1)}, {kParamSoloFeet, SFEET(3)},
                           {kParamSoloWave, 0.5f}, {kParamSoloCutoff, 0.38f}, {kParamSoloEgDepth, 0.3f} }, 6 },
    { "Glide Lead",      { {kParamString1, 0}, {kParamString2, 0}, {kParamSoloVol, VOL(0)}, {kParamGlide, 0.3f},
                           {kParamSoloFeet, SFEET(0)}, {kParamSoloCutoff, 0.45f}, {kParamSoloReso, 0.5f}, {kParamSoloEgDepth, 0.5f},
                           {kParamSoloMod, 0.1f}, {kParamLfoSpeed, 0.3f} }, 10 },
    { "Split Poly|Organ", { {kParamSplit, SPLIT(1)}, {kParamString1, 0}, {kParamPolySel, SEL(2)}, {kParamOrgSel, OSEL(2)},
                           {kParamOrgVol, VOL(-4)}, {kParamOrgEnsemble, 1} }, 6 },
    { "Slow Rotor Pad",  { {kParamString3, 1}, {kParamEnsemble, 0}, {kParamPolyTremolo, 1}, {kParamTremoloSpeed, 0},
                           {kParamSustainTime, SUS(1.4)}, {kParamVolume, VOL(-3)} }, 6 },
};
const int kNumFactoryPresets = (int)(sizeof kPresets / sizeof kPresets[0]);

inline void factoryPresetValues(int index, float* out)
{
    for (int i = 0; i < kNumParams; ++i) out[i] = defaultParam(i);
    if (index < 0 || index >= kNumFactoryPresets) return;
    for (int j = 0; j < kPresets[index].count; ++j) {
        const PresetValue& pv = kPresets[index].v[j];
        if (pv.index >= 0 && pv.index < kNumParams) out[pv.index] = pv.value;
    }
}

inline const char* factoryPresetName(int index)
{
    return (index >= 0 && index < kNumFactoryPresets) ? kPresets[index].name : "";
}

// ---------------------------------------------------------------------------
// Egna presets som textfil: en rad per parameter, "Namn=värde" (0..1).
// Okända rader ignoreras och saknade parametrar får standardvärden, så att
// presets från äldre versioner fortsätter fungera.
// ---------------------------------------------------------------------------
static const char* const kPresetHeader = "TrueSymph preset 1";

inline std::string cleanPresetName(const std::string& in)
{
    std::string out;
    for (char c : in) {
        const unsigned char u = (unsigned char)c;
        if (u >= 32 && u < 127 && c != '=') out += c;
        else if (u >= 128) out += '_';
        if (out.size() >= 31) break;
    }
    return out.empty() ? std::string("Untitled") : out;
}

inline std::string presetToText(const std::string& name, const float* v)
{
    std::string t = kPresetHeader;
    t += "\r\nName=" + cleanPresetName(name) + "\r\n";
    char buf[64];
    for (int i = 0; i < kNumParams; ++i) {
        std::snprintf(buf, sizeof buf, "%s=%.6f\r\n", kParamNames[i], (double)v[i]);
        t += buf;
    }
    return t;
}

// Föreslår ett namn som inte redan används: "Pad", "Pad 2", "Pad 3" ...
template <typename ExistsFn>
inline std::string uniquePresetName(const std::string& base, ExistsFn exists)
{
    const std::string clean = cleanPresetName(base);
    if (!exists(clean)) return clean;
    std::string stem = clean;
    int n = 2;
    const size_t sp = clean.find_last_of(' ');
    if (sp != std::string::npos && sp + 1 < clean.size()) {
        bool digits = true;
        for (size_t i = sp + 1; i < clean.size(); ++i) digits = digits && clean[i] >= '0' && clean[i] <= '9';
        if (digits && clean.size() - sp - 1 <= 3) { stem = clean.substr(0, sp); n = std::atoi(clean.c_str() + sp + 1) + 1; }
    }
    for (; n < 1000; ++n) {
        const std::string candidate = stem + " " + std::to_string(n);
        if (!exists(candidate)) return candidate;
    }
    return clean;
}

// Läser en preset. Returnerar false om texten inte är en TrueSymph-preset.
inline bool presetFromText(const std::string& text, float* v, std::string* name)
{
    if (text.compare(0, std::strlen(kPresetHeader), kPresetHeader) != 0) return false;
    for (int i = 0; i < kNumParams; ++i) v[i] = defaultParam(i);
    if (name) *name = "Untitled";
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find_first_of("\r\n", pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        if (key == "Name") { if (name) *name = cleanPresetName(val); continue; }
        for (int i = 0; i < kNumParams; ++i) {
            if (key != kParamNames[i]) continue;
            char* endp = nullptr;
            const double d = std::strtod(val.c_str(), &endp);
            if (endp != val.c_str() && d >= 0.0 && d <= 1.0) v[i] = (float)d;   // ogiltigt ignoreras
        }
    }
    return true;
}

} // namespace truesymph
