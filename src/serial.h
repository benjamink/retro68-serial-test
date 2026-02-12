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

/* Get clock time from fujinet-nio (returns true if successful) */
Boolean GetClockTime(char *timeStr, short maxLen);

/* Read host slots from fujinet-nio (returns true if successful) */
Boolean ReadHostSlots(char hostSlots[][32], short numSlots);

/* Write host slots to fujinet-nio (returns true if successful) */
Boolean WriteHostSlots(char hostSlots[][32], short numSlots);

/* Directory entry from ListDirectory */
typedef struct {
    char name[64];
    Boolean isDir;
} DirEntry;

/* List directory contents via FileService (returns true if successful) */
Boolean ListDirectory(const char *fsName, const char *path,
                      short startIndex, short maxEntries,
                      DirEntry *entries, short *entryCount,
                      Boolean *hasMore);

/* Mount a disk image via DiskService (returns true if successful) */
Boolean MountDisk(short slot, Boolean readOnly,
                  const char *fsName, const char *path);

#endif /* SERIAL_H */
