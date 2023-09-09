.global _start
_start:
	ldr r4, =msg

LOOP:
    LDRB R0, [R4]
    CMP R0, #0
    BEQ CONT // string is null-terminated
    BL PUT_JTAG // send the character in R0 to UART
    ADD R4, R4, #1
    B LOOP
CONT:
    B _exit

PUT_JTAG:
    LDR R1, =0xFF201000 // JTAG UART base address
    STR R0, [R1] // send the character
END_PUT:
    BX LR

_exit:
    b .

msg: .asciz "Hello World\n>"
.end