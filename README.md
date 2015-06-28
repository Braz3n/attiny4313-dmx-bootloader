# ATtiny4313 DMX Bootloader
### Currently under development

The purpose of this project is to design a bootloader that will support my [ATtiny4313 DMX Slave project](https://github.com/Braz3n/attiny4313-dmx-slave) project. 
The aim is the bootloader work using the DMX port on the back of the light.

The protocol itself is very simple, with only three types of messages, as well as using break characters for synchronisation and an internal "message" for keeping track of the stage.

Currently, there isn't any support for responses from the microcontroller (due to the design of the hardware), although there is the potential for some limited communication
by toggling the output of PD5 as long as the light is in manual control mode. As such, loss of synchronisation between the uploader and the bootloader is a concern.

The use of a checksum is also being considered to ensure there aren't any errors in communication. The need for this is mitigated by the use of a differential protocol however.

Message Byte    | Description
----------------|--------------
0x01            | DATA_START (Followed by one byte) Data messages have a single byte which indicates how many bytes follow. These bytes fill a continuous space in memory.
0x02            | DATA_BYTES This message is never actually transmitted, but rather used as a "state" that the bootloader occupies.
0x03            | PAGE_NUM (Followed by one byte) Number of the memory page for following data messages to address into.
0xFF            | EOF (No following bytes) Indicates the file is complete and there is nothing left to be written into memory.
Break Character | If followed by a zero byte, the light is receiving DMX packets and should boot to the application. Otherwise, the next received byte indicates the next message.

## Uploader Script
An uploader script has also been written for Python. This requires a USB-Serial device connected to an RS-485 transciever as well as the application code in the .hex file format.

Command Line Flag   | Description
--------------------|--------------
-p *serial_port*    | The serial port to upload the file over.
-f *hex_file*       | The hex file to upload to the microcontroller.
-v                  | Print information about the hex file to the terminal (Start/End address and total space used).
-d                  | Print the entire contents of the hex file to the terminal (Each page on a line in hexadecimal format).
