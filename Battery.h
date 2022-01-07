/*
 * Battery.h
 *
 * Created: 7/1/2017 11:45:22 AM
 *  Author: orencollaco
 */ 


#ifndef BATTERY_H_
#define BATTERY_H_
#include "func.h"
#define adcsra_c 0xE4
#define admux_c 0x60
#define hlevel 0xE8
#define llevel 0x10
void Init_Batt();
void indicateBattLow();
void stopIndicator();
void criticalBatt();
void powerDownFor(unsigned char prescale);
void WDT_Prescaler_Change(void);
void WDT_Init();
void WDT_off(void);
void ADC_Init( unsigned char, unsigned char);
void ADC_ChangeChannelTo(unsigned char admux);
unsigned char ADC_Read(unsigned char );
void ADC_Start();
uint8_t checkBatt();
void ADC_Finish();


#endif /* BATTERY_H_ */