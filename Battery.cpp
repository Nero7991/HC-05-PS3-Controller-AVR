/*
 * Battery.cpp
 *
 * Created: 7/1/2017 11:44:57 AM
 *  Author: orencollaco
 */ 

#include "Battery.h"
uint8_t BattOK;
void Init_Batt(){
	ADC_Init(admux_c,adcsra_c);
}

uint8_t checkBatt(){
	static unsigned char battL = 0,battC = 0;
	static unsigned int BattVoltage,avg,battLow = 700,battCritical = 670;
	BattVoltage = ADCL;
	BattVoltage |= (ADCH << 8);
	avg = BattVoltage;
	for(int i = 0; i < 20; i++){
		BattVoltage = ADCL;
		BattVoltage |= (ADCH << 8);
		avg = (avg + BattVoltage)/2;
		printStringCRNL1(hexToString(avg));
	}
	if((avg < battLow) && !battL && !battC){
		battL = 1;
		battLow = 715;
		indicateBattLow();
	}
	if((avg > battLow) && battL && !battC){
		battL = 0;
		battLow = 710;
		stopIndicator();
	}
	if(battL && !battC && (avg < battCritical)){
		OCR1A = 0;
		battCritical = 680;
		battC = 1;
		BattOK = 0;
		//Timer0_Init(0,0);
	}
	if(battL && battC && (avg > battCritical)){
		battCritical = 675;
		battC = 0;
		BattOK = 1;
		indicateBattLow();
	}
	if(!battC){
	}
	if(battC)
	criticalBatt();
}
void criticalBatt(){
	ADCSRA &= ~(1 << ADEN);
	;
	//PRR = (1 << PRTIM0) | (1 << PRADC) | (1 << PRTIM1);
	PORTB = 0;
	powerDownFor(0x06);
	PORTB = (1 << 0);
	powerDownFor(0x02);
	PORTB = 0;
	powerDownFor(0x06);
	WDT_off();
	//PRR = 0;
	ADCSRA |= (1 << ADEN);
	ADC_Start();
	//Timer1_Init(0x63,0x00);
}

void indicateBattLow(){
	setIndicatorCount(5);
	//Timer0_Init(0x81,0x01);
	//sei();
	//TIMSK = 0x02;
}

void stopIndicator(){
	setIndicatorCount(3);
	//Timer0_Init(0,0);
	//cli();
	//TIMSK = 0;
}

void ADC_Init(unsigned char admux, unsigned char adcsra)
{
	ADCSRA = adcsra;
	ADMUX = admux;
}
void ADC_ChangeChannelTo(unsigned char admux)
{
	admux &= 0x0F;
	ADMUX &= 0xF0;
	ADMUX |= admux;
}

unsigned char ADC_Read(unsigned char admux)
{
	admux &= 0x0F;
	ADMUX &= 0xF0;
	ADMUX |= admux;
	while(!(ADCSRA & (1<<ADIF)));
	return ADCH;
}
void ADC_Start()
{
	ADCSRA = adcsra_c | (1<<ADSC);
}

void ADC_Finish(){
	while(!(ADCSRA & (1<<ADIF)));
}

void WDT_off(void)
{
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Write logical one to WDCE and WDE */
	/* Keep old prescaler setting to prevent unintentional time-out */
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	/* Turn off WDT */
	WDTCSR = 0x00;
	sei();
}

void WDT_Init(){
	asm volatile("cli"::);
	asm volatile("wdr"::);
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Write logical one to WDCE and WDE */
	/* Keep old prescaler setting to prevent unintentional time-out */
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR |= (1 << WDIE);
	/* Turn off WDT */
	WDTCSR = 0x00;
	asm volatile("sei"::);
}

void WDT_Prescaler_Change(void)
{
	asm volatile("cli"::);
	asm volatile("wdr"::);
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Start timed sequence */
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	/* Set new prescaler (time-out) value = 64K cycles (~0.5 s) */
	WDTCSR = (1<<WDP2) | (1<<WDP0) | (1<<WDIE);
	asm volatile("sei"::);
	//asm volatile("sei"::);
	//MCUSR |= (1 << SE) | (1 << SM1);
	//asm volatile("sleep"::);
}

void powerDownFor(unsigned char prescale){
	unsigned char t;
	t = 0x07 & prescale;
	prescale &= 0x08;
	prescale = prescale << 2;
	prescale |= t;
	prescale |= (1 << WDIE) | (1 << WDE);
	TCNT0 = prescale;
	asm volatile("cli"::);
	asm volatile("wdr"::);
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Start timed sequence */
	WDTCSR = (1<<WDCE) | (1<<WDE);
	/* Set new prescaler (time-out) value = 64K cycles (~0.5 s) */
	WDTCSR = prescale;
	asm volatile("sei"::);
	sleep_bod_disable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	sleep_mode();
}

ISR(WDT_vect){
	MCUSR &= ~(1 << SE);
}
