/*
 * highscore.c - Score persistence with AmigaDOS
 * Amiga Pong - OS-friendly implementation
 */

#include <exec/types.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "highscore.h"

/* String copy with length limit */
static void StrCopy(char *dest, const char *src, WORD maxLen)
{
    WORD i;
    for (i = 0; i < maxLen && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void InitHighScores(HighScoreTable *table)
{
    WORD i;

    table->magic = HIGHSCORE_MAGIC;
    table->difficulty = 1;  /* Default to medium */
    table->reserved[0] = 0;
    table->reserved[1] = 0;
    table->reserved[2] = 0;

    for (i = 0; i < MAX_HIGHSCORES; i++) {
        StrCopy(table->entries[i].name, "--------", NAME_LENGTH);
        table->entries[i].score = 0;
    }
}

BOOL LoadHighScores(HighScoreTable *table)
{
    BPTR file;
    LONG bytesRead;

    file = Open(HIGHSCORE_FILE, MODE_OLDFILE);
    if (!file) {
        /* File doesn't exist, use defaults */
        InitHighScores(table);
        return FALSE;
    }

    bytesRead = Read(file, table, sizeof(HighScoreTable));
    Close(file);

    /* Validate magic number */
    if (bytesRead != sizeof(HighScoreTable) || table->magic != HIGHSCORE_MAGIC) {
        InitHighScores(table);
        return FALSE;
    }

    return TRUE;
}

BOOL SaveHighScores(HighScoreTable *table)
{
    BPTR file;
    LONG bytesWritten;

    table->magic = HIGHSCORE_MAGIC;

    file = Open(HIGHSCORE_FILE, MODE_NEWFILE);
    if (!file) {
        return FALSE;
    }

    bytesWritten = Write(file, table, sizeof(HighScoreTable));
    Close(file);

    return (bytesWritten == sizeof(HighScoreTable));
}

BOOL IsHighScore(HighScoreTable *table, WORD score)
{
    /* Check if score beats the lowest entry */
    return (score > table->entries[MAX_HIGHSCORES - 1].score);
}

WORD GetScoreRank(HighScoreTable *table, WORD score)
{
    WORD i;

    for (i = 0; i < MAX_HIGHSCORES; i++) {
        if (score > table->entries[i].score) {
            return i;
        }
    }

    return -1;
}

WORD AddHighScore(HighScoreTable *table, const char *name, WORD score)
{
    WORD rank;
    WORD i;

    rank = GetScoreRank(table, score);

    if (rank < 0) {
        return -1;  /* Score doesn't qualify */
    }

    /* Shift entries down to make room */
    for (i = MAX_HIGHSCORES - 1; i > rank; i--) {
        table->entries[i] = table->entries[i - 1];
    }

    /* Insert new entry */
    StrCopy(table->entries[rank].name, name, NAME_LENGTH);
    table->entries[rank].score = score;

    /* Save to file */
    SaveHighScores(table);

    return rank;
}
