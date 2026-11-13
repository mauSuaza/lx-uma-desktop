# Code copied from LX_UMA (the Eurorack firmware)

The sound engines are copied unchanged from [LX_UMA](https://github.com/mauSuaza/LX_UMA) (`~/src/eurorack/lxr-fm`).
When they change there, copy them again and update the commit below. Files changed here for the desktop are listed
under "Changed on the desktop".

| Files | LX_UMA commit | Copied |
|---|---|---|
| `Core/Inc/Soundengine/DSPAudio/{FmDrum,FmWaves,MdDrum,PlaitsDrum}.h`, `Core/Src/Soundengine/DSPAudio/{FmDrum.c,FmWaves.c,MdDrum.cpp,PlaitsDrum.cpp}`, `Core/Src/Soundengine/DSPAudio/mi/` (Plaits, MIT), `Core/Src/Soundengine/DSPAudio/mnmd/` (OP-XY-MNMD, MIT), `Core/Inc/uma_sdfiles.h`, `Core/Src/uma_sdfiles.c` | `a179390` (release 0.34 + manual) | 2026-11-12, 0.0c |

To check for differences: `diff -r ~/src/eurorack/lxr-fm/Core/Src/Soundengine/DSPAudio/mi Core/Src/Soundengine/DSPAudio/mi`
(and the same for each file above).

| `Core/Inc/uma_params.h` (generated from `Core/Inc/Parameters.h`'s UMA block and helper macros); in `menu.c` / `menu.h` / `MenuText.h` / `menuPages.h`: the FM names, lists, value types (`parameter_dtypes`' UMA block), pages, `menu_getPage`, extra screens, `menu_umaListEntries` (ported, not copied: the desktop's menu code differs) | `a179390` | 2026-11-12, 0.1c |

## Changed on the desktop

- `Core/Inc/uma_sdfiles.h`: `UMA_SDF_SLOTS` 6 (10 on the Eurorack), to save 16 KB of RAM (0.1a). Keep this when
  copying the file again.
