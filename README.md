# Amiga Pong

A classic Pong game for the Amiga 500, written in C using the OS-friendly approach (no hardware takeover).

## Features

- Smooth, flicker-free animation using hardware sprites
- Mouse-controlled player paddle
- AI opponent with prediction and reaction delay
- Persistent high scores (saved to S:pong.hiscore)
- First to 11 points wins
- Clean OS integration - returns properly to Workbench

## Requirements

- Amiga 500 or compatible (or emulator like FS-UAE)
- Kickstart 2.0+ (v37+)
- 512KB Chip RAM

## Building

Requires the amiga-gcc cross-compiler toolchain.

```bash
# Set compiler path
export PATH="/path/to/amiga-gcc/bin:$PATH"

# Build
make clean
make
```

The executable will be created at `bin/pong`.

## Controls

- **Mouse**: Move paddle up/down
- **Left Click**: Start game / Resume from pause
- **ESC**: Pause game / Quit to title / Exit game

## Technical Details

- 320x256 PAL low-res screen, 3 bitplanes (8 colors)
- Hardware sprites for ball and paddles (flicker-free)
- Fixed-point math (8.8 format) for smooth ball movement
- IDCMP message handling for input
- AmigaDOS file I/O for high score persistence

## Project Structure

```
Makefile        - Build configuration
pong.c          - Main entry, game loop, state machine
graphics.c/h    - Screen setup, sprite handling, drawing
game.c/h        - Ball physics, collision, AI logic
input.c/h       - Mouse and keyboard input via IDCMP
highscore.c/h   - High score loading/saving
```

## License

MIT License - Feel free to use and modify.
