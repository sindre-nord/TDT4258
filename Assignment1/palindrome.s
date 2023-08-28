.global _start


// Please keep the _start method and the input strings name ("input") as
// specified below
// For the rest, you are free to add and remove functions as you like,
// just make sure your code is clear, concise and well documented.

_start:
	// Here your execution starts
	ldr r10, =0xFF200000 //Load the address of the LEDs

	ldr r0, =input
	ldr r8, =input
	mov r1, #0 //Set the input length counter to 0
	bl check_input
	bl check_palindrom
	//b _exit

	
check_input:
	// You could use this symbol to check for your input length
	// you can assume that your input string is at least 2 characters 
	// long and ends with a null byte
	
	//Check the first char
	ldrb r2, [r0] //Load one byte from adr r0
	cmp r2, #0  // Check if its the null
	beq string_length_counted //If null string then the string is completed
	add r1, r1, #1 //If there is another char then just count it and continue
	add r0, r0, #1 //Increment the address
	b check_input
	
string_length_counted:
	bl check_palindrom //If the string is completed then check if its a palindrom
	b _exit
	
	
check_palindrom:
	// Here you could check whether input is a palindrom or not
	// You can assume that your input string is at least 2 characters

	// A palindrome will always be mirrored at the middle
	// So we can check the first and last char, then the second and second last char and so on
	// If we find a mismatch then we know its not a palindrome
	// If we reach the middle then we know its a palindrome

	// The length of the string was stored in r1
	// The string is stored at the address in r0

	// The offset from the right can be stored in r3
	// The offset from the left can be stored in r4
	// The first char can be stored in r5
	// The last char can be stored in r6

	mov r3, r8 //Offset
	mov r4, r1 //Offset
	sub r4, r4, #1 //Subtract 1 from the offset to get the last char
	add r4, r4, r8	//Add the offset to the address to get the last char
	
loop:
	ldrb r5, [r3] //Load the first char
	ldrb r6, [r4] //Load the last char
	cmp r5, r6 //Compare the chars
	bne is_no_palindrom //If they are not equal then its not a palindrome
	add r3, r3, #1 //Increment the offset from the left
	sub r4, r4, #1 //Decrement the offset from the right
	cmp r3, r4 //Check if we have reached the middle
	bne loop //If not then continue
	
	b is_palindrom //If we have reached the middle then its a palindrome

is_palindrom:
	// Switch on only the 5 leftmost LEDs
	// Write 'Palindrom detected' to UART
	//mov r9, lr //Save the return address
	bl reset_leds //Reset the LEDs
	//mov lr, r9 //Restore the return address
	ldr r11, =0x3E0 //Equates to 0000011111 calculated with at binary to HEX converter
	str r11, [r10] //Write the value to the LEDs
	b _exit
	
	
is_no_palindrom:
	// Switch on only the 5 rightmost LEDs
	// Write 'Not a palindrom' to UART
	//mov r9, lr //Save the return address
	bl reset_leds //Reset the LEDs
	//mov lr, r9 //Restore the return address
	ldr r11, =0x1F
	str r11, [r10]
	b _exit//Return

reset_leds:
    ldr r7, =0x00
    str r7, [r10]
    bx lr

_exit:
	// Branch here for exit
	b .
	
.data
.align
	// This is the input you are supposed to check for a palindrom
	// You can modify the string during development, however you
	// are not allowed to change the name 'input'!
	//input: .asciz "Grav ned den varg"
	input: .asciz "heieh h"
.end