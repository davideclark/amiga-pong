/*
 * game.h - Ball physics, collision, AI logic
 * Amiga Pong - OS-friendly implementation
 */

#ifndef GAME_H
#define GAME_H

#include <exec/types.h>

/* Fixed-point 8.8 format */
#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define INT_TO_FP(x) ((x) << FP_SHIFT)
#define FP_TO_INT(x) ((x) >> FP_SHIFT)

/* Game states */
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_HIGHSCORE_ENTRY
} GameState;

/* Ball structure with fixed-point position/velocity */
typedef struct {
    LONG x;      /* Fixed-point X position */
    LONG y;      /* Fixed-point Y position */
    LONG vx;     /* Fixed-point X velocity */
    LONG vy;     /* Fixed-point Y velocity */
} Ball;

/* Paddle structure */
typedef struct {
    WORD y;      /* Integer Y position (center) */
    WORD targetY; /* AI target position */
} Paddle;

/* Game context */
typedef struct {
    GameState state;
    Ball ball;
    Paddle playerPaddle;
    Paddle aiPaddle;
    WORD playerScore;
    WORD aiScore;
    WORD rallies;        /* Count of paddle hits for speed increase */
    BOOL servingPlayer;  /* TRUE if player serves */
} GameContext;

/* Initialize game state */
void InitGame(GameContext *ctx);

/* Reset ball to center with serve direction */
void ResetBall(GameContext *ctx);

/* Update game logic - called once per frame */
void UpdateGame(GameContext *ctx, WORD playerMouseY);

/* Check if game is over (someone reached 11) */
BOOL IsGameOver(GameContext *ctx);

/* Get winner (TRUE = player, FALSE = AI) */
BOOL PlayerWon(GameContext *ctx);

/* AI difficulty settings */
#define AI_REACTION_DELAY  4   /* Frames before AI starts moving */
#define AI_SPEED           5   /* Max pixels AI can move per frame */
#define AI_ERROR_MARGIN    24  /* Random error in prediction */

/* Ball speed settings */
#define BALL_INITIAL_SPEED INT_TO_FP(6)
#define BALL_MAX_SPEED     INT_TO_FP(12)
#define BALL_SPEED_INCREASE 48  /* Added each rally (fixed-point) */

/* Win condition */
#define WINNING_SCORE 11

#endif /* GAME_H */
