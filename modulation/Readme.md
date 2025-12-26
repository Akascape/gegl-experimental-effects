# Modulation Plugin

<img width="1353" height="603" alt="image" src="https://github.com/user-attachments/assets/0d482722-3982-4d88-ace1-07b225ed2dc7" />

<br> **Modulation** is a GEGL-based filter for GIMP that applies a **Directional Frequency Modulation (FM)** effect to images. It transforms image data into frequency-modulated lines, creating a retro signal processing, glitch-art, or "CRT scanline" aesthetic.

## Features

* **Directional Control:** Modulate signals in 4 directions (Right, Left, Up, Down).
* **Channel Selection:** Choose the source channel (Red, Green, or Blue) to drive the modulation intensity.
* **Controls:**
    * **Frequency:** Controls the density of the carrier waves.
    * **Density:** Adjusts the hardness/gamma of the signal peaks.
    * **Line Width:** Controls the thickness of the rendered waves.

<br> Source File: `modulation.c`
<br> Plugin File: `modulation.dll`

## Technical Details
The core algorithm utilizes a pre-calculated prefix-sum array to efficiently compute the integral of pixel intensity along the modulation axis in constant time $O(1)$, effectively avoiding the performance overhead of naive iterative summation. By treating the selected color channel as a signal, the filter performs 15 sub-pixel samples per output coordinate to determine waveform displacement, mapping the accumulated luminance to spatial frequency. This implementation leverages the GEGL 0.4 API's buffer iterators to process image chunks in parallel while maintaining a global context for the integral calculation, ensuring high-performance rendering even at high resolutions.

## License: GPLv3
## Author: Akash Bora [Akascape]
