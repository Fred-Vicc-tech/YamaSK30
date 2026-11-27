// TrueSymphEditor.cpp - plugin-fönstret (Windows, GDI+). Pratar bara med
// kärnan (TrueSymphCore.h). Samma familj som TrueSub och TruePan (logga,
// presets, manual på baksidan), men panelen är en kompakt SK30:
//   Rad 1: OUTPUT, PITCH, SOLO SYNTHESIZER
//   Rad 2: VIBRATO, ORGAN, POLY-SYNTH
//   Rad 3: knappraden (MANUAL BASS ... TREMOLO/ENSEMBLE) med SK30-knappar
// Skjutreglage som på SK30, guldfärgade grupplinjer och en display som visar
// reglaget under musen.
#include "TrueSymphCore.h"
#include "Manual.h"

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <new>
#include <windows.h>
// gdiplus.h behöver min/max, som NOMINMAX (i CMakeLists) stänger av.
#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <gdiplus.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <vector>

using namespace truesymph;

static HINSTANCE g_instance = nullptr;
static ULONG_PTR g_gdiplusToken = 0;
static int g_gdiplusUsers = 0;
static bool g_classRegistered = false;
static const wchar_t* kWindowClass = L"TrueSymphEditorWindow";

namespace {

using namespace Gdiplus;

const wchar_t* const kVersionText = L"v0.6.1";

// Färger: TrueSub-familjens mörka metall och cyan, SK30:s guldtryck och knappar.
const Color kBgTop(255, 38, 39, 42);
const Color kBgBottom(255, 18, 19, 21);
const Color kPanel(255, 14, 15, 17);                    // samma svart som i Fredriks knappbild
const Color kPanelEdge(255, 70, 74, 80);
const Color kGold(255, 235, 195, 115);                  // grupplinjerna i knappbilden
const Color kCyan(255, 0, 220, 255);
const Color kText(255, 225, 228, 232);
const Color kTextDim(255, 120, 124, 130);
const Color kLabel(255, 214, 214, 210);

// Layout (bildpunkter)
const float kHeaderH = 70.0f;
const float kRowAY = 76.0f, kRowAH = 222.0f;
const float kRowBY = 304.0f, kRowBH = 222.0f;
const float kRowCY = 532.0f, kRowCH = 128.0f;
const float kTrackLen = 100.0f;          // skjutreglagens längd
const float kTrackDY = 58.0f;            // reglagets topp under panelens överkant
const float kKnobR = 17.0f;

enum CtlKind { kFader, kKnob, kSwitch };
enum CapColor { kCapCream, kCapAmber };

struct Ctl
{
    int kind;
    int param;
    float x, y;                 // fader: mitt x, spårets topp; knob: mitt; switch: mitt x, topp
    const wchar_t* label;
    int steps;                  // 0 = steglös, annars antal lägen
    int cap;
    int npos;                   // switch: antal lägen
    const wchar_t* pos[3];      // switch: texter uppifrån och ner
    int posIndex[3];            // switch: parameterns index för varje läge (uppifrån)
};

struct RectDef
{
    float x, y, w, h;
    bool hit(int px, int py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

enum BtnAct { kActRadio, kActToggle, kActStrCancel, kActManBass, kActSoloMode, kActSplit };

struct Btn
{
    RectDef r;
    const wchar_t* label;
    int act;
    int param;
    int index;
    int count;
    bool cream;
};

struct Group
{
    float x0, x1, y;
    const wchar_t* label;
};

// Namn i displayen (som på SK30-panelen)
const wchar_t* const kDisplayNames[kNumParams] = {
    L"STRING VOLUME", L"STRING 1 (16')", L"STRING 2 (8')", L"STRING 3 (4')", L"SLOW ATTACK", L"POLY-SYNTH SUSTAIN",
    L"SUSTAIN TIME", L"POLY-SYNTH MODE", L"POLY-SYNTH DETUNE", L"VIBRATO DELAY", L"VIBRATO SPEED", L"VIBRATO DEPTH",
    L"POLY-SYNTH BRILLIANCE", L"POLY-SYNTH ENSEMBLE", L"TUNE (A4)",
    L"POLY-SYNTH", L"POLY-SYNTH VOLUME", L"POLY-SYNTH FEET", L"POLY CUTOFF FREQ", L"POLY RESONANCE", L"POLY EG DEPTH",
    L"POLY ATTACK TIME", L"POLY DECAY TIME", L"POLY SUSTAIN LEVEL", L"POLY RELEASE TIME", L"POLY-SYNTH TREMOLO", L"TREMOLO SPEED",
    L"ORGAN", L"ORGAN VOLUME", L"ORGAN DECAY", L"ORGAN SUSTAIN", L"ORGAN DECAY/SUSTAIN TIME", L"ORGAN BRILLIANCE",
    L"PERCUSSIVE DECAY TIME", L"PERCUSSIVE 2nd", L"PERCUSSIVE 3rd", L"PERCUSSIVE 5th",
    L"ORGAN 16'", L"ORGAN 8'", L"ORGAN 5 1/3'", L"ORGAN 4'", L"ORGAN 2 2/3'", L"ORGAN 2'", L"ORGAN 1 3/5'", L"ORGAN 1 1/3'", L"ORGAN 1'",
    L"ORGAN TREMOLO", L"ORGAN ENSEMBLE", L"MASTER VOLUME",
    L"SOLO VOLUME", L"SOLO MODE", L"SOLO LFO SPEED", L"PORTAMENTO", L"GLIDE DEPTH", L"SOLO VCO FEET", L"SOLO WAVE",
    L"SOLO PULSE WIDTH", L"SOLO PW SOURCE", L"SOLO CUTOFF FREQ", L"SOLO RESONANCE", L"SOLO VCF MODULATION", L"SOLO EG DEPTH",
    L"SOLO VCA", L"SOLO ATTACK TIME", L"SOLO DECAY TIME", L"SOLO SUSTAIN LEVEL", L"SOLO RELEASE TIME",
    L"TOUCH VIBRATO", L"TOUCH BRILLIANCE", L"TOUCH MODULATION", L"SOLO TUNE", L"KEYBOARD SPLIT", L"SPLIT AT", L"MANUAL BASS UP TO"
};

struct Editor
{
    Core* plugin = nullptr;
    HWND hwnd = nullptr;

    std::vector<Ctl> ctls;
    std::vector<Btn> btns;
    std::vector<Group> groups;      // grupplinjer i panelerna och knappraden

    int dragCtl = -1;
    int dragStartY = 0;
    float dragStartValue = 0.0f;
    int hoverParam = -1;            // visas i displayen
    int lastParam = -1;

    RectDef presetPrev = { 652.0f, 26.0f, 28.0f, 28.0f };
    RectDef presetNext = { 892.0f, 26.0f, 28.0f, 28.0f };
    RectDef btnSave = { 930.0f, 26.0f, 62.0f, 28.0f };
    RectDef btnLoad = { 998.0f, 26.0f, 62.0f, 28.0f };
    RectDef splitPrev, splitNext, bassPrev, bassNext;

    float peakShown = 0.0f;
    double peakHoldDb = -100.0;
    DWORD peakHoldUntil = 0, limitUntil = 0;

    // Baksidan (klick på loggan): credits och manual
    bool showBack = false;
    bool swedish = false;
    float logoRight = 200.0f;
    float scroll = 0.0f;
    float contentH = 0.0f;
    bool dragThumb = false;
    int dragThumbY = 0;
    float dragScroll0 = 0.0f;
    RectDef btnBack = { 1050.0f, 100.0f, 100.0f, 32.0f };
    RectDef btnEng = { 0, 0, 0, 0 }, btnSwe = { 0, 0, 0, 0 };
    HWND prevFocus = nullptr;
    bool tracking = false;
    bool overLogo(int x, int y) const { return x >= 15 && x <= logoRight && y >= 4 && y <= 50; }

    Editor() { build(); }

    void fader(int param, float x, float y, const wchar_t* label, int cap = kCapCream, int steps = 0)
    {
        Ctl c = {}; c.kind = kFader; c.param = param; c.x = x; c.y = y; c.label = label; c.cap = cap; c.steps = steps;
        ctls.push_back(c);
    }
    void knob(int param, float x, float y, const wchar_t* label)
    {
        Ctl c = {}; c.kind = kKnob; c.param = param; c.x = x; c.y = y; c.label = label;
        ctls.push_back(c);
    }
    void sw(int param, float x, float y, const wchar_t* label, int n, int steps, const wchar_t* p0, int i0,
            const wchar_t* p1, int i1, const wchar_t* p2 = nullptr, int i2 = 0)
    {
        Ctl c = {}; c.kind = kSwitch; c.param = param; c.x = x; c.y = y; c.label = label; c.npos = n; c.steps = steps;
        c.pos[0] = p0; c.pos[1] = p1; c.pos[2] = p2; c.posIndex[0] = i0; c.posIndex[1] = i1; c.posIndex[2] = i2;
        ctls.push_back(c);
    }

    void build()
    {
        const float ta = kRowAY + kTrackDY, tb = kRowBY + kTrackDY;

        // --- OUTPUT ---
        knob(kParamMaster, 60.0f, kRowAY + 112.0f, L"MASTER\nVOLUME");
        fader(kParamSoloVol, 114.0f, ta, L"SOLO");
        fader(kParamOrgVol, 148.0f, ta, L"ORGAN");
        fader(kParamVolume, 182.0f, ta, L"STRING");
        fader(kParamPolyVol, 216.0f, ta, L"POLY-\nSYNTH");

        // --- PITCH ---
        knob(kParamTune, 282.0f, kRowAY + 112.0f, L"TUNE");
        knob(kParamDetune, 337.0f, kRowAY + 112.0f, L"POLY-SYNTH\nDETUNE");
        knob(kParamSoloTune, 392.0f, kRowAY + 112.0f, L"SOLO\nTUNE");

        // --- SOLO SYNTHESIZER ---
        fader(kParamLfoSpeed, 456.0f, ta, L"LFO\nSPEED");
        fader(kParamPortamento, 494.0f, ta, L"PORTA-\nMENTO");
        fader(kParamGlide, 532.0f, ta, L"GLIDE\nDEPTH");
        fader(kParamSoloFeet, 588.0f, ta, L"FEET", kCapCream, kNumSoloFeet);
        fader(kParamSoloWave, 626.0f, ta, L"WAVE");
        fader(kParamSoloPw, 664.0f, ta, L"PW");
        sw(kParamPwSource, 696.0f, ta + 18.0f, L"", 3, kNumPwSources, L"LFO", 0, L"MAN", 1, L"EG", 2);
        groups.push_back({ 572.0f, 730.0f, kRowAY + 36.0f, L"VCO" });
        fader(kParamSoloCutoff, 770.0f, ta, L"CUTOFF");
        fader(kParamSoloReso, 808.0f, ta, L"RESO-\nNANCE");
        fader(kParamSoloMod, 846.0f, ta, L"MODU-\nLATION");
        fader(kParamSoloEgDepth, 884.0f, ta, L"EG\nDEPTH");
        groups.push_back({ 754.0f, 900.0f, kRowAY + 36.0f, L"VCF" });
        sw(kParamVcaMode, 922.0f, ta + 18.0f, L"", 3, kNumVcaModes, L"HOLD", 0, L"GATE", 1, L"EG", 2);
        groups.push_back({ 912.0f, 966.0f, kRowAY + 36.0f, L"VCA" });
        fader(kParamSoloAttack, 1000.0f, ta, L"A");
        fader(kParamSoloDecay, 1038.0f, ta, L"D");
        fader(kParamSoloSustain, 1076.0f, ta, L"S");
        fader(kParamSoloRelease, 1114.0f, ta, L"R");
        groups.push_back({ 984.0f, 1130.0f, kRowAY + 36.0f, L"EG" });

        // --- VIBRATO ---
        fader(kParamVibDelay, 46.0f, tb, L"DELAY");
        fader(kParamVibSpeed, 84.0f, tb, L"SPEED");
        fader(kParamVibDepth, 122.0f, tb, L"DEPTH");
        groups.push_back({ 32.0f, 136.0f, kRowBY + 36.0f, L"POLY-SYNTH" });

        // --- ORGAN --- (DECAY/SUSTAIN-knapparna läggs till i btns nedan)
        fader(kParamOrgTime, 222.0f, tb, L"S ~ L");
        fader(kParamOrgBrill, 257.0f, tb, L"BRILLI-\nANCE");
        fader(kParamPercDecay, 298.0f, tb, L"DECAY\nTIME", kCapAmber);
        fader(kParamPerc2, 332.0f, tb, L"2nd", kCapAmber);
        fader(kParamPerc3, 366.0f, tb, L"3rd", kCapAmber);
        fader(kParamPerc5, 400.0f, tb, L"5th", kCapAmber);
        groups.push_back({ 284.0f, 414.0f, kRowBY + 36.0f, L"PERCUSSIVE" });
        const wchar_t* feet[kNumFootages] = { L"16'", L"8'", L"5 1/3'", L"4'", L"2 2/3'", L"2'", L"1 3/5'", L"1 1/3'", L"1'" };
        for (int i = 0; i < kNumFootages; ++i) fader(kParamOrg16 + i, 442.0f + i * 32.0f, tb, feet[i], kCapAmber);
        groups.push_back({ 428.0f, 712.0f, kRowBY + 36.0f, L"TONE LEVER" });

        // --- POLY-SYNTH ---
        fader(kParamSustainTime, 790.0f, tb, L"S ~ L");
        fader(kParamBrilliance, 825.0f, tb, L"BRILLI-\nANCE");
        sw(kParamMode, 854.0f, tb + 24.0f, L"MODE", 2, 2, L"II", 1, L"I", 0);
        fader(kParamPolyFeet, 892.0f, tb, L"FEET", kCapAmber, kNumFeet);
        fader(kParamCutoff, 928.0f, tb, L"CUTOFF", kCapAmber);
        fader(kParamResonance, 963.0f, tb, L"RESO-\nNANCE", kCapAmber);
        fader(kParamEgDepth, 998.0f, tb, L"EG\nDEPTH", kCapAmber);
        fader(kParamAttack, 1036.0f, tb, L"A", kCapAmber);
        fader(kParamDecay, 1070.0f, tb, L"D", kCapAmber);
        fader(kParamSustainLevel, 1104.0f, tb, L"S", kCapAmber);
        fader(kParamRelease, 1138.0f, tb, L"R", kCapAmber);
        groups.push_back({ 912.0f, 1012.0f, kRowBY + 36.0f, L"VCF" });
        groups.push_back({ 1020.0f, 1152.0f, kRowBY + 36.0f, L"EG" });

        // DECAY/SUSTAIN (orgel) och SLOW ATTACK/SUSTAIN (poly-synth): fyrkantiga knappar
        btns.push_back({ { 167.0f, kRowBY + 62.0f, 34.0f, 34.0f }, L"DECAY", kActToggle, kParamOrgDecay, 0, 0, false });
        btns.push_back({ { 167.0f, kRowBY + 124.0f, 34.0f, 34.0f }, L"SUSTAIN", kActToggle, kParamOrgSustain, 0, 0, false });
        btns.push_back({ { 735.0f, kRowBY + 62.0f, 34.0f, 34.0f }, L"SLOW\nATTACK", kActToggle, kParamSlowAttack, 0, 0, false });
        btns.push_back({ { 735.0f, kRowBY + 124.0f, 34.0f, 34.0f }, L"SUSTAIN", kActToggle, kParamSustain, 0, 0, false });

        // --- Knappraden ---
        const float by = kRowCY + 32.0f, bw = 33.0f, pitch = 37.0f, gap = 12.0f;
        float x = 28.0f;
        auto group = [&](const wchar_t* label, int n) { groups.push_back({ x, x + (n - 1) * pitch + bw, kRowCY + 18.0f, label }); };
        auto add = [&](const wchar_t* label, int act, int param, int index, int count, bool cream) {
            btns.push_back({ { x, by, bw, bw }, label, act, param, index, count, cream });
            x += pitch;
        };
        group(L"", 1);
        add(L"MANUAL\nBASS", kActManBass, kParamSoloMode, 2, kNumSoloModes, false);
        x += gap;
        const float bassX = x - gap - pitch;
        group(L"MODE", 2);
        add(L"BASS", kActSoloMode, kParamSoloMode, 1, kNumSoloModes, true);
        add(L"NORMAL", kActSoloMode, kParamSoloMode, 0, kNumSoloModes, true);
        x += gap;
        group(L"TOUCH", 3);
        add(L"VIBRATO", kActToggle, kParamTouchVib, 0, 0, false);
        add(L"BRILLI-\nANCE", kActToggle, kParamTouchBrill, 0, 0, false);
        add(L"MODU-\nLATION", kActToggle, kParamTouchMod, 0, 0, false);
        x += gap;
        const float splitX = x;
        group(L"SPLIT", 2);
        add(L"POLY|\nORGAN", kActSplit, kParamSplit, 1, kNumSplit, false);
        add(L"ORGAN|\nPOLY", kActSplit, kParamSplit, 2, kNumSplit, false);
        x += gap;
        group(L"ORGAN", 5);
        add(L"CANCEL", kActRadio, kParamOrgSel, 0, kNumOrgSel, false);
        add(L"1", kActRadio, kParamOrgSel, 1, kNumOrgSel, true);
        add(L"2", kActRadio, kParamOrgSel, 2, kNumOrgSel, true);
        add(L"3", kActRadio, kParamOrgSel, 3, kNumOrgSel, true);
        add(L"MANUAL", kActRadio, kParamOrgSel, 4, kNumOrgSel, true);
        x += gap;
        group(L"STRING", 4);
        add(L"CANCEL", kActStrCancel, 0, 0, 0, false);
        add(L"1", kActToggle, kParamString1, 0, 0, true);
        add(L"2", kActToggle, kParamString2, 0, 0, true);
        add(L"3", kActToggle, kParamString3, 0, 0, true);
        x += gap;
        group(L"POLY-SYNTH", 5);
        add(L"CANCEL", kActRadio, kParamPolySel, 0, kNumPolySel, false);
        add(L"1", kActRadio, kParamPolySel, 1, kNumPolySel, true);
        add(L"2", kActRadio, kParamPolySel, 2, kNumPolySel, true);
        add(L"3", kActRadio, kParamPolySel, 3, kNumPolySel, true);
        add(L"MANUAL", kActRadio, kParamPolySel, 4, kNumPolySel, true);
        x += gap;
        group(L"TREMOLO / ENSEMBLE", 5);
        add(L"ORGAN\nTREM", kActToggle, kParamOrgTremolo, 0, 0, false);
        add(L"ORGAN\nENS", kActToggle, kParamOrgEnsemble, 0, 0, false);
        add(L"POLY\nTREM", kActToggle, kParamPolyTremolo, 0, 0, false);
        add(L"POLY\nENS", kActToggle, kParamEnsemble, 0, 0, false);
        add(L"TREM\nSPEED", kActToggle, kParamTremoloSpeed, 0, 0, false);

        // Delningspunkter: små pilväljare under knapparna
        const float sy = kRowCY + 102.0f;
        bassPrev = { bassX, sy, 18.0f, 20.0f };
        bassNext = { bassX + 3 * pitch + gap - 22.0f, sy, 18.0f, 20.0f };
        splitPrev = { splitX, sy, 18.0f, 20.0f };
        splitNext = { splitX + 2 * pitch - 22.0f, sy, 18.0f, 20.0f };
    }

    // Reglage under musen
    int ctlAt(int x, int y) const
    {
        for (size_t i = 0; i < ctls.size(); ++i) {
            const Ctl& c = ctls[i];
            if (c.kind == kFader) { if (x >= c.x - 15 && x <= c.x + 15 && y >= c.y - 10 && y <= c.y + kTrackLen + 10) return (int)i; }
            else if (c.kind == kKnob) { const float dx = x - c.x, dy = y - c.y; if (dx * dx + dy * dy <= (kKnobR + 10) * (kKnobR + 10)) return (int)i; }
            else { const float h = (c.npos - 1) * 18.0f; if (x >= c.x - 10 && x <= c.x + 34 && y >= c.y - 10 && y <= c.y + h + 10) return (int)i; }
        }
        return -1;
    }
};

// ---------------------------------------------------------------------------
// Ritning
// ---------------------------------------------------------------------------
void roundedRect(GraphicsPath& path, float x, float y, float w, float h, float r)
{
    const float d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
}

void drawPanel(Graphics& g, float x, float y, float w, float h)
{
    GraphicsPath path;
    roundedRect(path, x, y, w, h, 8);
    SolidBrush fill(kPanel);
    g.FillPath(&fill, &path);
    Pen edge(kPanelEdge, 1.0f);
    g.DrawPath(&edge, &path);
}

void drawText(Graphics& g, const wchar_t* text, const FontFamily& ff, float size, int style,
              const Color& color, float x, float y, float w, StringAlignment align, float h = 0.0f)
{
    Font font(&ff, size, style, UnitPixel);
    SolidBrush brush(color);
    StringFormat fmt;
    fmt.SetAlignment(align);
    RectF r(x, y, w, h > 0.0f ? h : size * 1.6f);
    g.DrawString(text, -1, &font, r, &fmt, &brush);
}

float textWidth(Graphics& g, const wchar_t* text, const FontFamily& ff, float size, int style)
{
    Font font(&ff, size, style, UnitPixel);
    RectF box;
    g.MeasureString(text, -1, &font, PointF(0.0f, 0.0f), StringFormat::GenericTypographic(), &box);
    return box.Width;
}

// Knapp i TrueSub-stil (PRESET, SAVE, LOAD, BACK, pilar)
void drawFlatButton(Graphics& g, const FontFamily& ff, const RectDef& r, const wchar_t* label, bool active = false, float size = 13.0f)
{
    GraphicsPath path;
    roundedRect(path, r.x, r.y, r.w, r.h, 5);
    SolidBrush fill(active ? Color(255, 10, 60, 80) : Color(255, 30, 33, 37));
    g.FillPath(&fill, &path);
    Pen edge(active ? kCyan : Color(255, 70, 74, 80), active ? 2.0f : 1.0f);
    g.DrawPath(&edge, &path);
    const bool arrow = label[0] != 0 && label[1] == 0 && (label[0] == L'<' || label[0] == L'>');
    drawText(g, label, ff, size, FontStyleBold, active ? kCyan : kTextDim, r.x, r.y + (r.h - size * 1.35f) / 2.0f + (arrow ? -1.5f : 0.0f),
             r.w, StringAlignmentCenter);
}

void drawBox(Graphics& g, float x, float y, float w, float h)
{
    GraphicsPath box;
    roundedRect(box, x, y, w, h, 5);
    SolidBrush boxFill(Color(255, 6, 7, 8));
    g.FillPath(&boxFill, &box);
    Pen boxEdge(Color(255, 70, 74, 80), 1.0f);
    g.DrawPath(&boxEdge, &box);
}

// SK30-knapp enligt Fredriks bild: platt grå (124) eller krämvit (242,238,230),
// tunn kant och en mörk lysdiodsspringa upptill (17 x 6 av 55 px), som lyser röd när knappen är på.
void drawSkButton(Graphics& g, const FontFamily& ff, const Btn& b, bool on)
{
    const RectDef& r = b.r;
    SolidBrush body(b.cream ? Color(255, 242, 238, 230) : Color(255, 124, 124, 124));
    g.FillRectangle(&body, r.x, r.y, r.w, r.h);
    Pen edge(kPanelEdge, 1.0f);
    g.DrawRectangle(&edge, r.x, r.y, r.w, r.h);
    // lysdioden
    const float lw = r.w * 17.0f / 55.0f, lh = r.w * 6.0f / 55.0f, lx = r.x + (r.w - lw) / 2.0f, ly = r.y + r.w * 6.0f / 55.0f;
    if (on) {
        SolidBrush glow(Color(90, 255, 60, 40));
        g.FillEllipse(&glow, lx - 5.0f, ly - 4.0f, lw + 10.0f, lh + 8.0f);
        SolidBrush led(Color(255, 255, 52, 36));
        g.FillRectangle(&led, lx, ly, lw, lh);
        SolidBrush hot(Color(255, 255, 170, 150));
        g.FillRectangle(&hot, lx + 2.0f, ly + 1.0f, lw - 4.0f, 1.5f);
    } else {
        SolidBrush led(kPanel);
        g.FillRectangle(&led, lx, ly, lw, lh);
    }
    drawText(g, b.label, ff, 8.5f, FontStyleBold, kLabel, r.x - 6.0f, r.y + r.h + 3.0f, r.w + 12.0f, StringAlignmentCenter, 30.0f);
}

// Guldfärgad grupplinje med rubrik (som på SK30-panelen)
void drawGroup(Graphics& g, const FontFamily& ff, const Group& gr)
{
    Pen pen(kGold, 2.0f);
    g.DrawLine(&pen, gr.x0, gr.y, gr.x1, gr.y);
    g.DrawLine(&pen, gr.x0, gr.y - 1.0f, gr.x0, gr.y + 10.0f);
    g.DrawLine(&pen, gr.x1, gr.y - 1.0f, gr.x1, gr.y + 10.0f);
    if (gr.label && gr.label[0]) {
        const float w = textWidth(g, gr.label, ff, 11.0f, FontStyleBold) + 10.0f;
        const float cx = (gr.x0 + gr.x1) * 0.5f;
        SolidBrush cut(kPanel);
        g.FillRectangle(&cut, cx - w / 2.0f, gr.y - 3.0f, w, 6.0f);
        drawText(g, gr.label, ff, 11.0f, FontStyleBold, kGold, cx - w / 2.0f - 20.0f, gr.y - 9.0f, w + 40.0f, StringAlignmentCenter);
    }
}

void drawSectionTitle(Graphics& g, const FontFamily& ff, const wchar_t* title, float x, float y, float w)
{
    drawText(g, title, ff, 13.0f, FontStyleBold, kGold, x, y + 8.0f, w, StringAlignmentCenter);
}

// Skjutreglage som på SK30
void drawFader(Graphics& g, const FontFamily& ff, const Ctl& c, float v, bool dim, bool active)
{
    const float x = c.x, y = c.y, L = kTrackLen;
    // skala
    Pen tick(Color(255, 96, 96, 96), 1.0f), tickBig(Color(255, 150, 150, 150), 1.0f);
    const int n = c.steps > 1 ? c.steps : 11;
    for (int i = 0; i < n; ++i) {
        const float ty = y + L * (1.0f - (float)i / (n - 1));
        const bool big = c.steps > 1 || i == 0 || i == 5 || i == 10;
        g.DrawLine(big ? &tickBig : &tick, x - 14.0f, ty, x - (big ? 7.0f : 10.0f), ty);
    }
    // spår
    SolidBrush slot(Color(255, 2, 2, 2));
    g.FillRectangle(&slot, x - 2.5f, y - 5.0f, 5.0f, L + 10.0f);
    Pen slotEdge(Color(255, 58, 58, 60), 1.0f);
    g.DrawRectangle(&slotEdge, x - 2.5f, y - 5.0f, 5.0f, L + 10.0f);
    // knopp
    const float cy = y + L * (1.0f - v);
    SolidBrush sh(Color(150, 0, 0, 0));
    g.FillRectangle(&sh, x - 11.0f + 1.5f, cy - 7.0f + 3.0f, 22.0f, 14.0f);
    Color top, bot;
    if (dim) { top = Color(255, 92, 90, 86); bot = Color(255, 70, 68, 64); }
    else if (c.cap == kCapAmber) { top = Color(255, 246, 190, 92); bot = Color(255, 206, 140, 44); }
    else { top = Color(255, 246, 243, 235); bot = Color(255, 206, 201, 188); }
    LinearGradientBrush capB(PointF(0.0f, cy - 7.0f), PointF(0.0f, cy + 7.0f), top, bot);
    g.FillRectangle(&capB, x - 11.0f, cy - 7.0f, 22.0f, 14.0f);
    Pen capEdge(active ? kCyan : Color(255, 20, 20, 20), active ? 1.5f : 1.0f);
    g.DrawRectangle(&capEdge, x - 11.0f, cy - 7.0f, 22.0f, 14.0f);
    Pen mid(Color(255, 40, 34, 26), 1.5f);
    g.DrawLine(&mid, x - 9.0f, cy, x + 9.0f, cy);
    drawText(g, c.label, ff, 9.0f, FontStyleBold, dim ? kTextDim : kLabel, x - 22.0f, y + L + 12.0f, 44.0f, StringAlignmentCenter, 30.0f);
}

// Ratt i TrueSub-stil (kuggkant, cyan båge)
void drawKnob(Graphics& g, const FontFamily& ff, const Ctl& c, float v, bool dim, bool bipolar)
{
    const float cx = c.x, cy = c.y, r = kKnobR;
    const float start = 135.0f, sweep = 270.0f, arcW = 4.0f, ar = r + 6.0f;
    Pen track(Color(255, 40, 43, 47), arcW);
    g.DrawArc(&track, cx - ar, cy - ar, ar * 2, ar * 2, start, sweep);
    Pen arc(dim ? Color(255, 70, 90, 98) : kCyan, arcW);
    if (bipolar) { const float d = v - 0.5f; if (std::fabs(d) > 0.001f) g.DrawArc(&arc, cx - ar, cy - ar, ar * 2, ar * 2, start + sweep * 0.5f, sweep * d); }
    else if (v > 0.001f) g.DrawArc(&arc, cx - ar, cy - ar, ar * 2, ar * 2, start, sweep * v);
    const float kPiF = 3.14159265f;
    const float angle = (start + sweep * v) * kPiF / 180.0f;
    for (int i = 0; i < 4; ++i) {
        const float rr = r + 2.5f - i * 1.0f;
        SolidBrush sh(Color((BYTE)(40 + i * 25), 0, 0, 0));
        g.FillEllipse(&sh, cx - rr + 1.5f, cy - rr + 3.0f, rr * 2, rr * 2);
    }
    const int n = 14;
    const float tip = r * 0.96f, root = tip - 2.5f;
    PointF pts[14 * 24];
    const int m = n * 24;
    for (int i = 0; i < m; ++i) {
        const float a = 2.0f * kPiF * i / m;
        float u = (a - angle + kPiF / n) * n / (2.0f * kPiF);
        u -= std::floor(u);
        const float xx = std::fabs(u - 0.5f) * 2.0f;
        float t = (xx - 0.25f) / 0.5f;
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        t = t * t * (3.0f - 2.0f * t);
        const float rr = tip - (tip - root) * t;
        pts[i] = PointF(cx + rr * std::cos(a), cy + rr * std::sin(a));
    }
    SolidBrush darkB(Color(255, 14, 14, 16));
    g.FillEllipse(&darkB, cx - r, cy - r, r * 2, r * 2);
    SolidBrush rimB(dim ? Color(255, 72, 80, 88) : Color(255, 115, 127, 139));
    g.FillPolygon(&rimB, pts, m);
    const float ringOut = root - 3.5f, ringIn = ringOut - 2.5f;
    SolidBrush ringB(Color(255, 26, 34, 43)), capB(dim ? Color(255, 68, 76, 84) : Color(255, 109, 122, 134));
    g.FillEllipse(&ringB, cx - ringOut, cy - ringOut, ringOut * 2, ringOut * 2);
    g.FillEllipse(&capB, cx - ringIn, cy - ringIn, ringIn * 2, ringIn * 2);
    Pen pointer(dim ? Color(255, 140, 146, 152) : Color(255, 250, 250, 250), 2.0f);
    g.DrawLine(&pointer, cx + std::cos(angle) * (ringOut * 0.2f), cy + std::sin(angle) * (ringOut * 0.2f),
               cx + std::cos(angle) * (tip - 2.0f), cy + std::sin(angle) * (tip - 2.0f));
    drawText(g, c.label, ff, 9.5f, FontStyleBold, dim ? kTextDim : kLabel, cx - 34.0f, cy + r + 12.0f, 68.0f, StringAlignmentCenter, 30.0f);
}

// Vippomkopplare med 2 eller 3 lägen (PW-källa, VCA, MODE)
void drawSwitch(Graphics& g, const FontFamily& ff, const Ctl& c, int posTop, bool dim)
{
    const float h = (c.npos - 1) * 18.0f;
    SolidBrush slot(Color(255, 2, 2, 2));
    g.FillRectangle(&slot, c.x - 4.0f, c.y - 6.0f, 8.0f, h + 12.0f);
    Pen edge(Color(255, 58, 58, 60), 1.0f);
    g.DrawRectangle(&edge, c.x - 4.0f, c.y - 6.0f, 8.0f, h + 12.0f);
    for (int i = 0; i < c.npos; ++i) {
        const float py = c.y + i * 18.0f;
        drawText(g, c.pos[i], ff, 9.5f, FontStyleBold, i == posTop && !dim ? kCyan : kTextDim, c.x + 9.0f, py - 7.5f, 40.0f, StringAlignmentNear);
    }
    const float ly = c.y + posTop * 18.0f;
    LinearGradientBrush lever(PointF(0.0f, ly - 5.0f), PointF(0.0f, ly + 5.0f),
                              dim ? Color(255, 110, 110, 110) : Color(255, 236, 236, 236), dim ? Color(255, 80, 80, 80) : Color(255, 160, 160, 160));
    g.FillRectangle(&lever, c.x - 6.0f, ly - 5.0f, 12.0f, 10.0f);
    Pen le(Color(255, 20, 20, 20), 1.0f);
    g.DrawRectangle(&le, c.x - 6.0f, ly - 5.0f, 12.0f, 10.0f);
    if (c.label && c.label[0]) drawText(g, c.label, ff, 9.5f, FontStyleBold, dim ? kTextDim : kLabel, c.x - 20.0f, c.y + h + 18.0f, 40.0f, StringAlignmentCenter);
}

void wideFromNarrow(const char* in, wchar_t* out, int max)
{
    int i = 0;
    for (; i < max - 1 && in[i]; ++i) out[i] = (wchar_t)(unsigned char)in[i];
    out[i] = 0;
}

// Värde som text (för displayen), med enhet
void valueText(int param, float v, wchar_t* out, int max)
{
    char buf[48];
    paramDisplayText(param, v, buf, sizeof buf);
    wchar_t w[48];
    wideFromNarrow(buf, w, 48);
    const char* unit = kParamLabels[param];
    if (unit && unit[0] && std::strcmp(buf, "-inf") != 0) {
        wchar_t u[16];
        wideFromNarrow(unit, u, 16);
        if (param == kParamSustainLevel || param == kParamSoloSustain) swprintf(out, max, L"%ls", w);
        else swprintf(out, max, L"%ls %ls", w, u);
    } else swprintf(out, max, L"%ls%ls", w, std::strcmp(buf, "-inf") == 0 ? L" dB" : L"");
}

// Är reglaget overksamt just nu? (ritas grått)
bool isDim(const float* pv, int param)
{
    const int polySel = stepIndex(pv[kParamPolySel], kNumPolySel);
    const int orgSel = stepIndex(pv[kParamOrgSel], kNumOrgSel);
    switch (param) {
    case kParamPolyFeet: case kParamCutoff: case kParamResonance: case kParamEgDepth:
    case kParamAttack: case kParamDecay: case kParamSustainLevel: case kParamRelease:
        return polySel != 4;
    case kParamDetune: return !isOn(pv[kParamMode]);
    case kParamSustainTime: return !isOn(pv[kParamSustain]);
    case kParamOrgTime: return !isOn(pv[kParamOrgDecay]) && !isOn(pv[kParamOrgSustain]);
    case kParamPercDecay: case kParamPerc2: case kParamPerc3: case kParamPerc5: case kParamOrgBrill:
        return orgSel == 0;
    case kParamSoloPw: return pv[kParamSoloWave] < 0.001f;
    default: break;
    }
    if (param >= kParamOrg16 && param <= kParamOrg1) return orgSel != 4;
    if ((param >= kParamLfoSpeed && param <= kParamSoloRelease) || param == kParamSoloTune) return pv[kParamSoloVol] <= 0.0f;
    return false;
}

bool btnOn(const float* pv, const Btn& b)
{
    switch (b.act) {
    case kActRadio: return stepIndex(pv[b.param], b.count) == b.index;
    case kActToggle: return isOn(pv[b.param]);
    case kActStrCancel: return !isOn(pv[kParamString1]) && !isOn(pv[kParamString2]) && !isOn(pv[kParamString3]);
    case kActManBass: return stepIndex(pv[kParamSoloMode], kNumSoloModes) == 2;
    case kActSoloMode: return stepIndex(pv[kParamSoloMode], kNumSoloModes) == b.index;
    case kActSplit: return stepIndex(pv[kParamSplit], kNumSplit) == b.index;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Baksidan: credits och manual (ENG / SWE)
// ---------------------------------------------------------------------------
const float kManX = 40.0f, kManW = 1080.0f, kManTop = 214.0f, kManBottom = 650.0f;
const float kThumbX = 1138.0f, kThumbW = 6.0f;

float manualMaxScroll(const Editor* ed) { return std::max(0.0f, ed->contentH - (kManBottom - kManTop)); }

void manualFont(int kind, float& size, int& style, Color& color)
{
    if (kind == kManHead) { size = 14.0f; style = FontStyleBold; color = kGold; }
    else if (kind == kManNote) { size = 12.0f; style = FontStyleItalic; color = kTextDim; }
    else { size = 13.0f; style = FontStyleRegular; color = kText; }
}

float layoutManual(Graphics& g, const FontFamily& ff, bool swedish, float y0, bool draw)
{
    const ManualPara* man = swedish ? kManualSwe : kManualEng;
    const int count = swedish ? kManualSweCount : kManualEngCount;
    float y = y0;
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentNear);
    for (int i = 0; i < count; ++i) {
        const ManualPara& mp = man[i];
        float size; int style; Color color;
        manualFont(mp.kind, size, style, color);
        if (mp.kind == kManHead && i > 0) y += 14.0f;
        Font font(&ff, size, style, UnitPixel);
        RectF box;
        g.MeasureString(mp.text, -1, &font, RectF(0.0f, 0.0f, kManW, 10000.0f), &fmt, &box);
        if (draw && y + box.Height > kManTop - 20.0f && y < kManBottom + 20.0f) {
            SolidBrush brush(color);
            g.DrawString(mp.text, -1, &font, RectF(kManX, y, kManW, box.Height + 2.0f), &fmt, &brush);
        }
        y += box.Height + (mp.kind == kManHead ? 3.0f : 7.0f);
    }
    return y - y0;
}

void paintBack(Editor* ed, Graphics& g, const FontFamily& ff)
{
    drawPanel(g, 16.0f, 78.0f, 1148.0f, 90.0f);
    wchar_t title[64];
    swprintf(title, 64, L"TrueSymph %ls", kVersionText);
    drawText(g, title, ff, 22, FontStyleBold, kText, 36.0f, 86.0f, 400.0f, StringAlignmentNear);
    drawText(g, L"Symphonic Ensemble  ·  after the Yamaha SK30 (1979)  ·  VST2  ·  Windows 7 and later", ff, 13,
             FontStyleItalic, kText, 36.0f, 116.0f, 800.0f, StringAlignmentNear);
    {
        // Fredrik Frigren | Copyright (c) 2F Productions 2026 | info@2f.se
        Font f(&ff, 15, FontStyleBold, UnitPixel);
        StringFormat typo(StringFormat::GenericTypographic());
        typo.SetFormatFlags(typo.GetFormatFlags() | StringFormatFlagsMeasureTrailingSpaces);
        SolidBrush cy(kCyan);
        const wchar_t* partA = L"Fredrik Frigren  |  Copyright ";
        const wchar_t* partB = L" 2F Productions 2026  |  info@2f.se";
        RectF boxA;
        g.MeasureString(partA, -1, &f, PointF(0.0f, 0.0f), &typo, &boxA);
        float x = 38.0f;
        const float y = 140.0f;
        g.DrawString(partA, -1, &f, PointF(x, y), &typo, &cy);
        x += boxA.Width;
        const float d = 15.0f, cyc = y + 9.5f;
        Pen ring(kCyan, 1.6f);
        g.DrawEllipse(&ring, x + 1.0f, cyc - d * 0.5f, d, d);
        drawText(g, L"C", ff, 10, FontStyleBold, kCyan, x + 1.0f, cyc - 7.5f, d, StringAlignmentCenter);
        x += d + 2.0f;
        g.DrawString(partB, -1, &f, PointF(x, y), &typo, &cy);
    }
    drawFlatButton(g, ff, ed->btnBack, L"BACK");

    drawPanel(g, 16.0f, 176.0f, 1148.0f, 484.0f);
    drawText(g, L"MANUAL", ff, 14, FontStyleBold, kGold, 36.0f, 184.0f, 80.0f, StringAlignmentNear);
    // ENG | SWE
    const float lx = 36.0f + textWidth(g, L"MANUAL", ff, 14, FontStyleBold) + 14.0f;
    const float we = textWidth(g, L"ENG", ff, 13, FontStyleBold), wsw = textWidth(g, L"SWE", ff, 13, FontStyleBold);
    ed->btnEng = { lx - 2.0f, 184.0f, we + 4.0f, 20.0f };
    ed->btnSwe = { lx + we + 20.0f, 184.0f, wsw + 4.0f, 20.0f };
    drawText(g, L"ENG", ff, 13, FontStyleBold, ed->swedish ? kTextDim : kCyan, ed->btnEng.x, 185.0f, ed->btnEng.w + 6.0f, StringAlignmentNear);
    drawText(g, L"|", ff, 13, FontStyleBold, kTextDim, lx + we + 6.0f, 185.0f, 12.0f, StringAlignmentNear);
    drawText(g, L"SWE", ff, 13, FontStyleBold, ed->swedish ? kCyan : kTextDim, ed->btnSwe.x, 185.0f, ed->btnSwe.w + 6.0f, StringAlignmentNear);
    drawText(g, ed->swedish ? L"rulla med mushjulet  ·  klicka på loggan eller BACK för att gå tillbaka"
                            : L"scroll with the mouse wheel  ·  click the logo or BACK to return",
             ff, 11, FontStyleRegular, kTextDim, 560.0f, 188.0f, 586.0f, StringAlignmentFar);

    if (ed->contentH <= 0.0f) ed->contentH = layoutManual(g, ff, ed->swedish, 0.0f, false);
    const float maxScroll = manualMaxScroll(ed);
    if (ed->scroll > maxScroll) ed->scroll = maxScroll;
    if (ed->scroll < 0.0f) ed->scroll = 0.0f;
    g.SetClip(RectF(kManX - 4.0f, kManTop, kManW + 8.0f, kManBottom - kManTop));
    layoutManual(g, ff, ed->swedish, kManTop - ed->scroll, true);
    g.ResetClip();

    const float viewH = kManBottom - kManTop;
    SolidBrush track(Color(255, 30, 33, 37));
    g.FillRectangle(&track, kThumbX, kManTop, kThumbW, viewH);
    if (maxScroll > 0.0f) {
        const float th = std::max(30.0f, viewH * viewH / ed->contentH);
        const float ty = kManTop + (viewH - th) * (ed->scroll / maxScroll);
        SolidBrush thumb(ed->dragThumb ? kCyan : Color(255, 90, 96, 104));
        g.FillRectangle(&thumb, kThumbX, ty, kThumbW, th);
    }
}

// ---------------------------------------------------------------------------
// Framsidan
// ---------------------------------------------------------------------------
void paint(Editor* ed, HDC hdc, int width, int height)
{
    Core* p = ed->plugin;
    if (width <= 0 || height <= 0) return;

    Bitmap buffer(width, height, PixelFormat32bppPARGB);
    Graphics g(&buffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    FontFamily segoe(L"Segoe UI");   // finns i Windows 7
    FontFamily arial(L"Arial");
    const FontFamily& ff = segoe.IsAvailable() ? segoe : arial;

    LinearGradientBrush bg(Point(0, 0), Point(0, height), kBgTop, kBgBottom);
    g.FillRectangle(&bg, 0, 0, width, height);
    Pen hair(Color(10, 255, 255, 255), 1.0f);
    for (int y = 0; y < height; y += 3) g.DrawLine(&hair, 0, y, width, y);

    float pv[kNumParams];
    for (int i = 0; i < kNumParams; ++i) pv[i] = p->params[i].load();

    const float pk = p->peak.exchange(0.0f);
    ed->peakShown = pk > ed->peakShown ? pk : ed->peakShown * 0.80f;

    // --- Header: logga, display, PRESET, version ---
    {
        Font logoFont(&ff, 38, FontStyleBoldItalic, UnitPixel);
        const StringFormat* typo = StringFormat::GenericTypographic();
        RectF trueBox, symBox;
        g.MeasureString(L"True", -1, &logoFont, PointF(0.0f, 0.0f), typo, &trueBox);
        g.MeasureString(L"Symph", -1, &logoFont, PointF(0.0f, 0.0f), typo, &symBox);
        SolidBrush white(kText), blue(kCyan);
        g.DrawString(L"True", -1, &logoFont, PointF(21.0f, 4.0f), typo, &white);
        g.DrawString(L"Symph", -1, &logoFont, PointF(21.0f + trueBox.Width, 4.0f), typo, &blue);
        ed->logoRight = 21.0f + trueBox.Width + symBox.Width + 6.0f;
    }
    drawText(g, L"Symphonic Ensemble", ff, 14, FontStyleItalic, kText, 22, 46, 260, StringAlignmentNear);

    wchar_t txt[96];
    // Display: reglaget under musen (eller det senast ändrade)
    {
        const float dx = 330.0f, dy = 14.0f, dw = 300.0f, dh = 44.0f;
        drawBox(g, dx, dy, dw, dh);
        const int prm = ed->dragCtl >= 0 ? ed->ctls[ed->dragCtl].param : (ed->hoverParam >= 0 ? ed->hoverParam : ed->lastParam);
        if (prm >= 0) {
            drawText(g, kDisplayNames[prm], ff, 11, FontStyleBold, kTextDim, dx + 10.0f, dy + 4.0f, dw - 20.0f, StringAlignmentNear);
            valueText(prm, pv[prm], txt, 96);
            drawText(g, txt, ff, 18, FontStyleBold, kCyan, dx + 10.0f, dy + 17.0f, dw - 20.0f, StringAlignmentNear);
        } else {
            drawText(g, L"SYMPHONIC ENSEMBLE", ff, 11, FontStyleBold, kTextDim, dx + 10.0f, dy + 4.0f, dw - 20.0f, StringAlignmentNear);
            const int sn = p->soloNote.load();
            if (sn >= 0 && pv[kParamSoloVol] > 0.0f) { char nb[16]; noteName(sn, nb, sizeof nb); wchar_t wn[16]; wideFromNarrow(nb, wn, 16); swprintf(txt, 96, L"SOLO  %ls", wn); }
            else swprintf(txt, 96, L"move the mouse over a control");
            drawText(g, txt, ff, sn >= 0 && pv[kParamSoloVol] > 0.0f ? 18.0f : 13.0f, FontStyleBold, kCyan, dx + 10.0f, dy + (sn >= 0 && pv[kParamSoloVol] > 0.0f ? 17.0f : 21.0f), dw - 20.0f, StringAlignmentNear);
        }
    }
    // PRESET
    drawText(g, L"PRESET", ff, 11, FontStyleBold, kGold, ed->presetPrev.x, 8.0f, 100.0f, StringAlignmentNear);
    drawFlatButton(g, ff, ed->presetPrev, L"<");
    drawFlatButton(g, ff, ed->presetNext, L">");
    {
        const float bx = ed->presetPrev.x + ed->presetPrev.w + 4.0f, bw = ed->presetNext.x - bx - 4.0f;
        drawBox(g, bx, ed->presetPrev.y, bw, ed->presetPrev.h);
        wchar_t nm[40];
        wideFromNarrow(p->presetName, nm, 40);
        drawText(g, nm, ff, 14, FontStyleBold, kCyan, bx, ed->presetPrev.y + 4.0f, bw - 10.0f, StringAlignmentCenter);
        // liten pil: klicka för lista
        PointF tri[3] = { PointF(bx + bw - 15.0f, ed->presetPrev.y + 12.0f), PointF(bx + bw - 7.0f, ed->presetPrev.y + 12.0f),
                          PointF(bx + bw - 11.0f, ed->presetPrev.y + 17.0f) };
        SolidBrush triB(kTextDim);
        g.FillPolygon(&triB, tri, 3);
    }
    drawFlatButton(g, ff, ed->btnSave, L"SAVE");
    drawFlatButton(g, ff, ed->btnLoad, L"LOAD");
    drawText(g, kVersionText, ff, 12, FontStyleRegular, kTextDim, 1074.0f, 8.0f, 86.0f, StringAlignmentFar);
    // Utnivå (dBFS, som i Live) och limiterns lampa
    {
        const float mx = 1074.0f, my = 30.0f, mw = 86.0f, mh = 8.0f;
        const double db = ed->peakShown > 1e-6f ? 20.0 * std::log10(ed->peakShown) : -100.0;
        if (db > ed->peakHoldDb || GetTickCount() > ed->peakHoldUntil) { ed->peakHoldDb = db; ed->peakHoldUntil = GetTickCount() + 1500; }
        const float lr = p->limitDb.exchange(0.0f);
        if (lr > 0.5f) ed->limitUntil = GetTickCount() + 800;
        SolidBrush back(Color(255, 6, 7, 8));
        g.FillRectangle(&back, mx, my, mw, mh);
        auto frac = [](double d) { double t = (d + 48.0) / 48.0; return (float)(t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t)); };
        const float f = frac(db);
        SolidBrush bar(db > -6.0 ? kGold : kCyan);
        g.FillRectangle(&bar, mx, my, mw * f, mh);
        Pen holdPen(kText, 1.5f);
        if (ed->peakHoldDb > -48.0) { const float hx = mx + mw * frac(ed->peakHoldDb); g.DrawLine(&holdPen, hx, my - 1.0f, hx, my + mh + 1.0f); }
        Pen edgeP(Color(255, 70, 74, 80), 1.0f);
        g.DrawRectangle(&edgeP, mx, my, mw, mh);
        if (ed->peakHoldDb > -100.0) swprintf(txt, 96, L"%.1f dB", ed->peakHoldDb); else swprintf(txt, 96, L"-inf dB");
        drawText(g, txt, ff, 11, FontStyleBold, kTextDim, mx - 10.0f, my + mh + 3.0f, 60.0f, StringAlignmentNear);
        const bool lim = GetTickCount() < ed->limitUntil;
        drawText(g, L"LIMIT", ff, 11, FontStyleBold, lim ? Color(255, 255, 70, 50) : Color(255, 50, 54, 58), mx + 40.0f, my + mh + 3.0f, 46.0f, StringAlignmentFar);
    }

    if (ed->showBack) {
        paintBack(ed, g, ff);
        Graphics screen(hdc);
        screen.DrawImage(&buffer, 0, 0, width, height);
        return;
    }

    // --- Paneler ---
    drawPanel(g, 16.0f, kRowAY, 226.0f, kRowAH);   drawSectionTitle(g, ff, L"OUTPUT", 16.0f, kRowAY, 226.0f);
    drawPanel(g, 250.0f, kRowAY, 170.0f, kRowAH);  drawSectionTitle(g, ff, L"PITCH", 250.0f, kRowAY, 170.0f);
    drawPanel(g, 428.0f, kRowAY, 736.0f, kRowAH);  drawSectionTitle(g, ff, L"SOLO SYNTHESIZER", 428.0f, kRowAY, 736.0f);
    drawPanel(g, 16.0f, kRowBY, 134.0f, kRowBH);   drawSectionTitle(g, ff, L"VIBRATO", 16.0f, kRowBY, 134.0f);
    drawPanel(g, 158.0f, kRowBY, 562.0f, kRowBH);  drawSectionTitle(g, ff, L"ORGAN", 158.0f, kRowBY, 562.0f);
    drawPanel(g, 728.0f, kRowBY, 436.0f, kRowBH);  drawSectionTitle(g, ff, L"POLY-SYNTH", 728.0f, kRowBY, 436.0f);
    drawPanel(g, 16.0f, kRowCY, 1148.0f, kRowCH);
    for (const Group& gr : ed->groups) drawGroup(g, ff, gr);

    // --- Reglage ---
    for (size_t i = 0; i < ed->ctls.size(); ++i) {
        const Ctl& c = ed->ctls[i];
        const float v = pv[c.param];
        const bool dim = isDim(pv, c.param);
        const bool active = (int)i == ed->dragCtl;
        if (c.kind == kFader) {
            const float shown = c.steps > 1 ? (float)stepNorm(stepIndex(v, c.steps), c.steps) : v;
            drawFader(g, ff, c, shown, dim, active);
        } else if (c.kind == kKnob) {
            drawKnob(g, ff, c, v, dim, c.param == kParamDetune || c.param == kParamSoloTune);
        } else {
            const int idx = stepIndex(v, c.steps);
            int top = 0;
            for (int k = 0; k < c.npos; ++k) if (c.posIndex[k] == idx) top = k;
            drawSwitch(g, ff, c, top, dim);
        }
    }

    // --- Knappar ---
    for (const Btn& b : ed->btns) drawSkButton(g, ff, b, btnOn(pv, b));

    // --- Delningspunkterna ---
    {
        char nb[16]; wchar_t wn[16];
        drawFlatButton(g, ff, ed->bassPrev, L"<", false, 11.0f);
        drawFlatButton(g, ff, ed->bassNext, L">", false, 11.0f);
        noteName(splitNote(pv[kParamBassPoint]), nb, sizeof nb); wideFromNarrow(nb, wn, 16);
        swprintf(txt, 96, L"BASS TO %ls", wn);
        const bool manBass = stepIndex(pv[kParamSoloMode], kNumSoloModes) == 2;
        drawText(g, txt, ff, 10.0f, FontStyleBold, manBass ? kCyan : kTextDim, ed->bassPrev.x + 18.0f, ed->bassPrev.y + 3.0f,
                 ed->bassNext.x - ed->bassPrev.x - 18.0f, StringAlignmentCenter);
        drawFlatButton(g, ff, ed->splitPrev, L"<", false, 11.0f);
        drawFlatButton(g, ff, ed->splitNext, L">", false, 11.0f);
        noteName(splitNote(pv[kParamSplitPoint]), nb, sizeof nb); wideFromNarrow(nb, wn, 16);
        swprintf(txt, 96, L"AT %ls", wn);
        const bool split = stepIndex(pv[kParamSplit], kNumSplit) != 0;
        drawText(g, txt, ff, 10.0f, FontStyleBold, split ? kCyan : kTextDim, ed->splitPrev.x + 18.0f, ed->splitPrev.y + 3.0f,
                 ed->splitNext.x - ed->splitPrev.x - 18.0f, StringAlignmentCenter);
    }

    Graphics screen(hdc);
    screen.DrawImage(&buffer, 0, 0, width, height);
}

// ---------------------------------------------------------------------------
// Interaktion
// ---------------------------------------------------------------------------
void setParamUI(Editor* ed, int param, float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    ed->plugin->setParamFromEditor(param, v);
    ed->lastParam = param;
    InvalidateRect(ed->hwnd, nullptr, FALSE);
}

void setParamOnce(Editor* ed, int param, float v)
{
    ed->plugin->beginEdit(param);
    setParamUI(ed, param, v);
    ed->plugin->endEdit(param);
}

void presetsChanged(Editor* ed)
{
    ed->plugin->hostUpdateDisplay();
    InvalidateRect(ed->hwnd, nullptr, FALSE);
}

std::wstring presetFolder()
{
    wchar_t docs[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, 0, docs) != S_OK) return std::wstring();
    std::wstring dir = std::wstring(docs) + L"\\TrueSymph Presets";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

bool fileExists(const std::wstring& path)
{
    const DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring widen(const std::string& s)
{
    std::wstring w;
    for (char c : s) w += (wchar_t)(unsigned char)c;
    return w;
}

std::string presetNameFromPath(const wchar_t* path)
{
    const wchar_t* base = path;
    for (const wchar_t* c = path; *c; ++c) if (*c == L'\\' || *c == L'/') base = c + 1;
    std::string out;
    for (const wchar_t* c = base; *c; ++c) {
        if (*c == L'.' && wcschr(c + 1, L'.') == nullptr) break;
        out += (*c >= 32 && *c < 127) ? (char)*c : '_';
    }
    return cleanPresetName(out);
}

std::vector<std::wstring> listUserPresets(const std::wstring& dir)
{
    std::vector<std::wstring> files;
    if (dir.empty()) return files;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.tsy").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return files;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.push_back(dir + L"\\" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(files.begin(), files.end(), [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
    return files;
}

bool readPresetFile(const std::wstring& path, float* v)
{
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) return false;
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && text.size() < 65536) text.append(buf, n);
    std::fclose(f);
    std::string name;
    return presetFromText(text, v, &name);
}

bool loadUserPreset(Editor* ed, const std::wstring& path, bool showErrors)
{
    Core* p = ed->plugin;
    float v[kNumParams];
    if (!readPresetFile(path, v)) {
        if (showErrors) MessageBoxW(ed->hwnd, L"This file is not a TrueSymph preset.", L"TrueSymph", MB_OK | MB_ICONWARNING);
        return false;
    }
    p->applyValues(v, true);
    copyString(p->presetName, presetNameFromPath(path.c_str()).c_str(), sizeof p->presetName);
    p->program = -1;
    presetsChanged(ed);
    return true;
}

void stepPreset(Editor* ed, int step)
{
    Core* p = ed->plugin;
    const std::vector<std::wstring> user = listUserPresets(presetFolder());
    const int total = kNumFactoryPresets + (int)user.size();
    int pos = -1;
    if (p->program >= 0) pos = p->program;
    else {
        for (size_t i = 0; i < user.size(); ++i)
            if (presetNameFromPath(user[i].c_str()) == p->presetName) { pos = kNumFactoryPresets + (int)i; break; }
    }
    if (pos < 0) pos = step > 0 ? -1 : total;
    for (int tries = 0; tries < total; ++tries) {
        pos += step;
        if (pos < 0) pos = total - 1;
        if (pos >= total) pos = 0;
        if (pos < kNumFactoryPresets) { p->loadFactoryPreset(pos, true); presetsChanged(ed); return; }
        if (loadUserPreset(ed, user[pos - kNumFactoryPresets], false)) return;
    }
}

void savePresetFile(Editor* ed)
{
    Core* p = ed->plugin;
    const std::wstring dir = presetFolder();
    const std::string suggestion = uniquePresetName(p->presetName, [&](const std::string& n) {
        return !dir.empty() && fileExists(dir + L"\\" + widen(n) + L".tsy");
    });
    wchar_t file[MAX_PATH];
    wideFromNarrow(suggestion.c_str(), file, MAX_PATH);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = ed->hwnd;
    ofn.lpstrFilter = L"TrueSymph preset (*.tsy)\0*.tsy\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrDefExt = L"tsy";
    ofn.lpstrTitle = L"Save TrueSymph preset";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&ofn)) return;
    float v[kNumParams];
    for (int i = 0; i < kNumParams; ++i) v[i] = p->params[i].load();
    const std::string name = presetNameFromPath(file);
    const std::string text = presetToText(name, v);
    FILE* f = _wfopen(file, L"wb");
    if (!f || std::fwrite(text.data(), 1, text.size(), f) != text.size()) {
        if (f) std::fclose(f);
        MessageBoxW(ed->hwnd, L"The preset could not be saved.", L"TrueSymph", MB_OK | MB_ICONWARNING);
        return;
    }
    std::fclose(f);
    copyString(p->presetName, name.c_str(), sizeof p->presetName);
    p->program = -1;
    presetsChanged(ed);
}

void loadPresetFile(Editor* ed)
{
    const std::wstring dir = presetFolder();
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = ed->hwnd;
    ofn.lpstrFilter = L"TrueSymph preset (*.tsy)\0*.tsy\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrTitle = L"Load TrueSymph preset";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;
    loadUserPreset(ed, file, true);
}

// Klick på presetrutan: lista med alla ljud (fabriksljud och egna presets)
void showPresetMenu(Editor* ed)
{
    Core* p = ed->plugin;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    const int kPerColumn = 18;
    for (int i = 0; i < kNumFactoryPresets; ++i) {
        wchar_t nm[40];
        wideFromNarrow(factoryPresetName(i), nm, 40);
        UINT flags = MF_STRING | (p->program == i ? MF_CHECKED : 0);
        if (i > 0 && i % kPerColumn == 0) flags |= MF_MENUBARBREAK;     // ny kolumn
        AppendMenuW(menu, flags, 1 + i, nm);
    }
    const std::vector<std::wstring> user = listUserPresets(presetFolder());
    HMENU sub = CreatePopupMenu();
    if (sub) {
        if (user.empty()) AppendMenuW(sub, MF_STRING | MF_GRAYED, 0, L"(no saved presets yet)");
        for (size_t i = 0; i < user.size() && i < 500; ++i) {
            const std::string n = presetNameFromPath(user[i].c_str());
            UINT flags = MF_STRING | (p->program < 0 && n == p->presetName ? MF_CHECKED : 0);
            if (i > 0 && i % 25 == 0) flags |= MF_MENUBARBREAK;
            AppendMenuW(sub, flags, 1000 + (UINT)i, widen(n).c_str());
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_POPUP | MF_STRING, (UINT_PTR)sub, L"My presets");
    }
    POINT pt = { (LONG)(ed->presetPrev.x + ed->presetPrev.w + 4.0f), (LONG)(ed->presetPrev.y + ed->presetPrev.h + 2.0f) };
    ClientToScreen(ed->hwnd, &pt);
    const int cmd = (int)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY, pt.x, pt.y, 0, ed->hwnd, nullptr);
    DestroyMenu(menu);                      // tar även bort undermenyn
    if (cmd >= 1 && cmd <= kNumFactoryPresets) { p->loadFactoryPreset(cmd - 1, true); presetsChanged(ed); }
    else if (cmd >= 1000 && cmd - 1000 < (int)user.size()) loadUserPreset(ed, user[cmd - 1000], true);
}

void pressButton(Editor* ed, const Btn& b)
{
    Core* p = ed->plugin;
    switch (b.act) {
    case kActRadio: setParamOnce(ed, b.param, (float)stepNorm(b.index, b.count)); break;
    case kActToggle: setParamOnce(ed, b.param, isOn(p->params[b.param].load()) ? 0.0f : 1.0f); break;
    case kActStrCancel:
        setParamOnce(ed, kParamString1, 0.0f); setParamOnce(ed, kParamString2, 0.0f); setParamOnce(ed, kParamString3, 0.0f);
        ed->lastParam = kParamString1;
        break;
    case kActManBass: {
        const int m = stepIndex(p->params[kParamSoloMode].load(), kNumSoloModes);
        setParamOnce(ed, kParamSoloMode, (float)stepNorm(m == 2 ? 0 : 2, kNumSoloModes));
        break;
    }
    case kActSoloMode: setParamOnce(ed, kParamSoloMode, (float)stepNorm(b.index, kNumSoloModes)); break;
    case kActSplit: {
        const int s = stepIndex(p->params[kParamSplit].load(), kNumSplit);
        setParamOnce(ed, kParamSplit, (float)stepNorm(s == b.index ? 0 : b.index, kNumSplit));
        break;
    }
    }
}

void stepNote(Editor* ed, int param, int step)
{
    const int m = splitNote(ed->plugin->params[param].load()) + step;
    if (m < 36 || m > 84) return;
    setParamOnce(ed, param, (float)normFromSplitNote(m));
}

bool handleButtons(Editor* ed, int x, int y)
{
    for (const Btn& b : ed->btns) if (b.r.hit(x, y)) { pressButton(ed, b); return true; }
    if (ed->presetPrev.hit(x, y)) { stepPreset(ed, -1); return true; }
    if (x > ed->presetPrev.x + ed->presetPrev.w && x < ed->presetNext.x && y >= ed->presetPrev.y && y <= ed->presetPrev.y + ed->presetPrev.h) {
        showPresetMenu(ed);                 // klick på namnet: lista
        return true;
    }
    if (ed->presetNext.hit(x, y)) { stepPreset(ed, +1); return true; }
    if (ed->btnSave.hit(x, y)) { savePresetFile(ed); return true; }
    if (ed->btnLoad.hit(x, y)) { loadPresetFile(ed); return true; }
    if (ed->bassPrev.hit(x, y)) { stepNote(ed, kParamBassPoint, -1); return true; }
    if (ed->bassNext.hit(x, y)) { stepNote(ed, kParamBassPoint, +1); return true; }
    if (ed->splitPrev.hit(x, y)) { stepNote(ed, kParamSplitPoint, -1); return true; }
    if (ed->splitNext.hit(x, y)) { stepNote(ed, kParamSplitPoint, +1); return true; }
    return false;
}

// Knapp eller väljare under musen -> parameter (för displayen)
int hoverParamAt(Editor* ed, int x, int y)
{
    const int c = ed->ctlAt(x, y);
    if (c >= 0) return ed->ctls[c].param;
    for (const Btn& b : ed->btns) if (b.r.hit(x, y)) return b.act == kActStrCancel ? kParamString1 : b.param;
    if (ed->bassPrev.hit(x, y) || ed->bassNext.hit(x, y)) return kParamBassPoint;
    if (ed->splitPrev.hit(x, y) || ed->splitNext.hit(x, y)) return kParamSplitPoint;
    return -1;
}

float snapValue(const Ctl& c, float v)
{
    if (c.steps > 1) return (float)stepNorm(stepIndex(v, c.steps), c.steps);
    return v;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Editor* ed = reinterpret_cast<Editor*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ed = static_cast<Editor*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)ed);
        ed->hwnd = hwnd;
        SetTimer(hwnd, 1, 40, nullptr); // 25 bilder/s
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (ed) {
            RECT rc; GetClientRect(hwnd, &rc);
            paint(ed, hdc, rc.right - rc.left, rc.bottom - rc.top);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SETCURSOR:
        if (ed && LOWORD(lParam) == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            const bool overPreset = !ed->showBack && pt.x > ed->presetPrev.x + ed->presetPrev.w && pt.x < ed->presetNext.x
                                    && pt.y >= ed->presetPrev.y && pt.y <= ed->presetPrev.y + ed->presetPrev.h;
            if (overPreset || ed->overLogo(pt.x, pt.y) || (ed->showBack && (ed->btnBack.hit(pt.x, pt.y) || ed->btnEng.hit(pt.x, pt.y) || ed->btnSwe.hit(pt.x, pt.y)))) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        if (!ed) return 0;
        const int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
        if (ed->overLogo(x, y) || (ed->showBack && ed->btnBack.hit(x, y))) {
            ed->showBack = !ed->showBack;
            ed->scroll = 0.0f;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (ed->showBack) {
            if (ed->btnEng.hit(x, y) || ed->btnSwe.hit(x, y)) {
                const bool sw = ed->btnSwe.hit(x, y);
                if (sw != ed->swedish) { ed->swedish = sw; ed->contentH = 0.0f; ed->scroll = 0.0f; }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (x >= kThumbX - 6 && x <= kThumbX + kThumbW + 6 && y >= kManTop && y <= kManBottom && manualMaxScroll(ed) > 0.0f) {
                const float viewH = kManBottom - kManTop, maxScroll = manualMaxScroll(ed);
                const float th = std::max(30.0f, viewH * viewH / ed->contentH);
                const float ty = kManTop + (viewH - th) * (ed->scroll / maxScroll);
                if (y < ty) ed->scroll -= viewH * 0.9f;
                else if (y > ty + th) ed->scroll += viewH * 0.9f;
                else { ed->dragThumb = true; ed->dragThumbY = y; ed->dragScroll0 = ed->scroll; SetCapture(hwnd); }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        if (handleButtons(ed, x, y)) return 0;
        const int k = ed->ctlAt(x, y);
        if (k < 0) return 0;
        const Ctl& c = ed->ctls[k];
        if (msg == WM_LBUTTONDBLCLK) { setParamOnce(ed, c.param, defaultParam(c.param)); return 0; }
        if (c.kind == kSwitch) {
            // Klick på ett läge väljer det
            int best = 0; float bd = 1e9f;
            for (int i = 0; i < c.npos; ++i) { const float d = std::fabs(y - (c.y + i * 18.0f)); if (d < bd) { bd = d; best = i; } }
            setParamOnce(ed, c.param, (float)stepNorm(c.posIndex[best], c.steps));
            return 0;
        }
        ed->dragCtl = k;
        ed->dragStartY = y;
        ed->dragStartValue = ed->plugin->params[c.param].load();
        ed->plugin->beginEdit(c.param);
        ed->lastParam = c.param;
        SetCapture(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (ed) {
            ed->tracking = false;
            ed->hoverParam = -1;
            if (GetFocus() == hwnd && ed->prevFocus && IsWindow(ed->prevFocus)) SetFocus(ed->prevFocus);
            ed->prevFocus = nullptr;
        }
        return 0;
    case WM_MOUSEMOVE:
        // Windows 7 skickar mushjulet till fönstret som har fokus: ta fokus medan musen är över TrueSymph
        if (ed && !ed->tracking) {
            HWND f = GetFocus();
            if (f != hwnd) { ed->prevFocus = f; SetFocus(hwnd); }
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof tme;
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            if (TrackMouseEvent(&tme)) ed->tracking = true;
        }
        if (ed && ed->dragThumb) {
            const float viewH = kManBottom - kManTop, maxScroll = manualMaxScroll(ed);
            const float th = std::max(30.0f, viewH * viewH / std::max(ed->contentH, 1.0f));
            if (viewH - th > 1.0f) ed->scroll = ed->dragScroll0 + ((short)HIWORD(lParam) - ed->dragThumbY) * maxScroll / (viewH - th);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (ed && ed->dragCtl >= 0) {
            const Ctl& c = ed->ctls[ed->dragCtl];
            const int dy = ed->dragStartY - (short)HIWORD(lParam);
            const float range = c.kind == kFader ? kTrackLen : 250.0f;           // reglaget följer musen
            const float fine = (wParam & MK_SHIFT) ? 4.0f : 1.0f;                // Shift = finjustering
            setParamUI(ed, c.param, snapValue(c, ed->dragStartValue + dy / (range * fine)));
            return 0;
        }
        if (ed && !ed->showBack) {
            const int hp = hoverParamAt(ed, (short)LOWORD(lParam), (short)HIWORD(lParam));
            if (hp != ed->hoverParam) { ed->hoverParam = hp; InvalidateRect(hwnd, nullptr, FALSE); }
        }
        return 0;
    case WM_LBUTTONUP:
        if (ed && ed->dragThumb) {
            ed->dragThumb = false;
            ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (ed && ed->dragCtl >= 0) {
            ed->plugin->endEdit(ed->ctls[ed->dragCtl].param);
            ed->dragCtl = -1;
            ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (ed) {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            if (ed->showBack) {
                ed->scroll -= (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 48.0f;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            const int clicks = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            const int k = ed->ctlAt(pt.x, pt.y);
            if (k >= 0) {
                const Ctl& c = ed->ctls[k];
                const float step = c.steps > 1 ? 1.0f / (c.steps - 1) : 0.01f;
                setParamOnce(ed, c.param, snapValue(c, ed->plugin->params[c.param].load() + clicks * step));
            } else {
                const int hp = hoverParamAt(ed, pt.x, pt.y);
                if (hp == kParamSplitPoint || hp == kParamBassPoint) stepNote(ed, hp, clicks);
            }
        }
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

namespace truesymph {

void editorOpen(Core* p, void* parent)
{
    if (p->editor || !parent) return;
    if (!g_instance) {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&windowProc), &g_instance);
    }
    if (g_gdiplusUsers++ == 0) {
        GdiplusStartupInput input;
        GdiplusStartup(&g_gdiplusToken, &input, nullptr);
    }
    if (!g_classRegistered) {
        WNDCLASSW wc = {};
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = windowProc;
        wc.hInstance = g_instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kWindowClass;
        if (RegisterClassW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS) g_classRegistered = true;
    }
    Editor* ed = new (std::nothrow) Editor();
    if (!ed) return;
    ed->plugin = p;
    p->editor = ed;
    CreateWindowExW(0, kWindowClass, L"TrueSymph", WS_CHILD | WS_VISIBLE, 0, 0, kEditorWidth, kEditorHeight,
                    static_cast<HWND>(parent), nullptr, g_instance, ed);
}

void editorClose(Core* p)
{
    Editor* ed = static_cast<Editor*>(p->editor);
    if (!ed) return;
    if (ed->hwnd) DestroyWindow(ed->hwnd);
    delete ed;
    p->editor = nullptr;
    if (--g_gdiplusUsers == 0) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

// Ritar fönstret (framsida eller baksida) till en PNG-fil utan att öppna något
// fönster. Används av RenderGui (bilder av gränssnittet i bygget).
bool editorRenderPng(Core* core, bool backSide, const wchar_t* path)
{
    GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &input, nullptr) != Ok) return false;
    bool ok = false;
    {
        Editor ed;
        ed.plugin = core;
        ed.showBack = backSide;
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = kEditorWidth;
        bi.bmiHeader.biHeight = -kEditorHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        HDC screen = GetDC(nullptr);
        HDC mem = CreateCompatibleDC(screen);
        HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (mem && dib) {
            HGDIOBJ old = SelectObject(mem, dib);
            paint(&ed, mem, kEditorWidth, kEditorHeight);
            GdiFlush();
            SelectObject(mem, old);
            Bitmap* bmp = Bitmap::FromHBITMAP(dib, nullptr);
            if (bmp) {
                UINT num = 0, size = 0;
                GetImageEncodersSize(&num, &size);
                std::vector<unsigned char> buf(size);
                ImageCodecInfo* codecs = reinterpret_cast<ImageCodecInfo*>(buf.data());
                GetImageEncoders(num, size, codecs);
                for (UINT i = 0; i < num; ++i) {
                    if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
                        ok = bmp->Save(path, &codecs[i].Clsid, nullptr) == Ok;
                        break;
                    }
                }
                delete bmp;
            }
        }
        if (dib) DeleteObject(dib);
        if (mem) DeleteDC(mem);
        ReleaseDC(nullptr, screen);
    }
    GdiplusShutdown(token);
    return ok;
}

} // namespace truesymph
