# Dino Crisis — ASI Cutscene Skip

**A native ASI plugin port of the original Dino Crisis Early Cutscene Skip project.**

This project migrates the cutscene-skipping logic from the original Cheat Engine-based implementation to a native ASI plugin, allowing the mod to run alongside the game without requiring Cheat Engine or a separate trainer.

**Original project:** [DC1_CUTSCENE_SKIP](https://github.com/GabiAle97/DC1_CUTSCENE_SKIP)

## Features

- **Cutscene Skip:** Skip supported cutscenes instead of relying on partial animation speed-ups whenever possible.
- **Door Skip:** Reduce waiting time during door transitions.
- **Hotkey controls:** Toggle each feature independently while playing.
- **Native ASI integration:** Load the functionality through an ASI loader without running Cheat Engine in the background.
- **C++ implementation:** A native plugin built with CMake and MinHook.

The goal remains the same as the original project: minimize unnecessary waiting and make faster, more consistent runs of Dino Crisis possible.

## Controls

| Key | Function |
| --- | --- |
| `F2` | Toggle Cutscene Skip |
| `F11` | Toggle Door Skip |

Both features start disabled and can be enabled independently.

## Installation

1. Download `CS_DS_ASILoader_DinoCrisis.zip` from [RELEASES](https://github.com/GabiAle97/ASI-DC1/releases).
3. Place the `script` folder and `dsound.dll` file in the games's exe directory.
4. Launch the game and use `F2` and `F11` to enable the desired features.

## Building from Source

### Requirements

- Windows
- Visual Studio with C++ development tools
- CMake 3.15 or later
- A Win32 (x86) build environment

### Compile

```bash
git clone https://github.com/GabiAle97/ASI-DC1.git
cd ASI-DC1

cmake -S . -B build -A Win32
cmake --build build --config Release
```

The build must target **Win32 (x86)** to match the game's process architecture.

The generated plugin is named `DinoCrisisCutsceneSkipper.asi`.

## Important Notes

- **Game compatibility:** The plugin relies on game-specific memory addresses and internal state. Compatibility with other executables, versions, or patches is not guaranteed.
- **Save files:** The original implementation tracks cutscene skips and uses that information to handle certain inventory items and internal decision states. Loading a save created at an arbitrary point may cause progression issues or crashes. Avoid loading existing saves when testing until compatibility has been verified.
- **Testing:** Some scenes, puzzles, animations, and transitions require special handling. Behavior may vary depending on the current game state.

## Project Structure

```text
ASI-DC1/
├── external/
│   └── minhook/
├── src/
├── CMakeLists.txt
└── README.md
```

## Credits

- **GabiAle97** — Development and porting.
- **iiLanceLM** and **MattGael** — Testing, guidance, and promotion of the original project.
- **EyeOfTheMind86** — Reference Cheat Table and useful memory addresses.
- **MinHook** — API hooking library.

## Disclaimer

This is an unofficial community modification for Dino Crisis. It is not affiliated with or endorsed by Capcom.

Use at your own risk. Back up your save files before testing.

## Related Project

For the original Cheat Engine script, standalone version, project background, and additional information, visit:

[**GabiAle97/DC1_CUTSCENE_SKIP**](https://github.com/GabiAle97/DC1_CUTSCENE_SKIP)
