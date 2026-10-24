# ZX4 Pixelizer

**A hardware-accurate Gigascreen (25Hz dual-VRAM) scene converter, pixel art editor, and animated sprite studio for ZX Spectrum 128K.**

---
<img width="2560" height="1528" alt="image" src="https://github.com/user-attachments/assets/7616674b-9a07-4fb0-b2f7-30ec40e97ca8" />
<img width="1447" height="1334" alt="image" src="https://github.com/user-attachments/assets/f7e882f0-603b-4004-a53e-b8b844b360db" />
---

## "Another PC converter? No thanks, I've seen enough dithered mush."

If you have been around the ZX Spectrum scene for more than five minutes, you know why most automated PC-to-ZX converters get laughed out of Pouët and ZX-Art:
1. They drown the image in Floyd-Steinberg or Bayer dither until it looks like a sandy newspaper scan.
2. They have no understanding of the **ZX Spectrum's stark black point (`0x00`)** vs. the first non-zero intensity level, turning dark night skies into washed-out gray (`#606060`) or random magenta blocks.
3. They treat 8×8 attribute cells in isolation, producing a hideous 8×8 "checkerboard" of clashing palettes across smooth gradients.
4. They give the artist zero control once the conversion is done.

**ZX4 Pixelizer** was built specifically to fix these exact problems. It does **not** try to fake true-color with noisy dithering. Instead, it treats the **128K Gigascreen mode** as a genuine **4-color-per-8×8-cell pixel art medium**—combining perceptual color math (`CIEDE2000` / `CIE94`) with shadow-gated contrast curves and giving you a full MDI pixel & attribute editor to hand-tune every single 8×8 character cell and pixel.

---

## How ZX4 Gigascreen Works Under the Hood

On a standard ZX Spectrum, each 8×8 character cell is limited to 2 colors (`INK` and `PAPER`) sharing 1 `BRIGHT` bit.

In **128K Gigascreen mode**, the Z80 switches the active video RAM bank between **Screen 0** (Bank 5, `#4000`) and **Screen 1** (Bank 7, `#C000`) every interrupt (`50Hz` -> `25Hz` effective frame rate). By pairing an attribute in Screen 0 (`INK0`, `PAPER0`, `BRIGHT0`) with an attribute in Screen 1 (`INK1`, `PAPER1`, `BRIGHT1`), every pixel in an 8×8 cell can independently select one of **4 temporally blended hardware colors**:

| Pixel LayerBits `(Layer0, Layer1)` | Screen 0 Bit | Screen 1 Bit | Resulting Blended Color |
| :--- | :---: | :---: | :--- |
| **`00`** | `0` | `0` | `PAPER 0` + `PAPER 1` |
| **`01`** | `0` | `1` | `PAPER 0` + `INK 1` |
| **`10`** | `1` | `0` | `INK 0` + `PAPER 1` |
| **`11`** | `1` | `1` | `INK 0` + `INK 1` |

Instead of 2 colors per 8×8 cell, you get **4 distinct solid colors per 8×8 cell** chosen from **~800 unique deduplicated 4-color hardware palettes**—without sacrificing a single pixel of the `256×192` resolution to spatial dithering.

---

## Key Features

### 1. Anti-Noise "Acid ZX" Conversion Engine
* **Zero Forced Dither:** Pure solid-color mapping per 8×8 cell for crisp, authentic ZX contours.
* **True `ZxBlack` Shadow Clamping:** Gigascreen has a massive luminance gap between pure black (`0`) and the darkest half-tone (`96` / `0x60`). ZX4 Pixelizer uses a non-linear S-curve black cutoff and shadow-gated saturation so near-black shadows stay pitch-black (`0,0,0`) instead of exploding into gray or purple attribute blocks.
* **Chroma-Protected Perceptual Matching:** Supports **CIEDE2000**, **CIE94 (Graphic Arts)**, **CIE76**, and **HSL** distance metrics accelerated by a precalculated 3D Lab Lookup Table (LUT) and OpenMP multi-threading. Includes anti-gray guards so dark saturated areas never get mapped to achromatic gray (`#606060`).
* **Context-Aware Multipass Scanning:** Optional `3×3`, `5×5`, and `7×7` neighborhood scanning weighs surrounding pixels when picking an 8×8 cell's palette, eliminating attribute boundary seams and checkerboard artifacts.
* **Instant Presets & Live Pre-Processing:** Built-in presets (*Default*, *Dark Sharp HQ*, *Acid ZX Contrast*, *Neon Gigascreen HQ*, *Clean Pixel Art*, *Ultra Sharp Poster*) plus real-time X/Y shift, scale, rotation, horizontal mirror, and Hue/Saturation/Lightness curves.

### 2. Full-Screen Scene Editor (`256×192`)
Automated conversion is only 80% of the job—a realZX artist always wants the final say:
* **Inspect & Override Any 8×8 Cell:** `Ctrl + Left Click` any 8×8 character block on the canvas to inspect its active 4-color Gigascreen palette in the Palette Dock.
* **On-Canvas Palette Cycling:** Hold `Ctrl` or `Shift` + `Mouse Wheel` over a selected 8×8 cell to cycle through palettes in real time and watch the cell re-quantize on the fly.
* **Attribute Cloning:** Hold `Shift + Left Click / Drag` to paint/clone the selected 8×8 cell's palette onto neighboring character cells.
* **Pixel-Level Touch-Up:** Select any of the 4 active slot colors (`00`, `01`, `10`, `11`) and draw pixels directly inside the selected 8×8 cell, or invert the cell's bitplanes with one click.
* **Full Undo/Redo History:** Step back and forth through every pixel stroke, palette tweak, and re-import.

### 3. Animated 4-Color Sprite Studio (Up to `255×192`)
Design standalone game sprites and multi-frame animations using any of the ~800 Gigascreen 4-color palettes:
* **4 Colors + 1-Bit Hardware Alpha Mask:** Every pixel has `Layer 0`, `Layer 1`, and `Alpha` (transparency).
* **Multi-Frame Animation Support:** Import animated `.apng`, `.gif`, `.webp`, or multi-frame sprite sheets, scrub through frames, and preview animations at 5 FPS right in the editor.
* **Live Interlace / Checkerboard Preview Dock:** See how your sprite blends against a 4-color background pattern in real time.
* **Drawing & Transform Tools:** Pencil (`1×1`), Brush, Eraser, interactive Sprite Move, Canvas Resize, Image Rescale, and Bitplane Color Inversion.
* **Lossless Project Saving:** Save editable sprites as indexed 5-color `.zx4.apng` (via the bundled `qapng` plugin) or native multi-frame `.zx4.png` Sprite Sheets.

### 4. Ready-for-Z80 Hardware Exports
No proprietary lock-in. Export your work directly into formats your assembler or emulator understands:
* **Dual Raw Screens (`.scr0` + `.scr1`):** Two standard `6912`-byte ZX Spectrum screen files (`6144` bytes bitmap + `768` bytes attributes each). Drop them straight into your Z80 source with `INCBIN`.
* **Executable 128K Tape (`.tap`):** Exports a ready-to-run `.tap` image containing a built-in 128K Z80 Gigascreen viewer (`GIGAVIEW.TAP`) followed by both screen banks. Load it directly into **Fuse**, **UnrealSpeccy**, **ZEsarUX**, or onto a real ZX Spectrum 128K / Pentagon via DivMMC.
* **Compressed `.zx4` Binary Sprite Format:** A compact planar binary format compressed with **LZ4HC** (or optional byte-level RLE8) designed for Z80 game engines.

---

## Binary Format Specifications (For Z80 Coders)

### `.scr0` / `.scr1` Export
Standard ZX Spectrum screen layout (`6912` bytes per file):
* `0x0000..0x17FF` (`6144` bytes): Display bitmap (`Layer 0` in `.scr0`, `Layer 1` in `.scr1`).
* `0x1800..0x1AFF` (`768` bytes): `32×24` standard ZX attributes (`FLASH=0`, `BRIGHT`, `PAPER`, `INK`).

### `.zx4` Sprite Binary Format
| Offset | Size | Description |
| :--- | :---: | :--- |
| `0x00` | `1 byte` | **Screen 0 Attribute:** `(BRIGHT0 << 6) \| (PAPER0 << 3) \| INK0` |
| `0x01` | `1 byte` | **Screen 1 Attribute:** `(BRIGHT1 << 6) \| (PAPER1 << 3) \| INK1` |
| `0x02` | `1 byte` | **Sprite Width** in pixels (`1..255`) |
| `0x03` | `1 byte` | **Sprite Height** in pixels (`1..192`) |
| `0x04` | `1 byte` | **Frames Count** (`1..255`) |
| `0x05..` | `N bytes` | **LZ4HC-compressed payload** containing: <br>1. `Alpha mask` bitstream (1 bit per pixel, column-major 8-pixel strips) <br>2. `Color Layer 0` bitstream (**only** for non-transparent pixels where `Alpha == 1`!) <br>3. `Color Layer 1` bitstream (**only** for non-transparent pixels where `Alpha == 1`!) |

> **Why omit transparent pixels from the color bitstreams?** Because transparent pixels don't need color bits! Storing color bits only where `Alpha == 1` shrinks the uncompressed payload before LZ4HC even touches it.

---

## Building from Source (CMake + Qt6)

ZX4 Pixelizer is written in **C++17** using **Qt6** and **OpenMP** for parallel multi-core conversion. If the `apng` submodule (`qapng`) is present, CMake automatically builds the `qapng` imageformat plugin alongside the main executable.

### Prerequisites
* **CMake** `>= 3.16`
* **Qt 6** (`Core`, `Gui`, `Widgets`, `Network`)
* C++17 compiler with **OpenMP** support (GCC, Clang, MinGW-w64, or MSVC)

### Build Instructions
```bash
git clone --recurse-submodules https://github.com/your-org/zx4pixelizer.git
cd zx4pixelizer

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```
The compiled binary will be placed in `build/bin/zx4pixelizer` (and the APNG plugin in `build/bin/imageformats/`).

---

## Quick Controls Cheat Sheet (Scene Editor)

| Action | Shortcut / Mouse |
| :--- | :--- |
| **Select / Deselect 8×8 attribute cell** | `Ctrl + Left Click` (or `Ctrl + Double Click` to deselect) |
| **Cycle palette at selected 8×8 cell** | `Ctrl + Mouse Wheel` or `Shift + Mouse Wheel` |
| **Clone selected palette to other cells** | `Shift + Left Click / Drag` |
| **Draw pixel with active color (`00..11`)** | `Left Click / Drag` inside the selected 8×8 cell |
| **Invert colors in selected cell / sprite** | **Tools -> Transform tools -> Invert colors** |
| **Undo / Redo** | `Ctrl + Z` / `Ctrl + Y` |

---

## License & Credits

* **Developed by:** ZXLDR — *ZX Lab of Dreams Reinvention* (2022–2026)
* **Third-Party Libraries:**
  * **LZ4 / LZ4HC** by Yann Collet (BSD 2-Clause)
  * **CIEDE2000** by Gregory Fiumara (MIT)
  * **qapng** plugin by Skycoder42 / mistificator
