# TrueSymph – Symphonic Ensemble

Privat VST2-instrument (64-bit) för Ableton Live 9.6 på Windows 7 och senare.
Efterliknar stråkdelen och ensemblen i Yamaha SK30 (1979–80), byggt efter
SK30:s servicemanual. Byggare: **2F Productions** (info@2f.se).

## Status: etapp 1 – stråkar + ensemble

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
| 6 fabriksljud i Lives presetlista | Klar, automatiskt testad |
| Fungerar i Live 9.6 på Win7 | Testat av användaren: fungerar |
| Poly-synth (filter, envelope), tremolo | Etapp 2–3 |
| Orgel | Senare (om den ska med) |
| Eget fönster (kompakt SK30-panel) | Senare etapp |

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
