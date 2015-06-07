import argparse

class Page(object):
    pageData = bytearray()
    pageNumber = int()


def main():
    parser = argparse.ArgumentParser(description='Loader script for a custom bootloader on the ATtiny4313.')
    parser.add_argument('-f', nargs=1, required=True, help="The hex file to be uploaded.")

    args = parser.parse_args()
    hex_file_name = args.f[0]


    with open(hex_file_name, 'r') as opened_hex_file:
        hex_file = opened_hex_file.readlines()


    pageArray = []
    for line in hex_file:
        pageArray.append(Page())


if __name__ == "__main__":
    main()
