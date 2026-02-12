/*
 * dialogs.c - Application dialogs
 */

#include <Dialogs.h>
#include <Controls.h>
#include <Windows.h>
#include <Sound.h>

#include <string.h>

#include "constants.h"
#include "dialogs.h"
#include "serial.h"
#include "window.h"

/* Local function prototypes */
static void SetRadioButton(DialogPtr dialog, short item, Boolean on);

/*
 * Show the About dialog
 */
void DoAboutDialog(void)
{
    DialogPtr dialog;
    short itemHit;

    dialog = GetNewDialog(kAboutDialogID, NULL, (WindowPtr)-1);
    if (dialog != NULL) {
        ModalDialog(NULL, &itemHit);
        DisposeDialog(dialog);
    }
}

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
 * Show the Booting dialog with the selected disk file name
 */
void DoBootDialog(const char *diskFile)
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

    /* Build "Booting <file>" message in item 2 */
    {
        Str255 msg;
        char *prefix = "Booting ";
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

            /* Refresh the hosts list display */
            PopulateHostsList();

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
