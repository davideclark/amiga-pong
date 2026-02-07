# Amiga Pong - Makefile
# Builds with m68k-amigaos-gcc (bebbo's GCC toolchain)

# Compiler settings
CC = m68k-amigaos-gcc
CFLAGS = -mcpu=68000 -O2 -Wall -noixemul -fomit-frame-pointer
LDFLAGS = -noixemul

# Directories
SRCDIR = .
BINDIR = bin

# Source files
SOURCES = pong.c graphics.c game.c input.c highscore.c
OBJECTS = $(SOURCES:.c=.o)

# Target
TARGET = $(BINDIR)/pong

# Default target
all: $(BINDIR) $(TARGET)

# Create bin directory
$(BINDIR):
	mkdir -p $(BINDIR)

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies
pong.o: pong.c graphics.h game.h input.h highscore.h
graphics.o: graphics.c graphics.h
game.o: game.c game.h graphics.h
input.o: input.c input.h
highscore.o: highscore.c highscore.h

# Clean
clean:
	rm -f *.o $(TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild
