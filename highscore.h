/*
 * highscore.h - Score persistence with AmigaDOS
 * Amiga Pong - OS-friendly implementation
 */

#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <exec/types.h>

/* High score settings */
#define MAX_HIGHSCORES  5
#define NAME_LENGTH     8

/* Single high score entry */
typedef struct {
    char name[NAME_LENGTH + 1];  /* 8 chars + null */
    WORD score;
} HighScoreEntry;

/* High score table */
typedef struct {
    HighScoreEntry entries[MAX_HIGHSCORES];
    ULONG magic;  /* For file validation */
} HighScoreTable;

/* Magic number for file validation */
#define HIGHSCORE_MAGIC 0x504F4E47  /* 'PONG' */

/* File path */
#define HIGHSCORE_FILE "S:pong.hiscore"

/* Load high scores from file (returns FALSE if file doesn't exist) */
BOOL LoadHighScores(HighScoreTable *table);

/* Save high scores to file */
BOOL SaveHighScores(HighScoreTable *table);

/* Initialize table with default values */
void InitHighScores(HighScoreTable *table);

/* Check if score qualifies for table */
BOOL IsHighScore(HighScoreTable *table, WORD score);

/* Add a new high score (returns position 0-4, or -1 if not added) */
WORD AddHighScore(HighScoreTable *table, const char *name, WORD score);

/* Get rank of a score (0-4 if qualifies, -1 if not) */
WORD GetScoreRank(HighScoreTable *table, WORD score);

#endif /* HIGHSCORE_H */
