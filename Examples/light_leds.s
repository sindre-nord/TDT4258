.global _start
//Simple script to light up the 5 leftmost LEDS on the board
_start:
    ldr r0, =0xFF200000 //base address of the 10 bit led Data register
    bl reset_leds //reset the leds to 0
    bl five_left_most //light up the 5 leftmost leds
    bl reset_leds //reset the leds to 0
    bl five_right_most //light up the 5 rightmost leds
    bl reset_leds //reset the leds to 0
    bl every_other //light up every other led
	b _exit //exit the program by halting
	
five_left_most:
    ldr r1, =0x3E0
    str r1, [r0]
    bx lr

five_right_most:
    ldr r1, =0x1F
    str r1, [r0]
    bx lr

every_other:
    ldr r1, =0x155
    str r1, [r0]
    bx lr

//every_other_with_bits:
//    ldr r1, =b'1010101010'
//    str r1, [r0]
//    bx lr

reset_leds:
    ldr r1, =0x00
    str r1, [r0]
    bx lr

_exit:
	b .
.end