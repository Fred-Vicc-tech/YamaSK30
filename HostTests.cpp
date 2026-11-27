// HostTests.cpp - låtsas vara en VST2-värd (som Live) och testar pluginets
// VST2-del: att det anmäler sig som instrument, tar emot MIDI, låter,
// sparar/läser tillstånd, har fabriksljud och tål felaktiga data.
#include "vst2.h"
#include "YamaSK30DSP.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback host);

static int failures = 0;
static void check(bool ok, const char* name)
{
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}

static VstIntPtr hostCallback(AEffect*, VstInt32 opcode, VstInt32, VstIntPtr, void*, float)
{
    if (opcode == audioMasterVersion) return 2400;
    return 0;
}

static void sendMidi(AEffect* e, int delta, uint8_t s, uint8_t d1, uint8_t d2)
{
    VstMidiEvent me;
    std::memset(&me, 0, sizeof me);
    me.type = kVstMidiType;
    me.byteSize = sizeof me;
    me.deltaFrames = delta;
    me.midiData[0] = (char)s; me.midiData[1] = (char)d1; me.midiData[2] = (char)d2;
    VstEvents evs;
    std::memset(&evs, 0, sizeof evs);
    evs.numEvents = 1;
    evs.events[0] = reinterpret_cast<VstEvent*>(&me);
    e->dispatcher(e, effProcessEvents, 0, 0, &evs, 0.0f);
}

// Returnerar topp, eller -1 om vänster och höger skiljer sig / ogiltiga tal
static double renderPeak(AEffect* e, int blocks, int blockSize)
{
    std::vector<float> l(blockSize), r(blockSize);
    float* outs[2] = { l.data(), r.data() };
    double pk = 0.0;
    for (int b = 0; b < blocks; ++b) {
        e->processReplacing(e, nullptr, outs, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            if (l[i] != r[i] || !std::isfinite(l[i])) return -1.0;
            pk = std::fmax(pk, std::fabs(l[i]));
        }
    }
    return pk;
}

int main()
{
    using namespace yamask30;
    AEffect* e = VSTPluginMain(hostCallback);
    check(e != nullptr && e->magic == kEffectMagic, "plugin skapas");
    if (!e) return 1;
    check((e->flags & effFlagsIsSynth) != 0, "anmaler sig som instrument");
    check(e->numInputs == 0 && e->numOutputs == 2, "0 in, 2 ut");
    check(e->numParams == kNumParams, "antal parametrar");
    check(e->dispatcher(e, effGetPlugCategory, 0, 0, nullptr, 0) == kPlugCategSynth, "kategori synth");
    check(e->dispatcher(e, effCanDo, 0, 0, (void*)"receiveVstMidiEvent", 0) == 1, "tar emot MIDI");

    char name[64] = { 0 };
    e->dispatcher(e, effGetEffectName, 0, 0, name, 0);
    check(std::strcmp(name, "YamaSK30") == 0, "namn YamaSK30");
    e->dispatcher(e, effGetVendorString, 0, 0, name, 0);
    check(std::strcmp(name, "2F Productions") == 0, "tillverkare 2F Productions");

    e->dispatcher(e, effOpen, 0, 0, nullptr, 0);
    e->dispatcher(e, effSetSampleRate, 0, 0, nullptr, 48000.0f);
    e->dispatcher(e, effSetBlockSize, 0, 512, nullptr, 0);
    e->dispatcher(e, effMainsChanged, 0, 1, nullptr, 0);

    check(renderPeak(e, 20, 512) == 0.0, "tyst utan ton");

    sendMidi(e, 100, 0x90, 57, 100);
    sendMidi(e, 100, 0x90, 60, 100);
    sendMidi(e, 100, 0x90, 64, 100);
    const double pk = renderPeak(e, 100, 512);
    check(pk > 0.01 && pk < 0.9, "ackord later, rimlig niva, vanster = hoger");

    sendMidi(e, 0, 0x80, 57, 0);
    sendMidi(e, 0, 0x80, 60, 0);
    sendMidi(e, 0, 0x80, 64, 0);
    renderPeak(e, 400, 512);      // ca 4 s: release (0,9 s) + ensemble
    check(renderPeak(e, 20, 512) < 1e-4, "tyst efter release");

    // Parametertext
    char txt[16] = { 0 };
    e->dispatcher(e, effGetParamName, kParamEnsemble, 0, txt, 0);
    check(std::strlen(txt) > 0 && std::strlen(txt) <= 7, "parameternamn finns (max 7 tecken)");
    for (int i = 0; i < kNumParams; ++i) {
        char t[16] = { 0 };
        e->dispatcher(e, effGetParamDisplay, i, 0, t, 0);
        if (std::strlen(t) == 0) { check(false, "parametervarde visas"); break; }
    }

    // Fabriksljud
    check(e->numPrograms >= 4, "minst fyra fabriksljud");
    char pn[32] = { 0 };
    check(e->dispatcher(e, effGetProgramNameIndexed, 0, 0, pn, 0) == 1 && std::strlen(pn) > 0, "fabriksljud har namn");
    e->dispatcher(e, effSetProgram, 0, 2, nullptr, 0);
    check(e->dispatcher(e, effGetProgram, 0, 0, nullptr, 0) == 2, "byte av fabriksljud");
    check(e->getParameter(e, kParamString3) == 1.0f, "Full Strings har STRING 3 pa");

    // Spara och läsa tillstånd
    e->setParameter(e, kParamTune, 0.25f);
    e->setParameter(e, kParamEnsemble, 0.0f);
    void* data = nullptr;
    const VstIntPtr size = e->dispatcher(e, effGetChunk, 0, 0, &data, 0);
    check(size > 0 && data != nullptr, "tillstand sparas");
    std::vector<unsigned char> saved((unsigned char*)data, (unsigned char*)data + size);
    e->setParameter(e, kParamTune, 0.9f);
    e->setParameter(e, kParamEnsemble, 1.0f);
    check(e->dispatcher(e, effSetChunk, 0, (VstIntPtr)saved.size(), saved.data(), 0) == 1, "tillstand lases");
    check(e->getParameter(e, kParamTune) == 0.25f && e->getParameter(e, kParamEnsemble) == 0.0f, "varden aterstalls");

    // Felaktiga data
    unsigned char junk[64]; std::memset(junk, 0x7F, sizeof junk);
    check(e->dispatcher(e, effSetChunk, 0, sizeof junk, junk, 0) == 0, "skrapdata avvisas");
    check(e->dispatcher(e, effSetChunk, 0, 8, saved.data(), 0) == 0, "for kort chunk avvisas");
    e->setParameter(e, kParamVolume, NAN);
    e->setParameter(e, kParamVolume, 7.0f);
    check(e->getParameter(e, kParamVolume) >= 0.0f && e->getParameter(e, kParamVolume) <= 1.0f, "ogiltiga parametervarden ignoreras");
    check(e->getParameter(e, 999) == 0.0f, "okant parameterindex");

    // Många MIDI-händelser i ett block
    for (int i = 0; i < 2000; ++i) sendMidi(e, i % 512, (i & 1) ? 0x80 : 0x90, 40 + (i % 40), 100);
    check(renderPeak(e, 4, 512) >= 0.0, "manga MIDI-handelser: inget krasch");

    e->dispatcher(e, effClose, 0, 0, nullptr, 0);

    std::printf("\n%s (%d fel)\n", failures ? "TESTER MISSLYCKADES" : "ALLA TESTER OK", failures);
    return failures ? 1 : 0;
}
