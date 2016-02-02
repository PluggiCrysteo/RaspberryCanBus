# CAN lib for RPi + MCP2510 (and MCP2515 most likely ?) lib

## How-to

1. Need wiringPi (SPI)
2. sudo access ( chmod u+s on the bainary is fine too)

## What's the main for ?

1. It reads the CAN bus (interrupt on pin 6/22/25 depending on your reference)
2. It sends the received data into a FIFO
3. It reads the data from a FIFO which needs to be sent through the CAN bus

## TODO

1. Replace FIFO with a socket (UNIX / IP ?)
2. Maybe accept asynch connection on the socket if there are multiple senders (not true yet)
