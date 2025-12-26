# Modulation Plugin for Gimp

<img width="1216" height="570" alt="image" src="https://github.com/user-attachments/assets/a07797b0-1d23-4d1f-9da0-46481da6b35e" />

<br> **Modulation** is a GEGL-based filter for GIMP that applies a **Directional Frequency Modulation (FM)** effect to images. It transforms image data into frequency-modulated lines, creating a retro signal processing, glitch-art, or "CRT scanline" aesthetic.

## Features
* **Directional Control:** Modulate signals in 4 directions (Right, Left, Up, Down).
* **Channel Selection:** Choose the source channel (Red, Green, or Blue) to drive the modulation intensity.
* **Controls:**
    * **Frequency:** Controls the density of the carrier waves.
    * **Density:** Adjusts the hardness/gamma of the signal peaks.
    * **Line Width:** Controls the thickness of the rendered waves.

<br> Source File: `modulation.c`
<br> Plugin Files are available in the `dist/` folder

## Technical Summary
The core algorithm utilizes a pre-calculated prefix-sum array to efficiently compute the integral of pixel intensity along the modulation axis in constant time $O(1)$, effectively avoiding the performance overhead of naive iterative summation. By treating the selected color channel as a signal, the filter performs 15 sub-pixel samples per output coordinate to determine waveform displacement, mapping the accumulated luminance to spatial frequency. This implementation leverages the GEGL 0.4 API's buffer iterators to process image chunks in parallel while maintaining a global context for the integral calculation, ensuring high-performance rendering even at high resolutions.

## Tips
- Use `Colors > Color Balance` to set the color for the modulation lines
- Use `Filter > Light and Shadow > Bloom` for adding glow effect

## License: GPLv3
## Author: Akash Bora 
Copyright (C) 2025 Akascape
