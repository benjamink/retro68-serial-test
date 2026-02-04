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

    done = false;
    while (!done) {
        ModalDialog(NULL, &itemHit);

        switch (itemHit) {
            case kSettingsOK:
                /* Apply settings */
                gCurrentPort = tempPort;
                gCurrentBaud = tempBaud;
                ReinitializeSerial();
                done = true;
                break;

            case kSettingsCancel:
                done = true;
                break;

            /* Port selection */
            case kSettingsModemPort:
                tempPort = kPortModem;
                SetRadioButton(dialog, kSettingsModemPort, true);
                SetRadioButton(dialog, kSettingsPrinterPort, false);
                break;

            case kSettingsPrinterPort:
                tempPort = kPortPrinter;
                SetRadioButton(dialog, kSettingsModemPort, false);
                SetRadioButton(dialog, kSettingsPrinterPort, true);
                break;

            /* Baud rate selection */
            case kSettingsBaud1200:
                tempBaud = kBaud1200;
                SetRadioButton(dialog, kSettingsBaud1200, true);
                SetRadioButton(dialog, kSettingsBaud2400, false);
                SetRadioButton(dialog, kSettingsBaud9600, false);
                SetRadioButton(dialog, kSettingsBaud19200, false);
                SetRadioButton(dialog, kSettingsBaud38400, false);
                SetRadioButton(dialog, kSettingsBaud57600, false);
                break;

            case kSettingsBaud2400:
                tempBaud = kBaud2400;
                SetRadioButton(dialog, kSettingsBaud1200, false);
                SetRadioButton(dialog, kSettingsBaud2400, true);
                SetRadioButton(dialog, kSettingsBaud9600, false);
                SetRadioButton(dialog, kSettingsBaud19200, false);
                SetRadioButton(dialog, kSettingsBaud38400, false);
                SetRadioButton(dialog, kSettingsBaud57600, false);
                break;

            case kSettingsBaud9600:
                tempBaud = kBaud9600;
                SetRadioButton(dialog, kSettingsBaud1200, false);
                SetRadioButton(dialog, kSettingsBaud2400, false);
                SetRadioButton(dialog, kSettingsBaud9600, true);
                SetRadioButton(dialog, kSettingsBaud19200, false);
                SetRadioButton(dialog, kSettingsBaud38400, false);
                SetRadioButton(dialog, kSettingsBaud57600, false);
                break;

            case kSettingsBaud19200:
                tempBaud = kBaud19200;
                SetRadioButton(dialog, kSettingsBaud1200, false);
                SetRadioButton(dialog, kSettingsBaud2400, false);
                SetRadioButton(dialog, kSettingsBaud9600, false);
                SetRadioButton(dialog, kSettingsBaud19200, true);
                SetRadioButton(dialog, kSettingsBaud38400, false);
                SetRadioButton(dialog, kSettingsBaud57600, false);
                break;

            case kSettingsBaud38400:
                tempBaud = kBaud38400;
                SetRadioButton(dialog, kSettingsBaud1200, false);
                SetRadioButton(dialog, kSettingsBaud2400, false);
                SetRadioButton(dialog, kSettingsBaud9600, false);
                SetRadioButton(dialog, kSettingsBaud19200, false);
                SetRadioButton(dialog, kSettingsBaud38400, true);
                SetRadioButton(dialog, kSettingsBaud57600, false);
                break;

            case kSettingsBaud57600:
                tempBaud = kBaud57600;
                SetRadioButton(dialog, kSettingsBaud1200, false);
                SetRadioButton(dialog, kSettingsBaud2400, false);
                SetRadioButton(dialog, kSettingsBaud9600, false);
                SetRadioButton(dialog, kSettingsBaud19200, false);
                SetRadioButton(dialog, kSettingsBaud38400, false);
                SetRadioButton(dialog, kSettingsBaud57600, true);
                break;
        }
    }

    SetPort(savePort);
    DisposeDialog(dialog);
}
