# KBass 0.3.0 — Independent Audio

**Windows 10/11 x64 VST3 bass instrument.** Dva audio oscilatora s izborom
**SAW / SQUARE / SINE**, dva zasebna ADSR-a i sučelje nadahnuto priloženom
BASSIMO referencom: tamni metal, drvene stranice i kromirani potenciometri.
Raspored je 1108 × 780 piksela, uz izvorno nacrtanu grafiku i naziv KBass.
Ovo je novi instrument s vlastitim DSP-om, ne izvorni WOK/BASSIMO plugin.

Verzija 0.2.0 dodaje odvojene **FACTORY PRESETS** i **USER PRESETS** izbornike
u donjoj traci te **SAVE** za spremanje vlastitih zvukova. KB pločica i sitni
natpis dolje lijevo su uklonjeni. Metal ima umjerenu patinu i fine ogrebotine.
Cutoff je precizniji pri povlačenju i ima dodatno zaglađivanje u audio engineu.

Verzija **0.2.1** pojačava istrošenost kućišta: oguljeni rubovi s vidljivim
metalom, dublje ogrebotine, tamne masne mrlje, izlizani rubovi potenciometara
i tragovi korištenja na kromu i drvenim stranicama. Natpisi i oznake kontrola
ostaju čitljivi.

Verzija **0.3.0** dodaje **SATURATION** pored smanjenog **VOLUME** gumba.
Trostupanjski **SAT TYPE** bira **Tube / Tape / Transistor** i nalazi se iznad
njih, u sredini. Saturacija pojačava i podebljava signal prije izlazne glasnoće.
Na 0% zvuk ostaje kao prije, uz fiksnu latenciju od 32 samplea koju plugin
prijavljuje DAW-u (0.67 ms pri 48 kHz).

## Windows instaler i VST3 ZIP

Gotove datoteke nakon uspješnog Windows builda nalaze se u
[Releases](https://github.com/voyagersquare/KBass/releases).
Preuzmi **KBass-0.3.0-Windows-x64-Setup.exe**. Zatvori DAW i pokreni instaler.
Instalira cijeli VST3 paket u
`C:\Program Files\Common Files\VST3\KBass.vst3`, a dokumentaciju i deinstaler
u `C:\Program Files\Independent Audio\KBass`. Windows traži administratorsko
odobrenje jer se plugin postavlja u sistemsku mapu. Ponovna instalacija
ažurira ili popravlja postojeći paket. Deinstalacija preko Windows Settings >
Apps uklanja plugin i čuva korisničke presete.

Za ručno kopiranje dostupan je i **KBass-0.3.0-Windows-x64-VST3.zip**: iz njega
kopiraj cijelu mapu `KBass.vst3` u `C:\Program Files\Common Files\VST3`.
U Abletonu uključi **Use VST3 Plug-In System Folders**, odaberi **Rescan**,
zatim pod proizvođačem **Independent Audio** odaberi **KBass** i stavi ga na
MIDI traku. Uključi Monitor In ili record-arm.
Plugin svira preko MIDI kontrolera ili DAW-ove MIDI tipkovnice.

Unutarnju `.vst3` datoteku nemoj odvajati od paketa. Nije potrebno instalirati
Visual Studio za korištenje. Ako Ableton još prikazuje staru grupu proizvođača,
drži Alt dok klikneš Rescan kako bi ponovno skenirao plugine.

Proizvođač ostaje **Independent Audio**. Postojeći VST3 identifikatori i izvorna
23 parametra zadržani su. Nova dva parametra dodana su na kraj. Projekti i
korisnički preseti iz verzija 0.1.x i 0.2.x učitavaju se sa saturacijom na nuli.

## Kontrole

| Sekcija | Funkcije |
|---|---|
| OSC1 | WAVE: sawtooth, square ili sine |
| OSC2 | Nezavisan WAVE: sawtooth, square ili sine; OCTAV: 0, −1, −2; DETUNE: −50 do +50 centi |
| OSC1/2 | GLIDE: 0–1.5 s; LFO dubina modulacije tona: 0–2 polutona; MIX: OSC1 lijevo / OSC2 desno |
| ENVELOPE | ATTACK, DECAY, SUSTAIN, RELEASE; AMP/FILT bira koji se ADSR uređuje |
| LFO | WAVE: saw, triangle, square; SPEED: 0.03–30 Hz |
| FILTER | CUTOFF: 20 Hz–18 kHz; RESO; ENV: 0–6 oktava; LFO: 0–4 oktave |
| VOLUME | Izlazna glasnoća nakon saturacije; minimum potpuno utišava izlaz |
| SATURATION | 0–100%; istodobno pojačava drive i udio obrađenog signala |
| SAT TYPE | Tube: topliji asimetrični drive; Tape: kompresija i mekši visoki; Transistor: čvršći drive |
| FACTORY PRESETS | Izbornik šest tvorničkih preseta, u donjoj traci |
| USER PRESETS | Susjedni izbornik trajno spremljenih korisničkih zvukova |
| SAVE | Unesi ime i spremi trenutni zvuk kao korisnički preset |

Klikni **SAVE**, upiši ime i potvrdi spremanje. Zvuk se odmah pojavljuje u
**USER PRESETS**. Za postojeće ime plugin pita želiš li zamijeniti taj preset.
Ponovni izbor već odabranog factory ili user preseta vraća spremljeni zvuk.
Korisnički preseti čuvaju se kao `.kbasspreset` datoteke u
`%APPDATA%\Independent Audio\KBass\Presets`. Mogu se kopirati za backup.
DAW projekt sprema cijeli zvuk i naziv odabranog preseta, pa njegov povrat ne
ovisi o dostupnosti izvorne preset datoteke. Čitanje i spremanje datoteka odvija
se izvan audio obrade.

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

Cutoff se sada pomiče ravnomjerno u odnosu na frekvenciju: jedan piksel daje
približno 0.31% promjene kroz cijeli raspon. **Shift + povlačenje** i **Shift +
kotačić** daju još deset puta finiji korak. Promjena Shifta tijekom povlačenja
ne uzrokuje skok vrijednosti. Audio koristi dvostupanjsko zaglađivanje cutoffa
u oktavnom prostoru; filter envelope i LFO ostaju brzi. Kontrole ADSR-a, pitch
bend, glasnoća, mix i dubine modulacije također imaju zaglađene prijelaze.

Saturacija koristi tri zasebna DSP modela, **4× oversampling** s FIR filtrima,
zaglađenu količinu i glatki prijelaz između tipova. Saturacijom možeš podići
razinu signala; **VOLUME** zatim postavlja konačni izlaz. Nema automatskog
izjednačavanja glasnoće. Sva tri tipa i količina spremaju se u projekt i user
preset, a factory preseti kreću od 0%. Detalji: [SATURATION.md](SATURATION.md).

## Audio i MIDI

- Dva audio oscilatora: PolyBLEP saw i square te sinusni generator.
- 24 dB/okt rezonantni low-pass; obrada oscilatora i filtera na 2× sample rateu.
- Jedan zajednički slobodno tekući LFO, s odvojenim količinama za pitch i filter.
- Odvojeni AMP/FILTER ADSR; izglađene promjene kontrola i valnih oblika.
- MIDI note on/off po sampleu; pitch bend ±2 polutona; CC64 sustain;
  CC120/123 panic i CC121 reset kontrolera.
- Mono ili stereo izlaz, bez audio ulaza. Stereo kanali nose isti mono signal.
- Zaštita od denormala, DC blocker i izvorno mekano oblikovanje prije saturacije.
- Saturacija na 4× sample rateu; fiksna latencija 32 samplea, pri svim postavkama.

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
Inno Setup 6 izrađuje Windows instaler. CI zatim stvarno instalira plugin u
sistemsku VST3 mapu, provjerava identičnost instaliranih datoteka, popravak
ponovnom instalacijom, pluginval level 5 nad instaliranim pluginom te
deinstalaciju koja čuva user presete. Tek nakon uspjeha objavljuje pakete,
instalacijske logove, SHA-256 i odgovarajući izvorni kod KBass-a i JUCE-a.
`Package-Windows.ps1 -SkipInstaller` i dalje omogućuje izradu samo ZIP-a.

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
