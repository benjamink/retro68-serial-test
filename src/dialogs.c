/*
 * dialogs.c - Application dialogs
 */

#include <Dialogs.h>
#include <Controls.h>
#include <Windows.h>
#include <Sound.h>

#include "constants.h"
#include "dialogs.h"
#include "serial.h"

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
 * Show the Booting dialog
 */
void DoBootDialog(void)
{
    DialogPtr dialog;
    short itemHit;

    dialog = GetNewDialog(kBootDialogID, NULL, (WindowPtr)-1);
    if (dialog != NULL) {
        ModalDialog(NULL, &itemHit);
        DisposeDialog(dialog);
    }
}
