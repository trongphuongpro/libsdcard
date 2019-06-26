#include "sdcard.h"
#include "libspi.h"
#include <string.h>
#include "libuart.h"


uint8_t buffer[15];
uint8_t responseBuffer[5];

static uint8_t sdcardType;

static uint8_t sdcard_waitReady(uint16_t timeout) {
	uint16_t time = timeout;

	while (time > 0) {
		if (spi_master_receive_byte() == 0xFF) {
			break;
		}
		time--;
	}

	if (time == 0)
		return 0;

	return 1;
}


uint8_t sdcard_enable(void) {
	SDCARD_PORT &= ~(1 << SDCARD_ADDRESS);

	if (!sdcard_waitReady(0xFF))
		return 0;
	return 1;
}


void sdcard_disable(void) {
	SDCARD_PORT |= (1 << SDCARD_ADDRESS);
}


uint8_t sdcard_getResponse(uint8_t code, uint8_t expect) {
	uint8_t timeout = 10;
	uint8_t responseLen;


	while (timeout) {
		responseBuffer[0] = spi_master_receive_byte();
		sprintf(buffer, "Response: %x\n\r", responseBuffer[0]);
		uart_print(buffer);

		if (responseBuffer[0] == expect) {
			sprintf(buffer, "R1: %x\n\r", responseBuffer[0]);
			uart_print(buffer);
			break;
		}

		timeout--;
	}

	switch (code) {
		case R1: responseLen = R1_LEN;
				break;
		case R2: responseLen = R2_LEN;
				break;
		case R3: case R7: responseLen = R3_LEN;
				break;

	}

	for (uint8_t i = 1; i < responseLen; i++) {
		responseBuffer[i] = spi_master_receive_byte();
	}

	if (timeout == 0) 
		return 0;
	else
		return 1;
}


void sdcard_sendCommand(uint8_t cmd, uint32_t arg) {

	if (!sdcard_waitReady(0xFF))
		uart_print("Card busy!\n\r");

	spi_master_transmit_byte(cmd | 0x40);
	spi_master_transmit_byte((uint8_t)(arg >> 24));
	spi_master_transmit_byte((uint8_t)(arg >> 16));
	spi_master_transmit_byte((uint8_t)(arg >> 8));
	spi_master_transmit_byte((uint8_t)arg);

	if (cmd == CMD8)
		spi_master_transmit_byte(0x87);
	else if (cmd == CMD0)
		spi_master_transmit_byte(0x95);
	else
		spi_master_transmit_byte(0xFF);

}


static void sdcard_wakeup() {
	sdcard_disable();
	// 80 pulses to wake up SDcard
	for (int i = 0; i < 10; i++) {
		spi_master_transmit_byte(0xFF);
	}
	sdcard_disable();
}


static void sdcard_config() {
	SDCARD_DDR |= (1 << SDCARD_ADDRESS);
}


static uint8_t sdcard_getType() {
	if ((responseBuffer[1] & 0xC0) == 0xC0)
		return SDHC;
	else
		return SDSC;
}



status_code sdcard_initialize(void) {
	sdcard_config();
	spi_master_initialize();
	sdcard_wakeup();

	sdcard_enable();

	uart_print("> send CMD0...\n\r");
	/* Reset card to Idle State */
	
	sdcard_sendCommand(CMD0, 0);
	if (!sdcard_getResponse(R1, 0x01)) {
		sdcard_disable();
		return CMD0_ERROR;
	}
	
	uart_print("> send CMD8...\n\r");
	
	sdcard_sendCommand(CMD8, 0x000001AA);
	if (!sdcard_getResponse(R7, 0x01)) {
		sdcard_disable();
		return CMD8_ERROR;
	}

	uart_print("> send CMD58...\n\r");
	
	sdcard_sendCommand(CMD58, 0);
	if (!sdcard_getResponse(R3, 0x01)) {
		sdcard_disable();
		return CMD58_ERROR;
	}

	// time out
	uint16_t time = 0xFF;
	
	uart_print("> send ACMD41...\n\r");
	while (time > 0) {
		sdcard_enable();
		sdcard_sendCommand(CMD55, 0);
		sdcard_getResponse(R1, 0x00);
		sdcard_sendCommand(ACMD41, 0x40000000);

		if (sdcard_getResponse(R1, 0x00))
			break;

		sdcard_disable();

		time--;
	}
	
	if (time == 0) {
		sdcard_disable();
		return CMD1_ERROR;
	}
	
	uart_print("> send CMD58...\n\r");

	sdcard_sendCommand(CMD58, 0);
	if (!sdcard_getResponse(R3, 0x00)) {
		sdcard_disable();
		return CMD58_ERROR;
	}
	
	sdcardType = sdcard_getType();

	if (sdcardType == SDSC) {
		uart_print("> send CMD16...\n");

		sdcard_sendCommand(CMD16, BLOCK_LEN);
		if (!sdcard_getResponse(R1, 0x00)) {
			sdcard_disable();
			return CMD16_ERROR;
		}
	}

	sdcard_disable();
	return OK;
}


status_code sdcard_transmitDataBlock(const uint8_t *buff, uint8_t token) {
	if (!sdcard_waitReady(MAX_BUSY_TIME))
		return CARD_BUSY;

	uart_print("> transmit data\n\r");
	spi_master_transmit_byte(token);

	for (int i = 0; i < BLOCK_LEN; i++) {
		spi_master_transmit_byte(*(buff+i));
	}
	
	spi_master_transmit_byte(0xFF);
	spi_master_transmit_byte(0xFF);

	uint16_t time = MAX_BUSY_TIME;
	uint8_t status;
	
	do {
		status = spi_master_receive_byte();
		time--;
	} while (((status & 0x1F) != 0x05) && time > 0);

	if (time == 0) {
		return WRITING_TIMEOUT;
	}

	return OK;
}


status_code sdcard_receiveDataBlock(uint8_t *buff, uint16_t len) {
	uint16_t time = 4000;
	uint8_t token;

	uart_print("> receive token\r\n");
	while (time > 0) {
		token = spi_master_receive_byte();
		sprintf(buffer, "token: %x\n\r", token);
		uart_print(buffer);

		if (token == SINGLE_START_TOKEN)
			break;
		time--;
	}

	if (time == 0) {
		return TOKEN_ERROR;
	}

	for (int i = 0; i < len; i++)
		*(buff+i) = spi_master_receive_byte();
	
	spi_master_receive_byte();
	spi_master_receive_byte();

	return OK;
}


status_code sdcard_writeBlock(uint32_t sector, 
							const uint8_t *buff,
							uint16_t count) {
	
	if (sdcardType == SDSC)
		sector *= BLOCK_LEN;

	sdcard_enable();

	if (count == 1) {
		uart_print("> send CMD24...\n\r");
		sdcard_sendCommand(CMD24, sector);
		if (!sdcard_getResponse(R1, 0x00)) {
			sdcard_disable();
			return CMD24_ERROR;
		}
	}
	else {
		uart_print("> send ACMD23...\n\r");
		sdcard_sendCommand(CMD55, 0);
		sdcard_sendCommand(ACMD23, count);


		uart_print("> send CMD25...\n\r");
		sdcard_sendCommand(CMD25, sector);
		if (!sdcard_getResponse(R1, 0x00)) {
			sdcard_disable();
			return CMD25_ERROR;
		}
	}
	sdcard_disable();
	sdcard_enable();
	if (count == 1) {
		if (sdcard_transmitDataBlock(buff, SINGLE_START_TOKEN) != OK) {
			sdcard_disable();
			return WRITING_TIMEOUT;
		}
	}
	else {
		while (count) {
			if (sdcard_transmitDataBlock(buff, MULTI_START_TOKEN) != OK) {
				sdcard_disable();
				return WRITING_TIMEOUT;
			}
			buff += BLOCK_LEN;
			count--;
		}
		if (!sdcard_waitReady(MAX_BUSY_TIME)) {
			sdcard_disable();
			return CARD_BUSY;
		}
		spi_master_transmit_byte(STOP_TRAN_TOKEN);
	}
	
	// busy may up to 500ms
	if (!sdcard_waitReady(MAX_BUSY_TIME)) {
		sdcard_disable();
		return CARD_BUSY;
	}
	
	/*uart_print("send CMD13...\n");
	sdcard_sendCommand(CMD13, 0);
	if (!sdcard_getResponse(R2, 0x00) || !sdcard_getResponse(R2, 0x00)) {
		sdcard_status = WRITING_TIMEOUT;

		return sdcard_status;
	}*/

	sdcard_disable();
	return OK;
}

status_code sdcard_readBlock(uint32_t sector,
							uint8_t *buff, 
							uint16_t count) {
	
	if (sdcardType == SDSC)
		sector *= BLOCK_LEN;

	sdcard_enable();

	if (count == 1) {
		uart_print("> send CMD17...\n\r");
		sdcard_sendCommand(CMD17, sector);
		if (!sdcard_getResponse(R1, 0x00)) {
			sdcard_disable();
			return CMD17_ERROR;
		}
	}
	else {
		uart_print("> send CMD18...\n\r");
		sdcard_sendCommand(CMD18, sector);
		if (!sdcard_getResponse(R1, 0x00)) {
			sdcard_disable();
			return CMD18_ERROR;
		}
	}
	

	if (count == 1) {
		if (sdcard_receiveDataBlock(buff, BLOCK_LEN) != OK) {
			uart_print("Err token!\n\r");
			sdcard_disable();
			return TOKEN_ERROR;
		}
	}
	else {
		while (count) {
			if (sdcard_receiveDataBlock(buff, BLOCK_LEN) != OK) {
				uart_print("Err token!\n\r");
				sdcard_disable();
				return TOKEN_ERROR;
			}
			buff += BLOCK_LEN;
			count--;
		}
		sdcard_sendCommand(CMD12, 0);
	}
	sdcard_disable();
	return OK;
}


const char* sdcard_getErrorMessage(status_code error) {
	const char *errorMessage;
	if (error == OK)
		errorMessage = "OK!\n\r";
	else if (error == CMD0_ERROR)
		errorMessage = "Err reset!\n\r";
	else if (error == CMD1_ERROR)
		errorMessage = "Err op-cond!\n\r";
	else if (error == CMD8_ERROR)
		errorMessage = "Err int-con!\n\r";
	else if (error == CMD9_ERROR)
		errorMessage = "Err CSD!\n\r";
	else if (error == CMD16_ERROR)
		errorMessage = "Err block size!\n\r";
	else if (error == CMD17_ERROR)
		errorMessage = "Err read block!\n\r";
	else if (error == CMD18_ERROR)
		errorMessage = "Err read multi-block!\n\r";
	else if (error == CMD24_ERROR)
		errorMessage = "Err write block!\n\r";
	else if (error == CMD25_ERROR)
		errorMessage = "Err write multi-block!\n\r";
	else if (error == CMD58_ERROR)
		errorMessage = "Err OCR!\n\r";
	else if (error == WRITING_TIMEOUT)
		errorMessage = "Writing timeout!\n\r";
	else if (error == CARD_BUSY)
		errorMessage = "Card busy!\n\r";
	else if (error == TOKEN_ERROR)
		errorMessage = "Err token!\n\r";
	else
		errorMessage = "Unknown error!\n\r";

	return errorMessage;
}
