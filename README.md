# TideTune Pro v0.3 — original monophonic pitch-correction plugin

Original JUCE-based VST3, Audio Unit, and standalone source project for macOS. TideTune is **not** an Antares product.

## What's new
- Native studio-style GUI: metallic cyan-ring controls, live input/target notes, centered cents tuner, live scrolling pitch trace, clickable two-octave pitch-class keyboard and functioning presets.
- Audio engine updates: bounded waveform-similarity grain alignment at silent grain resets, cubic sample interpolation, stable target-note confirmation and short unvoiced-consonant hold.
- Existing automatable controls: Retune Speed, Strength, Flex Tune, Humanize, Vibrato, Transpose, Mix, Bypass, vocal range, key, scale and correction style.
- Presets: HARD RAP, MELODIC, NATURAL.

## Install or update on your Mac
Quit Ableton before running:

```bash
bash ~/TideTune/update_mac.sh
```

This pulls the latest source, runs DSP regression tests, builds the JUCE plugin and installs VST3/AU in the user Library. The script preserves old installed bundles under `~/Desktop/TideTune-Backups`.

Alternatively, from inside the repository, use:

```bash
bash ./test_dsp.sh
bash ./build_mac.sh --install
```

Requires Apple Command Line Tools, CMake, Git and network access for the JUCE dependency. To restore a previous version, quit Ableton and copy the backup bundles back to the plugin folders.

## Limitations
- **Experimental** monophonic granular vocal shifter, not a studio-proven formant-preserving pitch correction engine.
- No true formant shifting/preservation, offline graph-editing, or automatic key detection. The **Pitch Trace** button is a visual monitor, not graph editing.
- Synthetic DSP tests pass in the development environment. Full macOS JUCE compilation, AU/VST3 scanning and real-voice listening must be tested on the user's Mac.
- Click reduction cannot guarantee completely artifact-free processing on every vocal or instrumental source.
- Follow JUCE licensing terms before commercial distribution.

## Verification
`test_dsp.sh` runs monophonic pitch tracking, scale quantization, correction, bypass, noise and harmonic transition/dropout regression tests using Clang or GCC. Use a blank Live Set to validate the VST3 build before replacing plugins in an important session.
