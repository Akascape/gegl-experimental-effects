# PS Adjustment GEGL
A collection of GEGL plugins that replicate Photoshop's adjustment layers for GIMP and other GEGL-based image editing applications.

## Overview
This project provides Photoshop-style adjustment layer functionality for GEGL, allowing users to apply familiar color correction and tonal adjustment tools in open-source image editing software. 

## Available Adjustment Layer Plugins

| Plugin Name | Description | Accuracy |
|------------|-------------|---------------------|
| **PS Black & White** | Advanced black and white conversion with 6 color channel sliders (Reds, Yellows, Greens, Cyans, Blues, Magentas) and tint options | High (~90%) |
| **PS Brightness / Contrast** | Intelligent brightness and contrast adjustment with both legacy (linear) and modern (sigmoid) algorithms | Very High (~95%) |
| **PS Channel Mixer** | Mix and blend color channels with precise control, including monochrome output option | High (~85%) |
| **PS Color Balance** | Adjust color balance separately for shadows, midtones, and highlights with preserve luminosity option | High (~90%) |
| **PS Curves** | Powerful tonal adjustment using control points with per-channel curve editing (RGB, Red, Green, Blue, Alpha) | High (~85%) |
| **PS Exposure** | HDR-style exposure control with exposure stops, offset, and gamma correction | Very High (~90%) |
| **PS Gradient Map** | Map grayscale values to color gradients for creative color grading effects | High (~88%) |
| **PS Hue/Saturation** | Comprehensive hue, saturation, and lightness adjustment with master and per-color range controls | High (~90%) |
| **PS Invert** | Simple image inversion with per-channel control (RGB, Red, Green, Blue, Alpha) | Common (100%) |
| **PS Levels** | Classic levels adjustment with input/output black/white points, gamma, and per-channel control | Very High (~95%) |
| **PS Photo Filter** | Apply warming/cooling filters and custom color overlays with density and luminosity controls | High (~85%) |
| **PS Posterize** | Reduce the number of tonal levels for artistic posterization effects | Very High (~95%) |
| **PS Selective Color** | Advanced color correction targeting specific color ranges (Reds, Yellows, Greens, Cyans, Blues, Magentas, Whites, Neutrals, Blacks) with CMYK adjustments | Medium-High (~80%) |
| **PS Threshold** | Convert images to pure black and white with adjustable threshold level | Common (100%) |
| **PS Vibrance** | Intelligent saturation boost that protects skin tones, with separate vibrance and saturation controls | High (~88%) |

## Features

- 🎨 **15 Adjustment Plugins** - Comprehensive suite of color and tonal adjustments
- 🔄 **Non-Destructive** - All adjustments are applied as GEGL operations
- 🎯 **High Accuracy** - Algorithms designed to closely match Photoshop's behavior, manually compared (using Photopea)
- 🎛️ **Similar Controls** - Familiar parameter ranges and options

### Accuracy Notes
The **Photoshop Similarity** ratings are based on:
- Algorithm accuracy compared to Photoshop's documented and observed behavior
- Visual output comparison using standardized test images
- Parameter range and control similarity

### Algorithm Sources
**Important:** The algorithms implemented in these plugins are **inspired by** Photoshop's adjustment layers and are **not copied or stolen** from Adobe's proprietary code. These implementations are based on:

- Publicly available documentation and technical resources
- Reverse engineering through observation and testing
- AI-assisted algorithm generation and optimization
- Community knowledge and open-source image processing literature

**Note:** This project aims to provide open-source alternatives to proprietary image editing tools. While we strive for accuracy, these plugins are not official Adobe products.

## License: GPLv3
### Author: Akash Bora
