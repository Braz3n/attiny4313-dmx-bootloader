DEVICE = attiny4313

all:
	avr-gcc -c -mmcu=$(DEVICE) -std=gnu11 -Wall -c assembly.s -o assembly.o
	avr-gcc -c -mmcu=$(DEVICE) -std=gnu11 -Wall main.c -o main.o
	avr-gcc -o main.elf main.o assembly.o -Wl,-Map,main.map -mmcu=$(DEVICE) -Wl,--gc-sections -Wl,--print-gc-sections
	avr-objcopy -O ihex main.elf main.hex
	#rm -f *.o *.elf

clean:
	rm -f *.o *.elf *.hex *.assm *.map

deassm:
	# Convert the hex file back into assembly code.
	avr-objdump --no-show-raw-insn -m avr -D main.hex > gcc.assm

load:
	# Load the hex file to the stk600
	sudo avrdude -p atmega8 -c stk600 -P usb -v -v -U flash:w:main.hex
