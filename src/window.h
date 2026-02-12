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

/* Serial testing window functions */
void OpenSerialTestingWindow(void);
void CloseSerialTestingWindow(void);
void UpdateSerialWindow(WindowPtr window);
void HandleSerialWindowClick(WindowPtr window, Point localPoint, EventRecord *event);

/* Generic update dispatcher */
void UpdateWindow(WindowPtr window);

#endif /* WINDOW_H */
