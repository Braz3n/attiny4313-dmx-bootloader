# ATtiny4313 DMX Bootloader

The purpose of this project was to design a bootloader that will support my [ATtiny4313 DMX Slave project](https://github.com/Braz3n/attiny4313-dmx-slave) project. 
The bootloader works using the DMX port on the back of the light. The bootloader communicates at 250,000 baud, simular to the DMX-512 protocol.

For application to be run correctly, the application code must begin at byte 0x400 (or word 0x200).

### Message Format
Due to the design of the hardware it isn't possible to achieve bi-directional communication, so a preamble byte and checksum are used to help ensure synchronisation and detect transmission errors. The checksum used is the 16-bit XMODEM CRC as defined in _crc_xmodem_update() from \<util/crc16.h\>.

Index         | Content         | Value     | Description
--------------|-----------------|-----------|-------------
0             | Start Sign      | 0xAA      | Indicates the beginning of a new message.
1             | Page Number     | 0-63      | The page number being updated. Any value below 0x10 will be ignored.
2             | Payload Length  | 0-64      | The number of bytes in the payload field.
3 to (n+3)    | Data            | 0-255     | A series of bytes that should be written into memory in little-endian order.
(n+4) to (n+5)| Checksum        | 0-65535   | 16-bit CRC-XMODEM applied over the page number, payload length and data bytes.

### Running the Application
There are three conditions which cause the application to be run:
1. The bootloader receives a valid message with a zero-length payload.
2. A break character is received, implying that the light is being sent DMX-512 data.
3. PD5 is high. This means the light is in "Manual Control" mode and the RS-485 receiver has been disabled.

### Interrupt Handling
The bootloader occupies the first 1024 bytes of memory. As such, the bootloader also controls the Interrupt Vector Table (IVT). To allow for the application to call interrupts, the bootloader has a implemented simple interrupt service routine (ISR) that redirects execution to the respective ISR in the application jump table. These ISRs do not do anything other than execute an "rjmp" instruction. All state preservation is expected to be performed by the application ISR. This configuration introduces 4 cycles of latency to the beginning of each ISR. There is the option of reducing this latency to 2 cycles if the bootloader IVT and pre-main code is manually written in assembly instead of being handled by the C compiler. 

## Uploader Script
An uploader script has also been written for Python. This requires a USB-Serial device connected to an RS-485 transciever as well as the application code in the .hex file format.
A DMX connector will also be required for connecting to the light.

The following command line flags have been implemented.

Command Line Flag   | Required | Description
--------------------|----------|----
-p *serial_port*    | Yes      |The serial port to upload the file over.
-f *hex_file*       | Yes      |The hex file to upload to the microcontroller.
-d                  | No       |Print information about the hex file to the terminal (Start/End address and total space used).
-h                  | No       |Print the usage information for the scrip.
