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
