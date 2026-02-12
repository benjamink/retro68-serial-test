/*
 * cdev_main.c - FujiNet Control Panel device entry point
 *
 * This is a classic Mac cdev (Control Panel device).
 * The Control Panel DA loads this code resource and sends messages.
 */

#include <Dialogs.h>
#include <Events.h>
#include <Memory.h>
#include <Sound.h>
#include <Types.h>
#include <Retro68Runtime.h>

#include "constants.h"
#include "dialogs.h"
#include "serial.h"

/* cdev message constants */
#define initDev     0
#define hitDev      1
#define closeDev    2
#define nulDev      3
#define updateDev   4
#define activDev    5
#define deactivDev  6
#define keyEvtDev   7
#define macDev      8
#define undoCmd     9
#define cutCmd      10
#define copyCmd     11
#define pasteCmd    12
#define clearCmd    13
#define cdevUnset   0

/* Host and disk slot data (globals, persist via RETRO68_RELOCATE BSS) */
char gHostSlots[8][32];
DiskSlotState gDiskSlots[8];

/* Forward declarations */
static long HandleInit(DialogPtr cpDialog, short numItems);
static long HandleHit(short item, long cdevStorage, DialogPtr cpDialog, short numItems);
static long HandleClose(long cdevStorage);
static void RefreshHostsDisplay(DialogPtr cpDialog, short numItems);
static void RefreshDisksDisplay(DialogPtr cpDialog, short numItems);

/*
 * cdev entry point - called by the Control Panel DA
 *
 * Parameters are pushed by the system.
 * Following the WDEF sample pattern from Retro68.
 */
pascal long cdev_entry(short message, short item, short numItems,
                       short cpanelID, EventRecord *event,
                       long cdevStorage, DialogPtr cpDialog)
{
    RETRO68_RELOCATE();

    switch (message) {
        case initDev:
            return HandleInit(cpDialog, numItems);

        case hitDev:
            return HandleHit(item - numItems, cdevStorage, cpDialog, numItems);

        case closeDev:
            return HandleClose(cdevStorage);

        case macDev:
            /* Return 1 to tell Control Panel we're compatible */
            return 1;

        case updateDev:
        case activDev:
        case deactivDev:
        case nulDev:
            return cdevStorage;

        default:
            return cdevStorage;
    }
}

/* Dummy main to satisfy the Retro68 runtime startup (not called) */
int main(int argc, char *argv[]) { return 0; }

/*
 * Handle initDev - open serial port, read host slots, update display
 */
static long HandleInit(DialogPtr cpDialog, short numItems)
{
    short s, b;

    /* Zero host and disk slot data */
    for (s = 0; s < kNumHostSlots; s++) {
        for (b = 0; b < 32; b++) {
            gHostSlots[s][b] = 0;
        }
    }
    for (s = 0; s < kNumDiskSlots; s++) {
        gDiskSlots[s].mounted = false;
        gDiskSlots[s].hostSlot = 0;
        gDiskSlots[s].readOnly = false;
        gDiskSlots[s].path[0] = '\0';
    }

    /* Initialize serial port */
    if (!InitializeSerial()) {
        SysBeep(10);
    }

    /* Read host slots from FujiNet */
    if (gSerialOutRef != 0) {
        if (!ReadHostSlots(gHostSlots, kNumHostSlots)) {
            /* Default first host slot to "host" */
            gHostSlots[0][0] = 'h';
            gHostSlots[0][1] = 'o';
            gHostSlots[0][2] = 's';
            gHostSlots[0][3] = 't';
            gHostSlots[0][4] = '\0';
        }
    }

    /* Update display */
    RefreshHostsDisplay(cpDialog, numItems);
    RefreshDisksDisplay(cpDialog, numItems);

    /* Return non-zero to indicate success (used as cdevStorage) */
    return 1;
}

/*
 * Read the slot number from the EditText field (item 4).
 * Returns 0-based slot index (0-7), or -1 if invalid.
 */
static short GetSlotFromField(DialogPtr cpDialog, short numItems)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    Str255 text;
    short slot;

    GetDialogItem(cpDialog, numItems + kCdevSlotText, &itemType, &itemHandle, &itemRect);
    GetDialogItemText(itemHandle, text);

    if (text[0] != 1) return -1;  /* Must be exactly one character */
    slot = text[1] - '1';         /* Convert '1'-'8' to 0-7 */
    if (slot < 0 || slot >= kNumHostSlots) return -1;
    return slot;
}

/*
 * Handle hitDev - user clicked an item in the panel
 */
static long HandleHit(short item, long cdevStorage, DialogPtr cpDialog, short numItems)
{
    switch (item) {
        case kCdevEditHostBtn: {
            short slot = GetSlotFromField(cpDialog, numItems);
            if (slot >= 0) {
                DoEditHostDialog(slot);
                RefreshHostsDisplay(cpDialog, numItems);
            } else {
                SysBeep(10);
            }
            break;
        }

        case kCdevBrowseBtn: {
            short slot = GetSlotFromField(cpDialog, numItems);
            if (slot >= 0 && gHostSlots[slot][0] != '\0') {
                if (DoFileBrowser(slot)) {
                    RefreshDisksDisplay(cpDialog, numItems);
                }
            } else {
                SysBeep(10);
            }
            break;
        }

        case kCdevClockBtn:
            DoClockDialog();
            break;

        case kCdevDeviceInfoBtn:
            DoDeviceInfoDialog();
            break;
    }

    return cdevStorage;
}

/*
 * Handle closeDev - close serial port, free globals
 */
static long HandleClose(long cdevStorage)
{
    CleanupSerial();
    Retro68FreeGlobals();
    return 0;
}

/*
 * Refresh the hosts display (DITL item 2) with current gHostSlots data
 * Format: "1. hostname\r2. hostname\r..." (multi-line static text)
 */
static void RefreshHostsDisplay(DialogPtr cpDialog, short numItems)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    char buf[320];
    short pos = 0;
    short i, j;

    for (i = 0; i < kNumHostSlots; i++) {
        if (i > 0 && pos < 310) {
            buf[pos++] = '\r';
        }
        /* Slot number (1-based) */
        buf[pos++] = '0' + (i + 1);
        buf[pos++] = '.';

        if (gHostSlots[i][0] != '\0') {
            buf[pos++] = ' ';
            for (j = 0; j < 31 && gHostSlots[i][j] != '\0' && pos < 310; j++) {
                buf[pos++] = gHostSlots[i][j];
            }
        }
    }

    /* Convert to Pascal string and set dialog item */
    {
        Str255 msg;
        short len = pos < 255 ? pos : 255;
        short k;
        msg[0] = len;
        for (k = 0; k < len; k++) {
            msg[k + 1] = buf[k];
        }
        GetDialogItem(cpDialog, numItems + kCdevHostsDisplay, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, msg);
    }
}

/*
 * Refresh the disks display (DITL item 8) with current gDiskSlots data
 * Mounted: "1R host:path"  Empty: "1."
 */
static void RefreshDisksDisplay(DialogPtr cpDialog, short numItems)
{
    short itemType;
    Handle itemHandle;
    Rect itemRect;
    char buf[320];
    short pos = 0;
    short i, j;

    for (i = 0; i < kNumDiskSlots; i++) {
        if (i > 0 && pos < 310) {
            buf[pos++] = '\r';
        }
        /* Slot number (1-based) */
        buf[pos++] = '0' + (i + 1);

        if (gDiskSlots[i].mounted) {
            buf[pos++] = gDiskSlots[i].readOnly ? 'R' : 'W';
            buf[pos++] = ' ';
            for (j = 0; gDiskSlots[i].path[j] != '\0' && pos < 310; j++) {
                buf[pos++] = gDiskSlots[i].path[j];
            }
        } else {
            buf[pos++] = '.';
        }
    }

    /* Convert to Pascal string and set dialog item */
    {
        Str255 msg;
        short len = pos < 255 ? pos : 255;
        short k;
        msg[0] = len;
        for (k = 0; k < len; k++) {
            msg[k + 1] = buf[k];
        }
        GetDialogItem(cpDialog, numItems + kCdevDisksDisplay, &itemType, &itemHandle, &itemRect);
        SetDialogItemText(itemHandle, msg);
    }
}
