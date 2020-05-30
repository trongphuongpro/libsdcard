//! \file sdcard.h
//! \brief initialization for sdcard library
//! \author Nguyen Trong Phuong
//! \date 2020 May 29

#ifndef __SDCARD__
#define __SDCARD__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "ff.h"    
#include "utils_tiva.h"

void tiva_sdcard_init(uint32_t SSI_base, PortPin_t SS);
void avr_sdcard_init();

#ifdef __cplusplus
}
#endif

#endif /* __SDCARD__ */

/************************* End Of File ***************************************/