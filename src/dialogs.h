/*
 * dialogs.h - Application dialogs
 */

#ifndef DIALOGS_H
#define DIALOGS_H

/* Show the Settings dialog */
void DoSettingsDialog(void);

/* Show the Clock dialog */
void DoClockDialog(void);

/* Show the "Mounting disk:" dialog for a single disk (double-click) */
void DoMountDiskDialog(const char *diskFile);

/* Show the "Mounting disks:" dialog listing all populated disk slots */
void DoMountDisksDialog(void);

/* Show the Device Info dialog */
void DoDeviceInfoDialog(void);

/* Show the Edit Host dialog for the given slot number (0-7) */
void DoEditHostDialog(short slotNum);

/* Show the Mount dialog. Returns true if user clicked OK.
 * hostSlot: 0-based index of the host being browsed
 * filePath: full path to the file to mount
 * outSlot: receives 1-based disk slot number
 * outReadOnly: receives true for Read mode, false for Write mode */
Boolean DoMountDialog(short hostSlot, const char *filePath,
                      short *outSlot, Boolean *outReadOnly);

/* Show the File Browser dialog for the given host slot (0-7).
 * Returns true if a disk was mounted, false if cancelled. */
Boolean DoFileBrowser(short hostSlot);

#endif /* DIALOGS_H */
