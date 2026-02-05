/*
 * FujiNet Mac Config - A classic Mac application for FujiNet configuration
 * Displays a window with a text input, a "Send" button, and a receive area.
 * Communicates with FujiNet over the serial port using the FujiBus protocol.
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Events.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <SegLoad.h>
#include <Sound.h>

#include "constants.h"
#include "serial.h"
#include "window.h"
#include "menus.h"

/* Application state */
Boolean gRunning = true;

/* Local function prototypes */
static void InitializeToolbox(void);
static void HandleEvent(EventRecord *event);
static void HandleMouseDown(EventRecord *event);
static void HandleKeyDown(EventRecord *event);

/*
 * Main entry point
 */
void main(void)
{
    EventRecord event;

    InitializeToolbox();
    InitializeMenus();

    if (!InitializeSerial()) {
        /* Serial port failed to open - show alert and continue anyway */
        SysBeep(10);
    }

    CreateMainWindow();

    /* Main event loop */
    while (gRunning) {
        if (WaitNextEvent(everyEvent, &event, 5, NULL)) {
            HandleEvent(&event);
        }

        /* Blink the text cursor */
        if (gSendText != NULL) {
            TEIdle(gSendText);
        }

        /* Check for incoming serial data */
        PollSerialInput();
    }

    /* Cleanup */
    if (gSendText != NULL) {
        TEDispose(gSendText);
    }
    if (gRecvText != NULL) {
        TEDispose(gRecvText);
    }
    if (gMainWindow != NULL) {
        DisposeWindow(gMainWindow);
    }
    CleanupSerial();
}

/*
 * Initialize the Mac Toolbox managers
 */
static void InitializeToolbox(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
    FlushEvents(everyEvent, 0);
}

/*
 * Handle all events
 */
static void HandleEvent(EventRecord *event)
{
    switch (event->what) {
        case mouseDown:
            HandleMouseDown(event);
            break;

        case keyDown:
        case autoKey:
            HandleKeyDown(event);
            break;

        case updateEvt:
            UpdateWindow((WindowPtr)event->message);
            break;

        case activateEvt:
            if (gSendText != NULL) {
                if (event->modifiers & activeFlag) {
                    TEActivate(gSendText);
                } else {
                    TEDeactivate(gSendText);
                }
            }
            break;
    }
}

/*
 * Handle mouse down events
 */
static void HandleMouseDown(EventRecord *event)
{
    WindowPtr window;
    short part;
    long menuChoice;
    ControlHandle control;
    Point localPoint;
    Rect textFrame;

    part = FindWindow(event->where, &window);

    switch (part) {
        case inMenuBar:
            menuChoice = MenuSelect(event->where);
            HandleMenuChoice(menuChoice);
            break;

        case inSysWindow:
            SystemClick(event, window);
            break;

        case inContent:
            if (window != FrontWindow()) {
                SelectWindow(window);
            } else if (window == gMainWindow) {
                SetPort(gMainWindow);
                localPoint = event->where;
                GlobalToLocal(&localPoint);

                /* Check if click is in a button */
                if (FindControl(localPoint, window, &control) == kControlButtonPart) {
                    if (TrackControl(control, localPoint, NULL) == kControlButtonPart) {
                        if (control == gSendButton) {
                            SendTextToSerial();
                        } else if (control == gResetButton) {
                            SendResetCommand();
                        }
                    }
                    return;
                }

                /* Check if click is in text area */
                if (gSendText != NULL) {
                    SetRect(&textFrame, kSendLeft, kSendTop, kSendRight, kSendBottom);
                    if (PtInRect(localPoint, &textFrame)) {
                        TEClick(localPoint, (event->modifiers & shiftKey) != 0, gSendText);
                    }
                }
            }
            break;

        case inDrag:
            DragWindow(window, event->where, &qd.screenBits.bounds);
            break;

        case inGoAway:
            if (TrackGoAway(window, event->where)) {
                gRunning = false;
            }
            break;
    }
}

/*
 * Handle key down events
 */
static void HandleKeyDown(EventRecord *event)
{
    char key;

    key = event->message & charCodeMask;

    /* Check for command key */
    if (event->modifiers & cmdKey) {
        /* Command+Return sends text */
        if (key == '\r') {
            SendTextToSerial();
        } else {
            HandleMenuChoice(MenuKey(key));
        }
    } else if (gSendText != NULL) {
        /* Pass key to TextEdit */
        TEKey(key, gSendText);
    }
}
