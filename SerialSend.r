/*
 * SerialSend.r - Resources for SerialSend application
 */

#include "Processes.r"
#include "Menus.r"
#include "Windows.r"
#include "MacTypes.r"
#include "Dialogs.r"
#include "Finder.r"
#include "Icons.r"

/* Application signature */
type 'SSND' as 'STR ';
resource 'SSND' (0, purgeable) {
    "SerialSend 1.0"
};

/* Apple Menu */
resource 'MENU' (128) {
    128, textMenuProc;
    allEnabled, enabled;
    apple;
    {
        "About SerialSend...", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain;
    }
};

/* File Menu */
resource 'MENU' (129) {
    129, textMenuProc;
    allEnabled, enabled;
    "File";
    {
        "Send", noIcon, "S", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Settings...", noIcon, ",", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

/* Edit Menu */
resource 'MENU' (130) {
    130, textMenuProc;
    allEnabled, enabled;
    "Edit";
    {
        "Undo", noIcon, "Z", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Cut", noIcon, "X", noMark, plain;
        "Copy", noIcon, "C", noMark, plain;
        "Paste", noIcon, "V", noMark, plain;
        "Clear", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Select All", noIcon, "A", noMark, plain;
    }
};

/* Menu Bar */
resource 'MBAR' (128) {
    { 128, 129, 130 };
};

/* SIZE resource for MultiFinder */
resource 'SIZE' (-1) {
    dontSaveScreen,
    acceptSuspendResumeEvents,
    enableOptionSwitch,
    canBackground,
    multiFinderAware,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    isHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    reserved,
    reserved,
    reserved,
    reserved,
    128 * 1024,
    128 * 1024
};

/* About Dialog */
resource 'DLOG' (128) {
    {40, 40, 160, 300},
    dBoxProc,
    visible,
    noGoAway,
    0,
    128,
    "",
    alertPositionMainScreen
};

resource 'DITL' (128) {
    {
        /* OK Button */
        {90, 95, 110, 165},
        Button {
            enabled,
            "OK"
        };
        /* Application Name */
        {10, 20, 30, 240},
        StaticText {
            disabled,
            "SerialSend"
        };
        /* Version */
        {35, 20, 55, 240},
        StaticText {
            disabled,
            "Version 1.0"
        };
        /* Description */
        {60, 20, 80, 240},
        StaticText {
            disabled,
            "Send text to serial port"
        };
    }
};

/* Application icon */
resource 'ICN#' (128, purgeable) {
    {
        /* Icon */
        $"0000 0000 0000 0000 0000 0000 0000 0000"
        $"07FF FFE0 0400 0020 0400 0020 0400 0020"
        $"0400 0020 0400 0020 0400 0020 07FF FFE0"
        $"0000 0000 0000 0000 0001 F800 0006 0600"
        $"0008 0100 0010 0080 0010 0080 0010 0080"
        $"0010 0080 0008 0100 0006 0600 0001 F800"
        $"0000 0000 0000 0000 0000 0000 0000 0000"
        $"0000 0000 0000 0000 0000 0000 0000 0000",
        /* Mask */
        $"0000 0000 0000 0000 0000 0000 0000 0000"
        $"07FF FFE0 07FF FFE0 07FF FFE0 07FF FFE0"
        $"07FF FFE0 07FF FFE0 07FF FFE0 07FF FFE0"
        $"0000 0000 0000 0000 0001 F800 0007 FE00"
        $"000F FF00 001F FF80 001F FF80 001F FF80"
        $"001F FF80 000F FF00 0007 FE00 0001 F800"
        $"0000 0000 0000 0000 0000 0000 0000 0000"
        $"0000 0000 0000 0000 0000 0000 0000 0000"
    }
};

/* Small icon */
resource 'ics#' (128, purgeable) {
    {
        /* Icon */
        $"0000 7FFE 4002 4002 4002 7FFE 0000 03C0"
        $"0420 0810 0810 0810 0420 03C0 0000 0000",
        /* Mask */
        $"0000 7FFE 7FFE 7FFE 7FFE 7FFE 0000 03C0"
        $"07E0 0FF0 0FF0 0FF0 07E0 03C0 0000 0000"
    }
};

/* File reference for Finder */
resource 'FREF' (128, purgeable) {
    'APPL',
    0,
    ""
};

/* Bundle for Finder */
resource 'BNDL' (128, purgeable) {
    'SSND',
    0,
    {
        'ICN#', { 0, 128 },
        'ics#', { 0, 128 },
        'FREF', { 0, 128 }
    }
};

/* Settings Dialog */
resource 'DLOG' (129) {
    {40, 40, 200, 300},
    dBoxProc,
    visible,
    noGoAway,
    0,
    129,
    "",
    alertPositionMainScreen
};

resource 'DITL' (129) {
    {
        /* Item 1: OK Button */
        {125, 170, 145, 240},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Cancel Button */
        {125, 85, 145, 155},
        Button {
            enabled,
            "Cancel"
        };
        /* Item 3: Port label */
        {10, 15, 26, 60},
        StaticText {
            disabled,
            "Port:"
        };
        /* Item 4: Modem Port radio */
        {10, 70, 26, 150},
        RadioButton {
            enabled,
            "Modem"
        };
        /* Item 5: Printer Port radio */
        {10, 155, 26, 245},
        RadioButton {
            enabled,
            "Printer"
        };
        /* Item 6: Baud rate label */
        {40, 15, 56, 95},
        StaticText {
            disabled,
            "Baud Rate:"
        };
        /* Item 7: 1200 baud radio */
        {40, 100, 56, 165},
        RadioButton {
            enabled,
            "1200"
        };
        /* Item 8: 2400 baud radio */
        {40, 170, 56, 235},
        RadioButton {
            enabled,
            "2400"
        };
        /* Item 9: 9600 baud radio */
        {60, 100, 76, 165},
        RadioButton {
            enabled,
            "9600"
        };
        /* Item 10: 19200 baud radio */
        {60, 170, 76, 245},
        RadioButton {
            enabled,
            "19200"
        };
        /* Item 11: 38400 baud radio */
        {80, 100, 96, 170},
        RadioButton {
            enabled,
            "38400"
        };
        /* Item 12: 57600 baud radio */
        {80, 170, 96, 245},
        RadioButton {
            enabled,
            "57600"
        };
        /* Item 13: Box around settings */
        {5, 10, 105, 255},
        UserItem {
            disabled
        };
    }
};
