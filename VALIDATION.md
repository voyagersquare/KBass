# KBass validation

Portable C++ DSP checks pass: sine/saw/square spectra, A4 oscillator tuning,
OSC2 two-octave transposition, envelope release, legato glide and note priority,
sustain pedal, channel-specific MIDI panic, filter attenuation, LFO movement,
independent AMP/FILT release and bounded finite audio at 8–192 kHz.

Native JUCE processor checks cover MIDI event offsets,
all six factory programs, complete DAW state round-trip, rejection of invalid
state, independent envelope editing, editor opening/reopening, all control hit
areas and mono/stereo output. Cutoff interaction checks exercise actual mouse
and wheel events across the full frequency range, Shift changes during a drag,
fine wheel adjustment and balanced host automation gestures.

User preset checks cover persistence across new instances, all 23 values,
case-insensitive duplicates, confirmed replacement, corrupt files, non-finite
values and independent DAW project recall. They exercise both separate menus,
reselection of the current preset and the Windows Save dialog callback.

DSP checks also cover sample-rate-independent cutoff sweep response without
overshoot, a continuous onset after a large cutoff change, smoothed live sustain
edits and correct non-legato note onset during a previous release tail.

The Linux VST3 passes pluginval 1.0.4 strictness level 5, including state,
automation, repeated editor opening and audio processing at 44.1/48/96 kHz.
The Windows workflow also requires pluginval level 5 before packaging.

The 0.2.1 test ZIP is gated by a native MSVC build, DSP, processor and user preset tests,
a real native-editor capture, Windows pluginval and PE x64 checks. The compiled
module must report Independent Audio for every VST3 class, while retaining the
three existing class IDs. This version does not build or execute an installer.
Logs and SHA-256 checks accompany the test release.

Run results: https://github.com/voyagersquare/KBass/actions

Playback and automation in the user's particular DAW, audio driver, MIDI setup
and display scale require verification on their Windows computer. The supplied
reference does not specify BASSIMO's original DSP, so sonic equivalence is not
asserted.
