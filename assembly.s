;
; assembly.s
; Zane Barker
; 28/08/2015
; Assembly subroutines for supporting the DMX bootloader on the ATtiny4313.
;

; GLOBAL SUBROUTINES
.global pageBufferWrite
.global pageBufferErase
.global pageWrite
.global pageErase
.global jumpToApplication

; MACRO DEFINITIONS
; Copy a 16-bit integer into the Z Register.
.macro WriteZReg source_reg
  movw r30, \source_reg
.endm

; Perform an operation related to writing data to or from the page buffer.
.macro PageOp op_code
  ldi r25, \op_code
  out SPMCSR, r25
  spm
.endm

; PAGEOP DEFINITIONS
.equiv SPMCSR, 0x37
.equiv PAGE_BUFF_WRITE, 0x01
.equiv PAGE_BUFF_ERASE, 0x11
.equiv PAGE_ERASE, 0x03
.equiv PAGE_WRITE, 0x05

pageBufferWrite:
  ; pageBufferWrite(data, address)
  ; Copy the data word into R0:R1.
  movw r0, r24
  ; Copy the address word into the Z register.
  WriteZReg r22
  ; Write the page buffer to the data memory.
  PageOp PAGE_BUFF_WRITE
  ; Clear R1 so that C is happy.
  clr r1
  ret

pageBufferErase:
  ; pageBufferErase()
  PageOp PAGE_BUFF_ERASE
  ret

pageWrite:
  ; pageWrite(pageAddress)
  ; Copy the page address to the Z register.
  WriteZReg r24
  ; The page must be erased before we can write to it.
  PageOp PAGE_ERASE
  ; Begin the Page Write operation.
  PageOp PAGE_WRITE
  ; Clear the page buffer.
  ; Just clear the buffer directly. Don't bother with calling the subroutine.
  PageOp PAGE_BUFF_ERASE
  ret

pageErase:
  ; pageErase(pageAddress)
  ; Copy the page address to the Z register.
  WriteZReg r24
  PageOp PAGE_ERASE
  ret

jumpToApplication:
  writeZReg r24
  ijmp
