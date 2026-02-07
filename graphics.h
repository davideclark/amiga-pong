/*
 * graphics.h - Screen setup, double buffering, drawing
 * Amiga Pong - OS-friendly implementation
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>

/* Screen dimensions */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 256
#define SCREEN_DEPTH  3   /* 8 colors */

/* Color indices */
#define COLOR_BACKGROUND 0
#define COLOR_WHITE      1
#define COLOR_CYAN       2
#define COLOR_YELLOW     3

/* Game element dimensions */
#define PADDLE_WIDTH   8
#define PADDLE_HEIGHT  32
#define BALL_SIZE      6
#define PADDLE_OFFSET  16  /* Distance from screen edge */

/* Initialize graphics system */
BOOL InitGraphics(void);

/* Clean up graphics system */
void CleanupGraphics(void);

/* Swap double buffers */
void SwapBuffers(void);

/* Clear the back buffer */
void ClearBackBuffer(void);

/* Just clear the screen (for static screens) */
void ClearDisplay(void);

/* Drawing functions */
void DrawPaddle(WORD x, WORD y, UBYTE color);
void DrawBall(WORD x, WORD y);
void DrawCenterLine(void);
void DrawScore(WORD playerScore, WORD aiScore);
void DrawText(WORD x, WORD y, const char *text, UBYTE color);
void DrawTitleScreen(void);
void DrawPausedText(void);
void DrawGameOver(BOOL playerWon);

/* Get the window for input */
struct Window *GetGameWindow(void);

/* Get current RastPort for drawing */
struct RastPort *GetBackRastPort(void);

/* Optimized game rendering - erases and redraws only what changed */
void UpdateGameGraphics(WORD ballX, WORD ballY, WORD playerY, WORD aiY,
                        WORD playerScore, WORD aiScore);

/* Request a full screen redraw on next frame */
void RequestFullRedraw(void);

/* Erase ball at specific position (for when ball goes off screen) */
void EraseBallAt(WORD x, WORD y);

/* For static screens (title, pause, etc) - returns TRUE if caller should draw */
BOOL DrawStaticScreen(void);
void ResetStaticScreen(void);

#endif /* GRAPHICS_H */
