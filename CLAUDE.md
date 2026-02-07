# Amiga Pong - Development Guide

## Build Instructions

```bash
cd C:/Users/david/source/repos/amiga-dev/amiga-game
PATH="/c/amiga-gcc/bin:$PATH"
rm -f *.o
m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o pong.o pong.c
m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o graphics.o graphics.c
m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o game.o game.c
m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o input.o input.c
m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o highscore.o highscore.c
m68k-amigaos-gcc -noixemul -o bin/pong pong.o graphics.o game.o input.o highscore.o
```

Or as a single command:
```bash
cd C:/Users/david/source/repos/amiga-dev/amiga-game && PATH="/c/amiga-gcc/bin:$PATH" && rm -f *.o && m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o pong.o pong.c && m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o graphics.o graphics.c && m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o game.o game.c && m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o input.o input.c && m68k-amigaos-gcc -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer -c -o highscore.o highscore.c && m68k-amigaos-gcc -noixemul -o bin/pong pong.o graphics.o game.o input.o highscore.o && echo "Build successful!"
```

## Deploy to Amiga

Upload and run on Amiga (IP: 192.168.1.94):
```bash
AMIGA_HOST=192.168.1.94 python C:/Users/david/source/repos/amiga-dev/amiga-bridge/amiga_client.py --upload C:/Users/david/source/repos/amiga-dev/amiga-game/bin/pong "Development:pong"
AMIGA_HOST=192.168.1.94 python C:/Users/david/source/repos/amiga-dev/amiga-bridge/amiga_client.py "Development:pong"
```

Or combined:
```bash
AMIGA_HOST=192.168.1.94 python C:/Users/david/source/repos/amiga-dev/amiga-bridge/amiga_client.py --upload C:/Users/david/source/repos/amiga-dev/amiga-game/bin/pong "Development:pong" && AMIGA_HOST=192.168.1.94 python C:/Users/david/source/repos/amiga-dev/amiga-bridge/amiga_client.py "Development:pong"
```

## Key Paths

| Description | Path |
|-------------|------|
| Cross-compiler | `C:\amiga-gcc\bin\m68k-amigaos-gcc` |
| Amiga bridge client | `C:\Users\david\source\repos\amiga-dev\amiga-bridge\amiga_client.py` |
| Amiga target | `Development:pong` |
| Amiga IP | `192.168.1.94` |

## Architecture

- **Hardware sprites** for ball and paddles (flicker-free)
- **WaitBOVP()** used for sprite timing to minimize ghosting
- **Ball speed**: Configurable in `game.h` (BALL_INITIAL_SPEED, BALL_MAX_SPEED)

## Known Issues

- Minor sprite ghosting visible during ball movement (reduced but not eliminated with WaitBOVP timing)
