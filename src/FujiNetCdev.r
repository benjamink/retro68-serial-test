/*
 * FujiNetCdev.r - Resources for FujiNet Control Panel device
 */

#include "Retro68.r"
#include "MacTypes.r"
#include "Dialogs.r"
#include "Icons.r"

/* Define cdev resource type using Retro68 flat code format */
type 'cdev' {
    RETRO68_CODE_TYPE
};

/* Embed the flat code binary as the cdev code resource */
resource 'cdev' (-4064, "FujiNet Config", sysheap, locked) {
    dontBreakAtEntry, $$read("FujiNetCdev.flt")
};

/* Number of rectangles - defines the panel area */
/* nrct: one rectangle (0,0,200,305) for the cdev panel */
type 'nrct' {
    integer = $$CountOf(rects);
    array rects {
        rect;
    };
};

resource 'nrct' (-4064) {
    {
        {0, 0, 200, 305}
    }
};

/* Machine type requirements */
type 'mach' {
    unsigned integer;    /* Machine type bitmask (0xFFFF = all machines) */
    unsigned integer;    /* Minimum system version (0 = any) */
};

resource 'mach' (-4064) {
    0xFFFF,    /* All machine types */
    0          /* Any system version */
};

/* Panel DITL - minimal for Phase 1 */
resource 'DITL' (-4064) {
    {
        /* Item 1: Hosts label */
        {2, 4, 16, 50},
        StaticText {
            disabled,
            "Hosts:"
        };
        /* Item 2: Hosts display (8 lines, updated by code) */
        {17, 4, 87, 300},
        StaticText {
            disabled,
            ""
        };
        /* Item 3: Slot label */
        {90, 4, 106, 36},
        StaticText {
            disabled,
            "Slot:"
        };
        /* Item 4: Slot number text field */
        {89, 40, 105, 60},
        EditText {
            enabled,
            "1"
        };
        /* Item 5: Edit Host button */
        {89, 66, 107, 142},
        Button {
            enabled,
            "Edit Host"
        };
        /* Item 6: Browse button */
        {89, 148, 107, 214},
        Button {
            enabled,
            "Browse"
        };
        /* Item 7: Disks label */
        {110, 4, 124, 50},
        StaticText {
            disabled,
            "Disks:"
        };
        /* Item 8: Disks display (8 lines, updated by code) */
        {125, 4, 181, 300},
        StaticText {
            disabled,
            ""
        };
        /* Item 9: Clock button */
        {183, 4, 199, 74},
        Button {
            enabled,
            "Clock"
        };
        /* Item 10: Device Info button */
        {183, 80, 199, 168},
        Button {
            enabled,
            "Device Info"
        };
    }
};

/* Application icon - FujiNet logo (white on black) */
resource 'ICN#' (-4064, purgeable) {
    {
        /* Icon */
        $"FFFF DFFF FFFF DFFF FFFF DFFF FFFF CFFF"
        $"FFFF 03FF FFFE 01FF FFDE 01EF FFDC 00EF"
        $"FE00 0003 FFDE 01EF FFDE 01EF FFDF 03EF"
        $"FF0F 87C7 FE06 0301 FC02 0001 BC00 0000"
        $"8000 0000 BC00 0000 BC00 0000 BE02 0301"
        $"FF07 0783 FFDF DF83 FFDF DF01 FFDF DE00"
        $"FFDF DE00 FF00 0000 FFDF DE00 FFDF DE01"
        $"FFDF DF01 FFDF DF87 FFFF DFEF FFFF DFEF",
        /* Mask */
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    }
};

/* Small icon - FujiNet logo (white on black) */
resource 'ics#' (-4064, purgeable) {
    {
        /* Icon */
        $"FFFF FFBF FF1F FA0B FA0B FF1F F111 E000"
        $"4000 E000 F331 FFF0 FBA0 FFE0 FFF1 FFFF",
        /* Mask */
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
        $"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    }
};

/* ================================================================
 * Modal dialogs (opened by cdev code, keep positive resource IDs)
 * ================================================================ */

/* Settings Dialog (serial port config) */
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

/* Clock Dialog */
resource 'DLOG' (130) {
    {40, 40, 190, 380},
    dBoxProc,
    visible,
    noGoAway,
    0,
    130,
    "",
    alertPositionMainScreen
};

resource 'DITL' (130) {
    {
        /* Item 1: OK Button */
        {115, 125, 135, 215},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Title */
        {10, 20, 30, 320},
        StaticText {
            disabled,
            "FujiNet Clock"
        };
        /* Item 3: Time display */
        {45, 20, 100, 320},
        StaticText {
            disabled,
            "Querying clock..."
        };
    }
};

/* Mount Disks Dialog */
resource 'DLOG' (131) {
    {80, 80, 280, 420},
    dBoxProc,
    visible,
    noGoAway,
    0,
    131,
    "",
    alertPositionMainScreen
};

resource 'DITL' (131) {
    {
        /* Item 1: OK Button */
        {165, 120, 185, 200},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Mounting message (filled by code) */
        {10, 20, 155, 320},
        StaticText {
            disabled,
            "Mounting disks:"
        };
    }
};

/* Edit Host Dialog */
resource 'DLOG' (132) {
    {80, 100, 180, 420},
    dBoxProc,
    visible,
    noGoAway,
    0,
    132,
    "",
    alertPositionMainScreen
};

resource 'DITL' (132) {
    {
        /* Item 1: OK Button */
        {65, 230, 85, 310},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Cancel Button */
        {65, 140, 85, 220},
        Button {
            enabled,
            "Cancel"
        };
        /* Item 3: Hostname label */
        {15, 10, 31, 90},
        StaticText {
            disabled,
            "Hostname:"
        };
        /* Item 4: Hostname text field */
        {12, 95, 30, 310},
        EditText {
            enabled,
            ""
        };
    }
};

/* Mount File Dialog */
resource 'DLOG' (133) {
    {60, 80, 260, 380},
    dBoxProc,
    visible,
    noGoAway,
    0,
    133,
    "",
    alertPositionMainScreen
};

resource 'DITL' (133) {
    {
        /* Item 1: OK Button */
        {165, 220, 185, 290},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Cancel Button */
        {165, 130, 185, 200},
        Button {
            enabled,
            "Cancel"
        };
        /* Item 3: File label */
        {10, 10, 26, 45},
        StaticText {
            disabled,
            "File:"
        };
        /* Item 4: File path (filled by code) */
        {10, 50, 26, 290},
        StaticText {
            disabled,
            ""
        };
        /* Item 5: Disk Slot label */
        {40, 10, 56, 100},
        StaticText {
            disabled,
            "Disk Slot:"
        };
        /* Items 6-9: Slot 1-4 radio buttons (column 1) */
        {58, 20, 74, 75},
        RadioButton {
            enabled,
            "1"
        };
        {76, 20, 92, 75},
        RadioButton {
            enabled,
            "2"
        };
        {94, 20, 110, 75},
        RadioButton {
            enabled,
            "3"
        };
        {112, 20, 128, 75},
        RadioButton {
            enabled,
            "4"
        };
        /* Items 10-13: Slot 5-8 radio buttons (column 2) */
        {58, 90, 74, 145},
        RadioButton {
            enabled,
            "5"
        };
        {76, 90, 92, 145},
        RadioButton {
            enabled,
            "6"
        };
        {94, 90, 110, 145},
        RadioButton {
            enabled,
            "7"
        };
        {112, 90, 128, 145},
        RadioButton {
            enabled,
            "8"
        };
        /* Item 14: Mode label */
        {40, 165, 56, 210},
        StaticText {
            disabled,
            "Mode:"
        };
        /* Items 15-16: Read/Write radio buttons */
        {58, 175, 74, 240},
        RadioButton {
            enabled,
            "Read"
        };
        {76, 175, 92, 250},
        RadioButton {
            enabled,
            "Write"
        };
    }
};

/* Device Info Dialog */
resource 'DLOG' (134) {
    {60, 80, 230, 370},
    dBoxProc,
    visible,
    noGoAway,
    0,
    134,
    "",
    alertPositionMainScreen
};

resource 'DITL' (134) {
    {
        /* Item 1: OK Button */
        {135, 105, 155, 185},
        Button {
            enabled,
            "OK"
        };
        /* Item 2: Title */
        {10, 20, 28, 270},
        StaticText {
            disabled,
            "FujiNet Device Info"
        };
        /* Item 3: MAC Address label */
        {38, 20, 54, 120},
        StaticText {
            disabled,
            "MAC Address:"
        };
        /* Item 4: MAC Address value (filled by code) */
        {38, 125, 54, 270},
        StaticText {
            disabled,
            ""
        };
        /* Item 5: IP Address label */
        {58, 20, 74, 120},
        StaticText {
            disabled,
            "IP Address:"
        };
        /* Item 6: IP Address value (filled by code) */
        {58, 125, 74, 270},
        StaticText {
            disabled,
            ""
        };
        /* Item 7: SSID label */
        {78, 20, 94, 120},
        StaticText {
            disabled,
            "SSID:"
        };
        /* Item 8: SSID value (filled by code) */
        {78, 125, 94, 270},
        StaticText {
            disabled,
            ""
        };
        /* Item 9: Firmware label */
        {98, 20, 114, 120},
        StaticText {
            disabled,
            "Firmware:"
        };
        /* Item 10: Firmware value (filled by code) */
        {98, 125, 114, 270},
        StaticText {
            disabled,
            ""
        };
    }
};

/* File Browser Dialog */
resource 'DLOG' (135) {
    {60, 60, 300, 380},
    dBoxProc,
    visible,
    noGoAway,
    0,
    135,
    "",
    alertPositionMainScreen
};

resource 'DITL' (135) {
    {
        /* Item 1: Mount button */
        {210, 165, 230, 235},
        Button {
            enabled,
            "Mount"
        };
        /* Item 2: Cancel button */
        {210, 240, 230, 310},
        Button {
            enabled,
            "Cancel"
        };
        /* Item 3: Up button */
        {210, 10, 230, 80},
        Button {
            enabled,
            "Up"
        };
        /* Item 4: Path display */
        {5, 10, 20, 310},
        StaticText {
            disabled,
            "/"
        };
        /* Item 5: List area (UserItem for ListHandle) */
        {22, 10, 205, 310},
        UserItem {
            disabled
        };
    }
};
