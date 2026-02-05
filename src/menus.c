/*
 * menus.c - Menu bar and menu handling
 */

#include <Menus.h>
#include <Devices.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#include "constants.h"
#include "menus.h"
#include "window.h"
#include "serial.h"
#include "dialogs.h"

/* Defined in main.c */
extern Boolean gRunning;

/* Local function prototypes */
static void HandleAppleMenu(short item);
static void HandleFileMenu(short item);
static void HandleEditMenu(short item);

/*
 * Set up the menu bar
 */
void InitializeMenus(void)
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
 * Handle menu selections
 */
void HandleMenuChoice(long menuChoice)
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
