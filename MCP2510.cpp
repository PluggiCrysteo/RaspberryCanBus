// B.Stefanelli le 30 mai 2010


#include "MCP2510.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <wiringPiSPI.h>
#include <stdlib.h>
#include <wiringPi.h>


//==================================================================================
// MCP2510
//==================================================================================

/*---------------------------------------------------------------------------------
  Set up MCP2510 chip select pin
csPin : chip select pin
---------------------------------------------------------------------------------*/

MCP2510::MCP2510(uint8_t csPin):_csPin(csPin){

	//  system("gpio load spi");
	wiringPiSetup();
	wiringPiSPISetup(_csPin,SPI_CLOCK_SPEED);

}

/*---------------------------------------------------------------------------------
  Read data from MCP2510 register
reg : MCP2510 register
return : data value
---------------------------------------------------------------------------------*/
uint8_t MCP2510::read(uint8_t reg) {

	// select register
	uint8_t sentData1[3] = {0x03,reg,0xFF};
	wiringPiSPIDataRW(_csPin,sentData1,3);
	// Disable slave
	return sentData1[2];
}

/*---------------------------------------------------------------------------------
  Write data to MCP2510 register
reg : MCP2510 register
val : data value
---------------------------------------------------------------------------------*/
void MCP2510::write(uint8_t reg, uint8_t val ) {

	uint8_t data[3] = {0x02,reg,val};
	wiringPiSPIDataRW(_csPin,data,3);
	// Disable slave pin CS
}


/*---------------------------------------------------------------------------------
  send requestToSend to  MCP2510 

  ---------------------------------------------------------------------------------*/
void MCP2510::requestToSend(uint8_t buffer) {
	// send command
	uint8_t data[1] = { 0x80 | buffer };
	wiringPiSPIDataRW(_csPin,data,1);

}

/*---------------------------------------------------------------------------------
  send bitModify to  MCP2510 

  ---------------------------------------------------------------------------------*/
void MCP2510::bitModify(uint8_t reg, uint8_t mask, uint8_t data) {
	// Disable slave pin CS
	uint8_t dataArr[4] = { 0x05, reg, mask, data};
	wiringPiSPIDataRW(_csPin,dataArr,4);
}

/*---------------------------------------------------------------------------------
  send readStatus to  MCP2510 

  ---------------------------------------------------------------------------------*/
uint8_t MCP2510::readStatus() {

	uint8_t order[1] = { 0xa0 };  

	uint8_t receivedData[3] = { 0xa0,0xFF, 0xFF }; 
	wiringPiSPIDataRW(_csPin,receivedData,3);
	// Disable slave
	return receivedData[2];
}
/*---------------------------------------------------------------------------------
  send reset to  MCP2510 

  ---------------------------------------------------------------------------------*/
void MCP2510::reset() {
	// Disable slave pin CS
	uint8_t resetMessage[1] = { 0xc0 };
	wiringPiSPIDataRW(_csPin,resetMessage,1);
}
