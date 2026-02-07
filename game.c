/*
 * game.c - Ball physics, collision, AI logic
 * Amiga Pong - OS-friendly implementation
 */

#include <exec/types.h>
#include "game.h"
#include "graphics.h"

/* Simple pseudo-random number generator */
static ULONG randomSeed = 12345;

static WORD Random(WORD max)
{
    if (max <= 0) return 0;
    randomSeed = randomSeed * 1103515245 + 12345;
    return (WORD)((randomSeed >> 16) % (UWORD)max);
}

/* AI difficulty settings per level */
typedef struct {
    WORD speed;          /* Max pixels AI can move per frame */
    WORD errorMargin;    /* Random error in prediction */
    WORD updateInterval; /* Frames between target recalculation */
} AISettings;

static const AISettings difficultySettings[3] = {
    { 3, 40, 20 },  /* EASY: slow, inaccurate, updates rarely */
    { 4, 24, 12 },  /* MEDIUM: moderate speed and accuracy */
    { 6, 8,  6 }    /* HARD: fast, accurate, updates frequently */
};

/* Current AI settings (set by difficulty) */
static AISettings currentAI = { 4, 24, 12 };

void SetDifficulty(GameContext *ctx, Difficulty diff)
{
    if (diff > DIFFICULTY_HARD) diff = DIFFICULTY_MEDIUM;
    ctx->difficulty = diff;
    currentAI = difficultySettings[diff];
}

void InitGame(GameContext *ctx)
{
    ctx->state = STATE_TITLE;
    ctx->playerScore = 0;
    ctx->aiScore = 0;
    ctx->rallies = 0;
    ctx->servingPlayer = TRUE;
    ctx->aiUpdateTimer = 0;

    /* Default to medium if not set */
    if (ctx->difficulty > DIFFICULTY_HARD) {
        ctx->difficulty = DIFFICULTY_MEDIUM;
    }
    currentAI = difficultySettings[ctx->difficulty];

    /* Center paddles */
    ctx->playerPaddle.y = SCREEN_HEIGHT / 2;
    ctx->playerPaddle.targetY = SCREEN_HEIGHT / 2;
    ctx->aiPaddle.y = SCREEN_HEIGHT / 2;
    ctx->aiPaddle.targetY = SCREEN_HEIGHT / 2;

    ResetBall(ctx);
}

void ResetBall(GameContext *ctx)
{
    LONG speed;
    LONG angle;

    /* Center the ball */
    ctx->ball.x = INT_TO_FP(SCREEN_WIDTH / 2);
    ctx->ball.y = INT_TO_FP(SCREEN_HEIGHT / 2);

    /* Reset rally count and speed */
    ctx->rallies = 0;
    speed = BALL_INITIAL_SPEED;

    /* Random vertical angle (-1 to 1 in fixed point) */
    angle = INT_TO_FP(Random(256) - 128) / 128;

    /* Set velocity based on who's serving */
    if (ctx->servingPlayer) {
        ctx->ball.vx = -speed;  /* Towards player */
    } else {
        ctx->ball.vx = speed;   /* Towards AI */
    }

    ctx->ball.vy = angle;

    /* Reset AI update timer so it recalculates on next serve */
    ctx->aiUpdateTimer = 0;
}

/* Clamp a value to a range */
static WORD Clamp(WORD value, WORD min, WORD max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/* Absolute value */
static LONG Abs(LONG x)
{
    return (x < 0) ? -x : x;
}

/* Absolute value for WORD */
static WORD AbsW(WORD x)
{
    return (x < 0) ? -x : x;
}

/* Update AI paddle */
static void UpdateAI(GameContext *ctx)
{
    WORD diff;

    /* Only recalculate target periodically to reduce jitter */
    ctx->aiUpdateTimer++;
    if (ctx->aiUpdateTimer >= currentAI.updateInterval) {
        ctx->aiUpdateTimer = 0;

        /* Only track when ball is moving towards AI */
        if (ctx->ball.vx > 0) {
            /* Predict where ball will be when it reaches AI paddle */
            LONG timeToReach;
            LONG aiPaddleX = INT_TO_FP(SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH);
            LONG vxShifted;
            WORD predictedY;
            WORD error;

            /* Safe division - avoid divide by zero */
            vxShifted = ctx->ball.vx >> 4;
            if (vxShifted < 1) vxShifted = 1;

            timeToReach = (aiPaddleX - ctx->ball.x) / vxShifted;
            if (timeToReach < 0) timeToReach = 0;
            if (timeToReach > 128) timeToReach = 128;

            predictedY = FP_TO_INT(ctx->ball.y + (ctx->ball.vy * timeToReach) / 16);

            /* Add some error based on difficulty */
            if (currentAI.errorMargin > 0) {
                error = Random(currentAI.errorMargin * 2 + 1) - currentAI.errorMargin;
                predictedY += error;
            }

            /* Clamp prediction to screen */
            predictedY = Clamp(predictedY, PADDLE_HEIGHT / 2,
                              SCREEN_HEIGHT - PADDLE_HEIGHT / 2);

            ctx->aiPaddle.targetY = predictedY;
        } else {
            /* Ball moving away - return to center slowly */
            ctx->aiPaddle.targetY = SCREEN_HEIGHT / 2;
        }
    }

    /* Calculate difference to target */
    diff = ctx->aiPaddle.targetY - ctx->aiPaddle.y;

    /* Dead zone: don't move if close enough to target (reduces jitter) */
    if (AbsW(diff) <= AI_DEAD_ZONE) {
        return;  /* Already close enough, don't move */
    }

    /* Move towards target with limited speed */
    if (diff > currentAI.speed) {
        ctx->aiPaddle.y += currentAI.speed;
    } else if (diff < -currentAI.speed) {
        ctx->aiPaddle.y -= currentAI.speed;
    } else {
        ctx->aiPaddle.y = ctx->aiPaddle.targetY;
    }

    /* Clamp to screen bounds */
    ctx->aiPaddle.y = Clamp(ctx->aiPaddle.y, PADDLE_HEIGHT / 2,
                            SCREEN_HEIGHT - PADDLE_HEIGHT / 2);
}

/* Check paddle collision and return TRUE if hit */
static BOOL CheckPaddleCollision(GameContext *ctx, WORD paddleX, WORD paddleY)
{
    WORD ballX = FP_TO_INT(ctx->ball.x);
    WORD ballY = FP_TO_INT(ctx->ball.y);
    WORD ballLeft = ballX - BALL_SIZE / 2;
    WORD ballRight = ballX + BALL_SIZE / 2;
    WORD ballTop = ballY - BALL_SIZE / 2;
    WORD ballBottom = ballY + BALL_SIZE / 2;
    WORD paddleTop = paddleY - PADDLE_HEIGHT / 2;
    WORD paddleBottom = paddleY + PADDLE_HEIGHT / 2;
    WORD paddleRight = paddleX + PADDLE_WIDTH;

    /* Check overlap */
    if (ballRight >= paddleX && ballLeft <= paddleRight &&
        ballBottom >= paddleTop && ballTop <= paddleBottom) {
        return TRUE;
    }

    return FALSE;
}

/* Calculate spin based on where ball hit paddle */
static LONG CalculateSpin(WORD ballY, WORD paddleY)
{
    WORD offset = ballY - paddleY;
    LONG spin;

    /* Offset ranges from -PADDLE_HEIGHT/2 to +PADDLE_HEIGHT/2 */
    /* Convert to fixed-point spin (-2 to +2) */
    spin = (offset * INT_TO_FP(3)) / (PADDLE_HEIGHT / 2);

    /* Clamp spin */
    if (spin > INT_TO_FP(3)) spin = INT_TO_FP(3);
    if (spin < INT_TO_FP(-3)) spin = INT_TO_FP(-3);

    return spin;
}

void UpdateGame(GameContext *ctx, WORD playerMouseY)
{
    WORD ballX, ballY;
    LONG speed;

    if (ctx->state != STATE_PLAYING) {
        return;
    }

    /* Update player paddle to follow mouse */
    ctx->playerPaddle.y = Clamp(playerMouseY, PADDLE_HEIGHT / 2,
                                 SCREEN_HEIGHT - PADDLE_HEIGHT / 2);

    /* Update AI */
    UpdateAI(ctx);

    /* Move ball */
    ctx->ball.x += ctx->ball.vx;
    ctx->ball.y += ctx->ball.vy;

    ballX = FP_TO_INT(ctx->ball.x);
    ballY = FP_TO_INT(ctx->ball.y);

    /* Wall collision (top/bottom) */
    /* Keep ball below score area (y=48 minimum to stay below scores at y=45) */
    if (ballY - BALL_SIZE / 2 <= 48) {
        ctx->ball.y = INT_TO_FP(48 + BALL_SIZE / 2);
        ctx->ball.vy = -ctx->ball.vy;
    } else if (ballY + BALL_SIZE / 2 >= SCREEN_HEIGHT) {
        ctx->ball.y = INT_TO_FP(SCREEN_HEIGHT - BALL_SIZE / 2);
        ctx->ball.vy = -ctx->ball.vy;
    }

    /* Player paddle collision */
    if (ctx->ball.vx < 0 && ballX < SCREEN_WIDTH / 2) {
        if (CheckPaddleCollision(ctx, PADDLE_OFFSET, ctx->playerPaddle.y)) {
            /* Bounce with spin */
            ctx->ball.vx = -ctx->ball.vx;
            ctx->ball.x = INT_TO_FP(PADDLE_OFFSET + PADDLE_WIDTH + BALL_SIZE / 2);
            ctx->ball.vy += CalculateSpin(ballY, ctx->playerPaddle.y);

            /* Speed up */
            ctx->rallies++;
            speed = Abs(ctx->ball.vx) + BALL_SPEED_INCREASE;
            if (speed > BALL_MAX_SPEED) speed = BALL_MAX_SPEED;
            ctx->ball.vx = speed;

            /* Reset AI timer so it recalculates after player hit */
            ctx->aiUpdateTimer = currentAI.updateInterval;
        }
    }

    /* AI paddle collision */
    if (ctx->ball.vx > 0 && ballX > SCREEN_WIDTH / 2) {
        if (CheckPaddleCollision(ctx, SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH,
                                  ctx->aiPaddle.y)) {
            /* Bounce with spin */
            ctx->ball.vx = -ctx->ball.vx;
            ctx->ball.x = INT_TO_FP(SCREEN_WIDTH - PADDLE_OFFSET - PADDLE_WIDTH - BALL_SIZE / 2);
            ctx->ball.vy += CalculateSpin(ballY, ctx->aiPaddle.y);

            /* Speed up */
            ctx->rallies++;
            speed = Abs(ctx->ball.vx) + BALL_SPEED_INCREASE;
            if (speed > BALL_MAX_SPEED) speed = BALL_MAX_SPEED;
            ctx->ball.vx = -speed;
        }
    }

    /* Clamp vertical velocity */
    if (ctx->ball.vy > INT_TO_FP(4)) ctx->ball.vy = INT_TO_FP(4);
    if (ctx->ball.vy < INT_TO_FP(-4)) ctx->ball.vy = INT_TO_FP(-4);

    /* Scoring - with safety bounds check */
    if (ballX < -BALL_SIZE || ctx->ball.x < INT_TO_FP(-50)) {
        /* AI scores */
        ctx->aiScore++;
        ctx->servingPlayer = TRUE;
        ResetBall(ctx);

        if (ctx->aiScore >= WINNING_SCORE) {
            ctx->state = STATE_GAMEOVER;
        }
    } else if (ballX > SCREEN_WIDTH + BALL_SIZE || ctx->ball.x > INT_TO_FP(SCREEN_WIDTH + 50)) {
        /* Player scores */
        ctx->playerScore++;
        ctx->servingPlayer = FALSE;
        ResetBall(ctx);

        if (ctx->playerScore >= WINNING_SCORE) {
            ctx->state = STATE_GAMEOVER;
        }
    }

    /* Safety: clamp ball Y to reasonable bounds */
    if (ctx->ball.y < INT_TO_FP(-50)) ctx->ball.y = INT_TO_FP(-50);
    if (ctx->ball.y > INT_TO_FP(SCREEN_HEIGHT + 50)) ctx->ball.y = INT_TO_FP(SCREEN_HEIGHT + 50);
}

BOOL IsGameOver(GameContext *ctx)
{
    return (ctx->playerScore >= WINNING_SCORE || ctx->aiScore >= WINNING_SCORE);
}

BOOL PlayerWon(GameContext *ctx)
{
    return (ctx->playerScore >= WINNING_SCORE);
}
