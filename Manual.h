// Manual.h - texten på TrueSymphs baksida (klicka på loggan).
// Två språk: engelska (ENG) och svenska (SWE). Varje rad är ett stycke:
//   kManHead = rubrik, kManBody = brödtext, kManNote = källa/notis (mindre).
#pragma once

enum { kManHead = 0, kManBody = 1, kManNote = 2 };

struct ManualPara
{
    int kind;
    const wchar_t* text;
};

static const ManualPara kManualEng[] = {
    { kManHead, L"WHAT TRUESYMPH IS" },
    { kManBody, L"TrueSymph recreates the Yamaha SK30 Symphonic Ensemble (1979-80): strings, poly-synth, organ, solo synthesizer "
                L"and the ensemble/tremolo effect. It is built from the SK30 service manual: the divide-down tone generators, the "
                L"stair-step sawtooth, the RC string filters, the preset resistor networks and the three-BBD ensemble use the "
                L"original circuit values." },
    { kManBody, L"Faders and knobs: drag up/down, hold Shift for fine adjustment, use the mouse wheel for small steps and "
                L"double-click to return to the default value. The display at the top shows the control under the mouse. "
                L"Every setting can be automated and is saved with your Live set." },

    { kManHead, L"OUTPUT AND PITCH" },
    { kManBody, L"MASTER VOLUME sets the total level. SOLO, ORGAN, STRING and POLY-SYNTH set the level of each section. "
                L"SOLO starts at zero, so the solo synthesizer is silent until you turn it up." },
    { kManBody, L"BOOST (under MASTER VOLUME, on by default) is the SK30's output stage: the service manual sets the main output to "
                L"3 times the mix level (+9.5 dB). It comes after the effects, so it changes only the level, and puts TrueSymph on a par "
                L"with other synths in Live. The limiter keeps big chords under -0.3 dBFS. Turn BOOST off for the quieter level of "
                L"earlier versions." },
    { kManBody, L"TUNE moves everything (A4 415-466 Hz). POLY-SYNTH DETUNE detunes tone generator II (-20 to +27 cents) and is "
                L"heard in MODE II. SOLO TUNE moves only the solo synthesizer (-500 to +700 cents)." },

    { kManHead, L"STRINGS" },
    { kManBody, L"STRING 1, 2 and 3 are the 16', 8' and 4' string voices, each with the SK30's own RC filter. They can be "
                L"combined; CANCEL turns them all off. The strings share keys, envelope and vibrato with the poly-synth: "
                L"SLOW ATTACK (80 ms instead of 3 ms) and SUSTAIN (release 0.03-1.6 s) in the POLY-SYNTH section shape the "
                L"strings too. BRILLIANCE (+/-12 dB) brightens or darkens strings and poly-synth." },
    { kManBody, L"MODE I uses one tone generator. MODE II adds generator II, detuned with POLY-SYNTH DETUNE, for a wider sound." },

    { kManHead, L"POLY-SYNTH" },
    { kManBody, L"POLY-SYNTH 1, 2 and 3 are the SK30's fixed sounds, read from the resistor networks on the CPC board. MANUAL "
                L"uses the faders: FEET selects the waveform (4' saw, 8' square, BP8' bandpass, 8' saw, 16' square, 16' saw), "
                L"CUTOFF FREQ, RESONANCE and EG DEPTH control the filter, and ATTACK, DECAY, SUSTAIN and RELEASE the filter "
                L"envelope. As on the SK30, all keys share one filter and the envelope restarts with every new key." },

    { kManHead, L"VIBRATO" },
    { kManBody, L"VIBRATO moves the pitch of strings and poly-synth (SPEED 5-7 Hz, DEPTH up to 30 cents). DELAY (0-3.2 s) "
                L"delays the vibrato as on the SK30: first about a fifth of the time without vibrato, then a gradual fade-in." },

    { kManHead, L"ORGAN" },
    { kManBody, L"ORGAN 1 = 16' 8' 5 1/3', ORGAN 2 = 16' 8' 4' 2' 1', ORGAN 3 = all nine footages. MANUAL uses the nine tone "
                L"levers. The organ has its own tone generator, so vibrato and detune do not affect it." },
    { kManBody, L"DECAY makes notes fade while held (like a piano); SUSTAIN lets them ring after release. Both use the S-L "
                L"fader (0.03-1.6 s). PERCUSSIVE 2nd, 3rd and 5th add a short attack on 4', 2 2/3' and 1 3/5'; DECAY TIME sets "
                L"how fast it fades. ORGAN BRILLIANCE is +/-7 dB." },

    { kManHead, L"SOLO SYNTHESIZER" },
    { kManBody, L"A monophonic analog-style synth: LFO (0.1-100 Hz), PORTAMENTO (0-3 s for three octaves), GLIDE (the note "
                L"slides in from above or below, center = off), VCO with FEET 4'-64', WAVE from saw to square and PW 50-85 % "
                L"(controlled by LFO, MANUAL or EG), VCF with CUTOFF, RESONANCE, MODULATION (LFO) and EG DEPTH, VCA (HOLD = "
                L"always on, GATE, EG) and an ADSR envelope." },
    { kManBody, L"MODE NORMAL plays the highest key, BASS the lowest key. MANUAL BASS turns the lowest keys (up to BASS UP TO) "
                L"into a separate bass keyboard for the solo synth only." },
    { kManBody, L"TOUCH: on the SK30 a light sensor measures how hard a key is pressed. In TrueSymph TOUCH follows aftertouch, "
                L"or the modulation wheel if your keyboard has no aftertouch. VIBRATO adds up to +/-120 cents, BRILLIANCE opens "
                L"the filter up to 5 octaves, MODULATION adds LFO movement to the filter." },

    { kManHead, L"KEYBOARD SPLIT" },
    { kManBody, L"POLY-SYNTH | ORGAN puts strings and poly-synth below the split point and the organ above; ORGAN | POLY-SYNTH "
                L"does the opposite. Each side has 7 voices. SPLIT AT sets the split point: by default between F#3 and G3, at the SK30's "
                L"marking. The SK30's keys C1-C6 are MIDI notes 36-96 (Yamaha and Live use the same note names, C3 = MIDI 60); MANUAL "
                L"BASS reaches up to F#2 by default, as the bracket on the SK30 panel shows." },

    { kManHead, L"TREMOLO / ENSEMBLE" },
    { kManBody, L"ENSEMBLE is the SK30's three-BBD chorus (0.64 Hz and 6.4 Hz stepped LFOs, 120 degrees apart) that makes the "
                L"strings wide and alive. TREMOLO modulates level and pitch at 0.64 Hz or 6.4 Hz (TREMOLO SPEED), like a rotating "
                L"speaker. Organ and poly-synth/strings each have their own buttons; they share one effect board, and ENSEMBLE "
                L"has priority. The solo synthesizer never goes through the effect." },

    { kManHead, L"EFFECTS" },
    { kManBody, L"DELAY is modelled on the Korg SDD-3000 (DIGI) and the Roland RE-201 Space Echo (TAPE); REVERB on the Lexicon MPX500 "
                L"(PLATE, HALL, CHAMBER, ROOM) and the AMS RMX16 (AMBIENCE, NONLIN). The effects work like an effect send: the dry "
                L"sound passes at full level and LEVEL sets how much effect is added. DELAY follows the SDD-3000 service manual: TIME "
                L"(1-1000 ms), FEEDBACK, MOD SPEED (0.1-15 Hz) and INTENSITY (up to 2:1; the delay time sways, left and right in "
                L"opposite directions, which makes the echo wide), HIGH CUT (8k/4k/2k) and LOW CUT (125/250/500 Hz) like the "
                L"SDD-3000's filters, and LEVEL (middle = -10 dB, top = as loud as the dry sound). REVERB: TYPE after the MPX500 banks "
                L"(PLATE bright and dense, HALL large and clean, CHAMBER even, ROOM small), DECAY (0.3-8 s) and LEVEL (middle = -2 dB, "
                L"top = +8 dB for a very wet sound; the reverb goes after the delay). LEVEL at the bottom = off, so the sound is mono "
                L"and dry like the SK30 itself. The solo synthesizer also goes through the effects." },
    { kManBody, L"The switch at the start of DELAY selects DIGI (clean SDD-3000 echo) or TAPE: the tape echo sways slightly (wow and "
                L"flutter), saturates when pushed, and every repeat is darker and thinner than the one before. "
                L"REVERB TYPE AMBIENCE is a short, dense sense of space with almost no tail (DECAY sets its length, about a fifth of the "
                L"value shown). NONLIN is the gated 80s reverb: an even burst that stops abruptly; DECAY sets the gate time (one tenth "
                L"of the value shown, 0.06-0.8 s)." },

    { kManHead, L"PRESETS" },
    { kManBody, L"Click the preset name to pick a sound from a list (your own presets are under My presets). The arrows step through the factory presets and then your own presets. SAVE stores the current sound as a "
                L"file in Documents\\TrueSymph Presets; LOAD opens any preset file. Factory presets also appear in Live's preset list." },

    { kManHead, L"REC: PRESETS" },
    { kManBody, L"The presets named Rec: are complete sounds with delay and reverb: Intro Lead (solo with portamento over strings), "
                L"Pluck + Bass and Wah Bass (solo synth as manual bass; the LFO sweeps the resonant filter at 3.2 Hz), High Lead, "
                L"Poly 2 Sweeps, Ensemble Pad, Swell Chords and Glide Solo. The leads use TOUCH VIBRATO: add a little aftertouch or "
                L"mod wheel (about 20 %) for vibrato." },

    { kManHead, L"ABOUT THE SOUND" },
    { kManBody, L"The SK30 is mono, so TrueSymph is mono too (left = right) until you turn on DELAY or REVERB." },
    { kManBody, L"MIDI: sustain pedal (CC64) is the SK30's FOOT SW SUSTAIN. An expression pedal (CC11) is FOOT CONT MIXED VOLUME. "
                L"A footswitch on CC65 is FOOT SW PORTAMENTO: pressed = portamento with PORTAMENTO TIME, released = no portamento "
                L"(without a pedal the PORTAMENTO fader works as usual). Mod wheel (CC1) and aftertouch work as TOUCH." },
    { kManNote, L"Yamaha and SK30 are trademarks of Yamaha Corporation. TrueSymph is not affiliated with Yamaha." },
};

static const ManualPara kManualSwe[] = {
    { kManHead, L"VAD TRUESYMPH ÄR" },
    { kManBody, L"TrueSymph återskapar Yamaha SK30 Symphonic Ensemble (1979-80): stråkar, poly-synth, orgel, solosynt och "
                L"ensemble/tremolo. Den är byggd efter SK30:s servicemanual: tongeneratorerna, trappstegs-sågtanden, stråkfiltren, "
                L"de fasta ljudens motståndsnät och ensemblen med tre delay-chip har originalets kretsvärden." },
    { kManBody, L"Reglage och rattar: dra upp/ner, håll Shift för finjustering, använd mushjulet för små steg och dubbelklicka "
                L"för standardvärdet. Displayen överst visar reglaget under musen. Allt kan automatiseras och sparas i Live-projektet." },

    { kManHead, L"OUTPUT OCH PITCH" },
    { kManBody, L"MASTER VOLUME styr den totala nivån. SOLO, ORGAN, STRING och POLY-SYNTH styr varje sektion. SOLO står på noll "
                L"från början, så solosynten är tyst tills du höjer den." },
    { kManBody, L"BOOST (under MASTER VOLUME, på från början) är SK30:s utgångssteg: enligt servicemanualen justeras huvudutgången "
                L"till 3 gånger mixens nivå (+9,5 dB). Den sitter efter effekterna, så den ändrar bara nivån, och gör att TrueSymph "
                L"ligger i nivå med andra syntar i Live. Limitern håller stora ackord under -0,3 dBFS. Stäng av BOOST för den lägre "
                L"nivån från tidigare versioner." },
    { kManBody, L"TUNE flyttar allt (A4 415-466 Hz). POLY-SYNTH DETUNE snedstämmer tongenerator II (-20 till +27 cent) och hörs "
                L"i MODE II. SOLO TUNE flyttar bara solosynten (-500 till +700 cent)." },

    { kManHead, L"STRÅKAR" },
    { kManBody, L"STRING 1, 2 och 3 är stråkarna i 16', 8' och 4', var och en med SK30:s eget RC-filter. De kan kombineras; "
                L"CANCEL stänger av alla. Stråkarna delar tangenter, envelope och vibrato med poly-synthen: SLOW ATTACK (80 ms i "
                L"stället för 3 ms) och SUSTAIN (utklingning 0,03-1,6 s) i POLY-SYNTH-delen formar även stråkarna. BRILLIANCE "
                L"(+/-12 dB) gör stråkar och poly-synth ljusare eller mörkare." },
    { kManBody, L"MODE I använder en tongenerator. MODE II lägger till generator II, snedstämd med POLY-SYNTH DETUNE, för ett bredare ljud." },

    { kManHead, L"POLY-SYNTH" },
    { kManBody, L"POLY-SYNTH 1, 2 och 3 är SK30:s fasta ljud, avlästa ur motståndsnäten på CPC-kortet. MANUAL använder reglagen: "
                L"FEET väljer vågform (4' sågtand, 8' fyrkant, BP8' bandpass, 8' sågtand, 16' fyrkant, 16' sågtand), CUTOFF FREQ, "
                L"RESONANCE och EG DEPTH styr filtret, och ATTACK, DECAY, SUSTAIN och RELEASE filtrets envelope. Som på SK30 delar "
                L"alla tangenter ett filter, och envelopen startar om vid varje ny tangent." },

    { kManHead, L"VIBRATO" },
    { kManBody, L"VIBRATO svajar tonhöjden för stråkar och poly-synth (SPEED 5-7 Hz, DEPTH upp till 30 cent). DELAY (0-3,2 s) "
                L"fördröjer vibratot som på SK30: först ungefär en femtedel av tiden utan vibrato, sedan en mjuk inmatning." },

    { kManHead, L"ORGEL" },
    { kManBody, L"ORGAN 1 = 16' 8' 5 1/3', ORGAN 2 = 16' 8' 4' 2' 1', ORGAN 3 = alla nio fotlägen. MANUAL använder de nio tone "
                L"levers. Orgeln har en egen tongenerator, så vibrato och detune påverkar den inte." },
    { kManBody, L"DECAY gör att tonen klingar av medan den hålls (som ett piano); SUSTAIN låter den klinga efter att tangenten "
                L"släppts. Båda använder S-L-reglaget (0,03-1,6 s). PERCUSSIVE 2nd, 3rd och 5th ger ett kort anslag på 4', 2 2/3' "
                L"och 1 3/5'; DECAY TIME styr hur snabbt det klingar av. ORGAN BRILLIANCE är +/-7 dB." },

    { kManHead, L"SOLOSYNT" },
    { kManBody, L"En monofonisk synt i analog stil: LFO (0,1-100 Hz), PORTAMENTO (0-3 s för tre oktaver), GLIDE (tonen glider in "
                L"ovanifrån eller underifrån, mitten = av), VCO med FEET 4'-64', WAVE från sågtand till fyrkant och PW 50-85 % "
                L"(styrd av LFO, MANUAL eller EG), VCF med CUTOFF, RESONANCE, MODULATION (LFO) och EG DEPTH, VCA (HOLD = alltid "
                L"på, GATE, EG) och en ADSR-envelope." },
    { kManBody, L"MODE NORMAL spelar den högsta tangenten, BASS den lägsta. MANUAL BASS gör de lägsta tangenterna (upp till "
                L"BASS UP TO) till en egen basmanual som bara spelar solosynten." },
    { kManBody, L"TOUCH: på SK30 mäter en ljussensor hur hårt tangenten trycks ner. I TrueSymph följer TOUCH aftertouch, eller "
                L"modulationshjulet om klaviaturen saknar aftertouch. VIBRATO ger upp till +/-120 cent, BRILLIANCE öppnar filtret "
                L"upp till 5 oktaver, MODULATION lägger LFO-svaj på filtret." },

    { kManHead, L"KEYBOARD SPLIT" },
    { kManBody, L"POLY-SYNTH | ORGAN lägger stråkar och poly-synth under delningspunkten och orgeln över; ORGAN | POLY-SYNTH gör "
                L"tvärtom. Varje halva har 7 röster. SPLIT AT väljer delningspunkten: från början mellan F#3 och G3, vid SK30:s "
                L"markering. SK30:s tangenter C1-C6 är MIDI-toner 36-96 (Yamaha och Live använder samma tonnamn, C3 = MIDI 60); MANUAL "
                L"BASS når från början upp till F#2, som klammern på SK30-panelen visar." },

    { kManHead, L"TREMOLO / ENSEMBLE" },
    { kManBody, L"ENSEMBLE är SK30:s chorus med tre delay-chip (trappstegs-LFO:er på 0,64 och 6,4 Hz, 120 grader isär) som gör "
                L"stråkarna breda och levande. TREMOLO svajar nivå och tonhöjd med 0,64 eller 6,4 Hz (TREMOLO SPEED), som en "
                L"roterande högtalare. Orgeln och poly-synth/stråkar har egna knappar; de delar ett effektkort och ENSEMBLE har "
                L"företräde. Solosynten går aldrig genom effekten." },

    { kManHead, L"EFFEKTER" },
    { kManBody, L"DELAY är modellerad efter Korg SDD-3000 (DIGI) och Roland RE-201 Space Echo (TAPE); REVERB efter Lexicon MPX500 "
                L"(PLATE, HALL, CHAMBER, ROOM) och AMS RMX16 (AMBIENCE, NONLIN). Effekterna fungerar som en effect send: det "
                L"torra ljudet går igenom i full nivå och LEVEL bestämmer hur mycket effekt som läggs till. DELAY följer SDD-3000:s "
                L"servicemanual: TIME (1-1000 ms), FEEDBACK, MOD SPEED (0,1-15 Hz) och INTENSITY (upp till 2:1; delaytiden svajar åt "
                L"olika håll i vänster och höger, vilket gör ekot brett), HIGH CUT (8k/4k/2k) och LOW CUT (125/250/500 Hz) som "
                L"SDD-3000:s filter, och LEVEL (mitten = -10 dB, toppen = lika starkt som torrt). REVERB: TYPE efter MPX500:s banker "
                L"(PLATE ljus och tät, HALL stor och ren, CHAMBER jämn, ROOM liten), DECAY (0,3-8 s) och LEVEL (mitten = -2 dB, "
                L"toppen = +8 dB för mycket klang; reverbet sitter efter delayen). LEVEL i botten = av, då är ljudet torrt och mono "
                L"som SK30 själv. Även solosynten går genom effekterna." },
    { kManBody, L"Omkopplaren först i DELAY väljer DIGI (rent SDD-3000-eko) eller TAPE: bandekot svajar lite (wow och flutter), "
                L"mättas när det drivs och varje upprepning blir mörkare och tunnare än den förra. "
                L"REVERB TYPE AMBIENCE är en kort, tät rymdkänsla nästan utan svans (DECAY styr längden, ungefär en femtedel av "
                L"visat värde). NONLIN är 80-talets grindade reverb: en jämn skur som tar tvärt slut; DECAY styr grindtiden (en "
                L"tiondel av visat värde, 0,06-0,8 s)." },

    { kManHead, L"PRESETS" },
    { kManBody, L"Klicka på presetnamnet för att välja ljud i en lista (egna presets finns under My presets). Pilarna stegar genom fabriksljuden och sedan dina egna presets. SAVE sparar ljudet som en fil i "
                L"Dokument\\TrueSymph Presets; LOAD öppnar valfri presetfil. Fabriksljuden finns också i Lives presetlista." },

    { kManHead, L"REC:-LJUDEN" },
    { kManBody, L"Ljuden som heter Rec: är färdiga ljud med delay och reverb: Intro Lead (solo med portamento över stråkar), "
                L"Pluck + Bass och Wah Bass (solosynten som manual bass; LFO:n sveper det resonanta filtret med 3,2 Hz), High Lead, "
                L"Poly 2 Sweeps, Ensemble Pad, Swell Chords och Glide Solo. Solostämmorna använder TOUCH VIBRATO: lägg på lite "
                L"aftertouch eller modulationshjul (ca 20 %) för vibrato." },

    { kManHead, L"OM LJUDET" },
    { kManBody, L"SK30 är mono, så TrueSymph är också mono (vänster = höger) tills du sätter på DELAY eller REVERB." },
    { kManBody, L"MIDI: sustainpedal (CC64) är SK30:s FOOT SW SUSTAIN. En expressionpedal (CC11) är FOOT CONT MIXED VOLUME. "
                L"En fotomkopplare på CC65 är FOOT SW PORTAMENTO: nedtryckt = portamento med PORTAMENTO TIME, släppt = ingen "
                L"portamento (utan pedal fungerar PORTAMENTO-reglaget som vanligt). Modhjul (CC1) och aftertouch fungerar som TOUCH." },
    { kManNote, L"Yamaha och SK30 är varumärken som tillhör Yamaha Corporation. TrueSymph har ingen koppling till Yamaha." },
};

static const int kManualEngCount = (int)(sizeof kManualEng / sizeof kManualEng[0]);
static const int kManualSweCount = (int)(sizeof kManualSwe / sizeof kManualSwe[0]);
