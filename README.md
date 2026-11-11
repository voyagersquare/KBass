# KBass 0.1.1 — Independent Audio

**Windows 10/11 x64 VST3 bass instrument.** Dva audio oscilatora s izborom
**SAW / SQUARE / SINE**, dva zasebna ADSR-a i sučelje nadahnuto priloženom
BASSIMO referencom: tamni metal, drvene stranice i kromirani potenciometri.
Raspored je 1108 × 780 piksela, uz izvorno nacrtanu grafiku i naziv KBass.
Ovo je novi instrument s vlastitim DSP-om, ne izvorni WOK/BASSIMO plugin.

## Testni VST3 ZIP

Gotove datoteke nakon uspješnog Windows builda nalaze se u
[Releases](https://github.com/voyagersquare/KBass/releases).
Preuzmi **KBass-0.1.1-Windows-x64-VST3.zip**. Ova testna verzija isporučuje
se bez novog Windows instalera. Zatvori DAW i iz ZIP-a kopiraj cijelu mapu
`KBass.vst3` u `C:\Program Files\Common Files\VST3`, zamjenjujući prethodni KBass.
U Abletonu uključi **Use VST3 Plug-In System Folders**, odaberi **Rescan**,
zatim pod proizvođačem **Independent Audio** odaberi **KBass** i stavi ga na
MIDI traku. Uključi Monitor In ili record-arm.
Plugin svira preko MIDI kontrolera ili DAW-ove MIDI tipkovnice.

Unutarnju `.vst3` datoteku nemoj odvajati od paketa. Nije potrebno instalirati
Visual Studio za korištenje. Ako Ableton još prikazuje staru grupu proizvođača,
drži Alt dok klikneš Rescan kako bi ponovno skenirao plugine.

Verzija 0.1.1 mijenja naziv proizvođača iz Voyager Square u Independent Audio
i usklađuje natpis na sučelju. Postojeći VST3 identifikatori, parametri, preseti
i audio engine zadržani su za kompatibilnost sa spremljenim projektima.

## Kontrole

| Sekcija | Funkcije |
|---|---|
| OSC1 | WAVE: sawtooth, square ili sine |
| OSC2 | Nezavisan WAVE: sawtooth, square ili sine; OCTAV: 0, −1, −2; DETUNE: −50 do +50 centi |
| OSC1/2 | GLIDE: 0–1.5 s; LFO dubina modulacije tona: 0–2 polutona; MIX: OSC1 lijevo / OSC2 desno |
| ENVELOPE | ATTACK, DECAY, SUSTAIN, RELEASE; AMP/FILT bira koji se ADSR uređuje |
| LFO | WAVE: saw, triangle, square; SPEED: 0.03–30 Hz |
| FILTER | CUTOFF: 20 Hz–18 kHz; RESO; ENV: 0–6 oktava; LFO: 0–4 oktave |
| VOLUME | Izlazna glasnoća; minimum potpuno utišava izlaz |
| Metalna KB pločica | Izbor između šest tvorničkih preseta |

**AMP i FILT pamte zasebne vrijednosti i oba su uvijek aktivna.** Prekidač mijenja
samo četiri prikazane kontrole. Ne isključuje amplitudnu ovojnicu.
Attack i decay idu do 4 s, release do 8 s; sustain je razina, ne vrijeme.
Glide djeluje pri legato sviranju preklopljenih nota. Monofonija koristi
prioritet zadnje note, s povratkom na prethodnu držanu notu.

Povuci potenciometar gore/dolje. **Shift + povlačenje** daje finiju promjenu,
**dvostruki klik** vraća zadanu vrijednost. Dok držiš pokazivač nad kontrolom
ili je pomičeš prikazuje se vrijednost. Sve audio kontrole podržavaju DAW
automatizaciju; izbor AMP/FILT je spremljena postavka sučelja.
Patch, izbor valova, obje ovojnice i preset spremaju se u DAW projekt.

## Audio i MIDI

- Dva audio oscilatora: PolyBLEP saw i square te sinusni generator.
- 24 dB/okt rezonantni low-pass; obrada oscilatora i filtera na 2× sample rateu.
- Jedan zajednički slobodno tekući LFO, s odvojenim količinama za pitch i filter.
- Odvojeni AMP/FILTER ADSR; izglađene promjene kontrola i valnih oblika.
- MIDI note on/off po sampleu; pitch bend ±2 polutona; CC64 sustain;
  CC120/123 panic i CC121 reset kontrolera.
- Mono ili stereo izlaz, bez audio ulaza. Stereo kanali nose isti mono signal.
- Zaštita od denormala, DC blocker i mekano ograničenje izlaza.

Prikaz referentne slike ne otkriva točan filter ni izvorni algoritam, pa KBass
ne obećava identičan zvuk BASSIMO-u. Nema zasebnog trećeg audio oscilatora:
sinus je dodatni valni oblik na oba postojeća oscilatora.

## Izgradnja iz izvornog koda

Na Windowsu instaliraj Visual Studio s **Desktop development with C++** i
**C++ CMake tools for Windows**. Raspakiraj izvorni projekt i pokreni
`Build-Windows.cmd`. Podržan je x64 build. JUCE 8.0.6 dohvaća se na točno
određenom commitu; prvi build zahtijeva internet.

GitHub Actions automatski gradi VST3 na Windowsu, izvršava DSP i processor/UI
testove, snima stvarni editor, provjerava Windows x64 modul i proizvođača
Independent Audio te izrađuje testni VST3 ZIP. Pokreće se i pluginval level 5.
Novi instaler se ne izrađuje ni pokreće. Paketi uključuju SHA-256 i odgovarajući
izvorni kod KBass-a i JUCE-a.

Za samostalne DSP testove bez JUCE-a:

```sh
cmake -S . -B build/dsp -DKBASS_DSP_ONLY=ON
cmake --build build/dsp
ctest --test-dir build/dsp --output-on-failure
```

## Licenca

KBass izvorni kod: **AGPL-3.0-or-later**; vidi `LICENSE`.
JUCE se koristi pod AGPLv3 opcijom; njegove izvorne licence i obavijesti ostaju
u priloženoj arhivi. Drugi uvjeti JUCE-a dostupni su od njegovog proizvođača.
Referentni WOK/BASSIMO naziv, logotip i originalna slika nisu ugrađeni u plugin.
Pojedinosti o ovisnostima: `ThirdParty/NOTICES.txt`.
