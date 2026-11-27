// RenderGui.cpp - ritar TrueSymphs fönster till PNG-filer med exakt samma kod
// som pluginet. Körs i bygget (Windows); bilderna laddas upp separat.
#include "TrueSymphCore.h"
#include <cstdio>

int main()
{
    truesymph::Core core;                                   // standardljudet (Dream Strings)
    const bool a = truesymph::editorRenderPng(&core, false, L"TrueSymph_front.png");
    const bool b = truesymph::editorRenderPng(&core, true, L"TrueSymph_back.png");
    // Ett ljud där fler sektioner är aktiva (solo, poly MANUAL, orgel MANUAL, split)
    truesymph::Core busy;
    for (int i = 0; i < truesymph::kNumFactoryPresets; ++i)
        if (std::strcmp(truesymph::factoryPresetName(i), "Split Poly|Organ") == 0) busy.loadFactoryPreset(i, false);
    busy.setParam(truesymph::kParamSoloVol, 0.8f);
    busy.setParam(truesymph::kParamPolySel, 1.0f);
    busy.setParam(truesymph::kParamOrgSel, 1.0f);
    busy.setParam(truesymph::kParamTouchVib, 1.0f);
    busy.setParam(truesymph::kParamDelayLevel, 0.6f);
    busy.setParam(truesymph::kParamReverbLevel, 0.7f);
    const bool c = truesymph::editorRenderPng(&busy, false, L"TrueSymph_front2.png");
    double fullMs = 0.0, meterMs = 0.0;
    truesymph::editorBenchPaint(&busy, 40, &fullMs, &meterMs);
    std::printf("paint: full panel %.2f ms, meter only %.2f ms per frame (25 frames/s)\n", fullMs, meterMs);
    std::printf("front %s, back %s, front2 %s\n", a ? "ok" : "FAILED", b ? "ok" : "FAILED", c ? "ok" : "FAILED");
    return (a && b && c) ? 0 : 1;
}
