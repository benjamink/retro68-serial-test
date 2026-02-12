/*
 * serial.c - Serial port communication
 */

#include <Devices.h>
#include <Serial.h>
#include <Memory.h>
#include <Sound.h>
#include <TextEdit.h>
#include <Controls.h>
#include <Windows.h>
#include <Quickdraw.h>
#include <ToolUtils.h>
#include <OSUtils.h>

#include "constants.h"
#include "serial.h"
#include "window.h"

/* Serial port driver reference numbers */
short gSerialOutRef = 0;
short gSerialInRef = 0;

/* Serial port settings */
short gCurrentPort = kPortModem;
short gCurrentBaud = kBaud9600;

/* Baud rate constants for SerReset (from Serial.h) */
static short gBaudRates[] = {
    baud1200,
    baud2400,
    baud9600,
    baud19200,
    baud38400,
    baud57600
};

/* Baud rate names for debug output */
char *gBaudNames[] = {
    "1200", "2400", "9600", "19200", "38400", "57600"
};

/*
 * Initialize the serial port using current settings
 * Port A (Modem) connects to /dev/tnt1 in emulator (use /dev/tnt0 on host)
 * Port B (Printer) outputs to ser_b.out file
 */
Boolean InitializeSerial(void)
{
    OSErr err;
    SerShk handshake;

    /* Select driver names based on port setting */
    if (gCurrentPort == kPortModem) {
        /* Open modem port (port A) */
        err = OpenDriver("\p.AOut", &gSerialOutRef);
        if (err != noErr) {
            return false;
        }
        err = OpenDriver("\p.AIn", &gSerialInRef);
        if (err != noErr) {
            return false;
        }
    } else {
        /* Open printer port (port B) */
        err = OpenDriver("\p.BOut", &gSerialOutRef);
        if (err != noErr) {
            return false;
        }
        err = OpenDriver("\p.BIn", &gSerialInRef);
        if (err != noErr) {
            return false;
        }
    }

    /* Configure handshaking - no flow control */
    handshake.fXOn = 0;
    handshake.fCTS = 0;
    handshake.errs = 0;
    handshake.evts = 0;
    handshake.fInX = 0;

    SerHShake(gSerialInRef, &handshake);

    /* Set baud rate based on current setting, 8N1 */
    SerReset(gSerialOutRef, gBaudRates[gCurrentBaud] + stop10 + noParity + data8);
    SerReset(gSerialInRef, gBaudRates[gCurrentBaud] + stop10 + noParity + data8);

    /* Send test message with port and baud info */
    {
        char msg[64];
        char *prefix = "Serial ready: ";
        char *portName = (gCurrentPort == kPortModem) ? "Modem" : "Printer";
        char *baudName = gBaudNames[gCurrentBaud];
        long count = 0;

        /* Build message: "Serial ready: Modem @ 9600\r\n" */
        while (*prefix) {
            msg[count++] = *prefix++;
        }
        while (*portName) {
            msg[count++] = *portName++;
        }
        msg[count++] = ' ';
        msg[count++] = '@';
        msg[count++] = ' ';
        while (*baudName) {
            msg[count++] = *baudName++;
        }
        msg[count++] = '\r';
        msg[count++] = '\n';

        FSWrite(gSerialOutRef, &count, msg);
    }

    return true;
}

/*
 * Close the serial port
 */
void CleanupSerial(void)
{
    if (gSerialOutRef != 0) {
        CloseDriver(gSerialOutRef);
        gSerialOutRef = 0;
    }
    if (gSerialInRef != 0) {
        CloseDriver(gSerialInRef);
        gSerialInRef = 0;
    }
}

/*
 * Reinitialize serial port with new settings
 */
Boolean ReinitializeSerial(void)
{
    CleanupSerial();
    return InitializeSerial();
}

/*
 * Send the text from the text edit field to the serial port
 * Converts Mac line endings (CR) to CR+LF for serial output
 */
void SendTextToSerial(void)
{
    Handle textHandle;
    long textLength;
    long count;
    char *textPtr;
    long i;
    char crlf[2] = {'\r', '\n'};

    if (gSendText == NULL) {
        return;
    }

    /* Get the text from TextEdit */
    textHandle = (Handle)TEGetText(gSendText);
    textLength = (*gSendText)->teLength;

    if (textLength == 0) {
        SysBeep(10);
        return;
    }

    if (gSerialOutRef == 0) {
        SysBeep(10);
        return;
    }

    /* Lock the handle and get pointer */
    HLock(textHandle);
    textPtr = *textHandle;

    /* Write to serial port, converting CR to CR+LF */
    for (i = 0; i < textLength; i++) {
        if (textPtr[i] == '\r') {
            /* Send CR+LF for line endings */
            count = 2;
            FSWrite(gSerialOutRef, &count, crlf);
        } else {
            /* Send single character */
            count = 1;
            FSWrite(gSerialOutRef, &count, &textPtr[i]);
        }
    }

    /* Send final CR+LF if text doesn't end with one */
    if (textLength > 0 && textPtr[textLength - 1] != '\r') {
        count = 2;
        FSWrite(gSerialOutRef, &count, crlf);
    }

    HUnlock(textHandle);

    /* Flash the button to indicate success */
    if (gSendButton != NULL) {
        HiliteControl(gSendButton, 1);
        Delay(8, NULL);
        HiliteControl(gSendButton, 0);
    }

    /* Clear the text field */
    TESetSelect(0, 32767, gSendText);
    TEDelete(gSendText);
}

/*
 * FujiBus protocol constants
 */
#define SLIP_END            0xC0    /* SLIP frame delimiter */
#define SLIP_ESCAPE         0xDB    /* SLIP escape byte */
#define SLIP_ESC_END        0xDC    /* SLIP escaped END */
#define SLIP_ESC_ESC        0xDD    /* SLIP escaped ESCAPE */
#define FUJINET_DEVICE      0x70    /* FujiNet device ID */
#define FUJI_CMD_RESET      0xFF    /* Reset command */
#define FUJIBUS_HEADER_SIZE 6       /* Header size for checksum calculation */

/*
 * Calculate fujinet-nio checksum (fold carry into low byte)
 */
static unsigned char CalcFujiChecksum(unsigned char *data, short length)
{
    unsigned short sum = 0;
    short i;

    for (i = 0; i < length; i++) {
        sum += data[i];
        sum = (sum >> 8) + (sum & 0xFF);
    }
    return (unsigned char)(sum & 0xFF);
}

/*
 * Build and send a FujiBus packet over serial.
 * Matches the Python build_fuji_packet() reference implementation:
 *   Header: device(1), command(1), length(2 LE), checksum(1), descriptor(1)
 *   Descriptor is always 0 (no params), payload follows header.
 *   SLIP-encoded: C0 [escaped payload] C0
 */
static void SendFujiBusPacket(unsigned char device, unsigned char command,
                              unsigned char *payload, short payloadLen)
{
    unsigned char pkt[520];  /* raw FujiBus packet (up to 512-byte payload) */
    unsigned char slip[1050]; /* SLIP-encoded output (worst case 2x + 2) */
    short pktLen;
    short slipLen;
    short i;
    long count;

    if (gSerialOutRef == 0) {
        SysBeep(10);
        return;
    }

    /* Build raw FujiBus packet */
    pktLen = FUJIBUS_HEADER_SIZE + payloadLen;
    pkt[0] = device;
    pkt[1] = command;
    pkt[2] = (unsigned char)(pktLen & 0xFF);        /* length low */
    pkt[3] = (unsigned char)((pktLen >> 8) & 0xFF);  /* length high */
    pkt[4] = 0;                                       /* checksum placeholder */
    pkt[5] = 0;                                       /* descriptor: no params */

    /* Copy payload after header */
    for (i = 0; i < payloadLen; i++) {
        pkt[FUJIBUS_HEADER_SIZE + i] = payload[i];
    }

    /* Calculate and insert checksum */
    pkt[4] = CalcFujiChecksum(pkt, pktLen);

    /* SLIP encode: C0 [escaped bytes] C0 */
    slipLen = 0;
    slip[slipLen++] = SLIP_END;
    for (i = 0; i < pktLen; i++) {
        if (pkt[i] == SLIP_END) {
            slip[slipLen++] = SLIP_ESCAPE;
            slip[slipLen++] = SLIP_ESC_END;
        } else if (pkt[i] == SLIP_ESCAPE) {
            slip[slipLen++] = SLIP_ESCAPE;
            slip[slipLen++] = SLIP_ESC_ESC;
        } else {
            slip[slipLen++] = pkt[i];
        }
    }
    slip[slipLen++] = SLIP_END;

    /* Send the SLIP-framed packet */
    count = slipLen;
    FSWrite(gSerialOutRef, &count, slip);
}

/*
 * Send fujinet-nio reset command over serial
 * Device 0x70 (FujiNet), Command 0xFF (Reset), no payload
 */
void SendResetCommand(void)
{
    SendFujiBusPacket(FUJINET_DEVICE, FUJI_CMD_RESET, NULL, 0);

    /* Flash the button to indicate success */
    if (gResetButton != NULL) {
        HiliteControl(gResetButton, 1);
        Delay(8, NULL);
        HiliteControl(gResetButton, 0);
    }
}

/*
 * Get clock time from fujinet-nio
 * Device 0x45 (Clock), Command 0x01 (GetTime), no payload
 * Returns true if successful, with formatted time in timeStr
 */
Boolean GetClockTime(char *timeStr, short maxLen)
{
    unsigned char recvSlip[256];
    unsigned char recvPkt[128];
    short recvPktLen;
    long count;
    long startTicks;
    long timeout = 300;  /* 5 second timeout (60 ticks per second) */
    short i, j;
    Boolean inFrame;
    Boolean escaped;
    unsigned long unixTime;
    DateTimeRec dt;
    
    #define CLOCK_DEVICE    0x45
    #define CLOCK_GET_TIME  0x01
    
    if (gSerialOutRef == 0 || gSerialInRef == 0) {
        return false;
    }
    
    /* Flush any pending input */
    SerGetBuf(gSerialInRef, &count);
    if (count > 0) {
        unsigned char dummy[256];
        if (count > sizeof(dummy)) count = sizeof(dummy);
        FSRead(gSerialInRef, &count, dummy);
    }
    
    /* Send GetTime command */
    SendFujiBusPacket(CLOCK_DEVICE, CLOCK_GET_TIME, NULL, 0);
    
    /* Wait for response with timeout */
    startTicks = TickCount();
    recvPktLen = 0;
    inFrame = false;
    escaped = false;
    
    while ((TickCount() - startTicks) < timeout) {
        long checkCount;
        
        SerGetBuf(gSerialInRef, &count);
        
        if (count > 0) {
            if (count > sizeof(recvSlip)) count = sizeof(recvSlip);
            FSRead(gSerialInRef, &count, recvSlip);
            
            /* Process SLIP frame */
            for (i = 0; i < count; i++) {
                unsigned char b = recvSlip[i];
                
                if (b == SLIP_END) {
                    if (inFrame && recvPktLen > 0) {
                        /* End of frame - process it */
                        goto frame_complete;
                    }
                    inFrame = true;
                    recvPktLen = 0;
                    escaped = false;
                } else if (escaped) {
                    if (b == SLIP_ESC_END) {
                        recvPkt[recvPktLen++] = SLIP_END;
                    } else if (b == SLIP_ESC_ESC) {
                        recvPkt[recvPktLen++] = SLIP_ESCAPE;
                    }
                    escaped = false;
                } else if (b == SLIP_ESCAPE) {
                    escaped = true;
                } else if (inFrame && recvPktLen < sizeof(recvPkt)) {
                    recvPkt[recvPktLen++] = b;
                }
            }
        }
        
        /* Small delay to avoid spinning CPU - 1 tick (~16ms) */
        Delay(1, NULL);
    }
    
    /* Timeout - no response */
    return false;
    
frame_complete:
    /* Validate packet: need header(6) + status param(1) + payload(12) = 19 bytes
       Response format: device(1) cmd(1) len(2) chk(1) desc(1) status(1) payload(12) */
    if (recvPktLen < 19) {
        return false;
    }

    /* Verify checksum: save it, zero it, recalculate, compare */
    {
        unsigned char savedChecksum = recvPkt[4];
        unsigned char calcChecksum;
        recvPkt[4] = 0;
        calcChecksum = CalcFujiChecksum(recvPkt, recvPktLen);
        recvPkt[4] = savedChecksum;
        if (calcChecksum != savedChecksum) {
            return false;
        }
    }

    /* Check device and command match */
    if (recvPkt[0] != CLOCK_DEVICE || recvPkt[1] != CLOCK_GET_TIME) {
        return false;
    }

    /* Check status param at offset 6 (descriptor 0x01 = one u8 param) */
    if (recvPkt[6] != 0) {
        return false;
    }

    /* Extract Unix timestamp from payload
       Payload starts at offset 7 (after header + status param):
       version(1) + flags(1) + reserved(2) + unixTime(8 LE)
       unixTime starts at offset 7 + 4 = 11 */
    i = 11;
    unixTime = ((unsigned long)recvPkt[i]) |
               ((unsigned long)recvPkt[i+1] << 8) |
               ((unsigned long)recvPkt[i+2] << 16) |
               ((unsigned long)recvPkt[i+3] << 24);
    
    /* Convert Unix time to Mac DateTimeRec
       Note: Mac epoch is Jan 1, 1904; Unix epoch is Jan 1, 1970
       Difference is 2082844800 seconds */
    {
        unsigned long macTime = unixTime + 2082844800UL;
        SecondsToDate(macTime, &dt);
    }
    
    /* Format as readable string */
    {
        char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char temp[20];
        short pos = 0;
        
        /* Day of week */
        char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char *dow = days[dt.dayOfWeek - 1];
        while (*dow && pos < maxLen - 1) timeStr[pos++] = *dow++;
        timeStr[pos++] = ' ';
        
        /* Month */
        char *mon = months[dt.month - 1];
        while (*mon && pos < maxLen - 1) timeStr[pos++] = *mon++;
        timeStr[pos++] = ' ';
        
        /* Day */
        if (dt.day >= 10) {
            timeStr[pos++] = '0' + (dt.day / 10);
        }
        timeStr[pos++] = '0' + (dt.day % 10);
        timeStr[pos++] = ',';
        timeStr[pos++] = ' ';
        
        /* Year */
        timeStr[pos++] = '0' + (dt.year / 1000);
        timeStr[pos++] = '0' + ((dt.year / 100) % 10);
        timeStr[pos++] = '0' + ((dt.year / 10) % 10);
        timeStr[pos++] = '0' + (dt.year % 10);
        
        /* Time */
        timeStr[pos++] = '\r';
        timeStr[pos++] = '\r';
        
        /* Hour */
        if (dt.hour >= 10) {
            timeStr[pos++] = '0' + (dt.hour / 10);
        }
        timeStr[pos++] = '0' + (dt.hour % 10);
        timeStr[pos++] = ':';
        
        /* Minute */
        timeStr[pos++] = '0' + (dt.minute / 10);
        timeStr[pos++] = '0' + (dt.minute % 10);
        timeStr[pos++] = ':';
        
        /* Second */
        timeStr[pos++] = '0' + (dt.second / 10);
        timeStr[pos++] = '0' + (dt.second % 10);
        
        timeStr[pos] = '\0';
    }
    
    return true;
}

/*
 * Read host slots from fujinet-nio
 * Device 0x70 (FujiNet), Command 0xF4 (ReadHostSlots), no payload
 * Response payload: 8 slots x 32 bytes = 256 bytes of null-terminated strings
 * Returns true if successful, fills hostSlots array
 */
Boolean ReadHostSlots(char hostSlots[][32], short numSlots)
{
    unsigned char recvSlip[600];
    unsigned char recvPkt[300];
    short recvPktLen;
    long count;
    long startTicks;
    long timeout = 300;  /* 5 second timeout */
    short i;
    Boolean inFrame;
    Boolean escaped;

    #define FUJINET_DEVICE_ID   0x70
    #define FUJI_CMD_READ_HOST_SLOTS  0xF4

    if (gSerialOutRef == 0 || gSerialInRef == 0) {
        return false;
    }

    /* Flush any pending input */
    SerGetBuf(gSerialInRef, &count);
    if (count > 0) {
        unsigned char dummy[256];
        if (count > sizeof(dummy)) count = sizeof(dummy);
        FSRead(gSerialInRef, &count, dummy);
    }

    /* Send ReadHostSlots command */
    SendFujiBusPacket(FUJINET_DEVICE_ID, FUJI_CMD_READ_HOST_SLOTS, NULL, 0);

    /* Wait for response with timeout */
    startTicks = TickCount();
    recvPktLen = 0;
    inFrame = false;
    escaped = false;

    while ((TickCount() - startTicks) < timeout) {
        SerGetBuf(gSerialInRef, &count);

        if (count > 0) {
            if (count > sizeof(recvSlip)) count = sizeof(recvSlip);
            FSRead(gSerialInRef, &count, recvSlip);

            /* Process SLIP frame */
            for (i = 0; i < count; i++) {
                unsigned char b = recvSlip[i];

                if (b == SLIP_END) {
                    if (inFrame && recvPktLen > 0) {
                        goto rhs_frame_complete;
                    }
                    inFrame = true;
                    recvPktLen = 0;
                    escaped = false;
                } else if (escaped) {
                    if (b == SLIP_ESC_END) {
                        recvPkt[recvPktLen++] = SLIP_END;
                    } else if (b == SLIP_ESC_ESC) {
                        recvPkt[recvPktLen++] = SLIP_ESCAPE;
                    }
                    escaped = false;
                } else if (b == SLIP_ESCAPE) {
                    escaped = true;
                } else if (inFrame && recvPktLen < sizeof(recvPkt)) {
                    recvPkt[recvPktLen++] = b;
                }
            }
        }

        Delay(1, NULL);
    }

    /* Timeout */
    return false;

rhs_frame_complete:
    /* Validate: header(6) + status(1) + payload(256) = 263 bytes minimum */
    if (recvPktLen < 263) {
        return false;
    }

    /* Verify checksum */
    {
        unsigned char savedChecksum = recvPkt[4];
        unsigned char calcChecksum;
        recvPkt[4] = 0;
        calcChecksum = CalcFujiChecksum(recvPkt, recvPktLen);
        recvPkt[4] = savedChecksum;
        if (calcChecksum != savedChecksum) {
            return false;
        }
    }

    /* Check device and command match */
    if (recvPkt[0] != FUJINET_DEVICE_ID || recvPkt[1] != FUJI_CMD_READ_HOST_SLOTS) {
        return false;
    }

    /* Check status */
    if (recvPkt[6] != 0) {
        return false;
    }

    /* Copy payload into hostSlots array (starts at offset 7) */
    {
        short slot;
        for (slot = 0; slot < numSlots && slot < 8; slot++) {
            for (i = 0; i < 32; i++) {
                hostSlots[slot][i] = recvPkt[7 + slot * 32 + i];
            }
            /* Ensure null termination even if slot is full */
            hostSlots[slot][31] = '\0';
        }
    }

    return true;
}

/*
 * Write host slots to fujinet-nio
 * Device 0x70 (FujiNet), Command 0xF3 (WriteHostSlots)
 * Payload: 8 slots x 32 bytes = 256 bytes of null-terminated strings
 * Returns true if response received with success status
 */
Boolean WriteHostSlots(char hostSlots[][32], short numSlots)
{
    unsigned char payload[256];
    unsigned char recvSlip[256];
    unsigned char recvPkt[128];
    short recvPktLen;
    long count;
    long startTicks;
    long timeout = 300;  /* 5 second timeout */
    short i, slot;
    Boolean inFrame;
    Boolean escaped;

    #define FUJI_CMD_WRITE_HOST_SLOTS 0xF3

    if (gSerialOutRef == 0 || gSerialInRef == 0) {
        return false;
    }

    /* Build 256-byte payload from hostSlots */
    for (i = 0; i < 256; i++) {
        payload[i] = 0;
    }
    for (slot = 0; slot < numSlots && slot < 8; slot++) {
        for (i = 0; i < 32; i++) {
            payload[slot * 32 + i] = (unsigned char)hostSlots[slot][i];
        }
    }

    /* Flush any pending input */
    SerGetBuf(gSerialInRef, &count);
    if (count > 0) {
        unsigned char dummy[256];
        if (count > sizeof(dummy)) count = sizeof(dummy);
        FSRead(gSerialInRef, &count, dummy);
    }

    /* Send WriteHostSlots command with payload */
    SendFujiBusPacket(FUJINET_DEVICE_ID, FUJI_CMD_WRITE_HOST_SLOTS, payload, 256);

    /* Wait for response with timeout */
    startTicks = TickCount();
    recvPktLen = 0;
    inFrame = false;
    escaped = false;

    while ((TickCount() - startTicks) < timeout) {
        SerGetBuf(gSerialInRef, &count);

        if (count > 0) {
            if (count > sizeof(recvSlip)) count = sizeof(recvSlip);
            FSRead(gSerialInRef, &count, recvSlip);

            for (i = 0; i < count; i++) {
                unsigned char b = recvSlip[i];

                if (b == SLIP_END) {
                    if (inFrame && recvPktLen > 0) {
                        goto whs_frame_complete;
                    }
                    inFrame = true;
                    recvPktLen = 0;
                    escaped = false;
                } else if (escaped) {
                    if (b == SLIP_ESC_END) {
                        recvPkt[recvPktLen++] = SLIP_END;
                    } else if (b == SLIP_ESC_ESC) {
                        recvPkt[recvPktLen++] = SLIP_ESCAPE;
                    }
                    escaped = false;
                } else if (b == SLIP_ESCAPE) {
                    escaped = true;
                } else if (inFrame && recvPktLen < sizeof(recvPkt)) {
                    recvPkt[recvPktLen++] = b;
                }
            }
        }

        Delay(1, NULL);
    }

    /* Timeout */
    return false;

whs_frame_complete:
    /* Validate: at least header(6) + status(1) = 7 bytes */
    if (recvPktLen < 7) {
        return false;
    }

    /* Verify checksum */
    {
        unsigned char savedChecksum = recvPkt[4];
        unsigned char calcChecksum;
        recvPkt[4] = 0;
        calcChecksum = CalcFujiChecksum(recvPkt, recvPktLen);
        recvPkt[4] = savedChecksum;
        if (calcChecksum != savedChecksum) {
            return false;
        }
    }

    /* Check device and command match */
    if (recvPkt[0] != FUJINET_DEVICE_ID || recvPkt[1] != FUJI_CMD_WRITE_HOST_SLOTS) {
        return false;
    }

    /* Check status */
    if (recvPkt[6] != 0) {
        return false;
    }

    return true;
}

/*
 * List directory contents via FileService
 * Device 0xFE (FileService), Command 0x02 (ListDirectory)
 *
 * Request payload:
 *   u8  version (1)
 *   u8  fsNameLen
 *   u8[] fsName
 *   u16 pathLen (LE)
 *   u8[] path
 *   u16 startIndex (LE)
 *   u16 maxEntries (LE)
 *
 * Response payload (at offset 7):
 *   u8  version
 *   u8  flags (bit0=more entries)
 *   u16 reserved
 *   u16 count (LE)
 *   entries: u8 flags (bit0=isDir) + u8 nameLen + name + u64 size + u64 modTime
 */
Boolean ListDirectory(const char *fsName, const char *path,
                      short startIndex, short maxEntries,
                      DirEntry *entries, short *entryCount,
                      Boolean *hasMore)
{
    unsigned char payload[512];
    unsigned char recvSlip[4096];
    unsigned char recvPkt[2048];
    short recvPktLen;
    long count;
    long startTicks;
    long timeout = 300;  /* 5 second timeout */
    short i, pos;
    Boolean inFrame;
    Boolean escaped;
    short fsNameLen, pathLen;

    #define FILE_SERVICE_DEVICE 0xFE
    #define FILE_CMD_LIST_DIR   0x02

    if (gSerialOutRef == 0 || gSerialInRef == 0) {
        return false;
    }

    /* Measure string lengths */
    for (fsNameLen = 0; fsName[fsNameLen] != '\0' && fsNameLen < 255; fsNameLen++) {}
    for (pathLen = 0; path[pathLen] != '\0' && pathLen < 255; pathLen++) {}

    /* Build request payload */
    pos = 0;
    payload[pos++] = 1;  /* version */
    payload[pos++] = (unsigned char)fsNameLen;
    for (i = 0; i < fsNameLen; i++) {
        payload[pos++] = (unsigned char)fsName[i];
    }
    payload[pos++] = (unsigned char)(pathLen & 0xFF);
    payload[pos++] = (unsigned char)((pathLen >> 8) & 0xFF);
    for (i = 0; i < pathLen; i++) {
        payload[pos++] = (unsigned char)path[i];
    }
    payload[pos++] = (unsigned char)(startIndex & 0xFF);
    payload[pos++] = (unsigned char)((startIndex >> 8) & 0xFF);
    payload[pos++] = (unsigned char)(maxEntries & 0xFF);
    payload[pos++] = (unsigned char)((maxEntries >> 8) & 0xFF);

    /* Flush any pending input */
    SerGetBuf(gSerialInRef, &count);
    if (count > 0) {
        unsigned char dummy[256];
        if (count > sizeof(dummy)) count = sizeof(dummy);
        FSRead(gSerialInRef, &count, dummy);
    }

    /* Send ListDirectory command */
    SendFujiBusPacket(FILE_SERVICE_DEVICE, FILE_CMD_LIST_DIR,
                      payload, pos);

    /* Wait for response with timeout */
    startTicks = TickCount();
    recvPktLen = 0;
    inFrame = false;
    escaped = false;

    while ((TickCount() - startTicks) < timeout) {
        SerGetBuf(gSerialInRef, &count);

        if (count > 0) {
            if (count > sizeof(recvSlip)) count = sizeof(recvSlip);
            FSRead(gSerialInRef, &count, recvSlip);

            for (i = 0; i < count; i++) {
                unsigned char b = recvSlip[i];

                if (b == SLIP_END) {
                    if (inFrame && recvPktLen > 0) {
                        goto ld_frame_complete;
                    }
                    inFrame = true;
                    recvPktLen = 0;
                    escaped = false;
                } else if (escaped) {
                    if (b == SLIP_ESC_END) {
                        recvPkt[recvPktLen++] = SLIP_END;
                    } else if (b == SLIP_ESC_ESC) {
                        recvPkt[recvPktLen++] = SLIP_ESCAPE;
                    }
                    escaped = false;
                } else if (b == SLIP_ESCAPE) {
                    escaped = true;
                } else if (inFrame && recvPktLen < sizeof(recvPkt)) {
                    recvPkt[recvPktLen++] = b;
                }
            }
        }

        Delay(1, NULL);
    }

    /* Timeout */
    return false;

ld_frame_complete:
    /* Validate: header(6) + status(1) + payload header(6) = 13 bytes minimum */
    if (recvPktLen < 13) {
        return false;
    }

    /* Verify checksum */
    {
        unsigned char savedChecksum = recvPkt[4];
        unsigned char calcChecksum;
        recvPkt[4] = 0;
        calcChecksum = CalcFujiChecksum(recvPkt, recvPktLen);
        recvPkt[4] = savedChecksum;
        if (calcChecksum != savedChecksum) {
            return false;
        }
    }

    /* Check device and command match */
    if (recvPkt[0] != FILE_SERVICE_DEVICE || recvPkt[1] != FILE_CMD_LIST_DIR) {
        return false;
    }

    /* Check status */
    if (recvPkt[6] != 0) {
        return false;
    }

    /* Parse payload starting at offset 7 */
    {
        short payloadOff = 7;
        /* u8 version, u8 flags, u16 reserved, u16 count */
        unsigned char respFlags;
        short respCount;
        short entry;

        if (payloadOff + 6 > recvPktLen) {
            return false;
        }

        /* skip version byte */
        payloadOff++;
        respFlags = recvPkt[payloadOff++];
        /* skip reserved u16 */
        payloadOff += 2;
        respCount = recvPkt[payloadOff] | (recvPkt[payloadOff + 1] << 8);
        payloadOff += 2;

        *hasMore = (respFlags & 0x01) != 0;
        *entryCount = 0;

        for (entry = 0; entry < respCount && *entryCount < maxEntries; entry++) {
            unsigned char eFlags;
            unsigned char nameLen;
            short j;

            if (payloadOff + 2 > recvPktLen) break;

            eFlags = recvPkt[payloadOff++];
            nameLen = recvPkt[payloadOff++];

            if (payloadOff + nameLen + 16 > recvPktLen) break;

            /* Copy name (truncate to 63 chars) */
            for (j = 0; j < nameLen && j < 63; j++) {
                entries[*entryCount].name[j] = (char)recvPkt[payloadOff + j];
            }
            entries[*entryCount].name[j] = '\0';
            payloadOff += nameLen;

            entries[*entryCount].isDir = (eFlags & 0x01) != 0;

            /* Skip u64 sizeBytes + u64 modifiedTime = 16 bytes */
            payloadOff += 16;

            (*entryCount)++;
        }
    }

    return true;
}

/*
 * Mount a disk image via DiskService
 * Device 0xFC (DiskService), Command 0x01 (Mount)
 *
 * Request payload:
 *   u8  version (1)
 *   u8  slot (1-based)
 *   u8  flags (bit0=readOnly)
 *   u8  typeRaw (0=auto)
 *   u16 sectorSizeHint (LE, 0=auto)
 *   u16 fsNameLen (LE) + u8[] fsName
 *   u16 pathLen (LE) + u8[] path
 */
Boolean MountDisk(short slot, Boolean readOnly,
                  const char *fsName, const char *path)
{
    unsigned char payload[512];
    unsigned char recvSlip[256];
    unsigned char recvPkt[128];
    short recvPktLen;
    long count;
    long startTicks;
    long timeout = 300;  /* 5 second timeout */
    short i, pos;
    Boolean inFrame;
    Boolean escaped;
    short fsNameLen, pathLen;

    #define DISK_SERVICE_DEVICE  0xFC
    #define DISK_CMD_MOUNT       0x01

    if (gSerialOutRef == 0 || gSerialInRef == 0) {
        return false;
    }

    /* Measure string lengths */
    for (fsNameLen = 0; fsName[fsNameLen] != '\0' && fsNameLen < 255; fsNameLen++) {}
    for (pathLen = 0; path[pathLen] != '\0' && pathLen < 255; pathLen++) {}

    /* Build request payload */
    pos = 0;
    payload[pos++] = 1;  /* version */
    payload[pos++] = (unsigned char)slot;  /* 1-based slot */
    payload[pos++] = readOnly ? 0x01 : 0x00;  /* flags */
    payload[pos++] = 0;  /* typeRaw = auto */
    payload[pos++] = 0;  /* sectorSizeHint low = 0 */
    payload[pos++] = 0;  /* sectorSizeHint high = 0 */
    /* fsNameLen as u16 LE */
    payload[pos++] = (unsigned char)(fsNameLen & 0xFF);
    payload[pos++] = (unsigned char)((fsNameLen >> 8) & 0xFF);
    for (i = 0; i < fsNameLen; i++) {
        payload[pos++] = (unsigned char)fsName[i];
    }
    /* pathLen as u16 LE */
    payload[pos++] = (unsigned char)(pathLen & 0xFF);
    payload[pos++] = (unsigned char)((pathLen >> 8) & 0xFF);
    for (i = 0; i < pathLen; i++) {
        payload[pos++] = (unsigned char)path[i];
    }

    /* Flush any pending input */
    SerGetBuf(gSerialInRef, &count);
    if (count > 0) {
        unsigned char dummy[256];
        if (count > sizeof(dummy)) count = sizeof(dummy);
        FSRead(gSerialInRef, &count, dummy);
    }

    /* Send Mount command */
    SendFujiBusPacket(DISK_SERVICE_DEVICE, DISK_CMD_MOUNT, payload, pos);

    /* Wait for response with timeout */
    startTicks = TickCount();
    recvPktLen = 0;
    inFrame = false;
    escaped = false;

    while ((TickCount() - startTicks) < timeout) {
        SerGetBuf(gSerialInRef, &count);

        if (count > 0) {
            if (count > sizeof(recvSlip)) count = sizeof(recvSlip);
            FSRead(gSerialInRef, &count, recvSlip);

            for (i = 0; i < count; i++) {
                unsigned char b = recvSlip[i];

                if (b == SLIP_END) {
                    if (inFrame && recvPktLen > 0) {
                        goto md_frame_complete;
                    }
                    inFrame = true;
                    recvPktLen = 0;
                    escaped = false;
                } else if (escaped) {
                    if (b == SLIP_ESC_END) {
                        recvPkt[recvPktLen++] = SLIP_END;
                    } else if (b == SLIP_ESC_ESC) {
                        recvPkt[recvPktLen++] = SLIP_ESCAPE;
                    }
                    escaped = false;
                } else if (b == SLIP_ESCAPE) {
                    escaped = true;
                } else if (inFrame && recvPktLen < sizeof(recvPkt)) {
                    recvPkt[recvPktLen++] = b;
                }
            }
        }

        Delay(1, NULL);
    }

    /* Timeout */
    return false;

md_frame_complete:
    /* Validate: header(6) + status(1) = 7 bytes minimum */
    if (recvPktLen < 7) {
        return false;
    }

    /* Verify checksum */
    {
        unsigned char savedChecksum = recvPkt[4];
        unsigned char calcChecksum;
        recvPkt[4] = 0;
        calcChecksum = CalcFujiChecksum(recvPkt, recvPktLen);
        recvPkt[4] = savedChecksum;
        if (calcChecksum != savedChecksum) {
            return false;
        }
    }

    /* Check device and command match */
    if (recvPkt[0] != DISK_SERVICE_DEVICE || recvPkt[1] != DISK_CMD_MOUNT) {
        return false;
    }

    /* Check status */
    if (recvPkt[6] != 0) {
        return false;
    }

    return true;
}

/*
 * Poll for incoming serial data and display in receive area
 */
void PollSerialInput(void)
{
    long count;
    char buffer[256];
    long i;
    Rect textFrame;

    if (gSerialInRef == 0 || gRecvText == NULL || gSerialWindow == NULL) {
        return;
    }

    /* Check how many bytes are available */
    SerGetBuf(gSerialInRef, &count);

    if (count <= 0) {
        return;
    }

    /* Limit to buffer size */
    if (count > sizeof(buffer)) {
        count = sizeof(buffer);
    }

    /* Read the data */
    FSRead(gSerialInRef, &count, buffer);

    /* Process received characters */
    SetPort(gSerialWindow);

    for (i = 0; i < count; i++) {
        char c = buffer[i];

        /* Convert LF to CR for Mac TextEdit */
        if (c == '\n') {
            c = '\r';
        }

        /* Skip CR if followed by LF (handle CRLF) */
        if (c == '\r' && i + 1 < count && buffer[i + 1] == '\n') {
            continue;
        }

        /* Insert character at end of receive text */
        TESetSelect(32767, 32767, gRecvText);
        TEKey(c, gRecvText);
    }

    /* Limit receive buffer size - remove oldest text if too large */
    if ((*gRecvText)->teLength > kMaxReceiveText) {
        TESetSelect(0, (*gRecvText)->teLength - kMaxReceiveText, gRecvText);
        TEDelete(gRecvText);
    }

    /* Update the receive area */
    SetRect(&textFrame, kRecvLeft, kRecvTop, kRecvRight, kRecvBottom);
    InvalRect(&textFrame);
}
