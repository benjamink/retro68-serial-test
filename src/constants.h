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
#define kSendButtonID     128

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
#define kHostsListBottom    170

/* Logo area (right column, extends from Hosts top to Disks bottom) */
#define kLogoLeft           338
#define kLogoTop            6
#define kLogoRight          472
#define kLogoBottom         252

/* Disks section layout (left column, below Hosts) */
#define kDisksHeaderLeft    8
#define kDisksHeaderTop     176
#define kDisksHeaderRight   330
#define kDisksHeaderBottom  190

#define kDisksListLeft      8
#define kDisksListTop       190
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
#define kNumHostSlots       10
#define kNumDiskSlots       8

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

/* Maximum receive buffer size */
#define kMaxReceiveText 4096

#endif /* CONSTANTS_H */
