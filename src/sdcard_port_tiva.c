//! \file sdcard_port_tiva.c
//! \brief portable functions for sdcard library
//! \author Nguyen Trong Phuong
//! \date 2020 May 29

#include "sdcard.h"
#include "sdcard_port.h"

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"


static PortPin_t SS;
static uint32_t SSI_base;


void tiva_sdcard_init(uint32_t SSI_base__, PortPin_t SS__) {
    SS = SS__;
    SSI_base = SSI_base__;

    ROM_SSIDisable(SSI_base);
    ROM_SSIConfigSetExpClk(SSI_base, ROM_SysCtlClockGet(),
                           SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 1000000, 8);
    ROM_SSIEnable(SSI_base);

    // clear FIFO
    uint32_t tmp;
    while (ROM_SSIDataGetNonBlocking(SSI_base, &tmp));
}

// asserts the CS pin to the card
void SELECT(void) {
    ROM_GPIOPinWrite(SS.base, SS.pin, 0);
}

// de-asserts the CS pin to the card
void DESELECT(void) {
    ROM_GPIOPinWrite(SS.base, SS.pin, SS.pin);
}


/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/

void xmit_spi(BYTE dat) {
    uint32_t ui32RcvDat;

    ROM_SSIDataPut(SSI_base, dat); /* Write the data to the tx fifo */

    ROM_SSIDataGet(SSI_base, &ui32RcvDat); /* flush data read during the write */
}


/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI  (Platform dependent)                 */
/*-----------------------------------------------------------------------*/

BYTE rcvr_spi(void) {
    uint32_t ui32RcvDat;

    ROM_SSIDataPut(SSI_base, 0xFF); /* write dummy data */

    ROM_SSIDataGet(SSI_base, &ui32RcvDat); /* read data frm rx fifo */

    return (BYTE)ui32RcvDat;
}


void rcvr_spi_m(BYTE *dst) {
    *dst = rcvr_spi();
}


void send_initial_clock_train(void) {
    unsigned int i;
    uint32_t ui32Dat;

    /* Ensure CS is held high. */
    DESELECT();

    /* Switch the SSI TX line to a GPIO and drive it high too. */
    //ROM_GPIOPinTypeGPIOOutput(SDC_GPIO_PORT_BASE, SDC_SSI_TX);
    //ROM_GPIOPinWrite(SDC_GPIO_PORT_BASE, SDC_SSI_TX, SDC_SSI_TX);

    /* Send 10 bytes over the SSI. This causes the clock to wiggle the */
    /* required number of times. */
    for(i = 0 ; i < 10 ; i++)
    {
        /* Write DUMMY data. SSIDataPut() waits until there is room in the */
        /* FIFO. */
        ROM_SSIDataPut(SSI_base, 0xFF);

        /* Flush data read during data write. */
        ROM_SSIDataGet(SSI_base, &ui32Dat);
    }

    /* Revert to hardware control of the SSI TX line. */
    //ROM_GPIOPinTypeSSI(SDC_GPIO_PORT_BASE, SDC_SSI_TX);
}


void set_max_speed(void)
{
    unsigned long i;

    /* Disable the SSI */
    ROM_SSIDisable(SSI_base);

    /* Set the maximum speed as half the system clock, with a max of 12.5 MHz. */
    i = ROM_SysCtlClockGet() / 2;
    if(i > 12500000)
    {
        i = 12500000;
    }

    /* Configure the SSI0 port to run at 12.5MHz */
    ROM_SSIConfigSetExpClk(SSI_base, ROM_SysCtlClockGet(),
                           SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, i, 8);

    /* Enable the SSI */
    ROM_SSIEnable(SSI_base);
}

/************************* End Of File ***************************************/