/*
 * func.h
 *
 * Created: 6/25/2017 12:26:50 PM
 *  Author: orencollaco
 */ 


#ifndef FUNC_H_
#define FUNC_H_
#define F_CPU 8000000UL
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>
#define adcsra_c 0xA2
#define admux_c 0x20
#define hlevel 0xE8
#define llevel 0x10
//#define F_CPU 8.000000
#define FOSC 1000000 // Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define MTA_SLAW_ACK 0x18
#define MTD_SLAW_ACK 0x28
#define MTDS_SLAR_ACK 0x40
#define MTDR_SLAR_NACK 0x58
//#define START 0x08
#define RESTART 0x10

#define HCI_ACL_DATA_PACKET 0x02
#define HCI_SCO_DATA_PACKET 0x03
#define HCI_EVENT_PACKET 0x04

//#define UART_DEBUG 1
//#define ULTRA_DEBUG 1


#define CTS GET_BITFIELD(PINB).bit7
#define RTS GET_BITFIELD(PORTB).bit6

typedef struct bitstruct
{
	unsigned char bit0 : 1;
	unsigned char bit1 : 1;
	unsigned char bit2 : 1;
	unsigned char bit3 : 1;
	unsigned char bit4 : 1;
	unsigned char bit5 : 1;
	unsigned char bit6 : 1;
	unsigned char bit7 : 1;
}bit_field;
#define GET_BITFIELD(addr)( *((volatile bit_field*)&(addr))  )


void setIndicatorCount(uint16_t count1);
void updateFade();
void setThingsUp();
uint64_t millis();
void delay(uint16_t t);
void setTimer3(unsigned int count, unsigned char prescale);
void Init_CTC_T2(unsigned char prescale);
void Init_PWM_T3(unsigned char prescale);
void Init_T4(unsigned char prescale);
void USART0_Init(unsigned int ubrr);
void USART0_Transmit(unsigned char data);
unsigned char USART0_Receive();
void USART1_Init(unsigned int ubrr);
void USART1_Transmit(unsigned char data);
unsigned char USART1_Receive();
void relayUSART();
void I2C_Init(unsigned char twbr, unsigned char twsr);
unsigned char I2C_Read(unsigned char daddr, unsigned char raddr);
void I2C_Write(unsigned char daddr, unsigned char raddr, unsigned char data);
void Init_PWM_T0(unsigned char prescale);
void Init_PWM_T1(unsigned char prescale);
void Init_PWM_T2(unsigned char prescale);
void ERROR(unsigned char twsr);
void printString1(const char*);
void printChar(unsigned char data);
unsigned char hexToASCII(unsigned char data);
char *hexToString(unsigned char data);
unsigned char charToHex(char *p);
void printStringNewline(char *);
void printStringCRNL0(const char *);
void printStringCRNL1(const char *p);
char *receiveStringCRNL();
void setTimer1(unsigned int count, unsigned char prescale);
void holdPosition(unsigned int x,unsigned int y);
void transmitSensorData();
void fanSpeed(unsigned char a0, unsigned char a2, unsigned char b0, unsigned char b2);
void BT_Init();
void USART1_Disable();
void Notify(const char* ,uint8_t);
void NotifyS(const char d, uint8_t num);
void D_PrintHex(uint8_t,uint8_t);

#endif /* FUNC_H_ */