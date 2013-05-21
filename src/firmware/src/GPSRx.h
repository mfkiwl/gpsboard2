#include "fx2.h"
#include "fx2regs.h"
#include "syncdly.h"            // SYNCDELAY macro

#define SPI_CLK PA1
#define MOSI PA0
#define SPI_SS PA3

#define GPS_SET_CONFIG 0xA1

void SPI_Init();
void SPI_ByteWrite(unsigned char byte);
void SPI_Begin();
void SPI_End();
void SPI_Delay(unsigned char time);