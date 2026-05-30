# Bad Apple on NES (NROM - 32KB)

<p align="center">
  <img src="Logo.png" width="200">
</p>

---

This is a Nintendo Entertainment System ROM that plays the MV of  
[Bad Apple!!](https://www.youtube.com/watch?v=9lNZ_Rnr7Jc) under strict hardware constraints: **NROM 32KB, no mapper, no audio**.

It renders the video at:

**10x9 resolution @ 1 FPS**


## Why is the quality so low?

The NES in NROM configuration is heavily limited:

- Maximum ROM size: 32KB
- No bank switching (mapper 0 only)
- Limited VRAM bandwidth and CPU timing constraints
- Strict memory layout restrictions

Because of this, the project prioritizes functional playback over visual fidelity

Resolution and frame rate were reduced to fit both memory and performance limits



## How to run it

### Requirements

Any NES emulator that supports:

- Mapper 0 (NROM)
- No mapper / no bank switching mode

### Recommended

- Run the emulator in 1:1 pixel scaling mode for correct aspect ratio perception.
- Use my emulator [NES_Emulator](https://github.com/0xagvz/NES-Emulator)

---

### If you're using my emulator:

```bash
./nes_emulator <pathToRom.nes>
```

## Build requirements

To build the ROM from source, you need the following tools installed:

### Required toolchain

- `cc65` (NES C compiler + assembler + linker toolchain)
  - Includes:
    - `cc65`
    - `ca65`
    - `ld65`

### Python

- Python 3.x


## Build system

The project uses a simple Make-based pipeline to generate the final NES ROM.

The process is split into two main parts:

### Video conversion

The `video` target converts the source MP4 into NES-compatible assembly data:

- Input: `utils/badapplevid.mp4` (download badapple vid for yourself)
- Output: `badapplevid.s`
- Parameters:
  - Resolution: 10x9
  - FPS: 1

```bash
make video
```

### ROM build

The all target compiles and links the final ROM using cc65:

- Compiles C code into assembly (cc65)
- Assembles both game logic and video data (ca65)
- Links everything using the NES NROM configuration (ld65)

```bash
make
```
