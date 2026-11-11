# KBass - test only, without an installer

This branch tests the already published Windows x64 VST3 ZIP from
v0.1.0-preview.4. It does not compile an installer, execute Setup.exe, change
the audio engine, or publish a new release.

The Windows job checks the exact existing package and manufacturer metadata,
runs pluginval level 5 from the extracted ZIP, copies the complete bundle into
the ephemeral runner's standard Common Files/VST3 directory, compares every
file checksum and validates the plugin again from that system path. The copy
is removed when the test finishes. Reports explicitly record that no installer
was built/executed and Ableton Live was not tested.

## Manual Ableton test

1. The standard bundle path is
   `C:\Program Files\Common Files\VST3\KBass.vst3`.
2. The native module inside is
   `Contents\x86_64-win\KBass.vst3`; preserve the complete bundle, including
   `Contents\Resources\moduleinfo.json`.
3. KBass's manufacturer is **Voyager Square**, not Independent Audio.
   Search for **KBass** in Ableton's Plug-Ins browser.
4. Enable **Use VST3 Plug-In System Folders** and rescan.
5. Load KBass onto a **MIDI track**, then enable monitoring/record arm to play.

The screenshot of the user's browser does not establish whether the file is
missing. The missing-file question needs a check on that Windows computer;
successful loading in pluginval is not proof that Ableton has indexed it.
