# QuickKey AI V2.0

Native OpenFX plugin project for DaVinci Resolve on Apple Silicon Macs.

**Development status:** Phase 1 is an alpha/transparency diagnostic. It does **not** yet run CorridorKey AI or key green screens.

## What Phase 1 tests
- OpenFX discovery in DaVinci Resolve
- RGBA passthrough
- Forced transparency, revealing the timeline beneath the selected clip
- Incoming alpha-channel preview

No background media is inserted, and other timeline clips remain untouched.

## Build
GitHub Actions builds an arm64 OpenFX bundle and an unsigned installer on macOS. Open the **Actions** tab after source files are uploaded.

## AI engine plan
After the native alpha test succeeds, connect to a separately installed CorridorKey 2.0 engine (via a cached per-shot render). Do not infer AI mattes from source alpha or run temporal inference during OFX per-frame callbacks.

Original CorridorKey: https://github.com/nikopueringer/CorridorKey2.0
Community Mac engine: https://github.com/MarcelineVPQ/corridorkey-desktop

Respect original authorship and license restrictions. No CorridorKey code or model weights are included here.
