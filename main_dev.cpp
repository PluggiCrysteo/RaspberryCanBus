#include "CanUtil.h"
#include "debug.h"
#include "tools.h"

#include <wiringPi.h>

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <error.h>

#include <string>
#include <vector>

/** the maximum size for a can_fifo_rcv_bufsize is 44 char:
 * stdId < 2024 	=> 4 char
 * exId < 262144	=> 6 char
 * 8 bytes (data)	=> 3*8 = 24 char
 * ';' separator * 9	=> 9 char
 * '\n' separator	=> 1 char
 */
#define CAN_FIFO_RCV_BUFSIZE 44
/** when it comes to sending data
 * we can remove the stdid (fixed here)
 * and the delimiter
 * which means 5 less char
 */
#define CAN_FIFO_SND_BUFSIZE 39
#define RPI_ID 0

void init_can(CanUtil,MCP2510);
void canCallback();
void send_can_msg(char*,CanUtil); 

/**
 *	INTERRUPT PIN : 6 VIA WIRINGPI
 *	25 BCM
 *	22 PHYSICAL
 **/

MCP2510 can_dev(0);
CanUtil canutil(can_dev);
pthread_mutex_t spilock = PTHREAD_MUTEX_INITIALIZER;
int cfd;


int main(int argc, char** argv) {

	DEBUG_("Setting up CAN handler process.\n");

	if(argc < 2) 
		error(1,errno,"Not enough arguments for CAN handler process. Aborting.");

	DEBUG_("argv[1]: %s\n",argv[1]);

	int sfd = get_unix_socket_fd(argv[1]);

	DEBUG_("Polling for accept");

	cfd = accept(sfd, NULL, NULL);

	DEBUG_("Received connection on UNIX socket, starting init_can()\n");
	init_can(canutil,can_dev);

	DEBUG_("CAN init finished.\n");

	if(wiringPiISR(6,INT_EDGE_FALLING, canCallback) < 0) {
		printf("Error binding\n");
		return 1;
	}
	//		error(1,errno,"Error binding interrupt.");
	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	char snd_buf[CAN_FIFO_SND_BUFSIZE];
	memset(snd_buf,0,CAN_FIFO_SND_BUFSIZE);

	DEBUG_("blinking led !");
	canutil.flashRxbf();
	DEBUG_("Finished config, about to loop.\n");
	while( readline(cfd,snd_buf,CAN_FIFO_SND_BUFSIZE) != -1 ) {
		DEBUG_("read a line from the can_client");
		send_can_msg(snd_buf,canutil);
	}

	error(1,errno,"Error reading the client socket.");
}

void init_can(CanUtil canutil,MCP2510 can_dev) {
	can_dev.write(CANINTE, 0x01); //disables all interrupts but RX0IE (received message in RX buffer 0)
	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	canutil.setClkoutMode(0, 0); // disables CLKOUT
	canutil.setTxnrtsPinMode(0, 0, 0); // all TXnRTS pins as all-purpose digital input

	canutil.setOpMode(4); // sets configuration mode
	// IMPORTANT NOTE: configuration mode is the ONLY mode where bit timing registers (CNF1, CNF2, CNF3), acceptance
	// filters and acceptance masks can be modified

	DEBUG_("Starting to wait for opmode\n");
	canutil.waitOpMode(4);  // waits configuration mode
	DEBUG_("Opmode received.\n");

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
	canutil.setRxOperatingMode(2, 1, 0);  // ext ID messages only  and rollover
	canutil.setAcceptanceFilter(0x000, 0x20000, 1, 0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, 1 = extended, filter# 0
	canutil.setAcceptanceMask(0x000, 0x20000, 0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, buffer# 0

	canutil.setOpMode(0); // sets normal mode
}

void canCallback() {
	static uint8_t recSize;
	static char rcv_buf[CAN_FIFO_RCV_BUFSIZE];
	static uint16_t stdId;
	static uint32_t extId;
	static uint8_t canDataReceived[8];

	DEBUG_("Callback called !\n");
	if(pthread_mutex_lock(&spilock) == -1)
		perror("Error lock SPI mutex: ");


	recSize = canutil.whichRxDataLength(0); 

	for (uint8_t i = 0; i < recSize; i++) { // gets the bytes
		canDataReceived[i] = canutil.receivedDataValue(0, i);
		DEBUG_("Data number %d: %c\n",i,canDataReceived[i]);
	}

	for( uint8_t i = recSize; i < 8; i++) {
		canDataReceived[i] = 0;
	}

	stdId = canutil.whichStdID(0);
	extId = canutil.whichExtdID(0);

	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	if(pthread_mutex_unlock(&spilock) == -1)
		perror("Error unlocking SPI mutex: ");

	memset(rcv_buf,0,CAN_FIFO_RCV_BUFSIZE);
	DEBUG_("About to send received data to the client fd.\n");
	snprintf(rcv_buf,CAN_FIFO_RCV_BUFSIZE,"%hu;%u;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu\n",
			stdId,
			extId,
			canDataReceived[0],
			canDataReceived[1],
			canDataReceived[2],
			canDataReceived[3],
			canDataReceived[4],
			canDataReceived[5],
			canDataReceived[6],
			canDataReceived[7]);

	if(write(cfd,rcv_buf,strlen(rcv_buf)) == -1)
		error(1,errno,"Error writing to the client_can_fd");
}



void send_can_msg(char* buf, CanUtil canutil) {
	static uint32_t extid;
	int16_t txstatus;
	uint8_t message[8];
	static uint8_t counter;
	std::string tosplit(buf);

	DEBUG_("read the line: %s",buf);

	std::vector<std::string> splitted = split(tosplit,';');
	try {
		extid = std::stoi(splitted[0]);
		DEBUG_("sending with extid of: %u",extid);

		for(size_t i=1; i < splitted.size();i++) {
			message[i-1] = (uint8_t)std::stoi(splitted[i]);
		}
	} catch(int e) {
		std::cout << "An error occured when sending data to the CAN bus.\
			Is your data properly formatted ?" << std::endl;
		return;
	}


	counter = 10;

	canutil.setTxBufferID(RPI_ID, extid , 1, 0); // sets the message ID, specifies standard message (i.e. short ID) with buffer 0
	canutil.setTxBufferDataField(message, 0);   // fills TX buffer
	canutil.setTxBufferDataLength(0, splitted.size()-1, 0);
	//      canutil.setTxBufferDataField(message, 0);   // fills TX buffer
	canutil.messageTransmitRequest(0, 1, 3);
	do {
		txstatus = 0;
		txstatus = canutil.isTxError(0);  // checks tx error
		txstatus = txstatus + canutil.isArbitrationLoss(0);  // checks for arbitration loss
		txstatus = txstatus + canutil.isMessageAborted(0);  // ckecks for message abort
		txstatus = txstatus + canutil.isMessagePending(0);   // checks transmission
		counter--;
	}
	while (txstatus != 0 && counter > 0);
}
