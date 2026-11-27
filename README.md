# TrueSymph – Symphonic Ensemble

Privat VST2-instrument (64-bit) för Ableton Live 9.6 på Windows 7 och senare.
Efterliknar Yamaha SK30 (1979–80), byggt efter SK30:s servicemanual.
Målet är att alla delar ska finnas med: stråkar, poly-synth, orgel, solosynt och ensemble. Byggare: **2F Productions** (info@2f.se).

## Status: etapp 2 – poly-synth + tremolo

| Funktion | Status |
|---|---|
| Två tongeneratorer (I/II) med heltalsdelare från 1 MHz, fasläsade oktaver | Klar, automatiskt testad |
| Trappstegs-sågtand av fyrkanter (som PG-kortet) | Klar, automatiskt testad |
| STRING 1/2/3 (16'/8'/4') med SK30:s RC-stråkfilter | Klar, automatiskt testad |
| SLOW ATTACK (3/80 ms), SUSTAIN (0,03–1,6 s), sustainpedal | Klar, automatiskt testad |
| MODE I/II och DETUNE (−20…+27 cent på generator II) | Klar, automatiskt testad |
| VIBRATO: DELAY 0–3,2 s, SPEED 5–7 Hz, DEPTH 0–30 cent | Klar, automatiskt testad |
| BRILLIANCE ±12 dB | Klar, automatiskt testad |
| ENSEMBLE: tre BBD-linjer, LFO 0,64 + 6,4 Hz i trappsteg, 120° | Klar, automatiskt testad |
| 7 röster, mjukt röstbyte (inget klick) | Klar, automatiskt testad |
| Etapp 1 i Live 9.6 på Win7 | Testat av användaren: fungerar |
| POLY-SYNTH: CANCEL / 1 / 2 / 3 / MANUAL | Klar, automatiskt testad |
| POLY-SYNTH 1/2/3 med värden ur motståndsnäten på CPC-kortet | Klar, automatiskt testad |
| FEET: 4'/\\, 8'⊓, BP8'/\\, 8'/\\, 16'⊓, 16'/\\ | Klar, automatiskt testad |
| VCF (lågpass/bandpass, resonans Q 0,5–10, EG DEPTH 0–10 oktaver) | Klar, automatiskt testad |
| Envelope ADSR (0,003–3 s / 0,03–30 s / 0–10 / 0,03–30 s) | Klar, automatiskt testad |
| POLY-SYNTH TREMOLO, TREMOLO SPEED (0,64 / 6,4 Hz) | Klar, automatiskt testad |
| 14 fabriksljud i Lives presetlista | Klar, automatiskt testad |
| Etapp 2 i Live | **Ej testat ännu** |
| Orgel (fotlägen, perkussion, ORGAN 1/2/3) | Etapp 3 |
| Solosynt, MANUAL BASS, KEYBOARD SPLIT | Etapp 4 |
| Eget fönster (kompakt SK30-panel) | Etapp 5 |

## Installera och testa

1. Gå till repot på GitHub → fliken **Actions** → senaste gröna körningen.
2. Ladda ner **TrueSymph-win64** (en zip-fil) och packa upp `TrueSymph.dll`.
3. Lägg `TrueSymph.dll` i din VST-mapp (samma mapp som TrueSub).
4. Starta Live → **Plug-ins** → dra TrueSymph till ett MIDI-spår.
5. Etapp 1 har inget eget fönster. Klicka på den lilla triangeln i pluginets
   titelrad i Live för att se reglagen.

## Parametrar (index ändras aldrig)

| # | Namn i Live | SK30-reglage | Område |
|---|---|---|---|
| 0 | Volume | STRING VOLUME | −inf…+6 dB |
| 1 | Str 1 | STRING 1 (16') | av/på |
| 2 | Str 2 | STRING 2 (8') | av/på |
| 3 | Str 3 | STRING 3 (4') | av/på |
| 4 | SlowAtk | SLOW ATTACK | av 3 ms / på 80 ms |
| 5 | Sustain | SUSTAIN-knappen | av/på |
| 6 | SusTime | SUSTAIN-reglaget | 0,03–1,6 s |
| 7 | Mode | MODE I/II | I = en generator, II = båda |
| 8 | Detune | POLY-SYNTH DETUNE | −20…+27 cent |
| 9 | VibDly | VIBRATO DELAY | 0–3,2 s |
| 10 | VibSpd | VIBRATO SPEED | 5–7 Hz |
| 11 | VibDep | VIBRATO DEPTH | 0–30 cent |
| 12 | Brill | BRILLIANCE | −12…+12 dB |
| 13 | Ensembl | POLY-SYNTH ENSEMBLE | av/på |
| 14 | Tune | TUNE | A4 415–466 Hz |
| 15 | PolySel | POLY-SYNTH-knapparna | CANCEL, 1, 2, 3, MANUAL |
| 16 | PolyVol | POLY-SYNTH VOLUME | −inf…+6 dB |
| 17 | Feet | FEET (bara MANUAL) | 4 SAW, 8 SQR, BP8 SAW, 8 SAW, 16 SQR, 16 SAW |
| 18 | Cutoff | CUTOFF FREQ | 20 Hz–20 kHz |
| 19 | Reso | VCF RESONANCE | Q 0,5–10 |
| 20 | EGDepth | EG DEPTH | 0–10 oktaver |
| 21 | Attack | ATTACK TIME | 0,003–3 s |
| 22 | Decay | DECAY TIME | 0,03–30 s |
| 23 | SusLvl | SUSTAIN LEVEL | 0–10 |
| 24 | Release | RELEASE TIME | 0,03–30 s |
| 25 | PolyTrm | POLY-SYNTH TREMOLO | av/på |
| 26 | TrmSpd | TREMOLO SPEED | av 0,64 Hz / på 6,4 Hz |

Reglagen 18–24 gäller bara i MANUAL. POLY-SYNTH 1/2/3 har egna fasta värden.
ENSEMBLE och TREMOLO gäller både stråkar och poly-synth, precis som på SK30.

## Hur ljudet är byggt (från servicemanualen)

- **Tongenerering (PG-kortet):** två YM70400 delar ner en masterklocka på 1 MHz.
  Toppoktaven (C7–B7) får heltalsdelare, och alla lägre oktaver är exakta
  halveringar. Därför är alla toner fasläsade mot varandra, och stämningen avviker
  högst 1,4 cent från liksvävande, precis som i chippet. Tonerna löper fritt
  (ingen fasåterställning vid tangenttryck).
- **Sågtand i trappsteg:** /\8 = 0,69·fyrkant 8' + 0,47·fyrkant 4',
  /\16 = 0,69·fyrkant 16' + 0,47·/\8, /\4 = fyrkant 4' (motstånden 68k/100k mot 47k).
- **Stråkfilter:** passiva RC-stegar, exakt modellerade:
  16': 12k/27n + 12k/(8,2k)/27n. 8' och 4': 12k/18n + 12k/(5,6k)/18n.
- **Keyer:** varje röst har en envelope (SLOW ATTACK och SUSTAIN), samma för
  stråkar och poly-synth.
- **Vibrato:** modulerar masterklockan (alla toner), med fördröjning som tonar in.
- **Poly-synth (CPC-kortet):** samma tangenter och keyer som stråkarna, men genom
  ett gemensamt spänningsstyrt filter (IG00156, lågpass eller bandpass) med egen
  envelope (IG00151). Ingen VCA: volymen kommer från keyern. Envelopen startar om
  vid varje ny tangent. POLY-SYNTH 1/2/3 är fasta motståndsnät (läge = 10k/(R+10k)):

  | | CUTOFF | RESONANCE | EG DEPTH | ATTACK | DECAY | SUSTAIN | RELEASE |
  |---|---|---|---|---|---|---|---|
  | 1 | 18k | 2,2k | 27k | 18k | 15k | 56k | 3,3k |
  | 2 | 56k | 1,2k | 10k | 12k | 6,8k | 8,2k | 5,6k |
  | 3 | 15k | 2,2k | 22k | 6,8k | 18k | 27k | 4,7k |

  Alla tre använder 16'-sågtanden.
- **Tremolo (TE-kortet):** när ENSEMBLE är av och TREMOLO på styr klockchippet
  både volymen (VCA) och delay-chippen med en LFO på 0,64 eller 6,4 Hz.
- **Ensemble (TE-kortet):** tre MN3009 parallellt. Varje linje svajas av en
  långsam (0,64 Hz) och en snabb (6,4 Hz) trappstegs-triangel från YM63300,
  120° förskjutna. Lågpass före (ca 6,6 kHz) och efter (ca 5,4 kHz). Mono, som originalet.

## Konventioner

- **Tonnamn:** Abletons konvention, MIDI 60 = C3.
- **Mono:** SK30 är mono, därför är vänster och höger identiska.
- **Inte anslagskänslig:** som SK30:s stråkdel.
- **Nivå:** standardljudet ger ca −16 dBFS RMS med ett fyrtonsackord.

## Namnet

Pluginet hette först YamaSK30 och heter nu TrueSymph. Det har samma interna ID
som förut, så Live-projekt som sparats med YamaSK30 öppnas med TrueSymph.
Ta bort den gamla `YamaSK30.dll` ur VST-mappen så att pluginet inte syns två gånger.
TrueSymph har ingen koppling till Yamaha.
