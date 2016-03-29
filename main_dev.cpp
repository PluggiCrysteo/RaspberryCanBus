#include "CanUtil.h"
#include <wiringPi.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <string.h>

/** the maximum size for a can_fifo_line is 44 char:
 * stdId < 2024 	=> 4 char
 * exId < 262144	=> 6 char
 * 8 bytes (data)	=> 3*8 = 24 char
 * ';' separator * 9	=> 9 char
 * '\n' separator	=> 1 char
 */
#define CAN_FIFO_MSG 44

#ifdef DEBUG
#include <iostream>
#define DEBUG_(x, ...) fprintf(stderr,"\033[31m%s/%s/%d: " x "\033[0m\n",__FILE__,__func__,__LINE__,##__VA_ARGS__);
#else
#define DEBUG_(x, ...)
#endif

void init_can(CanUtil,MCP2510);
void send_slave_ping(CanUtil);
void canCallback();
int readline(int,char*,int);

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
pthread_mutex_t spilock = PTHREAD_MUTEX_INITIALIZER;
int rcv_fd, snd_fd;


int main(int argc, char** argv) {

	DEBUG_("Setting up CAN handler process.\n");

	if(argc < 3) 
		error(1,errno,"Not enough arguments for CAN handler process. Aborting.");

	DEBUG_("argv[1]: %s\nargv[2]: %s\n",argv[1],argv[2]);

	if((rcv_fd = open(argv[1],O_WRONLY)) ==-1)
		error(1,errno,"Couldn't open the receive-FIFO");
	DEBUG_("Opened receive-FIFO\n");
	if((snd_fd = open(argv[2],O_RDONLY)) == -1)
		error(1,errno,"Couldn't open the send-FIFO");


	DEBUG_("Finished opening file descriptors, starting init_can()\n");
	init_can(canutil,can_dev);

	DEBUG_("CAN init finished.\n");

	if(wiringPiISR(6,INT_EDGE_FALLING, canCallback) < 0) {
		printf("Error binding\n");
		return 1;
	}
	//		error(1,errno,"Error binding interrupt.");
	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	uint8_t data_to_send[8];
	char snd_buf[CAN_FIFO_MSG];
	memset(snd_buf,0,CAN_FIFO_MSG);

	DEBUG_("blinking led !");
	canutil.flashRxbf();
	DEBUG_("Finished config, about to loop.\n");
	while( readline(snd_fd,snd_buf,CAN_FIFO_MSG) != -1 ) {
		// TODO parse the line to get stdid/extid/data
	}

	error(1,errno,"Error reading the send-FIFO.");
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
	//canutil.setAcceptanceMask(0x000, 0x00000, 0); // 0 <= stdID <= 2047, 0 <= extID <= 262143, buffer# 0

	canutil.setOpMode(0); // sets normal mode
	//  opmode = canutil.whichOpMode();

	//	canutil.setTxBufferDataLength(0, 1, 0); // TX normal data, 1 byte long, with buffer 0

}

void canCallback() {
	DEBUG_("Callback called !\n");
	if(pthread_mutex_lock(&spilock) == -1)
		perror("Error lock SPI mutex: ");


	uint8_t canDataReceived[8];

	uint8_t recSize = canutil.whichRxDataLength(0); 
	for (uint8_t i = 0; i < recSize; i++) { // gets the bytes
		canDataReceived[i] = canutil.receivedDataValue(0, i);
		DEBUG_("Data number %d: %c\n",i,canDataReceived[i]);
	}

	for( uint8_t i = recSize; i < 8; i++) {
		canDataReceived[i] = 0;
	}

	uint16_t stdId = canutil.whichStdID(0);
	uint32_t extId = canutil.whichExtdID(0);

	can_dev.write(CANINTF, 0x00);  // Clears all interrupts flags

	if(pthread_mutex_unlock(&spilock) == -1)
		perror("Error unlocking SPI mutex: ");

	char rcv_buf[CAN_FIFO_MSG];
	memset(rcv_buf,0,CAN_FIFO_MSG);
	DEBUG_("About to send received data into receive-FIFO.\n");
	snprintf(rcv_buf,CAN_FIFO_MSG,"%hu;%u;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu;%hhu\n",
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

	if(write(rcv_fd,rcv_buf,strlen(rcv_buf)) == -1)
		error(1,errno,"Error writing to the receive-FIFO");
}

int readline(int fd, char *buf, int nbytes) {
	int numread = 0;
	int returnval;
	while (numread < nbytes - 1) {
		returnval = read(fd, buf + numread, 1);
		/* If the read() syscall was interrupted by a signal */
		if ((returnval == -1) && (errno == EINTR))
			continue;
		/* A value of returnval equal to 0 indicates end of file */
		if ( (returnval == 0) && (numread == 0) )
			return 0;
		if (returnval == 0)
			break;
		if (returnval == -1)
			return -1;
		numread++;
		if (buf[numread-1] == '\n') {
			/* make this buffer a null-termianted string */
			buf[numread] = '\0';
			return numread;
		}
	}
	/* set errno to "invalid argument" */
	errno = EINVAL;
	return -1;
}
