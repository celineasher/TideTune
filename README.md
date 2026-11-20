# TideTune Pro v0.4.0 — Dynamic Sidechain Preview

Original JUCE-based VST3 and Audio Unit pitch-correction plugin for macOS, not an Antares product.

## Implemented in 0.4.0

- **True optional sidechain input bus** (mono/stereo) exposed to compatible plugin hosts.
- Internal vocal detector or host-routed external bus. If the external bus is missing, no internal substitution occurs.
- Three active modulation targets: **Output Gain**, **Wet Mix**, and **Correction Strength**. All use a sample-smoothed detector envelope.
- Automatable sidechain controls: enable, source, target, threshold, ratio, attack, release, soft knee, depth, detector high-pass, detector low-pass.
- An automatable final output-gain control.
- A bigger, glossy graphite/cyan native JUCE editor with editable sidechain controls, dynamic meter, and original V0.3 tuning controls.
- Existing V0.3 plugin identifiers and parameter IDs retained for backward compatibility.
- New sidechain regression tests added to the pitch and transient test suites.

## Not yet implemented

**This is an incremental V0.4 preview**, not the entire planned 0.4 specification. The PSOLA/pitch-synchronous shifter rewrite, professional formant preservation, HQ/Live quality modes, automatic key detection, manual graph editing, and MIDI target notes are **not included**. The granular V0.3 shifter is retained and may still produce clicking/warbling on complex vocals. Sidechain processing does not fix the core pitch-shifter algorithm. Real-vocal and host testing are required.

## Install/update

Quit Ableton Live, then run:

```bash
bash ~/TideTune/update_mac.sh
```

The installer pulls the latest **main** release, tests, compiles and installs the VST3/AU with backup copies under `~/Desktop/TideTune-Backups`. Do not use the preview branch with important projects until validation completes.

## Sidechain in Ableton Live

1. Insert TideTune VST3 on a vocal audio track.
2. Enable **SIDECHAIN ON** in TideTune.
3. Choose **External Bus** as the source and **Output Gain** as the target for ducking.
4. Enable the plugin's sidechain input in Ableton's plugin sidechain routing, and select a kick or other audio track.
5. Set threshold, ratio, attack, release and depth. The gain-reduction meter should respond to the sidechain signal.
6. Choose **Internal Vocal** for level-triggered modulation using the dry vocal, without external host routing.

The host must enable and route the optional input bus; displaying **NO EXTERNAL AUX BUS** means it has not been enabled.

## Design & DSP details

- Sidechain detector is allocation-free and processes bounded 4096-sample chunks.
- Envelope is based on peak magnitude with attack and release smoothing; a soft-knee gain curve controls output ducking or wet/strength modulation.
- Detector HPF/LPF use first-order filtering; the detector signal is not mixed into the vocal output.
- Wet and strength modulation update at chunk boundaries; the pitch engine's existing continuous smoothing helps avoid abrupt ratio changes.
- No external-audio listen mode or zero-latency claim.
- Latency is reported via JUCE as in V0.3. Source is optimized for monophonic vocal pitch correction rather than polyphonic instrumentation.
- macOS AU and VST3 sidechain exposure is host dependent; verify in Ableton before relying on production routing.

## Tests

```bash
bash ./test_dsp.sh
```

The command runs pitch tracking and correction regression tests and sidechain detector routing, gain, release and silence tests. Passing synthetic tests does not guarantee artifact-free real vocal processing.

## Licensing

See `LICENSE` and JUCE licensing requirements before distributing commercially.
