// B.Stefanelli le 26 Oct 2011


#include "CanUtil.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "MCP2510.h"


//==================================================================================
// CanUtil
//==================================================================================
//******************************************************************
//                     Constructor
//******************************************************************
CanUtil::CanUtil(MCP2510&can):_can(can) { 
}

//******************************************************************
//                     MODE selection routines
//******************************************************************

//***************************************************************
// sets operating mode
// argument: opMode 
// 0: normal, 1: sleep, 2: loopback, 3: listen only, 4: configuration
// returns: nothing
//****************************************************************
void CanUtil::setOpMode(uint8_t opMode){
  uint8_t canctrl;
  canctrl = _can.read(CANCTRL);
  canctrl = canctrl & 0b00011111;
  canctrl = canctrl | opMode << 5;
  _can.write(CANCTRL, canctrl);
}

//***************************************************************
// requests operating mode
// argument: none 
// 
// returns: operating mode number
// 0: normal, 1: sleep, 2: loopback, 3: listen only, 4: configuration
//****************************************************************
uint8_t CanUtil::whichOpMode(){
  uint8_t canstat;
  canstat = _can.read(CANSTAT);
  canstat = canstat >> 5;
  return canstat;
}

//***************************************************************
// waits until the requested operating mode is set
// argument: opMode 
// 0: normal, 1: sleep, 2: loopback, 3: listen only, 4: configuration
// returns: nothing
//****************************************************************
void CanUtil::waitOpMode(uint8_t reqMode){
  uint8_t canstat;
  do{ 
    canstat = _can.read(CANSTAT);
    canstat = canstat >> 5;
  } 
  while (canstat != reqMode);
}

//***************************************************************
// sets CLKOUT pin mode
// argument: enable, prescaler 
// enable = (0: CLKOUT pin disabled, 1: CLKOUT pin enabled)
// prescaler = (0: fout = sysclock, 1: fout = sysclock/2, 2: fout = sysclock/4, 3: fout = sysclock/8)
// returns: nothing
//****************************************************************
void CanUtil::setClkoutMode(uint8_t enable, uint8_t prescaler){
  uint8_t canctrl;
  canctrl = _can.read(CANCTRL);
  canctrl = canctrl & 0b11111000;
  canctrl = canctrl | (enable << 2) | prescaler;
  _can.write(CANCTRL, canctrl);
}


//******************************************************************
//                     RX routines
//******************************************************************

//***************************************************************
// sets RX operating mode
// argument: RXmode, rollover, RX buffer number 
// RXmode:( MASKOFF=3, EXTONLY=2,STDONLY=1,ALLVALID=0) , rollover = (O: disable, 1:enable), 0 <= RX buffer number <= 1
// returns: nothing
//****************************************************************
void CanUtil::setRxOperatingMode(uint8_t RRXmode, uint8_t rollover, uint8_t buffer){
  uint8_t temp,rxmode;

  switch (RRXmode){
  case 3: 
    rxmode=0x60;
    break;
  case 2:
    rxmode=0x40;
    break;
  case 1:
    rxmode=0x20;
    break;
  case 0:
    rxmode=0x00;
    break;
  default:
    rxmode=0x60;  // default value if RXmode does not match
  }


  if (rollover == 0){
    rollover = 0x00;
  }
  else{
    rollover = 0x04;
  }

  if (buffer == 0) {
    temp = _can.read(RXB0CTRL) & 0b10011011;  // resets RX mode and rollover
    temp = temp | rxmode | rollover;
    //Serial.println(rxmode,BIN);
    _can.write(RXB0CTRL, temp);  //sets new RX mode and rollover
  }
  else {
    temp = _can.read(RXB1CTRL) & 0b10011111;  // resets RX mode
    temp = temp | rxmode;
    _can.write(RXB1CTRL, temp);  //sets new RX mode
  }

}

//***************************************************************
// sets RX acceptance filter
// argument: standard ID, extended ID, extended flag, filter number 
// 0 <= stdID <= 2047, 0 <= extID <= 262143, extended =  1 if filter applied to extended frames, 0 <= filter number <= 5
// returns: nothing
//****************************************************************
void CanUtil::setAcceptanceFilter(uint16_t stdID, uint32_t extID, uint8_t extended, uint8_t filter){
  int8_t sidh, sidl,eid8, eid0;
  sidh = (int8_t) stdID >>3;
  extended = extended <<3 & 0x08;
  sidl = (int8_t) stdID << 5 & 0b11100000;
  sidl = sidl | extended;
  sidl  = sidl | (int8_t)extID >> 16;
  eid0 = (int8_t)extID;
  eid8 = (int8_t)extID >>8;
  switch (filter) {
  case 0:
    _can.write(RXF0SIDH, sidh);
    _can.write(RXF0SIDL, sidl);
    _can.write(RXF0EID8, eid8);
    _can.write(RXF0EID0, eid0);
    break;
  case 1:
    _can.write(RXF1SIDH, sidh);
    _can.write(RXF1SIDL, sidl);
    _can.write(RXF1EID8, eid8);
    _can.write(RXF1EID0, eid0);
    break;
  case 2:
    _can.write(RXF2SIDH, sidh);
    _can.write(RXF2SIDL, sidl);
    _can.write(RXF2EID8, eid8);
    _can.write(RXF2EID0, eid0);
    break;
  case 3:
    _can.write(RXF3SIDH, sidh);
    _can.write(RXF3SIDL, sidl);
    _can.write(RXF3EID8, eid8);
    _can.write(RXF3EID0, eid0);
    break;
  case 4:
    _can.write(RXF4SIDH, sidh);
    _can.write(RXF4SIDL, sidl);
    _can.write(RXF4EID8, eid8);
    _can.write(RXF4EID0, eid0);
    break;
  case 5:
    _can.write(RXF5SIDH, sidh);
    _can.write(RXF5SIDL, sidl);
    _can.write(RXF5EID8, eid8);
    _can.write(RXF5EID0, eid0);
    break;   
  }
}

//***************************************************************
// sets RX acceptance mask
// argument: standard ID, extended ID, buffer number 
// 0 <= stdID <= 2047, 0 <= extID <= 262143, 0 <= buffer number <= 1
// returns: nothing
//****************************************************************
void CanUtil::setAcceptanceMask(uint16_t stdID, uint32_t extID, uint8_t buffer){
  int8_t sidh, sidl,eid8, eid0;
  sidh = stdID >>3;
  sidl = stdID << 5 & 0b11100000;
  sidl  = sidl | (extID >> 16);
  eid0 = extID;
  eid8 = extID >>8;
  switch (buffer) {
  case 0:
    _can.write(RXM0SIDH, sidh);
    _can.write(RXM0SIDL, sidl);
    _can.write(RXM0EID8, eid8);
    _can.write(RXM0EID0, eid0);
    break;
  case 1:
    _can.write(RXM1SIDH, sidh);
    _can.write(RXM1SIDL, sidl);
    _can.write(RXM1EID8, eid8);
    _can.write(RXM1EID0, eid0);
    break;
  }
}

//***************************************************************
// requests if a RTR frame was received
// argument: buffer number 
//  0 <= buffer number <= 1
// returns: frame type
// frame type = (0: data frame received, 1: RTR received)
//****************************************************************
uint8_t  CanUtil::isRtrFrame(uint8_t buffer){
  uint8_t type = 0;
  switch (buffer) {
  case 0:
    type = _can.read(RXB0SIDL) >> 4;
    break;
  case 1:
    type = _can.read(RXB1SIDL) >> 4;
    break;
  }
  return type;
}

//***************************************************************
// requests if an extended frame was received
// argument: buffer number 
//  0 <= buffer number <= 1
// returns: frame type
// frame type = (0: std frame received, 1: extended received)
//****************************************************************
uint8_t CanUtil::isExtendedFrame(uint8_t buffer){
  uint8_t  type = 0;
  switch (buffer) {
  case 0:
    type = _can.read(RXB0SIDL) >> 3;
    break;
  case 1:
    type = _can.read(RXB1SIDL) >> 3;
    break;
  }
  return type;
}

//***************************************************************
// requests RX buffer standard ID
// argument: buffer number 
//  0 <= buffer number <= 1
// returns: standard ID
//****************************************************************
uint16_t  CanUtil::whichStdID(uint8_t buffer){
  uint8_t  stdid = 0;
  switch (buffer) {
  case 0:
    stdid = _can.read(RXB0SIDH) << 3;
    stdid = stdid | (_can.read(RXB0SIDL) >> 5);
    break;
  case 1:
    stdid = _can.read(RXB1SIDH) << 3;
    stdid = stdid | (_can.read(RXB1SIDL) >> 5);
    break;
  }
  return stdid;
}


//***************************************************************
// requests RX buffer extended ID
// argument: buffer number 
//  0 <= buffer number <= 1
// returns: extended ID
//****************************************************************
uint32_t CanUtil::whichExtdID(uint8_t buffer){
  uint32_t extid = 0;
  switch (buffer) {
  case 0:
    extid = (_can.read(RXB0SIDL) & 0x03) << 16;
    extid = extid | (_can.read(RXB0EID8) << 8);
    extid = extid | _can.read(RXB0EID0);
    break;
  case 1:
    extid = (_can.read(RXB1SIDL) & 0x03) << 16;
    extid = extid | (_can.read(RXB1EID8) << 8);
    extid = extid | _can.read(RXB1EID0);
    break;
  }

  return extid;
}

//***************************************************************
// requests RX buffer data length
// argument: buffer number 
//  0 <= buffer number <= 1
// returns: number of received bytes in buffer
//****************************************************************
uint8_t CanUtil::whichRxDataLength(uint8_t buffer){
  uint8_t length;
  if (buffer == 0){
    length = _can.read(RXB0DLC) & 0x0F;
  }
  else{
    length = _can.read(RXB1DLC) & 0x0F;
  }
  return length;
}  


//***************************************************************
// requests RX buffer data byte
// argument: buffer number, byte number 
//  0 <= buffer number <= 1, 0 <= byte number <= 1
// returns: requested byte value
//****************************************************************
uint8_t CanUtil::receivedDataValue(uint8_t buffer, uint8_t byteNum){
  uint8_t value;

  if (buffer == 0){
    value = _can.read(0x66 + byteNum);
  }
  else{
    value = _can.read(0x76 + byteNum);
  }
  return value;
}  


//******************************************************************
//                     TX routines
//******************************************************************

//***************************************************************
// sets TXnRTS pins mode 
// argument: B2RTS pin mode, B1RTS pin mode, B0RTS pin mode
// pin mode= (0: all-purpose digital input, 1: hardware tramsmit request)
// returns: nothing
//****************************************************************
void CanUtil::setTxnrtsPinMode(uint8_t b2rtsm, uint8_t b1rtsm, uint8_t b0rtsm){
  uint8_t temp;

  b2rtsm = b2rtsm << 2;
  b1rtsm = b1rtsm << 1;


  temp = _can.read(TXRTSCTRL);
  temp = temp & 0b11111000;
  temp = temp | b2rtsm |b1rtsm |b0rtsm;
  _can.write(TXRTSCTRL, temp);
}


//***************************************************************
// sets TX buffer ID
// argument: standard ID, extended ID, extended flag, buffer number 
// 0 <= stdID <= 2047, 0 <= extID <= 262143, extended =  1 if filter transmission of extended identifiers, 0 <= buffer number <= 2
// returns: nothing
//****************************************************************
void CanUtil::setTxBufferID(uint16_t stdID, uint32_t extID, uint8_t extended, uint8_t buffer){
  int8_t sidh, sidl,eid8, eid0;
  sidh = stdID >>3;
  extended = extended <<3 & 0x08;
  sidl = stdID << 5 & 0b11100000;
  sidl = sidl | extended;
  sidl  = sidl | extID >> 16;
  eid0 = extID;
  eid8 = extID >>8;
  switch (buffer) {
  case 0:
    _can.write(TXB0SIDH, sidh);
    _can.write(TXB0SIDL, sidl);
    _can.write(TXB0EID8, eid8);
    _can.write(TXB0EID0, eid0);
    break;
  case 1:
    _can.write(TXB1SIDH, sidh);
    _can.write(TXB1SIDL, sidl);
    _can.write(TXB1EID8, eid8);
    _can.write(TXB1EID0, eid0);
    break;
  case 2:
    _can.write(TXB2SIDH, sidh);
    _can.write(TXB2SIDL, sidl);
    _can.write(TXB2EID8, eid8);
    _can.write(TXB2EID0, eid0);
    break;

  }
}

//***************************************************************
// sets TX buffer data length
// argument: rtr, data length, buffer number 
// rtr =  (0: data frame, 1: remote transmit request), 0 <= data length <=8, 0 <= buffer number <= 2
// returns: nothing
//****************************************************************
void CanUtil::setTxBufferDataLength(uint8_t rtr,  uint8_t length, uint8_t buffer){
  int8_t txbdlc = 0;
  txbdlc = rtr << 6 | (length & 0x0F);

  switch (buffer) {
  case 0:
    _can.write(TXB0DLC, txbdlc);
    break;
  case 1:
    _can.write(TXB1DLC, txbdlc);
    break;
  case 2:
    _can.write(TXB2DLC, txbdlc);
    break;

  }
}
//***************************************************************
// sets TX buffer data field
// argument:data array, buffer number 
// data array: an array of exactly 8 bytes , 0 <= buffer number <= 2
// returns: nothing
//****************************************************************
void CanUtil::setTxBufferDataField(uint8_t data[8], uint8_t buffer){

  switch (buffer) {
  case 0:
    for (int i=0; i<8; i++){
      _can.write(0x36+i, data[i]);
    }
    break;
  case 1:
    for (int i=0; i<8; i++){
      _can.write(0x46+i, data[i]);
    }
    break;
  case 2:
    for (int i=0; i<8; i++){
      _can.write(0x56+i, data[i]);
    }
    break;

  }  
}

//***************************************************************
// requests transmission of a message
// argument: buffer number, transmit request, priority 
// 0 <= buffer number <= 2, transmit request = (0: abort, 1:request), priority = (O: lowest, 3: highest)
// returns: nothing
//****************************************************************
void CanUtil::messageTransmitRequest(uint8_t txbuffer, uint8_t transmit, uint8_t priority){
  uint8_t val;
  val = priority | (transmit << 3);
  switch (txbuffer) {
  case 0:
    _can.write(TXB0CTRL, val);
    break;
  case 1:
    _can.write(TXB1CTRL, val);
    break;
  case 2:
    _can.write(TXB2CTRL, val);
    break;
  }
}

//***************************************************************
// asks if a message is pending transmission in a particular buffer
// argument:buffer number 
// 0 <= buffer number <= 2
// returns: message status
// message status = (0: message sent, 1: transmission pending)
//****************************************************************
uint8_t CanUtil::isMessagePending(uint8_t buffer){
  uint8_t txreq;

  switch (buffer) {
  case 0:
    txreq = _can.read(TXB0CTRL);
    break;
  case 1:
    txreq = _can.read(TXB1CTRL);
    break;
  case 2:
    txreq = _can.read(TXB2CTRL);
    break;
  }
  return txreq & 0b00001000;
}

//***************************************************************
// asks for TX error
// argument:buffer number 
// 0 <= buffer number <= 2
// returns: TX error
// TX error = (0: no bus error, 1: bus error)
//****************************************************************
uint8_t CanUtil::isTxError(uint8_t buffer){
  uint8_t txreq = 0;

  switch (buffer) {
  case 0:
    txreq = _can.read(TXB0CTRL) >> 4;
    break;
  case 1:
    txreq = _can.read(TXB1CTRL) >> 4;
    break;
  case 2:
    txreq = _can.read(TXB2CTRL) >> 4;
    break;
  }
  return txreq;
}

//***************************************************************
// asks for arbitration loss
// argument:buffer number 
// 0 <= buffer number <= 2
// returns: MLOA
// MLOA = (0: OK, 1: message lost arbitration)
//****************************************************************
uint8_t CanUtil::isArbitrationLoss(uint8_t buffer){
  uint8_t txreq = 0;

  switch (buffer) {
  case 0:
    txreq = _can.read(TXB0CTRL) >> 5;
    break;
  case 1:
    txreq = _can.read(TXB1CTRL) >> 5;
    break;
  case 2:
    txreq = _can.read(TXB2CTRL) >> 5;
    break;
  }
  return txreq;
}

//***************************************************************
// asks for message aborted
// argument:buffer number 
// 0 <= buffer number <= 2
// returns: ABTF
// ABTF = (0: OK, 1: message aborted)
//****************************************************************
uint8_t CanUtil::isMessageAborted(uint8_t buffer){
  uint8_t txreq = 0;

  switch (buffer) {
  case 0:
    txreq = _can.read(TXB0CTRL) >> 6;
    break;
  case 1:
    txreq = _can.read(TXB1CTRL) >> 6;
    break;
  case 2:
    txreq = _can.read(TXB2CTRL) >> 6;
    break;
  }
  return txreq;
}


//***************************************************************
// flashes the RXB leds for test purpose
// argument: none 
// 
// returns: nothing
//****************************************************************

void CanUtil::flashRxbf(){
  _can.write(BFPCTRL, 0x3C);
  sleep(1);
  _can.write(BFPCTRL, 0x0C);
  sleep(1);
  _can.write(BFPCTRL, 0x3C);
  sleep(1);
  _can.write(BFPCTRL, 0x0C);
  sleep(1);
  _can.write(BFPCTRL, 0x00);  //RXnBF pins high Z
}

