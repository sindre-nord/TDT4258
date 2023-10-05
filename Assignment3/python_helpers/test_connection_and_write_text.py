# Simple script to display "Hello World" on
# the sense hat

from sense_hat import SenseHat
from time import sleep

sense = SenseHat()

sense.show_message("Hello World")
sleep(1)
sense.show_letter("!")
sleep(1)
sense.clear()

