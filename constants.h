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

/* Button positions - side by side */
#define kSendButtonLeft     60
#define kSendButtonTop      82
#define kSendButtonRight    140
#define kSendButtonBottom   102

#define kResetButtonLeft    180
#define kResetButtonTop     82
#define kResetButtonRight   260
#define kResetButtonBottom  102

/* Receive text area positions */
#define kRecvLeft       10
#define kRecvTop        122
#define kRecvRight      310
#define kRecvBottom     185

/* Maximum receive buffer size */
#define kMaxReceiveText 4096

#endif /* CONSTANTS_H */
