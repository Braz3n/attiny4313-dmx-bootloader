/*
 * main.c
 * Zane Barker
 * 28/08/2015
 * A simple DMX bootloader for an LED stage light running on the ATtiny4313.
 */

#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/crc16.h>
#include <stdio.h>

// Convenience macros for fixed-length integers.
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t

// Assembly Subroutine Definitions.
extern void pageBufferWrite(u16 data, u16 address);
extern void pageBufferErase(void);
extern void pageWrite(u16 pageAddress);
extern void pageErase(u16 pageAddress);
extern void jumpToApplication(u16 applicationAddress);

// Application Start Address.
#define APPLICATION_BOOT_ADDRESS  0x400
#define APPLICATION_START_PAGE    0x10

// UART Configuration.
// 250000 Baud
// For a 16MHz clock, use 0x00 and 0x03.
// For an 8MHz clock, use 0x00 and 0x01.
#define DMX_BAUD_DIVIDER_HIGH   0x00
#define DMX_BAUD_DIVIDER_LOW    0x01

#define MESSAGE_PREAMBLE 0xAA
#define PAGE_BUFFER_LENGTH  64

typedef enum {
  PREAMBLE,
  PAGE_NUMBER,
  PAYLOAD_LENGTH,
  PAYLOAD_DATA,
  CHECKSUM
} isr_state_t;

// MESSAGE FORMAT
// Preamble - One Byte (Always 0xAA)
// Page Number - One Byte
// Payload Length - One Byte
// Payload Data - One Byte * Payload Length
// Checksum - Two Bytes
//    _crc_ccitt_update() from <util/crc16.h>
// For all data sections, the data is transmitted in Little Endian Order
// A message with zero length indicates the end of the application data.

typedef struct {
  u8 page_number;
  u8 payload_length;
  u8 data[PAGE_BUFFER_LENGTH];
  u16 checksum;
  u8 message_valid;
} message_buffer_t;

message_buffer_t received_buffer;
u8 boot_application_flag = 0;

ISR(USART0_RX_vect) {
  static isr_state_t isr_state = PREAMBLE;
  static u8 bytes_received = 0;
  u8 status_register = UCSRA;
  u8 latest_data = UDR;
  // Detected a break character. Boot application.
  if (status_register & (1 << 4)) {
    boot_application_flag = 1;
  }

  switch (isr_state) {
    case (PREAMBLE):
      if (latest_data == MESSAGE_PREAMBLE) {
        isr_state = PAGE_NUMBER;
      }
      break;
    case (PAGE_NUMBER):
      received_buffer.page_number = latest_data;
      isr_state = PAYLOAD_LENGTH;
      break;
    case (PAYLOAD_LENGTH):
      received_buffer.payload_length = latest_data;
      if (latest_data == 0) {
        isr_state = CHECKSUM;
      }
      else {
        isr_state = PAYLOAD_DATA;
      }
      break;
    case (PAYLOAD_DATA):
      received_buffer.data[bytes_received] = latest_data;
      bytes_received++;
      if (bytes_received == received_buffer.payload_length || bytes_received == PAGE_BUFFER_LENGTH) {
        // Move to the next state if "payload_length" bytes are received or the buffer is filled.
        // Note that in the second case, the whole message is unlikely to be valid.
        bytes_received = 0;
        isr_state = CHECKSUM;
      }
	  break;
    case (CHECKSUM):
      if (bytes_received == 0) {
        received_buffer.checksum = (u16)latest_data;
        bytes_received = 1;
      }
      else {
        received_buffer.checksum += ((u16)latest_data << 8);
        received_buffer.message_valid = 1;
        bytes_received = 0;
        isr_state = PREAMBLE;
      }
	  break;
  }
}

void bootloaderInit(void) {
  // Configure the UART Controller.
  // Set the baud rate to 250000 baud.
  UBRRH = DMX_BAUD_DIVIDER_HIGH;
  UBRRL = DMX_BAUD_DIVIDER_LOW;
  // Set the UART frame format to 8N2.
  UCSRC = (1 << USBS) | (1 << UCSZ0) | (1 << UCSZ1);
  // Enable the Rx Complete Interrupt, UART Receiver and UART Transmitter.
  UCSRB = (1 << RXCIE) | (1 << RXEN) | (1 << TXEN);

  // Clear the Page Buffer.
  pageBufferErase();
  // Set the Global Interrupt Flag.
  sei();
}

void bootloaderReset(void) {
  // Clear the Global Interrupt Flag.
  cli();

  // Return all of the registers to their reset state.
  UCSRB = 0x00;
  UCSRC = 0x06;
  UBRRH = 0x00;
  UBRRL = 0x00;
}

void processBuffer(void) {
  u16 page_address;
  u16 *pointer;
  u16 checksum;

  cli();
  if (received_buffer.message_valid == 0) {
	sei();
    return;
  }
  if (received_buffer.page_number < APPLICATION_START_PAGE) {
    // Ignore pages lower than the application start page to protect the bootloader code.
    //
    received_buffer.message_valid = 0;
	sei();
    return;
  }

  // Verify the checksum. This is performed over the page number, payload_length and
  // the data bytes.
  checksum = 0x0000;
  checksum = _crc_xmodem_update(checksum, received_buffer.page_number);
  checksum = _crc_xmodem_update(checksum, received_buffer.payload_length);
  for (u8 i = 0; i <= received_buffer.payload_length - 1; i++) {
    checksum = _crc_xmodem_update(checksum, received_buffer.data[i]);
  }
  if (checksum != received_buffer.checksum) {
    // Invalid message. Discard.
    received_buffer.message_valid = 0;
	sei();
    return;
  }

  if (received_buffer.payload_length == 0) {
    // A zero length payload indicates the end of the file.
    received_buffer.message_valid = 0;
    boot_application_flag = 1;
	sei();
    return;
  }

  // If we've reached this stage, the message is valid and has page data.
  pointer = (u16*)received_buffer.data;
  for (u8 i=0;i < received_buffer.payload_length; i+=2) {
	  pageBufferWrite(*pointer, i);
	  pointer++;
  }
  page_address = received_buffer.page_number << 6;
  pageWrite(page_address);
  // Now that the data has been written to memory, mark the message as no longer valid.
  received_buffer.message_valid = 0x00;
  sei();
}

int main(void) {
  bootloaderInit();
  while (boot_application_flag == 0) {
    processBuffer();
  }
  // Make sure the last buffer has been processed.
  processBuffer();
  bootloaderReset();
  
  jumpToApplication(APPLICATION_BOOT_ADDRESS);

  // We should never reach here.
  return 0;
}
