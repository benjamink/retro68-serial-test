/*
 * FujiNet Mac Config - A classic Mac application for FujiNet configuration
 * Displays a main config window with host/disk lists and a logo area.
 * Serial testing is available from the Settings menu.
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
#include <Lists.h>

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

    /* Read host slots from fujinet-nio and populate the list */
    if (gSerialOutRef != 0) {
        /* Explicitly zero host slots in case DATA segment init is unreliable */
        short s, b;
        for (s = 0; s < kNumHostSlots; s++) {
            for (b = 0; b < 32; b++) {
                gHostSlots[s][b] = 0;
            }
        }
        if (!ReadHostSlots(gHostSlots, kNumHostSlots)) {
            /* Default first host slot to "host" (POSIX filesystem name) */
            gHostSlots[0][0] = 'h';
            gHostSlots[0][1] = 'o';
            gHostSlots[0][2] = 's';
            gHostSlots[0][3] = 't';
            gHostSlots[0][4] = '\0';
        }
        PopulateHostsList();
    }

    /* Main event loop */
    while (gRunning) {
        if (WaitNextEvent(everyEvent, &event, 5, NULL)) {
            HandleEvent(&event);
        }

        /* Blink the text cursor in serial testing window if open */
        if (gSendText != NULL && gSerialWindow != NULL) {
            TEIdle(gSendText);
        }

        /* Check for incoming serial data */
        PollSerialInput();
    }

    /* Cleanup */
    CloseFileBrowser();
    CloseSerialTestingWindow();

    if (gHostsList != NULL) {
        LDispose(gHostsList);
    }
    if (gDisksList != NULL) {
        LDispose(gDisksList);
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
            {
                WindowPtr evtWindow = (WindowPtr)event->message;
                Boolean active = (event->modifiers & activeFlag) != 0;

                if (evtWindow == gSerialWindow && gSendText != NULL) {
                    if (active) {
                        TEActivate(gSendText);
                    } else {
                        TEDeactivate(gSendText);
                    }
                }
                if (evtWindow == gMainWindow) {
                    if (gHostsList != NULL) {
                        LActivate(active, gHostsList);
                    }
                    if (gDisksList != NULL) {
                        LActivate(active, gDisksList);
                    }
                }
                if (evtWindow == gBrowserWindow) {
                    /* Browser list activation is handled internally */
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
    Point localPoint;

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
            } else {
                SetPort(window);
                localPoint = event->where;
                GlobalToLocal(&localPoint);

                if (window == gMainWindow) {
                    HandleMainWindowClick(window, localPoint, event);
                } else if (window == gSerialWindow) {
                    HandleSerialWindowClick(window, localPoint, event);
                } else if (window == gBrowserWindow) {
                    HandleBrowserWindowClick(window, localPoint, event);
                }
            }
            break;

        case inDrag:
            DragWindow(window, event->where, &qd.screenBits.bounds);
            break;

        case inGoAway:
            if (TrackGoAway(window, event->where)) {
                if (window == gMainWindow) {
                    gRunning = false;
                } else if (window == gSerialWindow) {
                    CloseSerialTestingWindow();
                } else if (window == gBrowserWindow) {
                    CloseFileBrowser();
                }
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
        /* Command+Return sends text (only when serial window is frontmost) */
        if (key == '\r' && gSerialWindow != NULL
            && FrontWindow() == gSerialWindow) {
            SendTextToSerial();
        } else {
            HandleMenuChoice(MenuKey(key));
        }
    } else if (gSendText != NULL && gSerialWindow != NULL
               && FrontWindow() == gSerialWindow) {
        /* Pass key to TextEdit only when serial window is front */
        TEKey(key, gSendText);
    }
}
