# Ändringslogg

## 0.9.6 (2026-11-27)
- Rattarna ser ut som på SK30: benvit rund knopp med svart ram och en tunn svart visare från mitten
  till kanten. Den blå värdebågen runt ratten är kvar.

## 0.9.5 (2026-11-27)
- DELAY: TAPE 3 HEADS (RE-201 med tre huvuden), ECHOPLEX (Maestro: varm, tiden glider), ECHOREC (Binson:
  fyra huvuden, rytmiska mönster) och MEMORY MAN (Electro-Harmonix BBD: mörk, inbyggd chorus).
- REVERB: SPRING (fjädertank), PLATE 140 (EMT 140), HALL 224 (Lexicon 224) och DIGITAL 250 (EMT 250).
- Klick på väljarrutan visar hela listan; pilarna stegar. Nivåerna kalibrerade som de andra typerna.
- Sparade projekt och presets behåller sitt delayläge och sin reverbtyp.

## 0.9.4 (2026-11-27)
- EFFECTS: enheten väljs med två pilväljare längst ner (som presetväljaren): DELAY (DIGI - Korg SDD-3000
  Style / TAPE - Roland Space Echo Style) och REVERB (sex typer, MPX500 och RMX16). Klicka på pilarna eller
  på rutan. Omkopplaren och TYPE-reglaget är borta, så EFFECTS är smalare.
- OUTPUT och PITCH är bredare; MASTER VOLUME och BOOST ligger i höjd med reglagen bredvid.
- Fönstret 1430 × 710 px (översta raden högre för väljarna).

## 0.9.3 (2026-11-27)
- EFFECTS visar vilken enhet som är vald, t.ex. "TAPE - Roland Space Echo Style" och
  "AMBIENCE - AMS RMX16 Style" (lyser blått när effektens LEVEL är uppe). Samma text i displayen.

## 0.9.2 (2026-11-27)
- DELAY: omkopplare DIGI / TAPE. TAPE är i Roland RE-201 Space Echo-stil: wow och flutter, bandmättning
  och filter i återmatningen så att varje eko blir mörkare och tunnare.
- REVERB TYPE: två nya typer i AMS RMX16-stil. AMBIENCE (kort, tät rymd nästan utan svans) och NONLIN
  (grindad reverb: jämn skur som tar tvärt slut; DECAY styr grindtiden). Nivån kalibrerad som de andra.
- Fönstret 1430 px brett för omkopplaren; ORGAN och POLY-SYNTH har fått lite mer luft.
- Sparade projekt och presets med REVERB TYPE från äldre versioner behåller sin typ.

## 0.9.1 (2026-11-27)
- DELAY LEVEL och REVERB LEVEL har ljudtaper: mitten = 10 dB under toppen, en fjärdedel = 20 dB under.
  Toppen som förut. Effekterna hörs nu över en större del av reglaget i stället för bara nära toppen.
  Projekt och presetfiler sparade med äldre versioner räknas om så att de låter likadant.
- Manualen: effekterna beskrivs som modellerade efter Korg SDD-3000 och Lexicon MPX500 (inga hänvisningar
  till inspelningen).

## 0.9.0 (2026-11-27)
Genomgång mot SK30:s servicemanual:
- BOOST-knapp under MASTER VOLUME, på som standard: SK30:s utgångssteg (MO = 3 × mixen, VR1, s. 67),
  +9,5 dB. Sitter efter effekterna, så bara nivån ändras. En ton ligger nu runt -15..-11 dBFS RMS, som
  andra syntar; limitern håller ackorden under -0,3 dBFS. Av = nivån från tidigare versioner.
- Översta tangenten C6 (MIDI 96) spelade en oktav för lågt på stråkar och poly-synth. Nu spelar den
  toppoktaven på 8'; 4' upprepar 8' (brytning), eftersom generatorn inte har oktaven över.
- KEYBOARD SPLIT delar som standard mellan F#3 och G3 (▼ på panelritningen, s. 4), och MANUAL BASS når
  upp till F#2 (klammern på panelen). Förut låg båda en oktav för lågt. SK30:s tangenter C1–C6 = MIDI 36–96.
- VIBRATO DELAY: först ca 0,7 s utan vibrato, sedan inmatning under ca 2,5 s på max (s. 69); kortare
  DELAY i samma proportioner.
- Fotpedaler: CC11 = FOOT CONT MIXED VOLUME, CC65 = FOOT SW PORTAMENTO (nedtryckt = portamento på).
  CC121 återställer dem.

## 0.8.4 (2026-11-27)
- Fönstret 1390 × 676 px: lika stor marginal (16 px) runt om, även längst ner. Reglagen ligger
  minst 26 px från panelkanterna och med samma avstånd mellan grupperna. LFO/PORTA/GLIDE har fått
  luft från SOLO SYNTHESIZER-panelens vänsterkant och en egen grupplinje.

## 0.8.3 (2026-11-27)
- REVERB LEVEL går nu upp till +8 dB, så att man kan få lika mycket reverb som i 0.8.0 och tidigare
  (reglaget i topp = samma som förut). 0 dB ≈ lika mycket klang som torrt ljud. Rec:-ljuden oförändrade.

## 0.8.2 (2026-11-27)
- Presetlistan: öppnas varje gång och stängs när man väljer ett ljud. Fönstret flyttade fokus
  när musen gick över listan, och Windows kräver att fönstret som äger en lista är förgrundsfönster.
- REVERB LEVEL höjde hela syntens volym (reverbet var 5-7 dB starkare än det torra ljudet på max).
  Nu ger max ungefär lika mycket klang som torrt ljud, och klangen blir inte starkare med längre
  DECAY. Rec:-ljuden låter som förut (deras REVERB LEVEL justerad från -12 till -4 dB).

## 0.8.1 (2026-11-27)
- Ljudet och effekterna man ställt in följer nu med när ett Live-projekt öppnas. Live skickar
  programnumret igen efter att ha läst tillståndet, och det skrev över allt med fabriksljudet.
- Tillbaka till 0.7.0:s nivå (de +4 dB i 0.8.0 togs bort) och 0.7.0:s fönster (presetlistan
  öppnades inte i 0.8.0). Limitern utan hård klippning från 0.8.0 är kvar; den märks bara när
  ljudet når taket.

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
