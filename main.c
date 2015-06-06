/*
 *  main.c
 *  Zane Barker
 *  1/6/2015
 *  A simple DMX bootloader for an LED stage light using the ATtiny4313.
 *
 */


#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>

// PWM Pin Definitions.
#define RGB_PORT    PORTD
#define RGB_DDR     DDRD
#define RGB_PIN     PIND
#define R_PWM_PIN   2
#define G_PWM_PIN   3
#define B_PWM_PIN   4
// 0x01 - DIV1
// 0x02 - DIV8
// 0x03 - DIV64
// 0x04 - DIV256
// 0x05 - DIV1024
#define TIMER_PRESCALE 0x01
// For a 16MHz clock, use 0x00 and 0x03.
// For a 8MHz clock, use 0x00 and 0x01.
#define DMX_BAUD_DIVIDER_HIGH   0x00
#define DMX_BAUD_DIVIDER_LOW    0x03

#define PAGE_BUFFER_LENGTH 64

typedef struct {
	uint16_t address;
	uint8_t dataBytesRemaining;
} UartState;

typedef struct {
	uint16_t pageNumber;
	char data[PAGE_BUFFER_LENGTH];
	uint8_t length;
} DataPacket;

typedef enum {
	NONE = 0x00,
	DATA_START = 0x01,
	DATA_BYTES = 0x02,
	PAGE_NUM = 0x03,
	EOF = 0x80
} messageType;

// Double Buffering Messages
DataPacket messageBuffer[2];
uint8_t activeBuffer = 0;

// MESSAGE FORMAT
// 0x01 - DATA_START (Followed by one byte)
// Data messages have a single byte which indicates how many bytes
// follow. These bytes fill a continuous space in memory.
// 0x02 - DATA_BYTES
// This message is never actually transmitted, but rather used as
// a "state" that the bootloader occupies.
// 0x03 - PAGE_NUM (Followed by one byte)
// Number of the memory page to address.
// 0xFF - EOF (No following bytes)
// Indicates that the file is complete and there is nothing left
// to be written to memory.
// BREAK CHARACTER (Followed by one byte)
// If followed by a zero byte, the light is receiving DMX packets
// and should boot to application. Otherwise, the next byte
// will be the beginning of a new message.

void bootApplication(void);
void writeZReg(uint16_t data);
void pageBufferErase();
void pageBufferWrite(uint16_t data, uint16_t address);
void pageErase(uint16_t pageNumber);
void pageWrite(uint16_t pageNumber);

ISR(USART0_RX_vect) {
	static uint8_t bufferIndex = 0;
	static uint8_t breakReceived = 0;
	static messageType currentMessage = NONE;
	static uint8_t expectedBytes = 0;
	uint8_t statusByte = UCSRA;
	char latestData = UDR;
	
	// Detected a break character. Boot application.
	if (statusByte & (1 << 4)) {
		breakReceived = 1;
	}
	else if (breakReceived && latestData == 0x00) {
		// Receiving DMX messages. Boot to application.
		bootApplication();
	}
	else if (breakReceived) {
		// Synchronization for new messages.
		breakReceived = 0;
		return;
	}
	
	switch (currentMessage) {
		case (NONE):
			bufferIndex = 0;
			switch (latestData) {
				case (char)DATA_START:
					expectedBytes = 1;
					break;
				case (char)PAGE_NUM:
					expectedBytes = 1;
					break;
				case (char)EOF:
					break;
			}
			break;
		case (DATA_START):
			expectedBytes = latestData;
			currentMessage = DATA_BYTES;
			break;
		case (DATA_BYTES):
			messageBuffer[activeBuffer].data[bufferIndex] = latestData;
			bufferIndex++;
			expectedBytes--;
			if (expectedBytes == 0) {
				messageBuffer[activeBuffer].length = bufferIndex;
				currentMessage = NONE;
				activeBuffer ^= 0x01;
			}
			break;
		case (PAGE_NUM):
			messageBuffer[activeBuffer].pageNumber = latestData;
			currentMessage = NONE;
			break;
		case (EOF):
			bootApplication();
			break;
	}
}

void processBuffer(void) {
	static uint8_t lastProcessed = 1;
	uint16_t dataWord;
	
	// Check if we have a new buffer to process.
	// Currently assuming we process data faster than receive it.
	if (activeBuffer != lastProcessed) {
		return;
	}
	
	uint8_t inactiveBuffer = activeBuffer ^ 0x01;
	
	uint16_t address = messageBuffer[inactiveBuffer].pageNumber << 5;
	
	for (uint8_t i=0; i < messageBuffer[inactiveBuffer].length; i+=2) {
		dataWord = (uint16_t)messageBuffer[inactiveBuffer].data[i+1] << 8;
		dataWord += messageBuffer[inactiveBuffer].data[i];
		pageBufferWrite(dataWord, address + i);
	}
	pageBufferErase();
}

void bootApplication(void) {
	// Needs to make sure that all of the buffers have been successfully processed.
}

void writeZReg(uint16_t data) {
	#warning Needs testing. Might require address of variable instead.
	//uint8_t lowByte = data & 0x00FF;
	//uint8_t highByte = data & 0xFF00;
	//asm("lds r30, %0"::"r"(lowByte):"r30");
	//asm("lds r31, %0"::"r"(highByte):"r31");
	*(uint8_t*)0x1E = data & 0x00FF;
	*(uint8_t*)0x1F = data & 0xFF00;
}

void pageBufferErase() {
	SPMCSR = 0x11;
	asm("spm"::);
}

void pageBufferWrite(uint16_t data, uint16_t address) {
	writeZReg(address);
	#warning Needs testing. Might require address of variable instead.
	//uint8_t lowByte = data & 0x00FF;
	//uint8_t highByte = data & 0xFF00;
	//asm("lds r0, %0"::"r"(&lowByte):"r0");
	//asm("lds r1, %0"::"r"(&highByte):"r1");
	*(uint8_t*)0x01 = data & 0x00FF;
	asm("mov r0, r1"::);
	*(uint8_t*)0x01 = data & 0xFF00;
}

void pageErase(uint16_t pageNumber) {
	// Shifted by six so it occupies the PCPAGE field.
	writeZReg(pageNumber << 6);  
	SPMCSR = 0x03;
	asm("spm"::);
}

void pageWrite(uint16_t pageNumber) {
	// Shifted by six so it occupies the PCPAGE field.
	writeZReg(pageNumber << 6);
	SPMCSR = 0x05;
	asm("spm"::);
}

void bootloaderInit(void) {
	// Configure the UART Controller.
	// Set the UART Baud Rate.
	// Baud Rate = 250000.
	UBRRH = DMX_BAUD_DIVIDER_HIGH;
	UBRRL = DMX_BAUD_DIVIDER_LOW;
	// Set the UART Frame Format.
	// 8N2 UART.
	UCSRC = (0x01 << 3) | (0x03 << 1);
	// Enable the Rx Complete Interrupt and the UART Receiver.
	UCSRB = (1 << 7) | (1 << 4);
	
	// Globally enable interrupts.
	sei();
}

void bootloaderReset(void) {
		// Reset the UART registers to their reset state.
		UBRRH = 0x00;
		UBRRL = 0x00;
		UCSRC = 0x06;
		UCSRB = 0x00;
		
		// Globally disable interrupts.
		cli();
}

int main() {
	while (1) {
		processBuffer();
	}
	
	return 0;
}