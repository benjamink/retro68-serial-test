/*
 * window.c - Main config window, file browser, and serial testing window
 */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Lists.h>
#include <Events.h>
#include <Sound.h>
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
ControlHandle gEditHostButton = NULL;

/* Host slot data from fujinet-nio (8 slots x 32 bytes) */
char gHostSlots[8][32] = {{0}};

/* Disk slot state tracking */
DiskSlotState gDiskSlots[8] = {{0}};

/* File browser window */
WindowPtr gBrowserWindow = NULL;

/* File browser state (file-scope) */
static ListHandle gBrowserList = NULL;
static ControlHandle gBrowserUpBtn = NULL;
static ControlHandle gBrowserCloseBtn = NULL;
static short gBrowserHostSlot = -1;
static char gBrowserFsName[32] = {0};
static char gBrowserPath[256] = "/";
static DirEntry gBrowserEntries[kMaxBrowserEntries];
static short gBrowserEntryCount = 0;

/* Serial testing window globals */
WindowPtr gSerialWindow = NULL;
TEHandle gSendText = NULL;
TEHandle gRecvText = NULL;
ControlHandle gSendButton = NULL;
ControlHandle gResetButton = NULL;

/* Forward declarations for static helpers */
static void BrowseDirectory(void);

/*
 * Create the main FujiNet Config window with hosts list, disks list,
 * logo area, boot button, and edit host button
 */
void CreateMainWindow(void)
{
    Rect windowRect, listRect, dataBounds, btnRect;
    Point cellSize;

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
        (*gHostsList)->selFlags = lOnlyOne;
        /* Populate from gHostSlots (initially empty, filled after ReadHostSlots) */
        PopulateHostsList();
    }

    /* Create Disks list - rView inset 1px from border, minus 15px for scrollbar */
    SetRect(&listRect, kDisksListLeft + 1, kDisksListTop + 1,
            kDisksListRight - 1 - 15, kDisksListBottom - 1);
    SetRect(&dataBounds, 0, 0, 1, kNumDiskSlots);
    SetPt(&cellSize, kDisksListRight - kDisksListLeft - 2 - 15, kDiskCellHeight);

    gDisksList = LNew(&listRect, &dataBounds, cellSize,
                      0, gMainWindow, true, false, false, true);

    if (gDisksList != NULL) {
        (*gDisksList)->selFlags = lOnlyOne;
        PopulateDisksList();
    }

    /* Create Boot Selected Disk button */
    SetRect(&btnRect, kBootButtonLeft, kBootButtonTop,
            kBootButtonRight, kBootButtonBottom);
    gBootButton = NewControl(gMainWindow, &btnRect,
                             "\pMount Disks",
                             true, 0, 0, 1, pushButProc, 0);

    /* Create Edit Host button (right column, aligned with Boot button) */
    SetRect(&btnRect, kEditHostBtnLeft, kEditHostBtnTop,
            kEditHostBtnRight, kEditHostBtnBottom);
    gEditHostButton = NewControl(gMainWindow, &btnRect,
                                 "\pEdit Host",
                                 true, 0, 0, 1, pushButProc, 0);
}

/*
 * Populate the hosts list from gHostSlots data.
 * Formats each entry as "N. hostname" or "N." if empty.
 */
void PopulateHostsList(void)
{
    short i;
    Cell theCell;
    char displayStr[48];

    if (gHostsList == NULL) {
        return;
    }

    for (i = 0; i < kNumHostSlots; i++) {
        short pos = 0;

        /* Format slot number */
        pos = 0;
        if (i + 1 >= 10) {
            displayStr[pos++] = '0' + ((i + 1) / 10);
        }
        displayStr[pos++] = '0' + ((i + 1) % 10);
        displayStr[pos++] = '.';

        /* Append hostname if present */
        if (gHostSlots[i][0] != '\0') {
            short j;
            displayStr[pos++] = ' ';
            for (j = 0; j < 31 && gHostSlots[i][j] != '\0'; j++) {
                displayStr[pos++] = gHostSlots[i][j];
            }
        }

        SetPt(&theCell, 0, i);
        LSetCell(displayStr, pos, theCell, gHostsList);
    }
}

/*
 * Populate the disks list from gDiskSlots state.
 * Mounted entries: "<N>[R|W] <host>:<path>"
 * Empty entries: "<N>."
 */
void PopulateDisksList(void)
{
    short i;
    Cell theCell;
    char displayStr[320];

    if (gDisksList == NULL) {
        return;
    }

    for (i = 0; i < kNumDiskSlots; i++) {
        short pos = 0;

        /* Slot number (1-based) */
        displayStr[pos++] = '0' + (i + 1);

        if (gDiskSlots[i].mounted) {
            short j;
            /* Mode letter */
            displayStr[pos++] = gDiskSlots[i].readOnly ? 'R' : 'W';
            displayStr[pos++] = ' ';
            /* Host slot number (1-based) */
            if (gDiskSlots[i].hostSlot >= 10) {
                displayStr[pos++] = '0' + (gDiskSlots[i].hostSlot / 10);
            }
            displayStr[pos++] = '0' + (gDiskSlots[i].hostSlot % 10);
            displayStr[pos++] = ':';
            /* Path */
            for (j = 0; gDiskSlots[i].path[j] != '\0' && pos < 310; j++) {
                displayStr[pos++] = gDiskSlots[i].path[j];
            }
        } else {
            displayStr[pos++] = '.';
        }

        SetPt(&theCell, 0, i);
        LSetCell(displayStr, pos, theCell, gDisksList);
    }
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

    /* Draw controls (boot button, edit host button) */
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

    /* Check buttons first */
    if (FindControl(localPoint, window, &control) == kControlButtonPart) {
        if (TrackControl(control, localPoint, NULL) == kControlButtonPart) {
            if (control == gBootButton) {
                /* Show all mounted disks */
                {
                    short d;
                    Boolean anyMounted = false;
                    for (d = 0; d < kNumDiskSlots; d++) {
                        if (gDiskSlots[d].mounted) {
                            anyMounted = true;
                            break;
                        }
                    }
                    if (anyMounted) {
                        DoMountDisksDialog();
                    } else {
                        SysBeep(10);
                    }
                }
            } else if (control == gEditHostButton) {
                /* Edit the selected host */
                Cell theCell;
                SetPt(&theCell, 0, 0);
                if (gHostsList != NULL && LGetSelect(true, &theCell, gHostsList)) {
                    DoEditHostDialog(theCell.v);
                } else {
                    SysBeep(10);
                }
            }
        }
        return;
    }

    /* Check hosts list click - double-click opens file browser */
    SetRect(&listRect, kHostsListLeft, kHostsListTop,
            kHostsListRight, kHostsListBottom);
    if (PtInRect(localPoint, &listRect) && gHostsList != NULL) {
        Boolean doubleClick = LClick(localPoint, event->modifiers, gHostsList);
        if (doubleClick) {
            Cell theCell;
            SetPt(&theCell, 0, 0);
            if (LGetSelect(true, &theCell, gHostsList)) {
                /* Only open browser if host slot is non-empty */
                if (gHostSlots[theCell.v][0] != '\0') {
                    OpenFileBrowser(theCell.v);
                } else {
                    SysBeep(10);
                }
            }
        }
        return;
    }

    /* Check disks list click - double-click mounts selected disk */
    SetRect(&listRect, kDisksListLeft, kDisksListTop,
            kDisksListRight, kDisksListBottom);
    if (PtInRect(localPoint, &listRect) && gDisksList != NULL) {
        Boolean doubleClick = LClick(localPoint, event->modifiers, gDisksList);
        if (doubleClick) {
            Cell diskCell;
            SetPt(&diskCell, 0, 0);
            if (LGetSelect(true, &diskCell, gDisksList)
                && diskCell.v >= 0 && diskCell.v < kNumDiskSlots
                && gDiskSlots[diskCell.v].mounted) {
                DoMountDiskDialog(gDiskSlots[diskCell.v].path);
            }
        }
        return;
    }
}

/* ----------------------------------------------------------------
 * File Browser Window
 * ---------------------------------------------------------------- */

/*
 * Refresh the browser list from the current path.
 * Calls ListDirectory via serial, then populates the list.
 */
static void BrowseDirectory(void)
{
    short i;
    Cell theCell;
    Boolean hasMore;
    short dataBoundsBottom;
    Rect dataBounds;

    if (gBrowserList == NULL) {
        return;
    }

    /* Query directory contents */
    gBrowserEntryCount = 0;
    hasMore = false;

    ListDirectory(gBrowserFsName, gBrowserPath,
                  0, kMaxBrowserEntries,
                  gBrowserEntries, &gBrowserEntryCount, &hasMore);

    /* Resize list to match entry count (minimum 1 row so list doesn't collapse) */
    dataBoundsBottom = gBrowserEntryCount > 0 ? gBrowserEntryCount : 0;
    LDelRow(0, 0, gBrowserList);  /* Remove all rows */
    if (dataBoundsBottom > 0) {
        LAddRow(dataBoundsBottom, 0, gBrowserList);
    }

    /* Populate list cells */
    for (i = 0; i < gBrowserEntryCount; i++) {
        char displayStr[80];
        short pos = 0;
        short j;

        /* Copy name */
        for (j = 0; gBrowserEntries[i].name[j] != '\0' && pos < 76; j++) {
            displayStr[pos++] = gBrowserEntries[i].name[j];
        }
        /* Append "/" for directories */
        if (gBrowserEntries[i].isDir && pos < 79) {
            displayStr[pos++] = '/';
        }

        SetPt(&theCell, 0, i);
        LSetCell(displayStr, pos, theCell, gBrowserList);
    }

    /* Force redraw of the browser window */
    if (gBrowserWindow != NULL) {
        Rect r;
        SetPort(gBrowserWindow);
        SetRect(&r, kBrowserListLeft, kBrowserListTop,
                kBrowserListRight, kBrowserListBottom);
        InvalRect(&r);
        /* Also invalidate the path area */
        SetRect(&r, kBrowserPathLeft, kBrowserPathTop,
                kBrowserPathRight, kBrowserPathBottom);
        InvalRect(&r);
    }
}

/*
 * Open a file browser window for the given host slot (0-based index)
 */
void OpenFileBrowser(short hostSlot)
{
    Rect windowRect, listRect, dataBounds, btnRect;
    Point cellSize;
    Str255 title;
    short i, len;

    /* If already open, just bring to front */
    if (gBrowserWindow != NULL) {
        SelectWindow(gBrowserWindow);
        return;
    }

    if (hostSlot < 0 || hostSlot >= kNumHostSlots) {
        return;
    }

    /* Save browser state */
    gBrowserHostSlot = hostSlot;
    /* Copy hostname as fsName */
    for (i = 0; i < 31 && gHostSlots[hostSlot][i] != '\0'; i++) {
        gBrowserFsName[i] = gHostSlots[hostSlot][i];
    }
    gBrowserFsName[i] = '\0';

    /* Start at root */
    gBrowserPath[0] = '/';
    gBrowserPath[1] = '\0';

    /* Build window title: "Browse: <hostname>" */
    title[0] = 0;
    {
        char *prefix = "Browse: ";
        short pos = 1;
        while (*prefix && pos < 254) {
            title[pos++] = *prefix++;
        }
        for (i = 0; gBrowserFsName[i] != '\0' && pos < 254; i++) {
            title[pos++] = gBrowserFsName[i];
        }
        title[0] = pos - 1;
    }

    /* Position offset from main window */
    SetRect(&windowRect,
            (qd.screenBits.bounds.right - kBrowserWindowWidth) / 2 + 30,
            (qd.screenBits.bounds.bottom - kBrowserWindowHeight) / 2 + 30,
            (qd.screenBits.bounds.right + kBrowserWindowWidth) / 2 + 30,
            (qd.screenBits.bounds.bottom + kBrowserWindowHeight) / 2 + 30);

    gBrowserWindow = NewWindow(NULL, &windowRect, title,
                                true, documentProc, (WindowPtr)-1, true, 0);

    if (gBrowserWindow == NULL) {
        return;
    }

    SetPort(gBrowserWindow);
    TextFont(kFontIDMonaco);
    TextSize(9);

    /* Create browser list - inset 1px from border, minus 15px for scrollbar */
    SetRect(&listRect, kBrowserListLeft + 1, kBrowserListTop + 1,
            kBrowserListRight - 1 - 15, kBrowserListBottom - 1);
    SetRect(&dataBounds, 0, 0, 1, 0);  /* Start with 0 rows, BrowseDirectory fills it */
    SetPt(&cellSize, kBrowserListRight - kBrowserListLeft - 2 - 15, kBrowserCellHeight);

    gBrowserList = LNew(&listRect, &dataBounds, cellSize,
                        0, gBrowserWindow, true, false, false, true);

    if (gBrowserList != NULL) {
        (*gBrowserList)->selFlags = lOnlyOne;
    }

    /* Create Up button */
    SetRect(&btnRect, kBrowserUpBtnLeft, kBrowserUpBtnTop,
            kBrowserUpBtnRight, kBrowserUpBtnBottom);
    gBrowserUpBtn = NewControl(gBrowserWindow, &btnRect, "\pUp",
                               true, 0, 0, 1, pushButProc, 0);

    /* Create Close button */
    SetRect(&btnRect, kBrowserCloseBtnLeft, kBrowserCloseBtnTop,
            kBrowserCloseBtnRight, kBrowserCloseBtnBottom);
    gBrowserCloseBtn = NewControl(gBrowserWindow, &btnRect, "\pClose",
                                  true, 0, 0, 1, pushButProc, 0);

    /* Load initial directory contents */
    BrowseDirectory();
}

/*
 * Close the file browser window and clean up resources
 */
void CloseFileBrowser(void)
{
    if (gBrowserWindow == NULL) {
        return;
    }

    if (gBrowserList != NULL) {
        LDispose(gBrowserList);
        gBrowserList = NULL;
    }

    /* Controls are disposed automatically with the window */
    gBrowserUpBtn = NULL;
    gBrowserCloseBtn = NULL;

    DisposeWindow(gBrowserWindow);
    gBrowserWindow = NULL;
    gBrowserHostSlot = -1;
    gBrowserEntryCount = 0;
}

/*
 * Draw the file browser window contents
 */
void UpdateBrowserWindow(WindowPtr window)
{
    Rect r;
    Str255 pPath;
    short len;

    if (window != gBrowserWindow) {
        return;
    }

    BeginUpdate(window);
    SetPort(window);

    /* Draw current path */
    len = strlen(gBrowserPath);
    if (len > 255) len = 255;
    pPath[0] = len;
    memcpy(&pPath[1], gBrowserPath, len);
    MoveTo(kBrowserPathLeft, kBrowserPathBottom - 2);
    /* Erase the path area first */
    SetRect(&r, kBrowserPathLeft, kBrowserPathTop,
            kBrowserPathRight, kBrowserPathBottom);
    EraseRect(&r);
    DrawString(pPath);

    /* Draw border around browser list area */
    SetRect(&r, kBrowserListLeft, kBrowserListTop,
            kBrowserListRight, kBrowserListBottom);
    FrameRect(&r);

    /* Update browser list contents */
    if (gBrowserList != NULL) {
        LUpdate(window->visRgn, gBrowserList);
    }

    /* Draw buttons */
    DrawControls(window);

    EndUpdate(window);
}

/*
 * Handle mouse clicks in the file browser window
 */
void HandleBrowserWindowClick(WindowPtr window, Point localPoint, EventRecord *event)
{
    ControlHandle control;
    Rect listRect;

    SetPort(window);

    /* Check buttons first */
    if (FindControl(localPoint, window, &control) == kControlButtonPart) {
        if (TrackControl(control, localPoint, NULL) == kControlButtonPart) {
            if (control == gBrowserUpBtn) {
                /* Navigate up: strip last path component */
                short len = strlen(gBrowserPath);
                if (len > 1) {
                    /* Remove trailing slash if not root */
                    if (gBrowserPath[len - 1] == '/' && len > 1) {
                        len--;
                    }
                    /* Find the last slash */
                    while (len > 0 && gBrowserPath[len - 1] != '/') {
                        len--;
                    }
                    /* Keep the trailing slash (at least "/") */
                    if (len == 0) len = 1;
                    gBrowserPath[len] = '\0';
                    BrowseDirectory();
                }
            } else if (control == gBrowserCloseBtn) {
                CloseFileBrowser();
            }
        }
        return;
    }

    /* Check browser list click */
    SetRect(&listRect, kBrowserListLeft, kBrowserListTop,
            kBrowserListRight, kBrowserListBottom);
    if (PtInRect(localPoint, &listRect) && gBrowserList != NULL) {
        Boolean doubleClick = LClick(localPoint, event->modifiers, gBrowserList);
        if (doubleClick) {
            Cell theCell;
            SetPt(&theCell, 0, 0);
            if (LGetSelect(true, &theCell, gBrowserList)) {
                short idx = theCell.v;
                if (idx >= 0 && idx < gBrowserEntryCount) {
                    if (gBrowserEntries[idx].isDir) {
                        /* Navigate into directory: append name to path */
                        short pathLen = strlen(gBrowserPath);
                        short nameLen = strlen(gBrowserEntries[idx].name);

                        /* Ensure trailing slash on current path */
                        if (pathLen > 0 && gBrowserPath[pathLen - 1] != '/') {
                            if (pathLen < kMaxPathLen - 1) {
                                gBrowserPath[pathLen++] = '/';
                            }
                        }
                        /* Append directory name */
                        if (pathLen + nameLen < kMaxPathLen - 1) {
                            memcpy(gBrowserPath + pathLen, gBrowserEntries[idx].name, nameLen);
                            pathLen += nameLen;
                            gBrowserPath[pathLen++] = '/';
                            gBrowserPath[pathLen] = '\0';
                            BrowseDirectory();
                        }
                    } else {
                        /* File: open mount dialog */
                        char fullPath[512];
                        short pathLen = strlen(gBrowserPath);
                        short nameLen = strlen(gBrowserEntries[idx].name);
                        short outSlot;
                        Boolean outReadOnly;

                        /* Build full path */
                        memcpy(fullPath, gBrowserPath, pathLen);
                        if (pathLen > 0 && fullPath[pathLen - 1] != '/') {
                            fullPath[pathLen++] = '/';
                        }
                        memcpy(fullPath + pathLen, gBrowserEntries[idx].name, nameLen);
                        fullPath[pathLen + nameLen] = '\0';

                        if (DoMountDialog(gBrowserHostSlot,
                                          fullPath,
                                          &outSlot, &outReadOnly)) {
                            /* Try to mount the disk on the wire */
                            short hostSlot = gBrowserHostSlot;
                            MountDisk(outSlot, outReadOnly,
                                      gBrowserFsName, fullPath);

                            /* Update disk slot state (outSlot is 1-based) */
                            {
                                short slotIdx = outSlot - 1;
                                if (slotIdx >= 0 && slotIdx < kNumDiskSlots) {
                                    gDiskSlots[slotIdx].mounted = true;
                                    gDiskSlots[slotIdx].hostSlot = hostSlot + 1;
                                    gDiskSlots[slotIdx].readOnly = outReadOnly;
                                    strncpy(gDiskSlots[slotIdx].path, fullPath, 255);
                                    gDiskSlots[slotIdx].path[255] = '\0';
                                }
                            }

                            /* Close browser, update disk list */
                            CloseFileBrowser();
                            SetPort(gMainWindow);
                            PopulateDisksList();
                            {
                                Rect r;
                                SetRect(&r, kDisksListLeft, kDisksListTop,
                                        kDisksListRight, kDisksListBottom);
                                InvalRect(&r);
                            }
                            return;
                        }
                    }
                }
            }
        }
        return;
    }
}

/* ----------------------------------------------------------------
 * Serial Testing Window
 * ---------------------------------------------------------------- */

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
    } else if (window == gBrowserWindow) {
        UpdateBrowserWindow(window);
    }
}
