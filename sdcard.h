#ifndef SD_CARD_H
#define SD_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

#define SDCARD_ADDRESS	2
#define SDCARD_DDR	DDRB
#define SDCARD_PORT	PORTB

#define BLOCK_LEN	512

#define CMD0		0
#define CMD1		1
#define CMD8		8
#define CMD9 		9
#define CMD12 		12	
#define CMD13		13	
#define CMD16		16
#define CMD17		17
#define CMD18		18
#define ACMD23		23	
#define CMD24		24
#define CMD25		25
#define CMD55		55
#define CMD58		58	
#define	ACMD41		41

#define SINGLE_START_TOKEN	0xFE
#define MULTI_START_TOKEN	0xFC
#define STOP_TRAN_TOKEN		0xFD

#define SDSC 		0
#define SDHC 		1
	
#define R1_LEN		1
#define R2_LEN		2
#define R3_LEN		5
#define R7_LEN		5

#define MAX_BUSY_TIME	8000

enum response {R1 = 0, R2, R3, R7};

typedef enum {OK = 0,
				CMD0_ERROR,
				CMD1_ERROR,
				CMD8_ERROR,
				CMD9_ERROR,
				CMD16_ERROR,
				CMD17_ERROR,
				CMD18_ERROR,
				CMD24_ERROR,
				CMD25_ERROR,
				CMD58_ERROR,
				WRITING_TIMEOUT,
				CARD_BUSY,
				TOKEN_ERROR} status_code;


// function  prototypes
status_code sdcard_initialize(void);
status_code sdcard_readBlock(uint32_t, uint8_t*, uint16_t);
status_code sdcard_writeBlock(uint32_t, const uint8_t*, uint16_t);

const char* sdcard_getErrorMessage(status_code);
void sdcard_sendCommand(uint8_t, uint32_t);
uint8_t sdcard_getResponse(uint8_t, uint8_t);
uint8_t sdcard_enable();
void sdcard_disable();
status_code sdcard_receiveDataBlock(uint8_t *, uint16_t);


#ifdef __cplusplus
}
#endif

#endif
