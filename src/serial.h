/*
 * serial.h - Serial port communication
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <Types.h>

/* Serial port driver reference numbers */
extern short gSerialOutRef;
extern short gSerialInRef;

/* Serial port settings */
extern short gCurrentPort;
extern short gCurrentBaud;

/* Baud rate names for display */
extern char *gBaudNames[];

/* Initialize serial port with current settings */
Boolean InitializeSerial(void);

/* Close serial port */
void CleanupSerial(void);

/* Close and reopen serial port with new settings */
Boolean ReinitializeSerial(void);

/* Send text from send TextEdit to serial port */
void SendTextToSerial(void);

/* Send fujinet-nio reset command */
void SendResetCommand(void);

/* Get clock time from fujinet-nio (returns true if successful) */
Boolean GetClockTime(char *timeStr, short maxLen);

/* Poll for incoming serial data */
void PollSerialInput(void);

#endif /* SERIAL_H */
