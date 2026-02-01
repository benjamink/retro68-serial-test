/*
 * SerialSend - A simple classic Mac application
 * Displays a window with a text input, a "Send" button, and a receive area.
 * Sends the text to the serial port when the button is clicked.
 * Displays received serial data in the receive area.
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
#include <Devices.h>
#include <Serial.h>
#include <Memory.h>
#include <ToolUtils.h>
#include <SegLoad.h>
#include <Sound.h>

/* Resource IDs */
#define kMenuBarID      128
#define kAppleMenuID    128
#define kFileMenuID     129
#define kEditMenuID     130

#define kAboutDialogID  128
#define kSettingsDialogID 129
#define kSendButtonID   128

/* Settings dialog item IDs */
#define kSettingsOK         1
#define kSettingsCancel     2
#define kSettingsPortLabel  3
#define kSettingsModemPort  4
#define kSettingsPrinterPort 5
#define kSettingsBaudLabel  6
#define kSettingsBaud1200   7
#define kSettingsBaud2400   8
#define kSettingsBaud9600   9
#define kSettingsBaud19200  10
#define kSettingsBaud38400  11
#define kSettingsBaud57600  12

/* Port selection */
#define kPortModem      0
#define kPortPrinter    1

/* Baud rate indices */
#define kBaud1200   0
#define kBaud2400   1
#define kBaud9600   2
#define kBaud19200  3
#define kBaud38400  4
#define kBaud57600  5

/* Window dimensions */
#define kWindowWidth    320
#define kWindowHeight   195

/* Send text area positions */
#define kSendLeft       10
#define kSendTop        30
#define kSendRight      310
#define kSendBottom     75

/* Button position */
#define kButtonLeft     120
#define kButtonTop      82
#define kButtonRight    200
#define kButtonBottom   102

/* Receive text area positions */
#define kRecvLeft       10
#define kRecvTop        122
#define kRecvRight      310
#define kRecvBottom     185

/* Maximum receive buffer size */
#define kMaxReceiveText 4096

/* Serial port driver reference numbers */
static short gSerialOutRef = 0;
static short gSerialInRef = 0;

/* Serial port settings */
static short gCurrentPort = kPortModem;     /* 0 = Modem (A), 1 = Printer (B) */
static short gCurrentBaud = kBaud9600;      /* Default to 9600 */

/* Baud rate constants for SerReset (from Serial.h) */
static short gBaudRates[] = {
    baud1200,   /* kBaud1200 */
    baud2400,   /* kBaud2400 */
    baud9600,   /* kBaud9600 */
    baud19200,  /* kBaud19200 */
    baud38400,  /* kBaud38400 */
    baud57600   /* kBaud57600 */
};

/* Baud rate names for debug output */
static char *gBaudNames[] = {
    "1200", "2400", "9600", "19200", "38400", "57600"
};

/* Application globals */
static WindowPtr gMainWindow = NULL;
static TEHandle gSendText = NULL;
static TEHandle gRecvText = NULL;
static ControlHandle gSendButton = NULL;
static Boolean gRunning = true;

/* Function prototypes */
static void InitializeToolbox(void);
static void InitializeMenus(void);
static Boolean InitializeSerial(void);
static void CleanupSerial(void);
static void CreateMainWindow(void);
static void HandleEvent(EventRecord *event);
static void HandleMouseDown(EventRecord *event);
static void HandleKeyDown(EventRecord *event);
static void HandleMenuChoice(long menuChoice);
static void HandleAppleMenu(short item);
static void HandleFileMenu(short item);
static void HandleEditMenu(short item);
static void UpdateWindow(WindowPtr window);
static void SendTextToSerial(void);
static void PollSerialInput(void);
static void DoAboutDialog(void);
static void DoSettingsDialog(void);
static Boolean ReinitializeSerial(void);

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
 * Set up the menu bar
 */
static void InitializeMenus(void)
{
    Handle menuBar;
    MenuHandle appleMenu;

    menuBar = GetNewMBar(kMenuBarID);
    if (menuBar != NULL) {
        SetMenuBar(menuBar);

        /* Add desk accessories to Apple menu */
        appleMenu = GetMenuHandle(kAppleMenuID);
        if (appleMenu != NULL) {
            AppendResMenu(appleMenu, 'DRVR');
        }

        DrawMenuBar();
    }
}

/*
 * Initialize the serial port using current settings
 * Port A (Modem) connects to /dev/tnt1 in emulator (use /dev/tnt0 on host)
 * Port B (Printer) outputs to ser_b.out file
 */
static Boolean InitializeSerial(void)
{
    OSErr err;
    SerShk handshake;

    /* Select driver names based on port setting */
    if (gCurrentPort == kPortModem) {
        /* Open modem port (port A) */
        err = OpenDriver("\p.AOut", &gSerialOutRef);
        if (err != noErr) {
            return false;
        }
        err = OpenDriver("\p.AIn", &gSerialInRef);
        if (err != noErr) {
            return false;
        }
    } else {
        /* Open printer port (port B) */
        err = OpenDriver("\p.BOut", &gSerialOutRef);
        if (err != noErr) {
            return false;
        }
        err = OpenDriver("\p.BIn", &gSerialInRef);
        if (err != noErr) {
            return false;
        }
    }

    /* Configure handshaking - no flow control */
    handshake.fXOn = 0;
    handshake.fCTS = 0;
    handshake.errs = 0;
    handshake.evts = 0;
    handshake.fInX = 0;

    SerHShake(gSerialInRef, &handshake);

    /* Set baud rate based on current setting, 8N1 */
    SerReset(gSerialOutRef, gBaudRates[gCurrentBaud] + stop10 + noParity + data8);
    SerReset(gSerialInRef, gBaudRates[gCurrentBaud] + stop10 + noParity + data8);

    /* Send test message with port and baud info */
    {
        char msg[64];
        char *prefix = "Serial ready: ";
        char *portName = (gCurrentPort == kPortModem) ? "Modem" : "Printer";
        char *baudName = gBaudNames[gCurrentBaud];
        long count = 0;

        /* Build message: "Serial ready: Modem @ 9600\r\n" */
        while (*prefix) {
            msg[count++] = *prefix++;
        }
        while (*portName) {
            msg[count++] = *portName++;
        }
        msg[count++] = ' ';
        msg[count++] = '@';
        msg[count++] = ' ';
        while (*baudName) {
            msg[count++] = *baudName++;
        }
        msg[count++] = '\r';
        msg[count++] = '\n';

        FSWrite(gSerialOutRef, &count, msg);
    }

    return true;
}

/*
 * Close the serial port
 */
static void CleanupSerial(void)
{
    if (gSerialOutRef != 0) {
        CloseDriver(gSerialOutRef);
        gSerialOutRef = 0;
    }
    if (gSerialInRef != 0) {
        CloseDriver(gSerialInRef);
        gSerialInRef = 0;
    }
}

/*
 * Create the main application window with send/receive text areas and button
 */
static void CreateMainWindow(void)
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

    gMainWindow = NewWindow(NULL, &windowRect, "\pSerial Terminal",
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
    SetRect(&buttonRect, kButtonLeft, kButtonTop, kButtonRight, kButtonBottom);
    gSendButton = NewControl(gMainWindow, &buttonRect, "\pSend",
                             true, 0, 0, 1, pushButProc, 0);

    /* Create receive text area - inset from frame border */
    SetRect(&textRect, kRecvLeft + 4, kRecvTop + 4, kRecvRight - 4, kRecvBottom - 4);
    gRecvText = TENew(&textRect, &textRect);

    /* Make receive text read-only by not activating it */
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

                /* Check if click is in button */
                if (gSendButton != NULL) {
                    if (FindControl(localPoint, window, &control) == kControlButtonPart) {
                        if (TrackControl(control, localPoint, NULL) == kControlButtonPart) {
                            SendTextToSerial();
                        }
                        return;
                    }
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

/*
 * Handle menu selections
 */
static void HandleMenuChoice(long menuChoice)
{
    short menuID;
    short menuItem;

    if (menuChoice == 0) {
        return;
    }

    menuID = HiWord(menuChoice);
    menuItem = LoWord(menuChoice);

    switch (menuID) {
        case kAppleMenuID:
            HandleAppleMenu(menuItem);
            break;

        case kFileMenuID:
            HandleFileMenu(menuItem);
            break;

        case kEditMenuID:
            HandleEditMenu(menuItem);
            break;
    }

    HiliteMenu(0);
}

/*
 * Handle Apple menu items
 */
static void HandleAppleMenu(short item)
{
    Str255 itemName;
    MenuHandle appleMenu;

    if (item == 1) {
        DoAboutDialog();
    } else {
        /* Open desk accessory */
        appleMenu = GetMenuHandle(kAppleMenuID);
        if (appleMenu != NULL) {
            GetMenuItemText(appleMenu, item, itemName);
            OpenDeskAcc(itemName);
        }
    }
}

/*
 * Handle File menu items
 */
static void HandleFileMenu(short item)
{
    switch (item) {
        case 1: /* Send */
            SendTextToSerial();
            break;

        case 3: /* Settings... */
            DoSettingsDialog();
            break;

        case 5: /* Quit */
            gRunning = false;
            break;
    }
}

/*
 * Handle Edit menu items
 */
static void HandleEditMenu(short item)
{
    if (gSendText == NULL) {
        return;
    }

    switch (item) {
        case 1: /* Undo */
            /* TextEdit doesn't support undo */
            break;

        case 3: /* Cut */
            TECut(gSendText);
            break;

        case 4: /* Copy */
            TECopy(gSendText);
            break;

        case 5: /* Paste */
            TEPaste(gSendText);
            break;

        case 6: /* Clear */
            TEDelete(gSendText);
            break;

        case 8: /* Select All */
            TESetSelect(0, 32767, gSendText);
            break;
    }
}

/*
 * Update window contents
 */
static void UpdateWindow(WindowPtr window)
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

/*
 * Send the text from the text edit field to the serial port
 * Converts Mac line endings (CR) to CR+LF for serial output
 */
static void SendTextToSerial(void)
{
    Handle textHandle;
    long textLength;
    long count;
    char *textPtr;
    long i;
    char crlf[2] = {'\r', '\n'};

    if (gSendText == NULL) {
        return;
    }

    /* Get the text from TextEdit */
    textHandle = (Handle)TEGetText(gSendText);
    textLength = (*gSendText)->teLength;

    if (textLength == 0) {
        SysBeep(10);
        return;
    }

    if (gSerialOutRef == 0) {
        SysBeep(10);
        return;
    }

    /* Lock the handle and get pointer */
    HLock(textHandle);
    textPtr = *textHandle;

    /* Write to serial port, converting CR to CR+LF */
    for (i = 0; i < textLength; i++) {
        if (textPtr[i] == '\r') {
            /* Send CR+LF for line endings */
            count = 2;
            FSWrite(gSerialOutRef, &count, crlf);
        } else {
            /* Send single character */
            count = 1;
            FSWrite(gSerialOutRef, &count, &textPtr[i]);
        }
    }

    /* Send final CR+LF if text doesn't end with one */
    if (textLength > 0 && textPtr[textLength - 1] != '\r') {
        count = 2;
        FSWrite(gSerialOutRef, &count, crlf);
    }

    HUnlock(textHandle);

    /* Flash the button to indicate success */
    HiliteControl(gSendButton, 1);
    Delay(8, NULL);
    HiliteControl(gSendButton, 0);

    /* Clear the text field */
    TESetSelect(0, 32767, gSendText);
    TEDelete(gSendText);
}

/*
 * Poll for incoming serial data and display in receive area
 */
static void PollSerialInput(void)
{
    long count;
    char buffer[256];
    long i;
    Rect textFrame;

    if (gSerialInRef == 0 || gRecvText == NULL) {
        return;
    }

    /* Check how many bytes are available */
    SerGetBuf(gSerialInRef, &count);

    if (count <= 0) {
        return;
    }

    /* Limit to buffer size */
    if (count > sizeof(buffer)) {
        count = sizeof(buffer);
    }

    /* Read the data */
    FSRead(gSerialInRef, &count, buffer);

    /* Process received characters */
    SetPort(gMainWindow);

    for (i = 0; i < count; i++) {
        char c = buffer[i];

        /* Convert LF to CR for Mac TextEdit */
        if (c == '\n') {
            c = '\r';
        }

        /* Skip CR if followed by LF (handle CRLF) */
        if (c == '\r' && i + 1 < count && buffer[i + 1] == '\n') {
            continue;
        }

        /* Insert character at end of receive text */
        TESetSelect(32767, 32767, gRecvText);
        TEKey(c, gRecvText);
    }

    /* Limit receive buffer size - remove oldest text if too large */
    if ((*gRecvText)->teLength > kMaxReceiveText) {
        TESetSelect(0, (*gRecvText)->teLength - kMaxReceiveText, gRecvText);
        TEDelete(gRecvText);
    }

    /* Update the receive area */
    SetRect(&textFrame, kRecvLeft, kRecvTop, kRecvRight, kRecvBottom);
    InvalRect(&textFrame);
}

/*
 * Show the About dialog
 */
static void DoAboutDialog(void)
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
 * Reinitialize serial port with new settings
 */
static Boolean ReinitializeSerial(void)
{
    CleanupSerial();
    return InitializeSerial();
}

/*
 * Set a radio button and clear others in the same group
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
 * Get the value of a radio button
 */
static Boolean GetRadioButton(DialogPtr dialog, short item)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;

    GetDialogItem(dialog, item, &itemType, &itemHandle, &itemRect);
    return GetControlValue((ControlHandle)itemHandle) != 0;
}

/*
 * Show the Settings dialog
 */
static void DoSettingsDialog(void)
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
