# KBass saturation

KBass 0.3.0 adds three original character models. They are inspired by common
analogue behaviours; they are not measured replicas of particular hardware or
full circuit/tape-physics simulations.

Signal order: oscillators → filter → amplitude envelope → existing DC removal
and soft shaping → saturation → Volume. The master Volume can reduce the extra
level produced by drive. There is no automatic loudness compensation.

The SATURATION knob simultaneously raises internal drive and blends in the
processed signal. Zero bypasses all new colour. The square-law drive mapping
leaves finer adjustment at lower settings.

| Type | Model and resulting character |
|---|---|
| Tube | Asymmetrically biased tanh curve, with an envelope-dependent gain sag. Adds even and odd harmonics; softer compression. |
| Tape | Symmetric tanh magnetic target, bounded feedback/relaxation memory, envelope compression, a mild bass shelf and drive-dependent treble roll-off. Thicker, darker sound. |
| Transistor | Stronger drive, mild high-frequency emphasis and a sharper symmetric rational knee. More prominent odd harmonics and a firmer edge. |

All three run continuously, including when another type is selected, so their
memories remain warm. A 20 ms one-pole crossfade smooths type changes; the amount
uses 12 ms smoothing. These are time constants, not total settling times.
DC removal follows the weighted models before decimation.

The stage runs at 4× the host rate using matched 129-tap Kaiser-window FIR
interpolation and decimation. The fixed FIR group delay is 32 host samples,
reported to the DAW. The dry path has the same delay, including at 0%, so
automation never changes the reported latency. Oversampling reduces aliasing;
it does not remove every possible alias at extreme drive. No further nonlinear
shaping is applied after decimation.

The original 23 parameter IDs and VST3 class IDs are retained. `saturation` and
`saturationType` are appended. New project/user-preset states use schema 2 with
25 parameters. Schema-1 states with 23 parameters are validated and migrated in
memory with amount 0 and type Tube. Existing preset files are not rewritten on
load. Factory sounds default to 0%; saved user sounds remember both new controls.

`KBassSaturationTests` measures dry bypass, reset, spectra, gain, model differences,
treble colour, folded-harmonic suppression, smoothed transitions and stability.
Native processor and user-preset tests check host latency, layout, state recall
and legacy migration. Windows CI requires all these tests and pluginval level 5
before publishing the Windows installer and portable VST3 ZIP. Installer tests
also verify system installation, repair, installed-plugin validation and
uninstall while preserving user presets.
