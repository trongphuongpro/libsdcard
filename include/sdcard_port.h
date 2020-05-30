#ifndef __SDCARD_PORT__
#define __SDCARD_PORT__

//! \file sdcard_port.h
//! \brief portable functions for sdcard library
//! \author Nguyen Trong Phuong
//! \date 2020 May 29

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ff.h"

void select_card(void);
void deselect_card(void);
void spi_transmit_byte(BYTE dat);
BYTE spi_receive_byte(void);
void spi_receive_byte_m(BYTE *dst);
void send_initial_clock_train(void);
void set_max_speed(void);

#ifdef __cplusplus
}
#endif

#endif /* __SDCARD_PORT__ */

/************************* End Of File ***************************************/