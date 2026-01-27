# LCD Plugin for GIMP
<img width="1382" height="668" alt="image" src="https://github.com/user-attachments/assets/1bb595ea-7f14-4860-bcce-8c64c5cc45cc" />

LCD is a GEGL-based filter for GIMP that simulates an LCD display
by recreating a subpixel RGB grid structure with brightness boosting
and scanline-style gaps. The effect mimics the physical behavior of
real LCD panels, producing a sharp digital, retro-display aesthetic.

## Features

* **LCD Subpixel Simulation**
  - Recreates RGB stripe subpixel layout found in LCD panels

* **Cell-Based Sampling**
  - Each LCD cell samples a single source pixel for a blocky,
    authentic display look

* **Brightness Boost**
  - Amplifies subpixel intensity to compensate for color masking

* **Scanline & Grid Gaps**
  - Horizontal and vertical dark gaps simulate LCD pixel separation


## Controls

* **Scale**
  - Controls the size of each LCD cell / subpixel group
  - Larger values create chunkier, more visible pixels

* **Brightness**
  - Boosts the intensity of active subpixels

## Source Information

Source File: `lcd.c`  
Plugin files are available in the `dist/` folder

## Technical Summary

The LCD filter operates by dividing the image into fixed-size grid
cells defined by the Scale parameter. For each output pixel, the
algorithm:

1. Determines the enclosing LCD cell
2. Samples the source image at the cell’s origin
3. Selectively activates one RGB subpixel based on horizontal
   position within the cell
4. Applies brightness amplification and dark gaps to simulate
   physical LCD pixel separation

## Tips

- Combine with `Filters > Blur > Gaussian Blur` for smoother LCD glow
- Use `Colors > Levels` or `Exposure` to fine-tune brightness

## License

GPLv3

## Author

Akascape  
Copyright (C) 2026 Akascape  
https://www.akascape.com
