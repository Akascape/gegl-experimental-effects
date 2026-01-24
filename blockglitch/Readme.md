# BlockGlitch Plugin for Gimp

**BlockGlitch** is a GEGL-based filter for GIMP that creates a chaotic, digital artifact aesthetic. It simulates data corruption by slicing the image into horizontal blocks and applying a channel-split offset, perfect for glitch art, cyberpunk visuals, or "corrupted video" looks.

## Features
* **Horizontal Slice Glitch:** Randomly shifts horizontal segments of the image to create a "torn" data effect.
* **Channel Splitting:** Separates and offsets specific color pairs (Red-Cyan, Green-Magenta, Blue-Yellow) to simulate chromatic aberration.
* **Block Control:** Customizable vertical thickness for the glitched slices.
* **Seed-Based Randomization:** Uses a seed generator to ensure unique glitch distributions.

## Controls
| Property | Range | Description |
| :--- | :--- | :--- |
| **Slice Offset** | 0.0 - 1.0 | Controls the intensity of horizontal displacement and color split. |
| **Seed** | 0.0 - 10.0 | Randomizes the glitch pattern (maps to internal speed/time). |
| **Block Size** | 0.0 - 0.5 | Adjusts the vertical thickness of the glitched slices. |
| **Split Color** | Enum | Choose the color pair for the offset (Red-Cyan, Green-Magenta, or Blue-Yellow). |

<br> Source File: `blockglitch.c`
<br> Plugin Files are available in the `dist/` folder

## Technical Summary
The algorithm processes the image in the **RGBA float** space for high precision. It utilizes a custom 2D pseudo-random function to determine the vertical position ($sliceY$) and height ($sliceH$) of glitch blocks. 

The filter calculates the displacement using:
$$f(n) = \text{fract}(\sin(d) \times 43758.5453)$$

For every pixel within a target slice, the filter performs a nearest-neighbor sample from a horizontally offset coordinate. Simultaneously, a secondary color-split pass displaces a single color channel relative to its complements, resulting in the characteristic "fringe" effect seen in digital signal interference. The implementation leverages the GEGL 0.4 API to process image chunks while maintaining high-performance rendering.

## Tips
- **Lower Block Size:** Use a small block size (0.05 - 0.1) for a "noisy" or "static" look.
- **Motion Blur:** Apply a subtle horizontal Motion Blur after this filter to soften the glitched edges.
- **Vignette:** Add a vignette to focus the viewer's eye on the center where the glitching is often most prominent.

## License: GPLv3
## Author: Akash Bora 
Copyright (C) 2026 Akascape
[www.akascape.com](http://www.akascape.com)
