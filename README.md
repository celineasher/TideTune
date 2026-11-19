# TideTune — Original Vocal Pitch Correction Plugin

TideTune is an experimental JUCE-based **VST3 / Audio Unit** monophonic vocal pitch corrector for macOS, not an Antares product. The v0.2 source adds Hard Rap / Melodic / Natural presets, pitch tracking, fast retune, scale masks, and a custom UI.

## First-time setup from existing v0.2 source

The repository was initialized as an update channel; the complete plugin source still needs a one-time import from your earlier TideTune v0.2 download. The installer checks for the ZIP in `~/Downloads/TideTune-v0.2.0-mac-source.zip` or the extracted `~/Downloads/TideTune` folder.

Quit Ableton before running:

```bash
git clone https://github.com/celineasher/TideTune.git ~/TideTune
bash ~/TideTune/import_existing_mac.sh
bash ~/TideTune/update_mac.sh
```

The import script copies your existing v0.2 source to the clone, commits and pushes it. Pushing requires GitHub authentication on your Mac. Do not run it if the downloaded source has been modified in unexpected ways.

## Every future update

Quit Ableton, then:

```bash
bash ~/TideTune/update_mac.sh
```

The updater fast-forwards from GitHub, runs DSP tests, compiles JUCE plugin formats, and installs to your user Library. Previous installed plugins are backed up by `build_mac.sh`.

## First install/update as one Bash command

After checking the script before running it:

```bash
bash <(curl -fsSL https://raw.githubusercontent.com/celineasher/TideTune/main/install_mac.sh)
```

Requires Git, CMake, macOS Command Line Tools and access to GitHub/JUCE. The code is experimental; DAW compatibility is not guaranteed. JUCE has separate licensing requirements.

## v0.2.1 — Pitch-transition click fix

- Fixed a discontinuity when automatic pitch shifting changes direction (upward correction to downward correction, or the reverse). The dual-grain read-head phase is now continuous, rather than jumping between delay positions.
- Smoothed the wet/dry blend, bypass and voiced/unvoiced crossfades.
- Increased the minimum HARD retune slew to 1.5 ms for fewer artifacts on sudden note transitions; the HARD RAP preset still targets fast, obvious tuning.
- Added standalone C++ regression tests for pitch-direction changes and wet/dry automation. Run `bash ./test_dsp.sh`.
- Preserved the plugin identity / bundle ID, so existing projects should continue recognizing the updated plugin.

For a local update, **quit Ableton completely** and run:

```bash
bash ~/TideTune/update_mac.sh
```

The update script pulls GitHub changes, runs DSP tests, builds VST3/AU, and backs up previous plugin installations on the Desktop before replacement. Reopen Ableton afterward. This patch has passed standalone DSP tests; compatibility and real vocal recordings still require testing in Ableton on macOS.

**Known limits:** Grain-based shifting can still introduce artifacts on noisy, breathy, or polyphonic sources. TideTune does not yet include production-grade formant preservation or pitch-synchronous overlap-add. Fixing the direction-switch click does not guarantee all vocal distortion is eliminated.
