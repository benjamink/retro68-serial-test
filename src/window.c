/*
 * window.c - Main window and UI elements
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Controls.h>
#include <ControlDefinitions.h>

#include "constants.h"
#include "window.h"

/* Window and UI globals */
WindowPtr gMainWindow = NULL;
TEHandle gSendText = NULL;
TEHandle gRecvText = NULL;
ControlHandle gSendButton = NULL;
ControlHandle gResetButton = NULL;

/*
 * Create the main application window with send/receive text areas and buttons
 */
void CreateMainWindow(void)
{
    Rect windowRect;
    Rect textRect;
    Rect buttonRect;

    /* Center the window on screen */
    SetRect(&windowRect,
            (qd.screenBits.bounds.right - kWindowWidth) / 2,
            (qd.screenBits.bounds.bottom - kWindowHeight) / 2 + 20,
            (qd.screenBits.bounds.right + kWindowWidth) / 2,
            (qd.screenBits.bounds.bottom + kWindowHeight) / 2 + 20);

    gMainWindow = NewWindow(NULL, &windowRect, "\pFujiNet Mac Config",
                            true, documentProc, (WindowPtr)-1, true, 0);

    if (gMainWindow == NULL) {
        return;
    }

    SetPort(gMainWindow);
    TextFont(kFontIDMonaco);
    TextSize(9);

    /* Create send text area - inset from frame border */
    SetRect(&textRect, kSendLeft + 4, kSendTop + 4, kSendRight - 4, kSendBottom - 4);
    gSendText = TENew(&textRect, &textRect);

    if (gSendText != NULL) {
        TEActivate(gSendText);
    }

    /* Create Send button */
    SetRect(&buttonRect, kSendButtonLeft, kSendButtonTop, kSendButtonRight, kSendButtonBottom);
    gSendButton = NewControl(gMainWindow, &buttonRect, "\pSend",
                             true, 0, 0, 1, pushButProc, 0);

    /* Create Reset button (for fujinet-nio) */
    SetRect(&buttonRect, kResetButtonLeft, kResetButtonTop, kResetButtonRight, kResetButtonBottom);
    gResetButton = NewControl(gMainWindow, &buttonRect, "\pReset",
                              true, 0, 0, 1, pushButProc, 0);

    /* Create receive text area - inset from frame border */
    SetRect(&textRect, kRecvLeft + 4, kRecvTop + 4, kRecvRight - 4, kRecvBottom - 4);
    gRecvText = TENew(&textRect, &textRect);

    /* Make receive text read-only by not activating it */
}

/*
 * Update window contents
 */
void UpdateWindow(WindowPtr window)
{
    Rect textFrame;

    if (window != gMainWindow) {
        return;
    }

    BeginUpdate(window);
    SetPort(window);

    /* Draw send label */
    MoveTo(kSendLeft, kSendTop - 5);
    DrawString("\pSend:");

    /* Draw frame around send text area */
    SetRect(&textFrame, kSendLeft, kSendTop, kSendRight, kSendBottom);
    FrameRect(&textFrame);

    /* Draw send text edit contents */
    if (gSendText != NULL) {
        TEUpdate(&textFrame, gSendText);
    }

    /* Draw button */
    if (gSendButton != NULL) {
        DrawControls(window);
    }

    /* Draw receive label */
    MoveTo(kRecvLeft, kRecvTop - 5);
    DrawString("\pReceived:");

    /* Draw frame around receive text area */
    SetRect(&textFrame, kRecvLeft, kRecvTop, kRecvRight, kRecvBottom);
    FrameRect(&textFrame);

    /* Draw receive text contents */
    if (gRecvText != NULL) {
        TEUpdate(&textFrame, gRecvText);
    }

    EndUpdate(window);
}
