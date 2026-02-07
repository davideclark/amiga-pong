/*
 * graphics.c - Screen setup and drawing
 * Amiga Pong - OS-friendly implementation
 *
 * Uses hardware sprites for ball and paddles - flicker-free!
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/rastport.h>
#include <graphics/sprite.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "graphics.h"

/* External library bases */
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;

/* Screen and window */
static struct Screen *gameScreen = NULL;
static struct Window *gameWindow = NULL;
static struct RastPort *screenRP = NULL;

/* Hardware sprites */
static struct SimpleSprite ballSprite;
static struct SimpleSprite playerSprite;
static struct SimpleSprite aiSprite;
static WORD ballSpriteNum = -1;
static WORD playerSpriteNum = -1;
static WORD aiSpriteNum = -1;

/* Sprite image data (must be in CHIP memory) */
static UWORD *ballSpriteData = NULL;
static UWORD *playerSpriteData = NULL;
static UWORD *aiSpriteData = NULL;

/* Blank pointer for hiding mouse */
static UWORD *blankPointer = NULL;

/* State tracking */
static BOOL staticScreenDrawn = FALSE;

/* Sprite data sizes */
#define BALL_SPRITE_HEIGHT 8
#define PADDLE_SPRITE_HEIGHT 36  /* 32 + some margin */

/* Screen colors (RGB4 format) */
static UWORD palette[8] = {
    0x000,  /* 0: Black - background */
    0xFFF,  /* 1: White - ball, player paddle */
    0x0CF,  /* 2: Cyan - AI paddle */
    0xFF0,  /* 3: Yellow - score text */
    0x444,  /* 4: Dark gray */
    0x888,  /* 5: Gray */
    0xF00,  /* 6: Red */
    0x0F0   /* 7: Green */
};

/* Digit patterns for score display (5x7 pixels) */
static const UBYTE digitPatterns[10][7] = {
    { 0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F },
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },
    { 0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F },
    { 0x1F, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x1F },
    { 0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01 },
    { 0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F },
    { 0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F },
    { 0x1F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F },
    { 0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F }
};

/* Create ball sprite data (8x8 white square) */
static UWORD *CreateBallSprite(void)
{
    UWORD *data;
    int i;

    /* Sprite format: 2 control words + height * 2 words + 2 terminator words */
    data = (UWORD *)AllocMem((2 + BALL_SPRITE_HEIGHT * 2 + 2) * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (!data) return NULL;

    /* Control words (will be set by system) */
    data[0] = 0;
    data[1] = 0;

    /* Image data - 8x8 solid block */
    /* Each line: plane0 word, plane1 word */
    /* For white (color 3 in sprite), both planes = 1 */
    for (i = 0; i < BALL_SPRITE_HEIGHT; i++) {
        data[2 + i * 2] = 0xFF00;     /* Plane 0: 8 pixels set */
        data[2 + i * 2 + 1] = 0xFF00; /* Plane 1: 8 pixels set */
    }

    /* Terminator */
    data[2 + BALL_SPRITE_HEIGHT * 2] = 0;
    data[2 + BALL_SPRITE_HEIGHT * 2 + 1] = 0;

    return data;
}

/* Create paddle sprite data (8x32 solid block) */
static UWORD *CreatePaddleSprite(BOOL isPlayer)
{
    UWORD *data;
    int i;
    UWORD plane0, plane1;

    data = (UWORD *)AllocMem((2 + PADDLE_SPRITE_HEIGHT * 2 + 2) * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (!data) return NULL;

    data[0] = 0;
    data[1] = 0;

    /* Player = white (color 3), AI = color 1 (will set sprite colors) */
    if (isPlayer) {
        plane0 = 0xFF00;
        plane1 = 0xFF00;
    } else {
        /* For AI, use color 1 in sprite palette */
        plane0 = 0xFF00;
        plane1 = 0x0000;
    }

    for (i = 0; i < PADDLE_SPRITE_HEIGHT; i++) {
        if (i >= 2 && i < PADDLE_SPRITE_HEIGHT - 2) {
            data[2 + i * 2] = plane0;
            data[2 + i * 2 + 1] = plane1;
        } else {
            data[2 + i * 2] = 0;
            data[2 + i * 2 + 1] = 0;
        }
    }

    data[2 + PADDLE_SPRITE_HEIGHT * 2] = 0;
    data[2 + PADDLE_SPRITE_HEIGHT * 2 + 1] = 0;

    return data;
}

static void FreeSprites(void)
{
    if (ballSpriteNum >= 0) {
        FreeSprite(ballSpriteNum);
        ballSpriteNum = -1;
    }
    if (playerSpriteNum >= 0) {
        FreeSprite(playerSpriteNum);
        playerSpriteNum = -1;
    }
    if (aiSpriteNum >= 0) {
        FreeSprite(aiSpriteNum);
        aiSpriteNum = -1;
    }
    if (ballSpriteData) {
        FreeMem(ballSpriteData, (2 + BALL_SPRITE_HEIGHT * 2 + 2) * sizeof(UWORD));
        ballSpriteData = NULL;
    }
    if (playerSpriteData) {
        FreeMem(playerSpriteData, (2 + PADDLE_SPRITE_HEIGHT * 2 + 2) * sizeof(UWORD));
        playerSpriteData = NULL;
    }
    if (aiSpriteData) {
        FreeMem(aiSpriteData, (2 + PADDLE_SPRITE_HEIGHT * 2 + 2) * sizeof(UWORD));
        aiSpriteData = NULL;
    }
}

static BOOL InitSprites(void)
{
    /* Create sprite image data */
    ballSpriteData = CreateBallSprite();
    playerSpriteData = CreatePaddleSprite(TRUE);
    aiSpriteData = CreatePaddleSprite(FALSE);

    if (!ballSpriteData || !playerSpriteData || !aiSpriteData) {
        FreeSprites();
        return FALSE;
    }

    /* Get sprites from system */
    ballSpriteNum = GetSprite(&ballSprite, 2);
    if (ballSpriteNum < 0) {
        /* Try another sprite number */
        ballSpriteNum = GetSprite(&ballSprite, -1);
    }

    playerSpriteNum = GetSprite(&playerSprite, 4);
    if (playerSpriteNum < 0) {
        playerSpriteNum = GetSprite(&playerSprite, -1);
    }

    aiSpriteNum = GetSprite(&aiSprite, 6);
    if (aiSpriteNum < 0) {
        aiSpriteNum = GetSprite(&aiSprite, -1);
    }

    if (ballSpriteNum < 0 || playerSpriteNum < 0 || aiSpriteNum < 0) {
        FreeSprites();
        return FALSE;
    }

    /* Set up sprite structures */
    ballSprite.posctldata = ballSpriteData;
    ballSprite.height = BALL_SPRITE_HEIGHT;
    ballSprite.x = 0;
    ballSprite.y = 0;

    playerSprite.posctldata = playerSpriteData;
    playerSprite.height = PADDLE_SPRITE_HEIGHT;
    playerSprite.x = 0;
    playerSprite.y = 0;

    aiSprite.posctldata = aiSpriteData;
    aiSprite.height = PADDLE_SPRITE_HEIGHT;
    aiSprite.x = 0;
    aiSprite.y = 0;

    /* Set sprite colors */
    /* Sprites 2-3 use colors 21-23, sprites 4-5 use 25-27, sprites 6-7 use 29-31 */
    /* But for SimpleSprite, colors are set in the ViewPort */

    return TRUE;
}

BOOL InitGraphics(void)
{
    struct NewScreen ns;
    struct NewWindow nw;
    int i;

    blankPointer = (UWORD *)AllocMem(12, MEMF_CHIP | MEMF_CLEAR);
    if (!blankPointer) return FALSE;

    /* Open screen */
    ns.LeftEdge = 0;
    ns.TopEdge = 0;
    ns.Width = SCREEN_WIDTH;
    ns.Height = SCREEN_HEIGHT;
    ns.Depth = SCREEN_DEPTH;
    ns.DetailPen = COLOR_WHITE;
    ns.BlockPen = COLOR_BACKGROUND;
    ns.ViewModes = 0;
    ns.Type = CUSTOMSCREEN | SCREENQUIET;
    ns.Font = NULL;
    ns.DefaultTitle = NULL;
    ns.Gadgets = NULL;
    ns.CustomBitMap = NULL;

    gameScreen = OpenScreen(&ns);
    if (!gameScreen) {
        CleanupGraphics();
        return FALSE;
    }

    screenRP = &gameScreen->RastPort;

    /* Set playfield palette */
    for (i = 0; i < 8; i++) {
        SetRGB4(&gameScreen->ViewPort, i,
                (palette[i] >> 8) & 0xF,
                (palette[i] >> 4) & 0xF,
                palette[i] & 0xF);
    }

    /* Set sprite colors */
    /* Sprite 2-3 colors are at index 21-23 */
    SetRGB4(&gameScreen->ViewPort, 21, 0xF, 0xF, 0xF);  /* White */
    SetRGB4(&gameScreen->ViewPort, 22, 0xF, 0xF, 0xF);  /* White */
    SetRGB4(&gameScreen->ViewPort, 23, 0xF, 0xF, 0xF);  /* White */

    /* Sprite 4-5 colors at 25-27 */
    SetRGB4(&gameScreen->ViewPort, 25, 0xF, 0xF, 0xF);  /* White */
    SetRGB4(&gameScreen->ViewPort, 26, 0xF, 0xF, 0xF);  /* White */
    SetRGB4(&gameScreen->ViewPort, 27, 0xF, 0xF, 0xF);  /* White */

    /* Sprite 6-7 colors at 29-31 */
    SetRGB4(&gameScreen->ViewPort, 29, 0x0, 0xC, 0xF);  /* Cyan */
    SetRGB4(&gameScreen->ViewPort, 30, 0x0, 0xC, 0xF);  /* Cyan */
    SetRGB4(&gameScreen->ViewPort, 31, 0x0, 0xC, 0xF);  /* Cyan */

    /* Initialize sprites */
    if (!InitSprites()) {
        CleanupGraphics();
        return FALSE;
    }

    /* Clear screen */
    SetRast(screenRP, COLOR_BACKGROUND);

    /* Open window */
    nw.LeftEdge = 0;
    nw.TopEdge = 0;
    nw.Width = SCREEN_WIDTH;
    nw.Height = SCREEN_HEIGHT;
    nw.DetailPen = COLOR_WHITE;
    nw.BlockPen = COLOR_BACKGROUND;
    nw.IDCMPFlags = IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS |
                    IDCMP_RAWKEY | IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW;
    nw.Flags = WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_ACTIVATE |
               WFLG_RMBTRAP | WFLG_REPORTMOUSE;
    nw.FirstGadget = NULL;
    nw.CheckMark = NULL;
    nw.Title = NULL;
    nw.Screen = gameScreen;
    nw.BitMap = NULL;
    nw.MinWidth = 0;
    nw.MinHeight = 0;
    nw.MaxWidth = 0;
    nw.MaxHeight = 0;
    nw.Type = CUSTOMSCREEN;

    gameWindow = OpenWindow(&nw);
    if (!gameWindow) {
        CleanupGraphics();
        return FALSE;
    }

    SetPointer(gameWindow, blankPointer, 1, 1, 0, 0);
    staticScreenDrawn = FALSE;

    return TRUE;
}

void CleanupGraphics(void)
{
    FreeSprites();

    if (gameWindow) {
        ClearPointer(gameWindow);
        CloseWindow(gameWindow);
        gameWindow = NULL;
    }
    if (gameScreen) {
        CloseScreen(gameScreen);
        gameScreen = NULL;
    }
    if (blankPointer) {
        FreeMem(blankPointer, 12);
        blankPointer = NULL;
    }
    screenRP = NULL;
}

void SwapBuffers(void)
{
    WaitTOF();
}

void ClearDisplay(void)
{
    SetRast(screenRP, COLOR_BACKGROUND);
}

void ClearBackBuffer(void)
{
    ClearDisplay();
    staticScreenDrawn = FALSE;
}

struct RastPort *GetBackRastPort(void) { return screenRP; }
struct Window *GetGameWindow(void) { return gameWindow; }

void DrawPaddle(WORD x, WORD y, UBYTE color)
{
    /* Paddles are now sprites - this is just for static screens */
    WORD top = y - PADDLE_HEIGHT / 2;
    WORD bottom = y + PADDLE_HEIGHT / 2;
    if (top < 0) top = 0;
    if (bottom >= SCREEN_HEIGHT) bottom = SCREEN_HEIGHT - 1;
    SetAPen(screenRP, color);
    RectFill(screenRP, x, top, x + PADDLE_WIDTH - 1, bottom);
}

void DrawBall(WORD x, WORD y)
{
    /* Ball is now a sprite - this is just for static screens */
    WORD left = x - BALL_SIZE / 2;
    WORD top = y - BALL_SIZE / 2;
    WORD right = x + BALL_SIZE / 2;
    WORD bottom = y + BALL_SIZE / 2;

    if (right < 0 || left >= SCREEN_WIDTH || bottom < 0 || top >= SCREEN_HEIGHT) return;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= SCREEN_WIDTH) right = SCREEN_WIDTH - 1;
    if (bottom >= SCREEN_HEIGHT) bottom = SCREEN_HEIGHT - 1;

    SetAPen(screenRP, COLOR_WHITE);
    RectFill(screenRP, left, top, right, bottom);
}

void DrawCenterLine(void)
{
    WORD y, x = SCREEN_WIDTH / 2;
    SetAPen(screenRP, COLOR_WHITE);
    for (y = 0; y < SCREEN_HEIGHT; y += 8) {
        RectFill(screenRP, x - 1, y, x + 1, y + 3);
    }
}

static void DrawDigit(WORD x, WORD y, WORD digit, UBYTE color)
{
    WORD row, col;
    UBYTE pattern;
    if (digit < 0 || digit > 9) return;

    SetAPen(screenRP, color);
    for (row = 0; row < 7; row++) {
        pattern = digitPatterns[digit][row];
        for (col = 0; col < 5; col++) {
            if (pattern & (0x10 >> col)) {
                RectFill(screenRP, x + col * 3, y + row * 3, x + col * 3 + 1, y + row * 3 + 1);
            }
        }
    }
}

void DrawScore(WORD playerScore, WORD aiScore)
{
    WORD px = SCREEN_WIDTH / 4 - 10;
    WORD ax = 3 * SCREEN_WIDTH / 4 - 10;
    WORD y = 16;

    /* Clear score areas first */
    SetAPen(screenRP, COLOR_BACKGROUND);
    RectFill(screenRP, 40, 14, 120, 40);
    RectFill(screenRP, 200, 14, 280, 40);

    if (playerScore >= 10) DrawDigit(px - 18, y, playerScore / 10, COLOR_YELLOW);
    DrawDigit(px, y, playerScore % 10, COLOR_YELLOW);
    if (aiScore >= 10) DrawDigit(ax - 18, y, aiScore / 10, COLOR_YELLOW);
    DrawDigit(ax, y, aiScore % 10, COLOR_YELLOW);
}

void DrawText(WORD x, WORD y, const char *text, UBYTE color)
{
    WORD len = 0;
    const char *p = text;
    while (*p++) len++;
    SetAPen(screenRP, color);
    Move(screenRP, x, y);
    Text(screenRP, text, len);
}

void DrawTitleScreen(void)
{
    /* Hide sprites on title screen */
    MoveSprite(&gameScreen->ViewPort, &ballSprite, -100, 0);
    MoveSprite(&gameScreen->ViewPort, &playerSprite, -100, 0);
    MoveSprite(&gameScreen->ViewPort, &aiSprite, -100, 0);

    /* Centered text: x = (320 - strlen*8) / 2 */
    DrawText(144, 80, "PONG", COLOR_WHITE);           /* 4 chars */
    DrawText(104, 120, "Click to Start", COLOR_YELLOW); /* 14 chars */
    DrawText(116, 160, "ESC to Quit", COLOR_CYAN);    /* 11 chars */
}

void DrawPausedText(void)
{
    /* Centered text: x = (320 - strlen*8) / 2 */
    DrawText(136, 128, "PAUSED", COLOR_YELLOW);       /* 6 chars */
    DrawText(100, 160, "Click to Resume", COLOR_WHITE); /* 15 chars */
}

void DrawGameOver(BOOL playerWon)
{
    /* Hide sprites */
    MoveSprite(&gameScreen->ViewPort, &ballSprite, -100, 0);
    MoveSprite(&gameScreen->ViewPort, &playerSprite, -100, 0);
    MoveSprite(&gameScreen->ViewPort, &aiSprite, -100, 0);

    /* Centered text: x = (320 - strlen*8) / 2 */
    DrawText(124, 100, "GAME OVER", COLOR_YELLOW);    /* 9 chars */
    DrawText(128, 130, playerWon ? "YOU WIN!" : "CPU WINS", playerWon ? COLOR_WHITE : COLOR_CYAN); /* 8 chars */
    DrawText(92, 180, "Click to Continue", COLOR_WHITE); /* 17 chars */
}

BOOL DrawStaticScreen(void)
{
    if (!staticScreenDrawn) {
        staticScreenDrawn = TRUE;
        return TRUE;
    }
    WaitTOF();
    return FALSE;
}

void ResetStaticScreen(void)
{
    staticScreenDrawn = FALSE;
}

/* Update game graphics using hardware sprites */
void UpdateGameGraphics(WORD ballX, WORD ballY, WORD playerY, WORD aiY,
                        WORD playerScore, WORD aiScore)
{
    static WORD lastPlayerScore = -1;
    static WORD lastAIScore = -1;
    static BOOL firstFrame = TRUE;

    /* First frame: draw static elements */
    if (firstFrame) {
        SetRast(screenRP, COLOR_BACKGROUND);
        DrawCenterLine();
        DrawScore(playerScore, aiScore);
        lastPlayerScore = playerScore;
        lastAIScore = aiScore;
        firstFrame = FALSE;
    }

    /* Update score if changed */
    if (playerScore != lastPlayerScore || aiScore != lastAIScore) {
        DrawScore(playerScore, aiScore);
        lastPlayerScore = playerScore;
        lastAIScore = aiScore;
    }

    /* Wait for bottom of viewport before moving sprites */
    WaitBOVP(&gameScreen->ViewPort);

    /* Move sprites during VBlank - no trail artifacts! */
    MoveSprite(&gameScreen->ViewPort, &ballSprite,
               ballX - BALL_SIZE/2, ballY - BALL_SIZE/2);
    MoveSprite(&gameScreen->ViewPort, &playerSprite,
               PADDLE_OFFSET, playerY - PADDLE_HEIGHT/2);
    MoveSprite(&gameScreen->ViewPort, &aiSprite,
               SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH, aiY - PADDLE_HEIGHT/2);
}

void RequestFullRedraw(void)
{
    /* Hack to reset first frame flag - just clear and redraw */
    SetRast(screenRP, COLOR_BACKGROUND);
    DrawCenterLine();
    staticScreenDrawn = FALSE;
}

void EraseBallAt(WORD x, WORD y) { (void)x; (void)y; }
