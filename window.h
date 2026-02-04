/*
 * window.h - Main window and UI elements
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <Windows.h>
#include <TextEdit.h>
#include <Controls.h>

/* Window and UI globals */
extern WindowPtr gMainWindow;
extern TEHandle gSendText;
extern TEHandle gRecvText;
extern ControlHandle gSendButton;
extern ControlHandle gResetButton;

/* Create the main application window */
void CreateMainWindow(void);

/* Redraw window contents */
void UpdateWindow(WindowPtr window);

#endif /* WINDOW_H */
