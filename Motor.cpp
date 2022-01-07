/*
 * Motor.cpp
 *
 * Created: 6/30/2017 8:50:24 PM
 *  Author: orencollaco
 */ 

#include "Motor.h"

void Init_Motor(){
	DDRB |= (1 << 0) | (1 << 3) | (1 << 4);
	DDRD |= (1 << 7);
	PORTB = (1 << 0) | (1 << 3);
	Init_PWM_T0(0x01);
	Init_PWM_T1(0x01);
}

void disable_Motor(){
	PORTB &= ~((1 << 3) | (1 << 4) | (1 << 0));
	PORTD &= ~(1 << 7);
	OCR0A = 0;
	OCR0B = 0;
	OCR1A = 0;
	OCR1B = 0;
}

void updateMotorSpeed(uint8_t Gas,uint8_t Steer){
	static uint16_t MotorL,MotorR;
	MotorL = computeLeft(Gas,Steer);
	MotorR = computeRight(Gas,Steer);
	if(0x100 & MotorL && !(PORTB & (1 << 0))){
		OCR0A = 0;
		OCR0B = 0;
		PORTB |= (1 << 0);
		PORTD &= ~(1 << 7);
		delay(5);
		OCR0B = MotorL & 0xFF;
	}
	if(0x100 & MotorL && PORTB & (1 << 0)){
		OCR0B = MotorL & 0xFF;
	}
	if(!(0x100 & MotorL) && !(PORTD & (1 << 7))){
		OCR0A = 0;
		OCR0B = 0;
		PORTD |= (1 << 7);
		PORTB &= ~(1 << 0);
		delay(5);
		OCR0A = MotorL & 0xFF;
	}
	if(!(0x100 & MotorL) && PORTD & (1 << 7)){
		OCR0A = MotorL & 0xFF;
	}
	
	if(0x100 & MotorR && !(PORTB & (1 << 3))){
		OCR1A = 0;
		OCR1B = 0;
		PORTB |= (1 << 3);
		PORTB &= ~(1 << 4);
		delay(5);
		OCR1B = MotorR & 0xFF;
	}
	if(0x100 & MotorR && PORTB & (1 << 3)){
		OCR1B = MotorR & 0xFF;
	}
	
	if(!(0x100 & MotorR) && !(PORTB & (1 << 4))){
		OCR1A = 0;
		OCR1B = 0;
		PORTB |= (1 << 4);
		PORTB &= ~(1 << 3);
		delay(5);
		OCR1A = MotorR & 0xFF;
	}
	if(!(0x100 & MotorR) && PORTB & (1 << 4)){
		OCR1A = MotorR & 0xFF;
	}
	
	
	
	/*
	if(computeRelayState(MotorL,PORTB & (1 << 0),PORTD & (1 << 7))){
		OCR0A = 0;
		OCR0B = 0;
		PORTB ^= (1 << 0);
		PORTD ^= (1 << 7);
		
	}
	else{
		if(PORTB & (1 << 0))	
			OCR0B = MotorL;
		else
			OCR0A = MotorL;
	}
	if(computeRelayState(MotorR,PORTB & (1 << 3),PORTB & (1 << 4))){
		OCR1A = 0;
		OCR1B = 0;
		PORTB ^= (1 << 3);
		PORTB ^= (1 << 4);
		
	}
	else{
		if(PORTB & (1 << 3))
		OCR1A = MotorR;
		else
		OCR1B = MotorR;
	}*/
}

unsigned int computeLeft(unsigned int Gas,unsigned int Steer)
{
	unsigned int MotorL,TSteer;
	MotorL = offsetForJoy(Gas);
	TSteer = offsetForJoy(Steer);
	if(MotorL & 0x0FF)					//Is Moving?
	{
		if(MotorL & 0x100)				//Moving forward?
		{
			if(TSteer & 0x100)			//Turn Right?
			return MotorL;
			else
			{
				if((TSteer & 0xFF) >= (MotorL & 0xFF))
				return 0x100;
				else
				return (MotorL - (TSteer & 0xFF));
			}
		}
		else
		{
			if(TSteer & 0x100)
			{
				if((TSteer & 0xFF) < (MotorL & 0xFF))
				return (MotorL - (TSteer & 0xFF));
				else
				return 0;
			}
			else
			return MotorL;
		}
		
	}
	else
	{
		if(TSteer & 0xFF)
		{
			if(TSteer & 0x100)      //Turn Right if true
			return (0x100 | TSteer);
			else
			return (0xFF & TSteer);
		}
		else
		return 0;
	}
}
unsigned int computeRight(unsigned int Gas,unsigned int Steer)
{
	unsigned int MotorL,TSteer;
	MotorL = offsetForJoy(Gas);
	TSteer = offsetForJoy(Steer);
	if(MotorL & 0x0FF)					//Is Moving?
	{
		if(MotorL & 0x100)				//Moving forward?
		{
			if(!(TSteer & 0x100))			//Turn Left?
			return MotorL;
			else
			{
				if((TSteer & 0xFF) >= (MotorL & 0xFF))
				return 0x100;
				else
				return (MotorL - (TSteer & 0xFF));
			}
		}
		else
		{
			if(!(TSteer & 0x100))
			{
				if((TSteer & 0xFF) < (MotorL & 0xFF))
				return (MotorL - (TSteer & 0xFF));
				else
				return 0;
			}
			else
			return MotorL;
		}
	}
	else
	{
		if(TSteer & 0xFF)
		{
			if(!(TSteer & 0x100))      //Turn left if true
			return (0x100 | TSteer);
			else
			return (0xFF & TSteer);
		}
		else
		return 0;
	}
}
unsigned int offsetForJoy(unsigned int data)
{
	unsigned int t;
	if(data >= 0x80)
	{
		data-=0x80;
		t = 0x100;
	}
	else
	{
		data-=0x80;
		data = ~data;
		t = 0;
	}
	data = (data << 1) | 0x01;
	if(data > 0xFA)
	data = 0xFF;
	if(data < 0x05)
	data = 0;
	data&=0xFF;
	data|=t;
	return data;
}
unsigned char computeRelayState(unsigned int data,uint8_t s1,uint8_t s2)
{	
	if((data & 0xFF) > 0x20)
	{
		if(s1)
		{
			if(!(data & 0x100))
			return 1;
		}
		if(s2)
		{
			if(data & 0x100)
			return 1;
		}
		else{
			return 0;
		}
	}
}
