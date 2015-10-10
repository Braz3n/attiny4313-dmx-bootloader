# ATtiny4313 DMX Bootloader
### Currently under development

The purpose of this project is to design a bootloader that will support my [ATtiny4313 DMX Slave project](https://github.com/Braz3n/attiny4313-dmx-slave) project. 
The aim is the bootloader work using the DMX port on the back of the light. The bootloader communicates at 250,000 baud, simular to the DMX-512 protocol.

Due to the design of the hardware it isn't possible to achieve bi-directional communication, so a preamble byte and checksum are used to help ensure synchronisation and detect transmission errors. The checksum used is the 16-bit XMODEM CRC as defined in _crc_xmodem_update() from \<util/crc16.h\>.

Index         | Content         | Value     | Description
--------------|-----------------|-----------|-------------
0             | Start Sign      | 0xAA      | Indicates the beginning of a new message.
1             | Page Number     | 0-63      | The page number being updated. Any value below 0x10 will be ignored.
2             | Payload Length  | 0-64      | The number of bytes in the payload field.
3 to (n+3)    | Data            | 0-255     | A series of bytes that should be written into memory in little-endian order.
(n+4) to (n+5)| Checksum        | 0-65535   | 16-bit CRC-XMODEM applied over the page number, payload length and data bytes.

## Uploader Script
An uploader script has also been written for Python. This requires a USB-Serial device connected to an RS-485 transciever as well as the application code in the .hex file format.

The following command line flags have yet to be implemented.

Command Line Flag   | Description
--------------------|--------------
-p *serial_port*    | The serial port to upload the file over.
-f *hex_file*       | The hex file to upload to the microcontroller.
-v                  | Print information about the hex file to the terminal (Start/End address and total space used).
-d                  | Print the entire contents of the hex file to the terminal (Each page on a line in hexadecimal format).

## To-Do List
- Implement the above command line flags.
- Implement a method of indicating that an malformed packed was received.
