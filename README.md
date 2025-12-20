# gegl-experimental-effects
A collection of experimental GEGL operations (ops) focused on creative, shader-inspired image effects.
The plugins are designed to work in GIMP, which uses GEGL as its core image-processing engine.

## Requirements
- GIMP 2.10+
- GEGL (matching the GIMP version)

## Installation Guide
Precompiled binaries are provided in the dist/ directory for convenience.
### Windows
- Download the .dll files: 
- Copy it to your GEGL plugin directory:
```
C:\Users\<username>\AppData\Local\gegl-0.4\plug-ins\
```
- Restart gimp
### Linux
- Download the .so files:
- Copy it to your GEGL plugin directory:
```
~/.local/lib/gegl-0.4/
```
- Restart gimp

## Usage
1. Open the `Tools > GEGL Operation...`
2. Sesrch for the new effect in the menu and select
3. Adjust the controls
   
## Effects
1. Modulation


## License
This project is licensed under the GNU General Public License v3.0 or later.
See the LICENSE file for details.

## About
Created by Akash Bora (Akascape).
This repository represents open-source work.
Commercial plugins and products by the author are separate projects and are not derived from this codebase.
