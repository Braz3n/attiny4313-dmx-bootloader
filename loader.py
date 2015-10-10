from intelhex import IntelHex
from crc16 import crc16xmodem
from time import sleep
import serial
import array

PAGE_COUNT = 64
PAGE_BYTES = 64

class BootloaderMessage(object):
    def __init__(self, page_number, payload_length, payload):
        if type(page_number) is not int:
            raise TypeError("Page Number is not an integer type.")
        if type(payload_length) is not int:
            raise TypeError("Payload Length is not an integer type.")
        if payload is None:
            pass
        elif type(payload) is not bytearray and type(payload) is not array.array:
            print type(payload)
            raise TypeError("Payload is not a bytearray or array type.")

        self.page_number = page_number
        self.payload_length = payload_length
        self.payload = payload
        self.checksum = self._calculate_checksum()

    def _calculate_checksum(self):
        checksum = crc16xmodem(chr(self.page_number))
        checksum = crc16xmodem(chr(self.payload_length), checksum)
        if self.payload is not None:
            for char in self.payload:
                checksum = crc16xmodem(chr(char), checksum)
        return checksum

    def transmit_message(self, port):
        # Preamble
        port.write(chr(0xAA))
        # Page Number
        port.write(chr(self.page_number))
        # Payload Length
        port.write(chr(self.payload_length))
        # Payload
        if self.payload is not None:
            for char in self.payload:
                port.write(chr(char))
                sleep(0.01)
        # Checksum
        port.write(chr(self.checksum & 0xFF))
        port.write(chr((self.checksum & 0xFF00) >> 8))

def transmit_hex_file(hex_file, serial_port):
    MIN_PAGE = hex_file.minaddr() / PAGE_BYTES
    MAX_PAGE = hex_file.maxaddr() / PAGE_BYTES

    for page_number in xrange(MIN_PAGE, MAX_PAGE + 1):
        page_address = page_number * PAGE_BYTES
        page_message = BootloaderMessage(page_number, PAGE_BYTES, hex_file.tobinarray(start=page_address, size=PAGE_BYTES))
        page_message.transmit_message(serial_port)
        sleep(1)

    page_message = BootloaderMessage(MAX_PAGE+1, 0, None)
    page_message.transmit_message(serial_port)

def main():
    serial_port = serial.Serial("COM5", 250000, stopbits=serial.STOPBITS_TWO)
    hex_file = IntelHex("dmxSlave.hex")
    transmit_hex_file(hex_file, serial_port)

if __name__ == "__main__":
    main()
