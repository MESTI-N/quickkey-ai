# QuickKey AI V2.0 | Mac DaVinci Resolve OpenFX (development build)

**Status:** Source implementation of a native OpenFX wrapper around an **independently installed** CorridorKey 2.0 Core ML engine. The GitHub Actions build validates compilation, but actual image output and button behavior must be tested in DaVinci Resolve on an Apple Silicon Mac. This is NOT the old V2.0 "Foundation Test" build.

**Key difference:** QuickKey V1.3/V1.4 used ordinary colour keying and sometimes removed other green objects. This plugin asks the actual CorridorKey 2.0 neural model to produce a subject mask and foreground image, using the result's real alpha channel. No other timeline tracks are changed.

## 1. Install the Mac AI engine once (no Terminal)

The plugin **does not contain CorridorKey AI models**.

1. Download the Apple Silicon macOS disk image of the free community [CorridorKey Desktop](https://github.com/MarcelineVPQ/corridorkey-desktop/releases/tag/v1.0.0-dev.9).
2. Open it and drag **CorridorKey** into **Applications**.
3. Open CorridorKey Desktop and complete its first-run downloads of the AI runtime/models (approximately 5.74 GB on Mac). If macOS requests normal app permissions, grant only the ones needed.
4. Ideally test a small green-screen clip within CorridorKey Desktop once. Confirm the AI itself can process video correctly.

The default engine path in the Inspector is:
`/Applications/CorridorKey.app/Contents/Resources/engine/corridorkey`.
If it's installed elsewhere, locate `corridorkey` in the app's Contents/Resources/engine folder and select it manually. You can navigate within a .app via Finder's **Show Package Contents**.

## 2. Install QuickKey on the Mac

1. Download the latest **successful** [GitHub Actions run](https://github.com/MESTI-N/quickkey-ai/actions) named "Build QuickKey AI OpenFX for Apple Silicon".
2. Under **Artifacts**, download `quickkey-ai-native-mac`.
3. Unzip it and locate `QuickKeyAI-V2-Native-unsigned.pkg` and `QuickKeyAI-V2-Native-bundle.zip`.
4. Quit Resolve, then install the package if macOS allows it. As an alternative, extract the bundle ZIP and place `QuickKeyAI.ofx.bundle` under `/Library/OFX/Plugins/` (admin password may be required).
5. Restart Resolve. The plugin should appear as **QuickKey AI - CorridorKey 2.0** under OpenFX.
6. If the older **Foundation Test** is also visible, do not use it for AI keying. The new plugin has its own unique OpenFX identifier.

The package is developer-built and not Apple-notarized. Never disable macOS security globally.

## 3. Use inside Resolve

Use a **duplicate project or test timeline** first. The original footage should be unkeyed, without the older QuickKey versions applied.

1. Place your green-screen clip on any video track (V2, V6, or elsewhere).
2. Go to the Fusion page for that clip. Add **QuickKey AI - CorridorKey 2.0** between `MediaIn1` and `MediaOut1`.
3. Select the QuickKey node, open Inspector.
4. For **Source video (original)**, browse to the actual original .mov/.mp4 video file on disk. This ensures the AI processes the complete footage, not a video frame after Fusion effects.
5. For **CorridorKey engine executable**, leave the default if the app was installed in /Applications. Otherwise choose its `Contents/Resources/engine/corridorkey`.
6. For **Output cache folder**, create a **new empty folder** specific to this shot (e.g. `~/Movies/QuickKeyCache/scene01`). You may need to select it in Finder.
7. Keep **Despill strength** at 0.5 initially.
8. Set **First Resolve frame (offset)** to the Resolve frame number at which this source video should start at its first frame. On a clip positioned at timeline frame 0, the default 0 is fine. For a clip placed later in the timeline, set this to its actual clip start frame to align AI frames.
9. Set **Output** to **AI Cutout (transparent)**.
10. Click **Process Shot with AI** once. The AI starts externally and renders the actual RGBA EXR cutout into the output folder. This can take a while.
11. Check `quickkey.log` in that cache folder for progress. The button itself cannot show continuous percentage completion.
12. After the log reports success, return to Resolve and click **Refresh Render**. The Inspector **AI status** field will report whether the output is ready.
13. Play or render the timeline. The plugin loads the AI-generated EXR RGBA images. Transparent areas should reveal whatever visible layers are already underneath the clip. No new background is added.
14. Change **Output** to **AI Matte** to view the actual black/white AI mask, **Original** to bypass.

**Important:** This first integration expects the selected clip's OFX image size to match the source video. Cropping, timeline speed changes, retimes, proxies, Fusion Resize, and colour-space transformations can require manual adjustments. EXR frames are indexed by their numeric order and the first-frame offset. Always check a short test shot first.

## 4. Troubleshooting

- **No AI status change:** use the new plugin, not the older Foundation Test. Confirm Source, Engine and Cache fields are all set.
- **AI status says missing engine or clip:** file paths are invalid. Check that CorridorKey Desktop is installed and its models finished downloading.
- **Processing failed:** open `quickkey.log` in the cache folder. If the log says no cutout EXRs, it cannot load an alpha result.
- **Result still looks unchanged:** wait until the AI job finishes and press Refresh Render. Check for `quickkey.log`, and ensure the clip's render dimensions and frame offset are correct.
- **Green clothing vanishes:** the model can still be uncertain. CorridorKey supports first-frame hints, but the current QuickKey native interface does not yet expose them.
- **Background looks gray/opaque:** change to AI Matte to inspect the predicted alpha. A solid-white mask means the AI produced an opaque result or an incorrect frame was loaded, and the log/cache should be checked.
- **Plugin missing:** confirm installation under `/Library/OFX/Plugins/`, restart Resolve, and ensure you downloaded the new native build.

## Technical notes and limits

The external worker runs `corridorkey render --outputs cutout --format exr --despill ...`, which is the community project's documented interface. Its EXR cutout is **premultiplied RGBA**, unlike preview video outputs that do not carry alpha. See [CorridorKey Desktop output docs](https://github.com/MarcelineVPQ/corridorkey-desktop/blob/main/docs/wiki/Outputs.md). The third-party TinyEXR library is fetched during GitHub Actions builds; it is not the CorridorKey model.

The worker writes logs, a ready marker after successful rendering, and an error marker if inference fails. This code is a real AI *integration attempt*, not a promise that it has been proven on a physical M3 Pro or Resolve version. The first functional test is needed before calling it production-ready.

Licensing: Original CorridorKey code/model weights are not redistributed with this plugin. Review the [CorridorKey licence](https://github.com/nikopueringer/CorridorKey2.0/blob/master/LICENSE) before any redistribution, especially restrictions on competing products and repackaging.
