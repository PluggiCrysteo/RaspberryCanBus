#include "CanUtil.h"
#include <wiringPi.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>

void init_can(CanUtil,MCP2510);
void send_slave_ping(CanUtil);
void canCallback();

/**
 *	INTERRUPT PIN : 6 VIA WIRINGPI
 *	25 BCM
 *	22 PHYSICAL
 **/

	//wiringPiSetup();
//extern MCP2510 can_dev;
//extern CanUtil canutil;
	MCP2510 can_dev(0);
	CanUtil canutil(can_dev);


int main() {
	//wiringPiSetup();
	MCP2510 can_dev(0);
	CanUtil canutil(can_dev);

	std::cout << "starting init_can()" << std::endl ;
	init_can(canutil,can_dev);

	/******************** end init, try sending shit**************/
	std::cout << "finished init_can(), starting send_slave_ping()" << std::endl;
	if(wiringPiISR(6,INT_EDGE_FALLING, canCallback) < 0)
		perror("error setting up interrupt :( :");
	send_slave_ping(canutil);
	pause();
}

void init_can(CanUtil canutil,MCP2510 can_dev) {
	can_dev.write(CANINTE, 0x01); //disables all interrupts but RX0IE (received message in RX buffer 0)
	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	canutil.setClkoutMode(0, 0); // disables CLKOUT
	canutil.setTxnrtsPinMode(0, 0, 0); // all TXnRTS pins as all-purpose digital input

	canutil.setOpMode(4); // sets configuration mode
	// IMPORTANT NOTE: configuration mode is the ONLY mode where bit timing registers (CNF1, CNF2, CNF3), acceptance
	// filters and acceptance masks can be modified

	std::cout << "starting to wait for opmode" <<std::endl;
	canutil.waitOpMode(4);  // waits configuration mode
	std::cout << "opmode received" <<std::endl;

	// Bit timing section
	//  setting the bit timing registers with Fosc = 16MHz -> Tosc = 62,5ns
	// data transfer = 125kHz -> bit time = 8us, we choose arbitrarily 8us = 16 TQ  (8 TQ <= bit time <= 25 TQ)
	// time quanta TQ = 2(BRP + 1) Tosc, so BRP =3
	// sync_seg = 1 TQ, we choose prop_seg = 2 TQ
	// Phase_seg1 = 7TQ yields a sampling point at 10 TQ (60% of bit length, recommended value)
	// phase_seg2 = 6 TQ SJSW <=4 TQ, SJSW = 1 TQ chosen
	can_dev.write(CNF1, 0x03); // SJW = 1, BRP = 3
	can_dev.write(CNF2, 0b10110001); //BLTMODE = 1, SAM = 0, PHSEG = 6, PRSEG = 1
	can_dev.write(CNF3, 0x05);  // WAKFIL = 0, PHSEG2 = 5

	// SETUP MASKS / FILTERS FOR CAN
	canutil.setRxOperatingMode(1, 1, 0);  // standard ID messages only  and rollover
	canutil.setAcceptanceFilter(0x102, 0, 0, 1); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 0
	canutil.setAcceptanceFilter(0x101, 0, 0, 5); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 0
	canutil.setAcceptanceFilter(0x100, 0, 0, 0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 1
	canutil.setAcceptanceMask(0x000, 0x00000000, 0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, buffer# 0

	canutil.setOpMode(0); // sets normal mode
	//  opmode = canutil.whichOpMode();

	canutil.setTxBufferDataLength(0, 1, 0); // TX normal data, 1 byte long, with buffer 0

}

void send_slave_ping(CanUtil canutil) {
	uint16_t message_id = 0x100;
	canutil.setTxBufferID(message_id, 0, 0, 0); // sets the message ID, specifies standard message (i.e. short ID) with buffer 0
	uint8_t message[8];
	message[0] = 0x01;

	canutil.setTxBufferDataField(message, 0);   // fills TX buffer
	canutil.messageTransmitRequest(0, 1, 3); // requests transmission of buffer 0 with highest priority*/
}

void canCallback() {
	std::cout << "Callback called !" << std::endl;

	//CanUtil canutil;
	int canDataReceived[8];

	uint8_t recSize = canutil.whichRxDataLength(0); // checks the number of bytes received in buffer 0 (max = 8)
	for (int i = 0; i < recSize; i++) { // gets the bytes
		canDataReceived[i] = canutil.receivedDataValue(0, i);
		std::cout << "Data number " << i << ":" << (int)canDataReceived[i] << std::endl;

	}
	uint16_t msgID = canutil.whichStdID(0);
	std::cout << "Message ID: " << msgID << std::endl;
	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags
}
