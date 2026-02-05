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

    dialog = GetNewDialog(kSettingsDialogID, NULL, (WindowPtr)-1);
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
    SetRadioButton(dialog, kSettingsModemPort, tempPort == kPortModem);
    SetRadioButton(dialog, kSettingsPrinterPort, tempPort == kPortPrinter);

    /* Set baud rate radio buttons */
    SetRadioButton(dialog, kSettingsBaud1200, tempBaud == kBaud1200);
    SetRadioButton(dialog, kSettingsBaud2400, tempBaud == kBaud2400);
    SetRadioButton(dialog, kSettingsBaud9600, tempBaud == kBaud9600);
    SetRadioButton(dialog, kSettingsBaud19200, tempBaud == kBaud19200);
    SetRadioButton(dialog, kSettingsBaud38400, tempBaud == kBaud38400);
    SetRadioButton(dialog, kSettingsBaud57600, tempBaud == kBaud57600);

    /* Dialog event loop */
    done = false;
    while (!done) {
        ModalDialog(NULL, &itemHit);

        if (itemHit == kSettingsOK) {
            /* Apply settings and reinitialize serial port */
            gCurrentPort = tempPort;
            gCurrentBaud = tempBaud;
            ReinitializeSerial();
            done = true;
        } else if (itemHit == kSettingsCancel) {
            /* Discard changes */
            done = true;
        } else if (itemHit == kSettingsModemPort || itemHit == kSettingsPrinterPort) {
            /* Port selection: kSettingsModemPort maps to kPortModem (0),
               kSettingsPrinterPort maps to kPortPrinter (1) */
            tempPort = itemHit - kSettingsModemPort;
            SetRadioButton(dialog, kSettingsModemPort, itemHit == kSettingsModemPort);
            SetRadioButton(dialog, kSettingsPrinterPort, itemHit == kSettingsPrinterPort);
        } else if (itemHit >= kSettingsBaud1200 && itemHit <= kSettingsBaud57600) {
            /* Baud rate selection: dialog item IDs are consecutive and map
               directly to kBaud constants (e.g., kSettingsBaud1200 -> kBaud1200) */
            short i;
            tempBaud = itemHit - kSettingsBaud1200;
            for (i = kSettingsBaud1200; i <= kSettingsBaud57600; i++) {
                SetRadioButton(dialog, i, i == itemHit);
            }
        }
    }

    SetPort(savePort);
    DisposeDialog(dialog);
}
