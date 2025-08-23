# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` - Compile the Amiga demo application
- `make clean` - Remove build artifacts (obj/ and bin/ directories)
- `make install` - Copy compiled binary to /tmp/amiga_demo

## Architecture Overview

This is an Amiga 500/1200 development framework written in C for the m68k-amigaos target. The codebase is structured as a modular system with separate components for different hardware subsystems:

**Core System (`amiga_base.h/c`)**
- Central initialization and cleanup coordination
- Defines main data structures: `AmigaDisplay`, `AmigaImage`, `ModFile`, `AudioSystem`
- Screen resolution: 320x256 with 32 colors (5-bit depth)

**Display System (`display.c`)**
- Manages Intuition screen/window creation
- Provides pixel manipulation primitives
- Uses RastPort for all drawing operations
- Screen opened with LORES_KEY display mode

**Image System (`image.c`)**
- Loads 24-bit BMP files and converts to Amiga palette
- Allocates CHIP memory for image data
- Color reduction: RGB24 → 8-bit indexed using bit shifting
- Images must fit within screen dimensions

**Audio System (`audio.c`)**
- ProTracker MOD file format support (31 samples)
- Manages audio.device allocation with 4-channel map
- Pattern data stored separately from sample data
- CHIP memory allocation required for sample playback

**Memory Management**
- Uses Exec memory allocation (AllocMem/FreeMem)
- CHIP memory required for graphics and audio data
- PUBLIC memory for pattern/working data
- All allocations must be explicitly freed

## Compilation Requirements

- m68k-amigaos-gcc cross-compiler
- Amiga NDK headers and libraries
- Target: 68020 CPU with optimizations enabled
- No ixemul.library dependency (-noixemul flag)