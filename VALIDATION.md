# KBass validation

Portable C++ DSP checks pass: sine/saw/square spectra, A4 oscillator tuning,
OSC2 two-octave transposition, envelope release, legato glide and note priority,
sustain pedal, channel-specific MIDI panic, filter attenuation, LFO movement,
independent AMP/FILT release and bounded finite audio at 8–192 kHz.

Native JUCE processor checks pass on Linux and Windows x64: MIDI event offsets,
all six factory programs, complete DAW state round-trip, rejection of invalid
state, independent envelope editing, editor opening/reopening, all control hit
areas and mono/stereo output.

The Linux VST3 passes pluginval 1.0.4 strictness level 5, including state,
automation, repeated editor opening and audio processing at 44.1/48/96 kHz.
The Windows workflow also requires pluginval level 5 before packaging.

The 0.1.1 test ZIP is gated by a native MSVC build, the DSP and processor tests,
a real native-editor capture, Windows pluginval and PE x64 checks. The compiled
module must report Independent Audio for every VST3 class, while retaining the
three existing class IDs. This version does not build or execute an installer.
Logs and SHA-256 checks accompany the test release.

Run results: https://github.com/voyagersquare/KBass/actions

Playback and automation in the user's particular DAW, audio driver, MIDI setup
and display scale require verification on their Windows computer. The supplied
reference does not specify BASSIMO's original DSP, so sonic equivalence is not
asserted.
