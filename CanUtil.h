#ifndef CanUtil_h
#define CanUtil_h

#include <inttypes.h>
#include "MCP2510.h"


class CanUtil {
private:
	MCP2510 &_can;
public:
	CanUtil(MCP2510 &can);
	void setOpMode(uint8_t);
	uint8_t whichOpMode();
	void waitOpMode(uint8_t);
	void setClkoutMode(uint8_t, uint8_t);
	void setRxOperatingMode(uint8_t, uint8_t, uint8_t);
	void setAcceptanceFilter(uint16_t , uint32_t, uint8_t, uint8_t);
	void setAcceptanceMask(uint16_t, uint32_t, uint8_t);
	uint8_t  isRtrFrame(uint8_t);
	uint8_t  isExtendedFrame(uint8_t);
	uint16_t  whichStdID(uint8_t);
	uint32_t whichExtdID(uint8_t);
	uint8_t whichRxDataLength(uint8_t);
	uint8_t receivedDataValue(uint8_t, uint8_t);
	void setTxnrtsPinMode(uint8_t, uint8_t, uint8_t);
	void setTxBufferID(uint16_t, uint32_t, uint8_t, uint8_t);
	void setTxBufferDataLength(uint8_t, uint8_t, uint8_t);
	void setTxBufferDataField(uint8_t*, uint8_t);
	void messageTransmitRequest(uint8_t, uint8_t, uint8_t);
	uint8_t isMessagePending(uint8_t);
	uint8_t isTxError(uint8_t);
	uint8_t isArbitrationLoss(uint8_t);
	uint8_t isMessageAborted(uint8_t);
	void flashRxbf();
	
};
                                   


#endif
