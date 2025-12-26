# gegl-experimental-effects
A collection of experimental GEGL operations (ops) focused on creative, shader-inspired image effects.
The plugins are designed to work in GIMP, which uses GEGL as its core image-processing engine.

## Requirements
- GIMP 2.10+
- GEGL (matching the GIMP version)

## Installation Guide
Precompiled binaries are provided in the `dist/` directory for convenience.
Or you can download all the plugins from the [releases](https://github.com/Akascape/gegl-experimental-effects/releases/tag/releases).
### Windows
- Download the `.dll` plugin files: 
- Copy it to your GEGL plugin directory:
```
C:\Users\<username>\AppData\Local\gegl-0.4\plug-ins\
```
- Restart gimp
### Linux
- Download the `.so` plugin files:
- Copy it to your GEGL plugin directory:
```
~/.local/lib/gegl-0.4/
```
- Restart gimp

## Usage
1. Open the `Tools > GEGL Operation...`
2. Sesrch for the new effect in the menu and select it
3. Adjust the controls and view the results
   
## Effects
| Effect Name | Description | Example | 
|-------------|-------------|---------| 
| [Modulation](https://github.com/Akascape/gegl-experimental-effects/tree/main/modulation) | Directional Frequency Modulation (FM) effect | <img width="757" height="641" alt="image" src="https://github.com/user-attachments/assets/4e1c2eb0-fe54-4172-8c88-07569c1b6c1f" /> |

More to be added soon...

## Build from source
- Using [MSYS](https://www.msys2.org/) (The Easiest Way)
1. Download: Simply download this Repository
2. Install Dependencies: Open the MSYS2 MinGW 64-bit terminal and run:
   ```
   pacman -S --needed mingw-w64-x86_64-gegl mingw-w64-x86_64-toolchain pkg-config
   ```
3. Compile: Navigate to the plugn source file directory and run:
   ```
   gcc -shared -o plugin_name.dll plugin_name.c -I. $(pkg-config --cflags --libs gegl-0.4)
   ```

## License
This project is licensed under the GNU General Public License v3.0 or later.
See the [LICENSE](https://github.com/Akascape/gegl-experimental-effects/blob/releases/LICENSE) file for details.

## About
Created by Akash Bora (Akascape).
This repository represents open-source work.
Commercial plugins and products by the author are separate projects and are not derived from this codebase/license.

Follow me for more tools: [`Akascape`](https://github.com/Akascape/)

