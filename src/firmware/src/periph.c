//-----------------------------------------------------------------------------
//   File:      periph.c
//   Contents:  Hooks required to implement USB peripheral function.
//
// $Archive: /USB/Target/Fw/lp/periph.c $
// $Date: 3/23/05 3:03p $
// $Revision: 3 $
//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//
// This software is owned by Cypress Semiconductor Corporation (Cypress) and is
// protected by United States copyright laws and international treaty provisions. Cypress
// hereby grants to Licensee a personal, non-exclusive, non-transferable license to copy,
// use, modify, create derivative works of, and compile the Cypress Source Code and
// derivative works for the sole purpose of creating custom software in support of Licensee
// product ("Licensee Product") to be used only in conjunction with a Cypress integrated
// circuit. Any reproduction, modification, translation, compilation, or representation of this
// software except as specified above is prohibited without the express written permission of
// Cypress.
//
// Disclaimer: Cypress makes no warranty of any kind, express or implied, with regard to
// this material, including, but not limited to, the implied warranties of merchantability and
// fitness for a particular purpose. Cypress reserves the right to make changes without
// further notice to the materials described herein. Cypress does not assume any liability
// arising out of the application or use of any product or circuit described herein. Cypress’
// products described herein are not authorized for use as components in life-support
// devices.
//
// This software is protected by and subject to worldwide patent coverage, including U.S.
// and foreign patents. Use may be limited by and subject to the Cypress Software License
// Agreement.
//-----------------------------------------------------------------------------
#pragma NOIV               // Do not generate interrupt vectors

#include "GPSRx.h"

extern BOOL   GotSUD;         // Received setup data flag
extern BOOL   Sleep;
extern BOOL   Rwuen;
extern BOOL   Selfpwr;

BYTE   Configuration;      // Current configuration
BYTE   AlternateSetting;   // Alternate settings

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

void TD_Init(void)             // Called once at startup
{
	// set the CPU clock to 48MHz
	CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1);
	SYNCDELAY;

	IFCONFIG = 0x13; 					// use IFCLK pin driven by external logic (5MHz to 48MHz)
														// use slave FIFO interface pins driven sync by external master
	SYNCDELAY;
	
	REVCTL = 0x03; 						// REVCTL.0 and REVCTL.1 set to 1
	SYNCDELAY;
		
	EP2CFG = 0xD8;						// EP2 enabled
														// direction IN (dev > host)
														// type isochronous
														// size 1024 bytes
														// quad buffering
	SYNCDELAY;
	
	EP2ISOINPKTS = 0x83;
	SYNCDELAY;
	
	EP4CFG &= 0x7F; 					// EP4 disabled
	SYNCDELAY;
	
	EP6CFG &= 0x7F; 					// EP6 disabled
	SYNCDELAY;
	
	EP8CFG &= 0x7F; 					// EP8 disabled
	SYNCDELAY;
	
	FIFORESET = 0x80; 				// reset all FIFOs
	SYNCDELAY;
	
	FIFORESET = 0x82;
	SYNCDELAY;
	
	FIFORESET = 0x84;
	SYNCDELAY;
	
	FIFORESET = 0x86;
	SYNCDELAY;
	
	FIFORESET = 0x88;
	SYNCDELAY;
	
	FIFORESET = 0x00;
	SYNCDELAY;
	
	EP2FIFOCFG = 0x0C;			// this defines the external interface to be the following:
													// this lets the EZ-USB auto commit IN packets, gives the
													// ability to send zero length packets,
													// and sets the slave FIFO data interface to 8-bits
	
	PINFLAGSAB = 0x00; 			// defines FLAGA as prog-level flag, pointed to by FIFOADR[1:0]
	SYNCDELAY; 							// FLAGB as full flag, as pointed to by FIFOADR[1:0]
	
	PINFLAGSCD = 0x00; 			// FLAGC as empty flag, as pointed to by FIFOADR[1:0]
													// won't generally need FLAGD
	
	PORTACFG = 0x00; 				// used PA7/FLAGD as a port pin, not as a FIFO flag
	SYNCDELAY;
	
	FIFOPINPOLAR = 0x04; 		// set all slave FIFO interface pins as active low
	SYNCDELAY;
	
	EP2AUTOINLENH = 0x04; 	// EZ-USB automatically commits data in 1024-byte chunks
	SYNCDELAY;
	
	EP2AUTOINLENL = 0x00;
	SYNCDELAY;
	
	EP2FIFOPFH = 0x80; 			// you can define the programmable flag (FLAGA)
	SYNCDELAY; 							// to be active at the level you wish
	
	EP2FIFOPFL = 0x00;
	
	BREAKPT &= ~bmBPEN;      // to see BKPT LED go out TGE
}

void TD_Poll(void)             // Called repeatedly while the device is idle
{
}

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
   return(TRUE);
}

BOOL TD_Resume(void)          // Called after the device resumes
{
   return(TRUE);
}

//-----------------------------------------------------------------------------
// SPI interface
//-----------------------------------------------------------------------------

void SPI_Init()
{
	OEA |= 0x0B;		// Port A config for SPI
									// PA0 = MOSI 
									// PA1 = SPI_CLK
									// PA3 = SPI_SS
	SPI_SS = 1;
	SPI_CLK = 0;
	MOSI = 0;
}

void SPI_Begin()
{
	SPI_CLK = 0;
	MOSI = 0;
	SPI_SS = 0;
	SPI_Delay(20);
}

void SPI_End()
{
	SPI_Delay(20);
	SPI_SS = 1;
	SPI_CLK = 0;
	MOSI = 0;
}

void SPI_ByteWrite(unsigned char byte)
{
	unsigned char i;
	for(i = 0; i < 8; i++) // Loop through each bit
	{
		MOSI = (byte & 0x80) ? 1 : 0;
		
		SPI_Delay(20);
		SPI_CLK = 1;
		
		SPI_Delay(20);
		SPI_CLK = 0;
		
		byte = byte << 1;
	}
}

void SPI_Delay(unsigned char time)
{
	while(time-- != 0);	
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------

BOOL DR_GetDescriptor(void)
{
   return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{
   Configuration = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
   EP0BUF[0] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
   AlternateSetting = SETUPDAT[2];
   return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
   EP0BUF[0] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
   return(TRUE);
}

BOOL DR_ClearFeature(void)
{
   return(TRUE);
}

BOOL DR_SetFeature(void)
{
   return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
   return(TRUE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
   GotSUD = TRUE;            // Set flag
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

void ISR_Sof(void) interrupt 0
{
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSOF;            // Clear SOF IRQ
}

void ISR_Ures(void) interrupt 0
{
   // whenever we get a USB reset, we should revert to full speed mode
   pConfigDscr = pFullSpeedConfigDscr;
   ((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
   pOtherConfigDscr = pHighSpeedConfigDscr;
   ((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
   
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR();
   USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
   if (EZUSB_HIGHSPEED())
   {
      pConfigDscr = pHighSpeedConfigDscr;
      ((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
      pOtherConfigDscr = pFullSpeedConfigDscr;
      ((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
   }

   EZUSB_IRQ_CLEAR();
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}
