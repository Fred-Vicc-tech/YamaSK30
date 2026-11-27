# Ändringslogg

## 0.8.0 (2026-11-27)
- Knaster borta: den gamla limitern hann inte alltid sänka helt innan en topp kom och klippte då
  resten hårt (hördes som knaster när man vred upp nivåerna). Nya limitern håller kravet över hela
  framförhållningen och sänker med en mjuk ramp, så den klipper aldrig. Framförhållningen (2 ms)
  rapporteras nu till Live, som kompenserar för den.
- Starkare: hela utgången +4 dB, så TrueSymph ligger i nivå med andra syntar i Live (limitern tar
  topparna).
- Fönstret ritar bara om panelen när något ändras; annars bara nivåmätaren. Mycket mindre CPU för
  grafiken (viktigt på äldre datorer, där tung grafik kan ge avbrott i ljudet).

## 0.7.0 (2026-11-27)
- EFFECTS högst upp till höger (SOLO-panelen smalare, fönstret 1280 px): DELAY efter Korg SDD-3000:s
  servicemanual (TIME 1-1000 ms, FEEDBACK, MOD SPEED 0,1-15 Hz, INTENSITY upp till 2:1, HIGH CUT 8k/4k/2k,
  LOW CUT 125/250/500 Hz, bandbredd 17 kHz, LEVEL) och REVERB med typerna från Lexicon MPX500:s banker
  (PLATE, HALL, CHAMBER, ROOM; DECAY, LEVEL). Stereo när effekterna är på; LEVEL i botten = av.
- Wah-basen från inspelningen (0:25): solosynten med LFO 3,2 Hz på ett resonant filter. Nytt ljud Rec: Wah Bass,
  och Rec: Pluck + Bass har fått wah-basen. Alla Rec:-ljud har delay och reverb som på inspelningen.
- Limitern är stereo (samma sänkning i båda kanalerna).

## 0.6.1 (2026-11-27)
- Solosynten var ca 6 dB för stark jämfört med sektionerna (slog i taket på max). Nu lika stark
  som stråkarna vid samma inställning; solo-ljuden höjda så att de låter som förut.
- Riktig säkerhetslimiter sist (1,5 ms framförhållning, tak -0,3 dBFS) i stället för den enkla mjukklippningen.
- Nivåmätare i fönstret (dBFS med topphållning, samma skala som Live) och en LIMIT-lampa.

## 0.6.0 (2026-11-27)
- Klick på presetnamnet öppnar en lista med alla fabriksljud och egna presets (My presets).
- Sju nya ljud (Rec:) som återskapar ljuden i Fredriks SK30-inspelning, uppmätta ur ljudfilen.
- Solosyntens PORTAMENTO är nu en analog RC-glidning (samma tid oavsett intervall), som i inspelningen.

## 0.5.0 – etapp 5 (2026-11-27)
- Eget fönster (GDI+) i SK30-stil: skjutreglage, SK30-knappar med lysdiod (mått och färger
  från Fredriks bild), guldfärgade grupplinjer, display för reglaget under musen.
- PRESET med pilar, SAVE och LOAD (Dokument\TrueSymph Presets, .tsy).
- Baksida med credits (2F Productions) och manual på engelska och svenska.
- Koden omstrukturerad som TruePan: TrueSymphCore.h (kärna), Presets.h, TrueSymphEditor.cpp, Manual.h.
- Bygget ritar skärmbilder av fönstret och kontrollerar Windows 7-kompatibiliteten.

## 0.4.0 – etapp 4 (2026-11-27)
- Solosynt (CPB 7/9): VCO med FEET 4'–64', WAVE, PW (LFO/MANUAL/EG), VCF med tangentföljning,
  VCA HOLD/GATE/EG, ADSR, LFO 0,1–100 Hz, PORTAMENTO, GLIDE, SOLO TUNE, TOUCH (aftertouch/modhjul).
- MODE NORMAL / BASS / MANUAL BASS, KEYBOARD SPLIT (7 + 7 röster) med inställbar delningspunkt.
- 28 fabriksljud. Nya parametrar 49–73.

## 0.3.0 – etapp 3 (2026-11-27)
- Orgel enligt OG1/CPB: ORGAN CANCEL/1/2/3/MANUAL, nio tone levers med OG1:s tonfilter,
  egen 2 MHz-generator, DECAY/SUSTAIN, ORGAN BRILLIANCE, PERCUSSIVE 2nd/3rd/5th, ORGAN TREMOLO/ENSEMBLE.
- MASTER VOLUME (ny parameter 48). STRING VOLUME gäller nu bara stråkarna.
- Ensemblens extra lågpass (som inte fanns i schemat) borttaget: lite mer diskant.
- Snabbare ljudmotor (ca hälften så mycket processorkraft).
- 22 fabriksljud.

## 0.2.0 – etapp 2 (2026-11-27)
- Poly-synth enligt CPC-kortet: POLY-SYNTH CANCEL/1/2/3/MANUAL, FEET (6 lägen inkl. BP8'),
  VCF med resonans och EG DEPTH, ADSR. POLY-SYNTH 1/2/3 har värden ur motståndsnäten.
- Tremolo: POLY-SYNTH TREMOLO och TREMOLO SPEED (0,64/6,4 Hz), volym + delay-svaj (TE-kortet).
- 14 fabriksljud. Nya parametrar 15-26 (gamla index oförändrade).

## 0.1.1 – nytt namn (2026-11-27)
- Pluginet heter nu TrueSymph (tidigare YamaSK30). Samma interna ID: sparade Live-projekt fungerar.
- Etapp 1 testad i Live 9.6 på Windows 7: fungerar.

## 0.1.0 – etapp 1 (2026-11-27)
- Ny ljudmotor efter SK30:s servicemanual: två divide-down-generatorer, trappstegs-sågtand,
  RC-stråkfilter (STRING 1/2/3), keyer (SLOW ATTACK/SUSTAIN), MODE I/II, DETUNE, vibrato med
  fördröjning, BRILLIANCE och ensemble med tre BBD-linjer (0,64 + 6,4 Hz).
- VST2-instrument utan eget fönster (Lives reglage), 6 fabriksljud, sparar tillstånd i projektet.
- Automatiska tester för ljudmotorn och VST2-delen.
