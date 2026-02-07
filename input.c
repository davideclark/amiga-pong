/*
 * input.c - Mouse input via IDCMP
 * Amiga Pong - OS-friendly implementation
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "input.h"

/* Raw key codes */
#define RAWKEY_ESC 0x45
#define ASCII_ESC  27

void InitInput(InputState *input)
{
    input->mouseY = 128;  /* Center of screen */
    input->events = INPUT_NONE;
    input->lastKey = 0;
}

void ProcessInput(struct Window *window, InputState *input)
{
    struct IntuiMessage *msg;
    ULONG class;
    UWORD code;
    WORD mouseY;

    if (!window) return;

    /* Clear events from last frame */
    input->events = INPUT_NONE;
    input->lastKey = 0;

    /* Process all pending messages */
    while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)) != NULL) {
        class = msg->Class;
        code = msg->Code;
        mouseY = msg->MouseY;

        /* Reply immediately */
        ReplyMsg((struct Message *)msg);

        switch (class) {
            case IDCMP_MOUSEMOVE:
                input->events |= INPUT_MOUSE_MOVE;
                input->mouseY = mouseY;
                break;

            case IDCMP_MOUSEBUTTONS:
                if (code == SELECTDOWN) {
                    input->events |= INPUT_CLICK;
                }
                break;

            case IDCMP_RAWKEY:
                /* Ignore key up events (bit 7 set) */
                if (!(code & 0x80)) {
                    input->events |= INPUT_KEY;
                    if (code == RAWKEY_ESC) {
                        input->events |= INPUT_ESC;
                    }
                }
                break;

            case IDCMP_VANILLAKEY:
                /* ASCII key for name entry */
                input->events |= INPUT_KEY;
                input->lastKey = (UBYTE)code;
                /* Also check for ESC via ASCII */
                if (code == ASCII_ESC) {
                    input->events |= INPUT_ESC;
                }
                break;

            case IDCMP_CLOSEWINDOW:
                input->events |= INPUT_CLOSE;
                break;
        }
    }
}

void ClearInputEvents(InputState *input)
{
    input->events = INPUT_NONE;
}
