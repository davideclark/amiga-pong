/*
 * pong.c - Main entry, library init, game loop
 * Amiga Pong - OS-friendly implementation
 *
 * A classic Pong game for Amiga 500
 * - Mouse controlled player paddle
 * - AI opponent
 * - Persistent high scores
 * - OS-friendly (no hardware takeover)
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "graphics.h"
#include "game.h"
#include "input.h"
#include "highscore.h"

/* Library bases */
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;

/* Game data */
static GameContext gameCtx;
static InputState inputState;
static HighScoreTable highScores;

/* Name entry state */
static char entryName[NAME_LENGTH + 1];
static WORD entryPos = 0;

/* Quit flag */
static BOOL wantQuit = FALSE;

/* Forward declarations */
static BOOL OpenLibraries(void);
static void CloseLibraries(void);
static void GameLoop(void);
static void RenderFrame(void);
static void HandleTitleInput(void);
static void HandlePlayingInput(void);
static void HandlePausedInput(void);
static void HandleGameOverInput(void);
static void HandleHighScoreEntry(void);
static void DrawHighScoreEntry(void);
static void DrawHighScoreTable(void);

int main(void)
{
    /* Open libraries */
    if (!OpenLibraries()) {
        return 20;
    }

    /* Initialize graphics */
    if (!InitGraphics()) {
        CloseLibraries();
        return 20;
    }

    /* Initialize game systems */
    InitInput(&inputState);
    InitGame(&gameCtx);
    LoadHighScores(&highScores);

    /* Main game loop */
    GameLoop();

    /* Cleanup */
    CleanupGraphics();
    CloseLibraries();

    return 0;
}

static BOOL OpenLibraries(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase) {
        return FALSE;
    }

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
        return FALSE;
    }

    return TRUE;
}

static void CloseLibraries(void)
{
    if (GfxBase) {
        CloseLibrary((struct Library *)GfxBase);
        GfxBase = NULL;
    }

    if (IntuitionBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase = NULL;
    }
}

static void GameLoop(void)
{
    BOOL running = TRUE;
    struct Window *window = GetGameWindow();
    GameState prevState;

    wantQuit = FALSE;

    while (running) {
        /* Process input */
        ProcessInput(window, &inputState);

        /* Check for close request or quit flag */
        if ((inputState.events & INPUT_CLOSE) || wantQuit) {
            running = FALSE;
            continue;
        }

        /* Remember state before handling input */
        prevState = gameCtx.state;

        /* Handle input based on game state */
        switch (gameCtx.state) {
            case STATE_TITLE:
                HandleTitleInput();
                break;

            case STATE_PLAYING:
                HandlePlayingInput();
                UpdateGame(&gameCtx, inputState.mouseY);
                break;

            case STATE_PAUSED:
                HandlePausedInput();
                break;

            case STATE_GAMEOVER:
                HandleGameOverInput();
                break;

            case STATE_HIGHSCORE_ENTRY:
                HandleHighScoreEntry();
                break;
        }

        /* If state changed to a static screen, reset it */
        if (gameCtx.state != prevState) {
            if (gameCtx.state == STATE_GAMEOVER) {
                ResetStaticScreen();
            }
        }

        /* Render and swap buffers */
        RenderFrame();
    }
}

static void RenderFrame(void)
{
    /* Draw based on game state */
    switch (gameCtx.state) {
        case STATE_TITLE:
            /* Only draw title screen once, then just wait */
            if (DrawStaticScreen()) {
                ClearDisplay();
                DrawTitleScreen();
                DrawHighScoreTable();
            }
            break;

        case STATE_PLAYING:
            /* Use optimized rendering - only redraws what changed */
            UpdateGameGraphics(
                FP_TO_INT(gameCtx.ball.x), FP_TO_INT(gameCtx.ball.y),
                gameCtx.playerPaddle.y, gameCtx.aiPaddle.y,
                gameCtx.playerScore, gameCtx.aiScore);
            break;

        case STATE_PAUSED:
            /* Only draw paused screen once */
            if (DrawStaticScreen()) {
                ClearDisplay();
                DrawCenterLine();
                DrawScore(gameCtx.playerScore, gameCtx.aiScore);
                DrawPaddle(PADDLE_OFFSET, gameCtx.playerPaddle.y, COLOR_WHITE);
                DrawPaddle(SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH,
                          gameCtx.aiPaddle.y, COLOR_CYAN);
                DrawBall(FP_TO_INT(gameCtx.ball.x), FP_TO_INT(gameCtx.ball.y));
                DrawPausedText();
            }
            break;

        case STATE_GAMEOVER:
            /* Only draw game over screen once */
            if (DrawStaticScreen()) {
                ClearDisplay();
                DrawCenterLine();
                DrawScore(gameCtx.playerScore, gameCtx.aiScore);
                DrawGameOver(PlayerWon(&gameCtx));
            }
            break;

        case STATE_HIGHSCORE_ENTRY:
            /* High score entry needs redraw for cursor */
            if (DrawStaticScreen()) {
                ClearDisplay();
                DrawHighScoreEntry();
            }
            break;
    }
}

static void HandleTitleInput(void)
{
    if (inputState.events & INPUT_ESC) {
        /* Quit game */
        wantQuit = TRUE;
    } else if (inputState.events & INPUT_CLICK) {
        /* Start new game */
        gameCtx.state = STATE_PLAYING;
        gameCtx.playerScore = 0;
        gameCtx.aiScore = 0;
        ResetBall(&gameCtx);
        RequestFullRedraw();
    }
}

static void HandlePlayingInput(void)
{
    if (inputState.events & INPUT_ESC) {
        gameCtx.state = STATE_PAUSED;
        ResetStaticScreen();
    }
}

static void HandlePausedInput(void)
{
    if (inputState.events & INPUT_CLICK) {
        /* Resume game */
        gameCtx.state = STATE_PLAYING;
        RequestFullRedraw();
    } else if (inputState.events & INPUT_ESC) {
        /* Quit to title */
        gameCtx.state = STATE_TITLE;
        InitGame(&gameCtx);
        ResetStaticScreen();
    }
}

static void HandleGameOverInput(void)
{
    if (inputState.events & INPUT_CLICK) {
        /* Check for high score */
        if (PlayerWon(&gameCtx) && IsHighScore(&highScores, gameCtx.playerScore)) {
            /* Start name entry */
            gameCtx.state = STATE_HIGHSCORE_ENTRY;
            entryPos = 0;
            {
                WORD i;
                for (i = 0; i <= NAME_LENGTH; i++) {
                    entryName[i] = '\0';
                }
            }
        } else {
            /* Return to title */
            gameCtx.state = STATE_TITLE;
            InitGame(&gameCtx);
            ResetStaticScreen();
        }
    }
}

static void HandleHighScoreEntry(void)
{
    UBYTE key = inputState.lastKey;

    if (key == 0) return;

    if (key == 13 || key == 10) {
        /* Enter pressed - save score */
        if (entryPos > 0) {
            AddHighScore(&highScores, entryName, gameCtx.playerScore);
        }
        gameCtx.state = STATE_TITLE;
        InitGame(&gameCtx);
        ResetStaticScreen();
    } else if (key == 8 || key == 127) {
        /* Backspace */
        if (entryPos > 0) {
            entryPos--;
            entryName[entryPos] = '\0';
        }
    } else if (key >= 32 && key < 127 && entryPos < NAME_LENGTH) {
        /* Printable character */
        entryName[entryPos] = (char)key;
        entryPos++;
        entryName[entryPos] = '\0';
    }
}

static void DrawHighScoreEntry(void)
{
    WORD i;
    char displayName[NAME_LENGTH + 2];

    DrawText(80, 60, "NEW HIGH SCORE!", COLOR_YELLOW);
    DrawText(60, 90, "Enter your name:", COLOR_WHITE);

    /* Build display string with cursor */
    for (i = 0; i < NAME_LENGTH; i++) {
        if (i < entryPos) {
            displayName[i] = entryName[i];
        } else if (i == entryPos) {
            displayName[i] = '_';
        } else {
            displayName[i] = '.';
        }
    }
    displayName[NAME_LENGTH] = '\0';

    DrawText(120, 120, displayName, COLOR_CYAN);
    DrawText(60, 170, "Press ENTER to save", COLOR_WHITE);
}

static void DrawHighScoreTable(void)
{
    WORD i;
    WORD y = 190;
    char line[32];
    char *p;
    WORD score;
    WORD digit;

    DrawText(100, 175, "HIGH SCORES", COLOR_YELLOW);

    for (i = 0; i < MAX_HIGHSCORES; i++) {
        if (highScores.entries[i].score > 0) {
            /* Build score display string manually (no sprintf) */
            p = line;

            /* Rank */
            *p++ = '1' + i;
            *p++ = '.';
            *p++ = ' ';

            /* Copy name */
            {
                const char *src = highScores.entries[i].name;
                WORD j;
                for (j = 0; j < NAME_LENGTH && src[j]; j++) {
                    *p++ = src[j];
                }
            }

            *p++ = ' ';

            /* Score (max 11, so 2 digits) */
            score = highScores.entries[i].score;
            if (score >= 10) {
                digit = score / 10;
                *p++ = '0' + digit;
                score = score % 10;
            }
            *p++ = '0' + score;

            *p = '\0';

            DrawText(80, y, line, COLOR_WHITE);
            y += 10;
        }
    }
}
