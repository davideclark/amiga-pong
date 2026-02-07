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

/* Difficulty names */
static const char *difficultyNames[3] = { "EASY", "MEDIUM", "HARD" };

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
static void DrawDifficultySelection(void);

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

    /* Load high scores and settings */
    LoadHighScores(&highScores);

    /* Initialize game systems */
    InitInput(&inputState);

    /* Apply saved difficulty before InitGame */
    gameCtx.difficulty = (Difficulty)highScores.difficulty;
    InitGame(&gameCtx);

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
            if (gameCtx.state == STATE_GAMEOVER ||
                gameCtx.state == STATE_HIGHSCORE_ENTRY ||
                gameCtx.state == STATE_TITLE ||
                gameCtx.state == STATE_PAUSED) {
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
                DrawDifficultySelection();
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

static void DrawDifficultySelection(void)
{
    WORD i;
    WORD x;
    const char *name;
    UBYTE color;

    /* Draw label: "Difficulty:" centered, 11 chars = 88 pixels, x = (320-88)/2 = 116 */
    DrawText(72, 140, "Difficulty: 1/2/3", COLOR_WHITE);  /* 17 chars */

    /* Draw difficulty options */
    /* "EASY  MEDIUM  HARD" with spacing */
    /* Easy at x=72, Medium at x=128, Hard at x=200 */
    for (i = 0; i < 3; i++) {
        name = difficultyNames[i];

        /* Position each option */
        if (i == 0) x = 80;       /* EASY: 4 chars */
        else if (i == 1) x = 128; /* MEDIUM: 6 chars */
        else x = 208;             /* HARD: 4 chars */

        /* Highlight current selection */
        if (i == (WORD)gameCtx.difficulty) {
            color = COLOR_YELLOW;
        } else {
            color = COLOR_CYAN;
        }

        DrawText(x, 152, name, color);
    }
}

static void HandleTitleInput(void)
{
    UBYTE key = inputState.lastKey;
    BOOL difficultyChanged = FALSE;

    if (inputState.events & INPUT_ESC) {
        /* Quit game */
        wantQuit = TRUE;
    } else if (inputState.events & INPUT_CLICK) {
        /* Start new game with current difficulty */
        SetDifficulty(&gameCtx, gameCtx.difficulty);
        gameCtx.state = STATE_PLAYING;
        gameCtx.playerScore = 0;
        gameCtx.aiScore = 0;
        ResetBall(&gameCtx);
        RequestFullRedraw();
    } else if (key == '1') {
        /* Easy difficulty */
        if (gameCtx.difficulty != DIFFICULTY_EASY) {
            gameCtx.difficulty = DIFFICULTY_EASY;
            difficultyChanged = TRUE;
        }
    } else if (key == '2') {
        /* Medium difficulty */
        if (gameCtx.difficulty != DIFFICULTY_MEDIUM) {
            gameCtx.difficulty = DIFFICULTY_MEDIUM;
            difficultyChanged = TRUE;
        }
    } else if (key == '3') {
        /* Hard difficulty */
        if (gameCtx.difficulty != DIFFICULTY_HARD) {
            gameCtx.difficulty = DIFFICULTY_HARD;
            difficultyChanged = TRUE;
        }
    }

    /* Save and redraw if difficulty changed */
    if (difficultyChanged) {
        highScores.difficulty = (UBYTE)gameCtx.difficulty;
        SaveHighScores(&highScores);
        ResetStaticScreen();  /* Force title screen redraw */
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
            ResetStaticScreen();  /* Redraw to show change */
        }
    } else if (key >= 32 && key < 127 && entryPos < NAME_LENGTH) {
        /* Printable character */
        entryName[entryPos] = (char)key;
        entryPos++;
        entryName[entryPos] = '\0';
        ResetStaticScreen();  /* Redraw to show new character */
    }
}

static void DrawHighScoreEntry(void)
{
    WORD i;
    char displayName[NAME_LENGTH + 2];

    /* Centered text: x = (320 - strlen*8) / 2 */
    DrawText(100, 60, "NEW HIGH SCORE!", COLOR_YELLOW);  /* 15 chars */
    DrawText(96, 90, "Enter your name:", COLOR_WHITE);   /* 16 chars */

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

    DrawText(128, 120, displayName, COLOR_CYAN);         /* 8 chars */
    DrawText(84, 170, "Press ENTER to save", COLOR_WHITE); /* 19 chars */
}

static void DrawHighScoreTable(void)
{
    WORD i;
    WORD y = 200;
    char line[32];
    char *p;
    WORD score;
    WORD digit;

    DrawText(116, 185, "HIGH SCORES", COLOR_YELLOW);  /* 11 chars, centered */

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
