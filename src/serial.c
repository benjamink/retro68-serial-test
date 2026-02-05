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
    HiliteControl(gSendButton, 1);
    Delay(8, NULL);
    HiliteControl(gSendButton, 0);

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
    unsigned char pkt[64];   /* raw FujiBus packet */
    unsigned char slip[130]; /* SLIP-encoded output (worst case 2x + 2) */
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
    HiliteControl(gResetButton, 1);
    Delay(8, NULL);
    HiliteControl(gResetButton, 0);
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

    if (gSerialInRef == 0 || gRecvText == NULL) {
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
    SetPort(gMainWindow);

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
