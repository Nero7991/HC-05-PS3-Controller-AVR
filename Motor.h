/*
 * Motor.h
 *
 * Created: 6/30/2017 8:50:53 PM
 *  Author: orencollaco
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_
#include "Battery.h"

void Init_Motor();
void disable_Motor();
void updateMotorSpeed(uint8_t Gas,uint8_t Steer);
unsigned int offsetForJoy(unsigned int data);
unsigned char computeRelayState(unsigned int data,uint8_t s1,uint8_t s2);
unsigned int computeLeft(unsigned int Gas,unsigned int Steer);
unsigned int computeRight(unsigned int Gas,unsigned int Steer);



#endif /* MOTOR_H_ */