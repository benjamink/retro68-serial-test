/*
 * constants.h - Resource IDs, dimensions, and configuration constants
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Resource IDs */
#define kMenuBarID      128
#define kAppleMenuID    128
#define kFileMenuID     129
#define kEditMenuID     130
#define kSettingsMenuID 131

#define kAboutDialogID    128
#define kSerialDialogID   129
#define kClockDialogID    130
#define kBootDialogID     131
#define kEditHostDialogID 132
#define kSendButtonID     128

/* Edit host dialog item IDs */
#define kEditHostOK       1
#define kEditHostCancel   2
#define kEditHostLabel    3
#define kEditHostText     4

/* Serial settings dialog item IDs */
#define kSerialSettingsOK         1
#define kSerialSettingsCancel     2
#define kSerialSettingsPortLabel  3
#define kSerialSettingsModemPort  4
#define kSerialSettingsPrinterPort 5
#define kSerialSettingsBaudLabel  6
#define kSerialSettingsBaud1200   7
#define kSerialSettingsBaud2400   8
#define kSerialSettingsBaud9600   9
#define kSerialSettingsBaud19200  10
#define kSerialSettingsBaud38400  11
#define kSerialSettingsBaud57600  12

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

/* Main window (FujiNet Config) dimensions */
#define kMainWindowWidth    480
#define kMainWindowHeight   296

/* Hosts section layout (left column) */
#define kHostsHeaderLeft    8
#define kHostsHeaderTop     6
#define kHostsHeaderRight   330
#define kHostsHeaderBottom  20

#define kHostsListLeft      8
#define kHostsListTop       20
#define kHostsListRight     330
#define kHostsListBottom    140

/* Logo area (right column, extends from Hosts top to Disks bottom) */
#define kLogoLeft           338
#define kLogoTop            6
#define kLogoRight          472
#define kLogoBottom         252

/* Disks section layout (left column, below Hosts) */
#define kDisksHeaderLeft    8
#define kDisksHeaderTop     146
#define kDisksHeaderRight   330
#define kDisksHeaderBottom  160

#define kDisksListLeft      8
#define kDisksListTop       160
#define kDisksListRight     330
#define kDisksListBottom    252

/* Boot button (spans left column width) */
#define kBootButtonLeft     8
#define kBootButtonTop      262
#define kBootButtonRight    330
#define kBootButtonBottom   284

/* List cell dimensions */
#define kHostCellHeight     15
#define kDiskCellHeight     15
#define kNumHostSlots       8
#define kNumDiskSlots       8
#define kMaxHostnameLen     32

/* Serial testing window dimensions */
#define kSerialWindowWidth    320
#define kSerialWindowHeight   195

/* Serial testing window - Send text area positions */
#define kSendLeft       10
#define kSendTop        30
#define kSendRight      310
#define kSendBottom     75

/* Serial testing window - Button positions */
#define kSendButtonLeft     60
#define kSendButtonTop      82
#define kSendButtonRight    140
#define kSendButtonBottom   102

#define kResetButtonLeft    180
#define kResetButtonTop     82
#define kResetButtonRight   260
#define kResetButtonBottom  102

/* Serial testing window - Receive text area positions */
#define kRecvLeft       10
#define kRecvTop        122
#define kRecvRight      310
#define kRecvBottom     185

/* Edit Host button (right column, aligned with Boot button) */
#define kEditHostBtnLeft    338
#define kEditHostBtnTop     262
#define kEditHostBtnRight   472
#define kEditHostBtnBottom  284

/* File browser window */
#define kBrowserWindowWidth   350
#define kBrowserWindowHeight  280

#define kBrowserPathLeft      10
#define kBrowserPathTop       8
#define kBrowserPathRight     340
#define kBrowserPathBottom    22

#define kBrowserListLeft      10
#define kBrowserListTop       24
#define kBrowserListRight     340
#define kBrowserListBottom    230

#define kBrowserCellHeight    15
#define kMaxBrowserEntries    32

#define kBrowserUpBtnLeft     10
#define kBrowserUpBtnTop      240
#define kBrowserUpBtnRight    80
#define kBrowserUpBtnBottom   260

#define kBrowserCloseBtnLeft  270
#define kBrowserCloseBtnTop   240
#define kBrowserCloseBtnRight 340
#define kBrowserCloseBtnBottom 260

/* Mount dialog */
#define kMountDialogID        133

#define kMountOK              1
#define kMountCancel          2
#define kMountFileLabel       3
#define kMountFilePath        4
#define kMountSlotLabel       5
#define kMountSlot1           6
#define kMountSlot2           7
#define kMountSlot3           8
#define kMountSlot4           9
#define kMountSlot5           10
#define kMountSlot6           11
#define kMountSlot7           12
#define kMountSlot8           13
#define kMountModeLabel       14
#define kMountRead            15
#define kMountWrite           16

#define kMaxPathLen           256

/* Device Info dialog */
#define kDeviceInfoDialogID   134

/* File browser dialog */
#define kBrowserDialogID      135
#define kBrowserDlgMount      1
#define kBrowserDlgCancel     2
#define kBrowserDlgUp         3
#define kBrowserDlgPath       4
#define kBrowserDlgList       5

/* Disk slot state tracking */
typedef struct {
    Boolean mounted;
    short hostSlot;      /* 1-based */
    Boolean readOnly;
    char path[256];
} DiskSlotState;

/* Global host and disk slot data (defined in cdev_main.c) */
extern char gHostSlots[8][32];
extern DiskSlotState gDiskSlots[8];

/* cdev panel DITL item numbers (1-based, relative to cdev items) */
#define kCdevHostsLabel       1
#define kCdevHostsDisplay     2
#define kCdevSlotLabel        3
#define kCdevSlotText         4
#define kCdevEditHostBtn      5
#define kCdevBrowseBtn        6
#define kCdevDisksLabel       7
#define kCdevDisksDisplay     8
#define kCdevClockBtn         9
#define kCdevDeviceInfoBtn    10

#endif /* CONSTANTS_H */
