/*
 * window.c - Main config window and serial testing window
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Lists.h>
#include <Events.h>
#include <string.h>

#include "constants.h"
#include "window.h"
#include "dialogs.h"
#include "serial.h"
#include "logo_data.h"

/* Main config window globals */
WindowPtr gMainWindow = NULL;
ListHandle gHostsList = NULL;
ListHandle gDisksList = NULL;
ControlHandle gBootButton = NULL;

/* Serial testing window globals */
WindowPtr gSerialWindow = NULL;
TEHandle gSendText = NULL;
TEHandle gRecvText = NULL;
ControlHandle gSendButton = NULL;
ControlHandle gResetButton = NULL;

/*
 * Create the main FujiNet Config window with hosts list, disks list,
 * logo area, and boot button
 */
void CreateMainWindow(void)
{
    Rect windowRect, listRect, dataBounds, btnRect;
    Point cellSize;
    Cell theCell;
    short i;

    static char *hostData[] = {
        "1. SD",
        "2. tnfs.fujinet.online",
        "3. apps.fujinet.online",
        "4. smb://myfiles.example.com/retro",
        "5. https://morefiles.example.com/fujienet",
        "6.",
        "7.",
        "8.",
        "9.",
        "10."
    };

    static char *diskData[] = {
        "1R 2:/retro/files/coolapp.dsk",
        "2W 5:/mydocs.dsk",
        "MOOF rogue.moof",
        "",
        "",
        "",
        "",
        ""
    };

    /* Center the window on screen */
    SetRect(&windowRect,
            (qd.screenBits.bounds.right - kMainWindowWidth) / 2,
            (qd.screenBits.bounds.bottom - kMainWindowHeight) / 2 + 20,
            (qd.screenBits.bounds.right + kMainWindowWidth) / 2,
            (qd.screenBits.bounds.bottom + kMainWindowHeight) / 2 + 20);

    gMainWindow = NewWindow(NULL, &windowRect, "\pFujiNet Config",
                            true, documentProc, (WindowPtr)-1, true, 0);

    if (gMainWindow == NULL) {
        return;
    }

    SetPort(gMainWindow);
    TextFont(kFontIDMonaco);
    TextSize(9);

    /* Create Hosts list - rView inset 1px from border, minus 15px for scrollbar */
    SetRect(&listRect, kHostsListLeft + 1, kHostsListTop + 1,
            kHostsListRight - 1 - 15, kHostsListBottom - 1);
    SetRect(&dataBounds, 0, 0, 1, kNumHostSlots);
    SetPt(&cellSize, kHostsListRight - kHostsListLeft - 2 - 15, kHostCellHeight);

    gHostsList = LNew(&listRect, &dataBounds, cellSize,
                      0, gMainWindow, true, false, false, true);

    if (gHostsList != NULL) {
        /* Enforce single selection */
        (*gHostsList)->selFlags = lOnlyOne;

        /* Populate host slots */
        for (i = 0; i < kNumHostSlots; i++) {
            SetPt(&theCell, 0, i);
            LSetCell(hostData[i], strlen(hostData[i]), theCell, gHostsList);
        }
    }

    /* Create Disks list - rView inset 1px from border, minus 15px for scrollbar */
    SetRect(&listRect, kDisksListLeft + 1, kDisksListTop + 1,
            kDisksListRight - 1 - 15, kDisksListBottom - 1);
    SetRect(&dataBounds, 0, 0, 1, kNumDiskSlots);
    SetPt(&cellSize, kDisksListRight - kDisksListLeft - 2 - 15, kDiskCellHeight);

    gDisksList = LNew(&listRect, &dataBounds, cellSize,
                      0, gMainWindow, true, false, false, true);

    if (gDisksList != NULL) {
        /* Enforce single selection */
        (*gDisksList)->selFlags = lOnlyOne;

        /* Populate disk slots */
        for (i = 0; i < kNumDiskSlots; i++) {
            SetPt(&theCell, 0, i);
            LSetCell(diskData[i], strlen(diskData[i]), theCell, gDisksList);
        }
    }

    /* Create Boot Selected Disk button */
    SetRect(&btnRect, kBootButtonLeft, kBootButtonTop,
            kBootButtonRight, kBootButtonBottom);
    gBootButton = NewControl(gMainWindow, &btnRect,
                             "\pBoot Selected Disk",
                             true, 0, 0, 1, pushButProc, 0);
}

/*
 * Draw the FujiNet logo in the logo area using CopyBits.
 * Logo is white on black: PaintRect fills black, then srcBic
 * clears bits where logo data is 1, making those pixels white.
 */
static void DrawLogo(WindowPtr window)
{
    BitMap logoBitmap;
    Rect srcRect, dstRect, logoRect;
    short logoX, logoY;

    /* Fill logo area with black */
    SetRect(&logoRect, kLogoLeft, kLogoTop, kLogoRight, kLogoBottom);
    PaintRect(&logoRect);

    /* Set up bitmap for logo data */
    logoBitmap.baseAddr = (Ptr)gLogoData;
    logoBitmap.rowBytes = kLogoRowBytes;
    SetRect(&logoBitmap.bounds, 0, 0, kLogoBitmapWidth, kLogoBitmapHeight);

    SetRect(&srcRect, 0, 0, kLogoBitmapWidth, kLogoBitmapHeight);

    /* Center logo within the logo area */
    logoX = kLogoLeft + (kLogoRight - kLogoLeft - kLogoBitmapWidth) / 2;
    logoY = kLogoTop + (kLogoBottom - kLogoTop - kLogoBitmapHeight) / 2;
    SetRect(&dstRect, logoX, logoY,
            logoX + kLogoBitmapWidth, logoY + kLogoBitmapHeight);

    /* srcBic: where source bit=1, destination becomes white (0) */
    CopyBits(&logoBitmap, &window->portBits,
             &srcRect, &dstRect, srcBic, NULL);
}

/*
 * Draw the main FujiNet Config window contents
 */
void UpdateMainWindow(WindowPtr window)
{
    Rect r;

    if (window != gMainWindow) {
        return;
    }

    BeginUpdate(window);
    SetPort(window);

    /* Draw Hosts header bar (white text on black background) */
    SetRect(&r, kHostsHeaderLeft, kHostsHeaderTop,
            kHostsHeaderRight, kHostsHeaderBottom);
    PaintRect(&r);
    ForeColor(whiteColor);
    MoveTo(kHostsHeaderLeft + 4, kHostsHeaderBottom - 4);
    DrawString("\pHosts");
    ForeColor(blackColor);

    /* Draw logo area with FujiNet logo bitmap */
    DrawLogo(window);

    /* Draw Disks header bar (white text on black background) */
    SetRect(&r, kDisksHeaderLeft, kDisksHeaderTop,
            kDisksHeaderRight, kDisksHeaderBottom);
    PaintRect(&r);
    ForeColor(whiteColor);
    MoveTo(kDisksHeaderLeft + 4, kDisksHeaderBottom - 4);
    DrawString("\pDisks");
    ForeColor(blackColor);

    /* Draw border around hosts list area */
    SetRect(&r, kHostsListLeft, kHostsListTop,
            kHostsListRight, kHostsListBottom);
    FrameRect(&r);

    /* Update hosts list contents */
    if (gHostsList != NULL) {
        LUpdate(window->visRgn, gHostsList);
    }

    /* Draw border around disks list area */
    SetRect(&r, kDisksListLeft, kDisksListTop,
            kDisksListRight, kDisksListBottom);
    FrameRect(&r);

    /* Update disks list contents */
    if (gDisksList != NULL) {
        LUpdate(window->visRgn, gDisksList);
    }

    /* Draw controls (boot button) */
    DrawControls(window);

    EndUpdate(window);
}

/*
 * Handle mouse clicks in the main config window
 */
void HandleMainWindowClick(WindowPtr window, Point localPoint, EventRecord *event)
{
    ControlHandle control;
    Rect listRect;

    SetPort(window);

    /* Check boot button first */
    if (FindControl(localPoint, window, &control) == kControlButtonPart) {
        if (TrackControl(control, localPoint, NULL) == kControlButtonPart) {
            if (control == gBootButton) {
                DoBootDialog();
            }
        }
        return;
    }

    /* Check hosts list click */
    SetRect(&listRect, kHostsListLeft, kHostsListTop,
            kHostsListRight, kHostsListBottom);
    if (PtInRect(localPoint, &listRect) && gHostsList != NULL) {
        LClick(localPoint, event->modifiers, gHostsList);
        return;
    }

    /* Check disks list click */
    SetRect(&listRect, kDisksListLeft, kDisksListTop,
            kDisksListRight, kDisksListBottom);
    if (PtInRect(localPoint, &listRect) && gDisksList != NULL) {
        LClick(localPoint, event->modifiers, gDisksList);
        return;
    }
}

/*
 * Open the serial testing window (modeless, from Settings menu)
 */
void OpenSerialTestingWindow(void)
{
    Rect windowRect, textRect, buttonRect;

    /* If already open, just bring to front */
    if (gSerialWindow != NULL) {
        SelectWindow(gSerialWindow);
        return;
    }

    /* Position offset from main window */
    SetRect(&windowRect,
            (qd.screenBits.bounds.right - kSerialWindowWidth) / 2 + 20,
            (qd.screenBits.bounds.bottom - kSerialWindowHeight) / 2 + 40,
            (qd.screenBits.bounds.right + kSerialWindowWidth) / 2 + 20,
            (qd.screenBits.bounds.bottom + kSerialWindowHeight) / 2 + 40);

    gSerialWindow = NewWindow(NULL, &windowRect, "\pSerial Testing",
                              true, documentProc, (WindowPtr)-1, true, 0);

    if (gSerialWindow == NULL) {
        return;
    }

    SetPort(gSerialWindow);
    TextFont(kFontIDMonaco);
    TextSize(9);

    /* Create send text area - inset from frame border */
    SetRect(&textRect, kSendLeft + 4, kSendTop + 4, kSendRight - 4, kSendBottom - 4);
    gSendText = TENew(&textRect, &textRect);

    if (gSendText != NULL) {
        TEActivate(gSendText);
    }

    /* Create Send button */
    SetRect(&buttonRect, kSendButtonLeft, kSendButtonTop,
            kSendButtonRight, kSendButtonBottom);
    gSendButton = NewControl(gSerialWindow, &buttonRect, "\pSend",
                             true, 0, 0, 1, pushButProc, 0);

    /* Create Reset button */
    SetRect(&buttonRect, kResetButtonLeft, kResetButtonTop,
            kResetButtonRight, kResetButtonBottom);
    gResetButton = NewControl(gSerialWindow, &buttonRect, "\pReset",
                              true, 0, 0, 1, pushButProc, 0);

    /* Create receive text area - inset from frame border */
    SetRect(&textRect, kRecvLeft + 4, kRecvTop + 4, kRecvRight - 4, kRecvBottom - 4);
    gRecvText = TENew(&textRect, &textRect);

    /* Make receive text read-only by not activating it */
}

/*
 * Close the serial testing window and clean up its resources
 */
void CloseSerialTestingWindow(void)
{
    if (gSerialWindow == NULL) {
        return;
    }

    if (gSendText != NULL) {
        TEDispose(gSendText);
        gSendText = NULL;
    }
    if (gRecvText != NULL) {
        TEDispose(gRecvText);
        gRecvText = NULL;
    }

    /* Controls are disposed automatically with the window */
    gSendButton = NULL;
    gResetButton = NULL;

    DisposeWindow(gSerialWindow);
    gSerialWindow = NULL;
}

/*
 * Draw the serial testing window contents
 */
void UpdateSerialWindow(WindowPtr window)
{
    Rect textFrame;

    if (window != gSerialWindow) {
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

    /* Draw buttons */
    DrawControls(window);

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

/*
 * Handle mouse clicks in the serial testing window
 */
void HandleSerialWindowClick(WindowPtr window, Point localPoint, EventRecord *event)
{
    ControlHandle control;
    Rect textFrame;

    SetPort(window);

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

    /* Check if click is in send text area */
    if (gSendText != NULL) {
        SetRect(&textFrame, kSendLeft, kSendTop, kSendRight, kSendBottom);
        if (PtInRect(localPoint, &textFrame)) {
            TEClick(localPoint, (event->modifiers & shiftKey) != 0, gSendText);
        }
    }
}

/*
 * Dispatch window update to the correct handler
 */
void UpdateWindow(WindowPtr window)
{
    if (window == gMainWindow) {
        UpdateMainWindow(window);
    } else if (window == gSerialWindow) {
        UpdateSerialWindow(window);
    }
}
