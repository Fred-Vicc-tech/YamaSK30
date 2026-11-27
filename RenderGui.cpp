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
    busy.setParam(truesymph::kParamReverbDuck, 0.5f);
    busy.setParam(truesymph::kParamReverbHighCut, (float)truesymph::stepNorm(1, truesymph::kNumHighCut));
    busy.setParam(truesymph::kParamDelayMode, (float)truesymph::stepNorm(truesymph::kDlyTape3, truesymph::kNumDelayModes));
    busy.setParam(truesymph::kParamReverbType, (float)truesymph::stepNorm(truesymph::kRevAmbience, truesymph::kNumReverbTypes));
    const bool c = truesymph::editorRenderPng(&busy, false, L"TrueSymph_front2.png");
    std::printf("front %s, back %s, front2 %s\n", a ? "ok" : "FAILED", b ? "ok" : "FAILED", c ? "ok" : "FAILED");
    return (a && b && c) ? 0 : 1;
}
