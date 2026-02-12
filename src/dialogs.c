/*
 * dialogs.c - Application dialogs
 */

#include <Dialogs.h>
#include <Controls.h>
#include <Lists.h>
#include <Windows.h>
#include <Sound.h>

#include <string.h>

#include "constants.h"
#include "dialogs.h"
#include "serial.h"

/* Local function prototypes */
static void SetRadioButton(DialogPtr dialog, short item, Boolean on);

/*
 * Set a radio button value
 */
static void SetRadioButton(DialogPtr dialog, short item, Boolean on)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;

    GetDialogItem(dialog, item, &itemType, &itemHandle, &itemRect);
    SetControlValue((ControlHandle)itemHandle, on ? 1 : 0);
}

/*
 * Show the Settings dialog
 */
void DoSettingsDialog(void)
{
    DialogPtr dialog;
    short itemHit;
    short tempPort;
    short tempBaud;
    Boolean done;
    GrafPtr savePort;

    dialog = GetNewDialog(kSerialDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return;
    }

    GetPort(&savePort);
    SetPort(dialog);

    /* Initialize dialog with current settings */
    tempPort = gCurrentPort;
    tempBaud = gCurrentBaud;

    /* Set port radio buttons */
    SetRadioButton(dialog, kSerialSettingsModemPort, tempPort == kPortModem);
    SetRadioButton(dialog, kSerialSettingsPrinterPort, tempPort == kPortPrinter);

    /* Set baud rate radio buttons */
    SetRadioButton(dialog, kSerialSettingsBaud1200, tempBaud == kBaud1200);
    SetRadioButton(dialog, kSerialSettingsBaud2400, tempBaud == kBaud2400);
    SetRadioButton(dialog, kSerialSettingsBaud9600, tempBaud == kBaud9600);
    SetRadioButton(dialog, kSerialSettingsBaud19200, tempBaud == kBaud19200);
    SetRadioButton(dialog, kSerialSettingsBaud38400, tempBaud == kBaud38400);
    SetRadioButton(dialog, kSerialSettingsBaud57600, tempBaud == kBaud57600);

    /* Dialog event loop */
    done = false;
    while (!done) {
        ModalDialog(NULL, &itemHit);

        if (itemHit == kSerialSettingsOK) {
            /* Apply settings and reinitialize serial port */
            gCurrentPort = tempPort;
            gCurrentBaud = tempBaud;
            ReinitializeSerial();
            done = true;
        } else if (itemHit == kSerialSettingsCancel) {
            /* Discard changes */
            done = true;
        } else if (itemHit == kSerialSettingsModemPort || itemHit == kSerialSettingsPrinterPort) {
            /* Port selection: kSerialSettingsModemPort maps to kPortModem (0),
               kSerialSettingsPrinterPort maps to kPortPrinter (1) */
            tempPort = itemHit - kSerialSettingsModemPort;
            SetRadioButton(dialog, kSerialSettingsModemPort, itemHit == kSerialSettingsModemPort);
            SetRadioButton(dialog, kSerialSettingsPrinterPort, itemHit == kSerialSettingsPrinterPort);
        } else if (itemHit >= kSerialSettingsBaud1200 && itemHit <= kSerialSettingsBaud57600) {
            /* Baud rate selection: dialog item IDs are consecutive and map
               directly to kBaud constants (e.g., kSerialSettingsBaud1200 -> kBaud1200) */
            short i;
            tempBaud = itemHit - kSerialSettingsBaud1200;
            for (i = kSerialSettingsBaud1200; i <= kSerialSettingsBaud57600; i++) {
                SetRadioButton(dialog, i, i == itemHit);
            }
        }
    }

    SetPort(savePort);
    DisposeDialog(dialog);
}

/*
 * Show the Clock dialog
 */
void DoClockDialog(void)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    char timeStr[256];
    Str255 pTimeStr;

    dialog = GetNewDialog(kClockDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return;
    }

    /* Try to get clock time from fujinet-nio */
    if (GetClockTime(timeStr, sizeof(timeStr))) {
        /* Convert C string to Pascal string */
        short len = 0;
        while (timeStr[len] && len < 255) {
            pTimeStr[len + 1] = timeStr[len];
            len++;
        }
        pTimeStr[0] = len;

        /* Update the time display text (item 3) */
        GetDialogItem(dialog, 3, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, pTimeStr);
    } else {
        /* Show error message */
        GetDialogItem(dialog, 3, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, "\pError: Could not read clock");
    }

    /* Wait for OK button */
    ModalDialog(NULL, &itemHit);
    DisposeDialog(dialog);
}

/*
 * Show the "Mounting disk:" dialog for a single disk (double-click)
 */
void DoMountDiskDialog(const char *diskFile)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;

    dialog = GetNewDialog(kBootDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        return;
    }

    /* Build "Mounting disk: <file>" message in item 2 */
    {
        Str255 msg;
        char *prefix = "Mounting disk: ";
        short pos = 1;
        short i;

        while (*prefix && pos < 254) {
            msg[pos++] = *prefix++;
        }
        for (i = 0; diskFile[i] != '\0' && pos < 254; i++) {
            msg[pos++] = diskFile[i];
        }
        msg[0] = pos - 1;

        GetDialogItem(dialog, 2, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, msg);
    }

    ModalDialog(NULL, &itemHit);
    DisposeDialog(dialog);
}

/*
 * Show the "Mounting disks:" dialog listing all populated disk slots
 */
void DoMountDisksDialog(void)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    char buf[512];
    short pos;
    short i, j;

    dialog = GetNewDialog(kBootDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        return;
    }

    /* Build message listing all mounted disks */
    pos = 0;
    {
        char *header = "Mounting disks:";
        while (*header && pos < 500) {
            buf[pos++] = *header++;
        }
    }

    for (i = 0; i < kNumDiskSlots; i++) {
        if (gDiskSlots[i].mounted) {
            /* Add newline */
            if (pos < 500) buf[pos++] = '\r';
            /* Slot number */
            if (pos < 500) buf[pos++] = '0' + (i + 1);
            if (pos < 500) buf[pos++] = gDiskSlots[i].readOnly ? 'R' : 'W';
            if (pos < 500) buf[pos++] = ' ';
            /* Path */
            for (j = 0; gDiskSlots[i].path[j] != '\0' && pos < 500; j++) {
                buf[pos++] = gDiskSlots[i].path[j];
            }
        }
    }

    /* Convert to Pascal string and set dialog text */
    {
        Str255 msg;
        short len = pos < 255 ? pos : 255;
        short k;
        msg[0] = len;
        for (k = 0; k < len; k++) {
            msg[k + 1] = buf[k];
        }
        GetDialogItem(dialog, 2, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, msg);
    }

    ModalDialog(NULL, &itemHit);
    DisposeDialog(dialog);
}

/*
 * Show the Device Info dialog
 * Displays MAC address, IP address, SSID, and firmware version.
 * Currently shows "N/A" since fujinet-nio doesn't implement these queries yet.
 */
void DoDeviceInfoDialog(void)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;

    dialog = GetNewDialog(kDeviceInfoDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return;
    }

    /* Set all value fields to "N/A" (items 4, 6, 8, 10) */
    GetDialogItem(dialog, 4, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, "\pN/A");
    GetDialogItem(dialog, 6, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, "\pN/A");
    GetDialogItem(dialog, 8, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, "\pN/A");
    GetDialogItem(dialog, 10, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, "\pN/A");

    ModalDialog(NULL, &itemHit);
    DisposeDialog(dialog);
}

/*
 * Show the Edit Host dialog for the given slot number (0-7)
 */
void DoEditHostDialog(short slotNum)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    Str255 pHostName;
    Boolean done;
    GrafPtr savePort;

    if (slotNum < 0 || slotNum >= kNumHostSlots) {
        return;
    }

    dialog = GetNewDialog(kEditHostDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return;
    }

    GetPort(&savePort);
    SetPort(dialog);

    /* Pre-populate text field with current hostname */
    {
        /* Use bounded scan instead of strlen to stay within the 32-byte slot */
        short len = 0;
        while (len < 31 && gHostSlots[slotNum][len] != '\0') {
            len++;
        }
        pHostName[0] = len;
        memcpy(&pHostName[1], gHostSlots[slotNum], len);
    }
    GetDialogItem(dialog, kEditHostText, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, pHostName);

    /* Select all text for easy replacement */
    SelectDialogItemText(dialog, kEditHostText, 0, 32767);

    /* Dialog event loop */
    done = false;
    while (!done) {
        ModalDialog(NULL, &itemHit);

        if (itemHit == kEditHostOK) {
            /* Get the new hostname text */
            Str255 newName;
            short nameLen;

            GetDialogItem(dialog, kEditHostText, &itemType, &itemHandle, &itemRect);
            GetDialogItemText(itemHandle, newName);

            /* Convert Pascal string to C string into gHostSlots */
            nameLen = newName[0];
            if (nameLen > 31) nameLen = 31;
            memcpy(gHostSlots[slotNum], &newName[1], nameLen);
            gHostSlots[slotNum][nameLen] = '\0';

            /* Send updated host slots to fujinet-nio */
            WriteHostSlots(gHostSlots, kNumHostSlots);

            done = true;
        } else if (itemHit == kEditHostCancel) {
            done = true;
        }
    }

    SetPort(savePort);
    DisposeDialog(dialog);
}

/*
 * Show the Mount dialog for selecting disk slot and R/W mode.
 * Returns true if user clicked OK, false if Cancel.
 */
Boolean DoMountDialog(short hostSlot, const char *filePath,
                      short *outSlot, Boolean *outReadOnly)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    Str255 pFilePath;
    Boolean done;
    GrafPtr savePort;
    short selectedSlot;
    Boolean selectedReadOnly;
    short i;

    dialog = GetNewDialog(kMountDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return false;
    }

    GetPort(&savePort);
    SetPort(dialog);

    /* Set file path text (item 4) */
    {
        short len = strlen(filePath);
        if (len > 255) len = 255;
        pFilePath[0] = len;
        memcpy(&pFilePath[1], filePath, len);
    }
    GetDialogItem(dialog, kMountFilePath, &itemType, &itemHandle, &itemRect);
    SetDialogItemText(itemHandle, pFilePath);

    /* Default: slot 1, Read mode */
    selectedSlot = 1;
    selectedReadOnly = true;

    /* Initialize slot radio buttons (slot 1 selected) */
    for (i = kMountSlot1; i <= kMountSlot8; i++) {
        SetRadioButton(dialog, i, i == kMountSlot1);
    }

    /* Initialize mode radio buttons (Read selected) */
    SetRadioButton(dialog, kMountRead, true);
    SetRadioButton(dialog, kMountWrite, false);

    /* Dialog event loop */
    done = false;
    while (!done) {
        ModalDialog(NULL, &itemHit);

        if (itemHit == kMountOK) {
            *outSlot = selectedSlot;
            *outReadOnly = selectedReadOnly;
            SetPort(savePort);
            DisposeDialog(dialog);
            return true;
        } else if (itemHit == kMountCancel) {
            done = true;
        } else if (itemHit >= kMountSlot1 && itemHit <= kMountSlot8) {
            /* Slot radio button clicked */
            selectedSlot = itemHit - kMountSlot1 + 1;  /* 1-based */
            for (i = kMountSlot1; i <= kMountSlot8; i++) {
                SetRadioButton(dialog, i, i == itemHit);
            }
        } else if (itemHit == kMountRead || itemHit == kMountWrite) {
            /* Mode radio button clicked */
            selectedReadOnly = (itemHit == kMountRead);
            SetRadioButton(dialog, kMountRead, itemHit == kMountRead);
            SetRadioButton(dialog, kMountWrite, itemHit == kMountWrite);
        }
    }

    SetPort(savePort);
    DisposeDialog(dialog);
    return false;
}

/* ================================================================
 * File Browser (modal dialog version)
 * ================================================================ */

/* File browser state (file-scope statics) */
static ListHandle sBrowserList;
static short sBrowserHostSlot;
static char sBrowserFsName[32];
static char sBrowserPath[256];
static DirEntry sBrowserEntries[kMaxBrowserEntries];
static short sBrowserEntryCount;
static short sBrowserSelectedIdx;

/*
 * Refresh the browser list from the current directory path.
 */
static void BrowserRefresh(DialogPtr dialog)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    Boolean hasMore;
    short i, j;
    Cell theCell;

    /* Query directory contents */
    sBrowserEntryCount = 0;
    hasMore = false;

    ListDirectory(sBrowserFsName, sBrowserPath,
                  0, kMaxBrowserEntries,
                  sBrowserEntries, &sBrowserEntryCount, &hasMore);

    /* Resize list to match entry count */
    if (sBrowserList != NULL) {
        LDelRow(0, 0, sBrowserList);
        if (sBrowserEntryCount > 0) {
            LAddRow(sBrowserEntryCount, 0, sBrowserList);
        }

        /* Populate list cells */
        for (i = 0; i < sBrowserEntryCount; i++) {
            char displayStr[80];
            short pos = 0;

            for (j = 0; sBrowserEntries[i].name[j] != '\0' && pos < 76; j++) {
                displayStr[pos++] = sBrowserEntries[i].name[j];
            }
            if (sBrowserEntries[i].isDir && pos < 79) {
                displayStr[pos++] = '/';
            }

            SetPt(&theCell, 0, i);
            LSetCell(displayStr, pos, theCell, sBrowserList);
        }
    }

    /* Update path display (item 4) */
    {
        Str255 pPath;
        short len = strlen(sBrowserPath);
        if (len > 255) len = 255;
        pPath[0] = len;
        memcpy(&pPath[1], sBrowserPath, len);
        GetDialogItem(dialog, kBrowserDlgPath, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, pPath);
    }

    /* Invalidate list area to force redraw */
    GetDialogItem(dialog, kBrowserDlgList, &itemType, &itemHandle, &itemRect);
    InvalRect(&itemRect);
}

/*
 * Navigate up one directory level.
 */
static void BrowserNavigateUp(DialogPtr dialog)
{
    short len = strlen(sBrowserPath);
    if (len > 1) {
        /* Remove trailing slash if not root */
        if (sBrowserPath[len - 1] == '/' && len > 1) {
            len--;
        }
        /* Find the last slash */
        while (len > 0 && sBrowserPath[len - 1] != '/') {
            len--;
        }
        if (len == 0) len = 1;
        sBrowserPath[len] = '\0';
        BrowserRefresh(dialog);
    }
}

/*
 * Navigate into a subdirectory by index.
 */
static void BrowserNavigateInto(DialogPtr dialog, short idx)
{
    short pathLen = strlen(sBrowserPath);
    short nameLen = strlen(sBrowserEntries[idx].name);

    /* Ensure trailing slash on current path */
    if (pathLen > 0 && sBrowserPath[pathLen - 1] != '/') {
        if (pathLen < kMaxPathLen - 1) {
            sBrowserPath[pathLen++] = '/';
        }
    }
    /* Append directory name + trailing slash */
    if (pathLen + nameLen < kMaxPathLen - 1) {
        memcpy(sBrowserPath + pathLen, sBrowserEntries[idx].name, nameLen);
        pathLen += nameLen;
        sBrowserPath[pathLen++] = '/';
        sBrowserPath[pathLen] = '\0';
        BrowserRefresh(dialog);
    }
}

/*
 * UserItem draw proc for the list area.
 */
static pascal void BrowserListDrawProc(DialogPtr dialog, short itemNo)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;

    GetDialogItem(dialog, itemNo, &itemType, &itemHandle, &itemRect);
    FrameRect(&itemRect);

    if (sBrowserList != NULL) {
        LUpdate(((WindowPtr)dialog)->visRgn, sBrowserList);
    }
}

/*
 * Modal dialog filter proc for the file browser.
 * Handles mouse clicks in the list area.
 */
static pascal Boolean BrowserFilterProc(DialogPtr dialog,
                                        EventRecord *event,
                                        short *itemHit)
{
    if (event->what == mouseDown) {
        Point localPt;
        short itemType;
        Handle itemHandle;
        Rect listRect;
        GrafPtr savePort;

        GetPort(&savePort);
        SetPort(dialog);
        localPt = event->where;
        GlobalToLocal(&localPt);

        GetDialogItem(dialog, kBrowserDlgList, &itemType, &itemHandle, &listRect);

        if (PtInRect(localPt, &listRect) && sBrowserList != NULL) {
            Boolean dblClick = LClick(localPt, event->modifiers, sBrowserList);
            if (dblClick) {
                Cell theCell;
                SetPt(&theCell, 0, 0);
                if (LGetSelect(true, &theCell, sBrowserList)) {
                    short idx = theCell.v;
                    if (idx >= 0 && idx < sBrowserEntryCount) {
                        if (sBrowserEntries[idx].isDir) {
                            BrowserNavigateInto(dialog, idx);
                        } else {
                            /* File double-clicked: trigger mount */
                            sBrowserSelectedIdx = idx;
                            *itemHit = kBrowserDlgMount;
                            SetPort(savePort);
                            return true;
                        }
                    }
                }
            }
            SetPort(savePort);
            return true;  /* We consumed the click */
        }
        SetPort(savePort);
    }
    return false;
}

/*
 * Show the File Browser as a modal dialog.
 * Returns true if a disk was mounted, false if cancelled.
 */
Boolean DoFileBrowser(short hostSlot)
{
    DialogPtr dialog;
    short itemHit;
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    GrafPtr savePort;
    Boolean done;
    Boolean mounted = false;
    short i;
    Rect dataBounds;
    Point cellSize;

    if (hostSlot < 0 || hostSlot >= kNumHostSlots
        || gHostSlots[hostSlot][0] == '\0') {
        SysBeep(10);
        return false;
    }

    dialog = GetNewDialog(kBrowserDialogID, NULL, (WindowPtr)-1);
    if (dialog == NULL) {
        SysBeep(10);
        return false;
    }

    GetPort(&savePort);
    SetPort(dialog);

    /* Initialize browser state */
    sBrowserHostSlot = hostSlot;
    for (i = 0; i < 31 && gHostSlots[hostSlot][i] != '\0'; i++) {
        sBrowserFsName[i] = gHostSlots[hostSlot][i];
    }
    sBrowserFsName[i] = '\0';
    sBrowserPath[0] = '/';
    sBrowserPath[1] = '\0';
    sBrowserEntryCount = 0;
    sBrowserSelectedIdx = -1;
    sBrowserList = NULL;

    /* Get the UserItem rect for the list area */
    GetDialogItem(dialog, kBrowserDlgList, &itemType, &itemHandle, &itemRect);

    /* Install the draw proc for the UserItem */
    SetDialogItem(dialog, kBrowserDlgList, itemType,
                  (Handle)BrowserListDrawProc, &itemRect);

    /* Create ListHandle inside the UserItem area */
    {
        Rect listRect;
        SetRect(&listRect, itemRect.left + 1, itemRect.top + 1,
                itemRect.right - 1 - 15, itemRect.bottom - 1);
        SetRect(&dataBounds, 0, 0, 1, 0);
        SetPt(&cellSize, itemRect.right - itemRect.left - 2 - 15,
              kBrowserCellHeight);

        sBrowserList = LNew(&listRect, &dataBounds, cellSize,
                            0, (WindowPtr)dialog, true, false, false, true);
    }

    if (sBrowserList != NULL) {
        (*sBrowserList)->selFlags = lOnlyOne;
    }

    /* Load initial directory */
    BrowserRefresh(dialog);

    /* Modal dialog loop */
    done = false;
    while (!done) {
        ModalDialog((ModalFilterProcPtr)BrowserFilterProc, &itemHit);

        switch (itemHit) {
            case kBrowserDlgMount: {
                Cell theCell;
                short idx = -1;

                /* Check if set by filter proc (double-click on file) */
                if (sBrowserSelectedIdx >= 0) {
                    idx = sBrowserSelectedIdx;
                    sBrowserSelectedIdx = -1;
                } else {
                    /* Mount button: use current list selection */
                    SetPt(&theCell, 0, 0);
                    if (sBrowserList != NULL
                        && LGetSelect(true, &theCell, sBrowserList)) {
                        idx = theCell.v;
                    }
                }

                if (idx >= 0 && idx < sBrowserEntryCount) {
                    if (sBrowserEntries[idx].isDir) {
                        BrowserNavigateInto(dialog, idx);
                    } else {
                        /* Build full path */
                        char fullPath[512];
                        short pathLen = strlen(sBrowserPath);
                        short nameLen = strlen(sBrowserEntries[idx].name);
                        short outSlot;
                        Boolean outReadOnly;

                        memcpy(fullPath, sBrowserPath, pathLen);
                        if (pathLen > 0 && fullPath[pathLen - 1] != '/') {
                            fullPath[pathLen++] = '/';
                        }
                        memcpy(fullPath + pathLen,
                               sBrowserEntries[idx].name, nameLen);
                        fullPath[pathLen + nameLen] = '\0';

                        if (DoMountDialog(sBrowserHostSlot, fullPath,
                                          &outSlot, &outReadOnly)) {
                            /* Mount the disk */
                            MountDisk(outSlot, outReadOnly,
                                      sBrowserFsName, fullPath);

                            /* Update disk slot state (outSlot is 1-based) */
                            {
                                short slotIdx = outSlot - 1;
                                if (slotIdx >= 0 && slotIdx < kNumDiskSlots) {
                                    short k;
                                    gDiskSlots[slotIdx].mounted = true;
                                    gDiskSlots[slotIdx].hostSlot =
                                        sBrowserHostSlot + 1;
                                    gDiskSlots[slotIdx].readOnly = outReadOnly;
                                    for (k = 0; fullPath[k] != '\0'
                                         && k < 255; k++) {
                                        gDiskSlots[slotIdx].path[k] =
                                            fullPath[k];
                                    }
                                    gDiskSlots[slotIdx].path[k] = '\0';
                                }
                            }

                            mounted = true;
                            done = true;
                        }
                    }
                }
                break;
            }

            case kBrowserDlgCancel:
                done = true;
                break;

            case kBrowserDlgUp:
                BrowserNavigateUp(dialog);
                break;
        }
    }

    /* Clean up */
    if (sBrowserList != NULL) {
        LDispose(sBrowserList);
        sBrowserList = NULL;
    }

    SetPort(savePort);
    DisposeDialog(dialog);

    return mounted;
}
