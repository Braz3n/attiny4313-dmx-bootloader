from crc16 import crc16xmodem
from time import sleep
import intelhex
import os
import serial
import argparse
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

def print_details(hex_file):
    available_bytes = 4096 - 1024 # The first 1024 bytes are reserved for the bootloader.
    min_addr = hex_file.minaddr()
    max_addr = hex_file.maxaddr()
    used_bytes = max_addr - min_addr
    percent_used = (float(used_bytes) / float(available_bytes))
    print "Min Addr: {0}".format(min_addr)
    print "Max Addr: {0}".format(max_addr)
    print "Total Space Used: {0} bytes, {1:.2%}".format(used_bytes, percent_used)

def main():
    parser = argparse.ArgumentParser(description="DMX Bootloader for the ATtiny4313 in an LED stage light.")
    parser.add_argument("-p", "--port", dest="port", action="store", required=True, help="The serial port to communicate over.")
    parser.add_argument("-f", "--file", dest="file", action="store", required=True, help="The hex file to upload.")
    parser.add_argument("-d", "--detail", dest="detail", action="store_const", const=True, default=False, help="Print the minimum and maximum addresses and the amount of space used.")
    args = parser.parse_args()

    # Perform some error checking on the inputs.
    try:
        hex_file = intelhex.IntelHex(args.file)
    except intelhex.HexRecordError:
        print "Error: Hex file \"{0}\" contains invalid records. The file may be corrupted".format(args.file)
    except IOError:
        print "Error: Hex file \"{0}\" does not exist.".format(args.file)
    finally:
        exit()

    try:
        serial_port = serial.Serial(args.port, 250000, stopbits=serial.STOPBITS_TWO)
    except OSError:
        print "Error: Serial port \"{0}\" is currently busy and cannot be used.".format(args.port)
    except IOError:
        print "Error: Serial port \"{0}\" does not exist.".format(args.port)
    finally:
        exit()

    # If the user has asked for details about the file, print the information to stdout.
    if (args.detail):
        print_details(hex_file)

    # Actually transmit the data.
    transmit_hex_file(hex_file, serial_port)

if __name__ == "__main__":
    main()
