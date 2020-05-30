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

void SELECT(void);
void DESELECT(void);
void xmit_spi(BYTE dat);
BYTE rcvr_spi(void);
void rcvr_spi_m(BYTE *dst);
void send_initial_clock_train(void);
void set_max_speed(void);

#ifdef __cplusplus
}
#endif

#endif /* __SDCARD_PORT__ */

/************************* End Of File ***************************************/