# Installation and Compatibility — Design

**Index v1.0.0 · User guide**

## Install Index

**Current version:** Index 1.0.0. The plugin descriptor targets **Unreal Engine 5.8.0**, **Windows 64-bit**, and the **Editor**. The README describes a UE 5.8.2 release target; use the package actually built for your installed engine and verify compatibility before mixing versions.

### Precompiled package

1. Close Unreal Editor.
2. Obtain a compatible Index package from [GitHub Releases](https://github.com/mippi-the-dork/Index/releases) when available, or from Fab if distributed there.
3. Extract `Index/` beneath `YourProject/Plugins/`, so `YourProject/Plugins/Index/Index.uplugin` exists.
4. Open Unreal, find Index under **Edit → Plugins**, enable it if required, and restart when prompted.
5. Open the normal **World Outliner** to use custom ordering.

Do not mistake GitHub's automatically generated source archive for a precompiled release.

### Install from source

Clone [Index](https://github.com/mippi-the-dork/Index) into `YourProject/Plugins/Index`; prepare a C++-enabled Unreal project and build its Editor target. Use the toolchain required by your Unreal Engine installation.

<figure class="doc-media" data-media-id="installation-folder" data-kind="image">
<img src="../../assets/media-placeholder.png" alt="Index plugin directory correctly installed in an Unreal project" loading="lazy">
<figcaption>A correctly installed Index folder contains the plugin descriptor directly beneath the project’s Plugins directory.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture File Explorer showing YourProject/Plugins/Index in the address bar, with Index.uplugin, Source, Resources, Config and Doc visible. Avoid showing a nested Index/Index folder.</span></figcaption>
</figure>

## Verify the installation

Open **Project Settings → Plugins → Index** and look for Enable Custom Ordering, insertion-line colors, Edge Threshold, and Force Reorder Modifier Key. Index adds no separate content or toolkit window.

## Updating

Close the editor and replace the entire installed Index plugin folder, or pull the intended source commit and rebuild. Saved manual order is stored with the level, not in the plugin folder. Back up production projects as you would for any editor-plugin update.

---

For engine integration and implementation, [read Technical](TECHNICAL.md).
