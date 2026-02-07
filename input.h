/*
 * input.h - Mouse input via IDCMP
 * Amiga Pong - OS-friendly implementation
 */

#ifndef INPUT_H
#define INPUT_H

#include <exec/types.h>
#include <intuition/intuition.h>

/* Input event types */
#define INPUT_NONE       0
#define INPUT_MOUSE_MOVE 1
#define INPUT_CLICK      2
#define INPUT_ESC        4
#define INPUT_CLOSE      8
#define INPUT_KEY        16

/* Input state */
typedef struct {
    WORD mouseY;       /* Current mouse Y position */
    UBYTE events;      /* Bitmask of events this frame */
    UBYTE lastKey;     /* Last key pressed (for name entry) */
} InputState;

/* Process all pending IDCMP messages */
void ProcessInput(struct Window *window, InputState *input);

/* Clear event flags (call after processing) */
void ClearInputEvents(InputState *input);

/* Initialize input state */
void InitInput(InputState *input);

#endif /* INPUT_H */
