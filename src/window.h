/*
 * window.h - Main config window and serial testing window
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <Windows.h>
#include <TextEdit.h>
#include <Controls.h>
#include <Lists.h>

/* Main config window */
extern WindowPtr gMainWindow;
extern ListHandle gHostsList;
extern ListHandle gDisksList;
extern ControlHandle gBootButton;
extern ControlHandle gEditHostButton;

/* Host slot data from fujinet-nio */
extern char gHostSlots[8][32];

/* Disk slot state tracking */
typedef struct {
    Boolean mounted;
    short hostSlot;      /* 1-based */
    Boolean readOnly;
    char path[256];
} DiskSlotState;

extern DiskSlotState gDiskSlots[8];

/* File browser window */
extern WindowPtr gBrowserWindow;

/* Serial testing window (modeless, opened from Settings menu) */
extern WindowPtr gSerialWindow;
extern TEHandle gSendText;
extern TEHandle gRecvText;
extern ControlHandle gSendButton;
extern ControlHandle gResetButton;

/* Main config window functions */
void CreateMainWindow(void);
void UpdateMainWindow(WindowPtr window);
void HandleMainWindowClick(WindowPtr window, Point localPoint, EventRecord *event);
void PopulateHostsList(void);
void PopulateDisksList(void);

/* File browser functions */
void OpenFileBrowser(short hostSlot);
void CloseFileBrowser(void);
void UpdateBrowserWindow(WindowPtr window);
void HandleBrowserWindowClick(WindowPtr window, Point localPoint, EventRecord *event);

/* Serial testing window functions */
void OpenSerialTestingWindow(void);
void CloseSerialTestingWindow(void);
void UpdateSerialWindow(WindowPtr window);
void HandleSerialWindowClick(WindowPtr window, Point localPoint, EventRecord *event);

/* Generic update dispatcher */
void UpdateWindow(WindowPtr window);

#endif /* WINDOW_H */
