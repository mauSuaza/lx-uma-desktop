# Phase 4: files from the SD card (spec)

Status: **draft** (2026-11-14), short: most of it came across with the engines in Phase 2. Phase 4 of
[port-plan.md](port-plan.md). The code is the Eurorack's (LX_UMA `a179390`, release 0.34).

## 1. What there is already

| Part | State |
|---|---|
| `uma_sdfiles.c` (folder list, 4 KB slot pool, `.SYX` / `.BIN` checks) | ported in Phase 0, compiled, **never called** |
| Plaits: the 6-op's banks after `A B C`, the wavetable's and terrain's `fil` | ported (Phase 2); asks for its file every block (`umaSdf_setWant`), plays it once loaded |
| Menus: `fil` lists with the files' names, "loading", "bad file" | ported (Phase 2) |
| The Plaits sub on the wavetable and terrain | ported (Phase 2), in the 0.2 manual |
| Card slots | 6 × 4 KB (Phase 0 decision; the Eurorack has 10). One per voice: enough for every voice to play its own file; more would only cache, and RAM has ~25 KB left for the stack |

## 2. What's to do

1. **List the folders** (`umaSdf_scan`): at power-on with a card, and when the card goes back in (the desktop's
   main loop already re-mounts it there).
2. **Load the wanted files** (`umaSdf_service`, one file per pass of the main loop), then repaint the display (the
   patch names replace "loading").
3. **Audio while the card reads.** The desktop renders audio in the main loop; a 4 KB read takes about 1.5–3 ms.
   As for the kits' UMA records (Phase 1), the timer interrupt renders the audio while a card file is listed or
   read (`umaKit_irqOn` / `Off`, shared).
4. The card layout as on the Eurorack: `DX7/` (one level of subfolders), `WAVES/`, `TERRAIN/` at the top; the same
   files work on both. The manual chapter "Your own sounds" from the Eurorack's.

## 3. Builds

| Build | What | You test |
|---|---|---|
| **0.4a** | The hooks above | DX7 banks (the 4 in `lxr-fm/sdcard/DX7`), a wavetable and a terrain from the editor; card out and back in with the LXR on; no clicks while a file loads with a pattern playing |
| **0.4** | The manual, release notes; the release (the user decides) | |
