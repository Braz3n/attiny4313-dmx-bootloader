import argparse
import serial

NUM_FLASH_PAGES = 64
NUM_PAGE_BYTES = 64

PAGE_ADDRESS_MASK = 0x3F
PAGE_NUMBER_MASK = 0xFC0

SERIAL_PORT = "None"
SERIAL_BAUD = 250000

# Message Identifiers
MESSAGE_DATA_START = 0x01
MESSAGE_PAGE_NUMBER = 0x03
MESSAGE_EOF = 0xFF

# This is the first page of the flash memory that can be used by the program
# code. The earlier pages are occupied by the bootloader, and thus are protected.
PROGRAM_START_PAGE = 32

class HexLine(object):
    def __init__(self, length, address, record, data, checksum):
        self.length = length
        self.address = address
        self.record = record
        self.data = data
        self.checksum = checksum

class Page(object):
    def __init__(self, pageNumber):
        self.pageNumber = pageNumber
        self.pageData = bytearray([])
        self.used = False
        for i in xrange(0, NUM_PAGE_BYTES):
            self.pageData.append(0x00)

    def __str__(self):
        output = ""
        for i in xrange(0, NUM_PAGE_BYTES):
            output += "{0:02X} ".format(self.pageData[i])
        return output

def processHexLine(line):
    line = line.strip()  # Remove the newline character at the end of each line.
    length = int(line[1:3], 16)
    address = int(line[3:7], 16)  # Address is always big endian.
    record = int(line[7:9], 16)
    dataString = line[9:-2]
    checksum = int(line[-2:], 16)
    dataArray = bytearray.fromhex(dataString)

    return HexLine(length, address, record, dataArray, checksum)

def fillPageArray(hexFile, pageArray):
    for line in hexFile:
        hexLine = processHexLine(line)
        pageNumber = (hexLine.address >> 6) & PAGE_ADDRESS_MASK
        pageAddress = hexLine.address & PAGE_ADDRESS_MASK
        for i in xrange(0, len(hexLine.data)):
            pageArray[pageNumber].pageData[i + pageAddress] = hexLine.data[i]
            pageArray[pageNumber].used = True

    return pageArray

def sendPageMessage(port, pageNumber):
    port.write(MESSAGE_PAGE_NUMBER)
    port.write(pageNumber)

def sendPageData(port, page):
    sendPageMessage(page.pageNumber)
    port.write(MESSAGE_DATA_START)
    port.write(NUM_PAGE_BYTES)
    port.write(page.pageData)

def uploadCode(pageArray):
    port = Serial()
    port.port = SERIAL_PORT
    port.baud = SERIAL_BAUD
    for page in pageArray:
        if not page.used:
            continue
        sendPageData(port, page)
    page.write(MESSAGE_EOF)

def printCodeStats(pageArray, hexFile):
    lowPage = NUM_FLASH_PAGES
    highPage = 0
    lowAddress = NUM_FLASH_PAGES * NUM_PAGE_BYTES
    highAddress = 0

    for page in pageArray:
        if page.pageNumber < lowPage:
            lowPage = page.pageNumber
        if page.pageNumber > highPage:
            highPage = page.pageNumber

    for line in hexFile:
        hexLine = processHexLine(line)
        if hexLine.address < lowAddress:
            lowAddress = hexLine.address
        elif (hexLine.address + hexLine.length - 1) > highAddress:
            highAddress = hexLine.address + hexLine.length - 1

    dataLength = highAddress - lowAddress

    print "Low Address: 0x{0:X}".format(lowAddress)
    print "High Address: 0x{0:X}".format(highAddress)
    print "Total Space Occupied: {0} bytes".format(dataLength)

def main():
    parser = argparse.ArgumentParser(description='Loader script for a custom bootloader on the ATtiny4313.')
    parser.add_argument('-p', nargs='?', required=False, help="The serial port to upload the file over.")
    parser.add_argument('-f', nargs=1, required=True, help="The hex file to be uploaded.")
    parser.add_argument('-v', nargs='?', const=True, required=False, help="Print information about the hex file to the terminal.")
    parser.add_argument('-d', nargs='?', const=True, required=False, help="Print hex bytes to the terminal.")

    args = parser.parse_args()
    hexFileName = args.f[0]

    with open(hexFileName, 'r') as openedHexFile:
        hexFile = openedHexFile.readlines()

    pageArray = []
    for i in xrange(NUM_FLASH_PAGES):
        pageArray.append(Page(i))

    fillPageArray(hexFile, pageArray)

    # Print out the Hex Data.
    if args.d is not None:
        for i in xrange(NUM_FLASH_PAGES):
            print pageArray[i]

    # Print out size data for the Hex File.
    if args.v is not None:
        printCodeStats(pageArray, hexFile)

    # Upload the file.
    if args.p is not None:
        SERIAL_PORT = args.p
        uploadCode(pageArray)


if __name__ == "__main__":
    main()
