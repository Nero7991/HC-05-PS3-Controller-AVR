/*
 * MonsterTruck-BT-PS3.cpp
 *
 * Created: 6/28/2017 8:45:59 PM
 * Author : orencollaco
 */ 

/*
 * MonsterTruck BT2.cpp
 *
 * Created: 6/24/2017 8:43:32 PM
 * Author : orencollaco
 */ 


/*
 * MonsterTruck BT.c
 *
 * Created: 6/20/2017 4:59:38 PM
 * Author : orencollaco
 */ 

/*
 * ATmega328PB_PWM.c
 *
 * Created: 4/23/2017 6:37:39 PM
 * Author : orencollaco
 */ 

//#include "BTD.h"
#include "PS3BT.h"

extern volatile uint8_t EVENT_RECEIVED;
bool EVENT_SERVICED = 0,RESPONSE_AWAITING = 0;
//extern volatile uint16_t poll_t;
int main(void)
{
	DDRD = 0xFF;
	DDRB = 0xFF;
	DDRE = 0xFF;
	DDRC = 0xFF;
	PORTC = 0;
	PORTE = 0;
	PORTD = 0;
	PORTB = 0;
	setIndicatorCount(0);
	bool IsFirstTime = 1;
	setThingsUp();
	//Init_Batt();
	//Init_Motor();
	printStringCRNL1("MonstruckTruck Logic Board Firmware v1.0");
	printStringCRNL1("Waiting for Wake Event");
	uint16_t t = millis();
	for(uint8_t i = 0; i < 3; i++){
		while(!EVENT_RECEIVED && ((millis() - t) < 5000));
		EVENT_RECEIVED = 0;
	}
	printStringCRNL1("HCI Wake Event Received");
	printStringCRNL1("HC-05 Alive!");
	setIndicatorCount(2);
	BTD b;
	PS3BT PS3(&b);
	b.Initialize();
	b.hci_state = HCI_INIT_STATE;
	b.HCI_task();
	while (1) 
    {
		if(!PS3.PS3Connected){
			setIndicatorCount(2);
		}
		//checkBatt();
		b.BPoll();
		if (PS3.PS3Connected) {
			setIndicatorCount(8);
			if (PS3.getButtonClick(PS)) {
				printStringCRNL1(PSTR("PS Button"));
				PS3.disconnect();
			}
			updateMotorSpeed(PS3.getAnalogHat(RightHatY),PS3.getAnalogHat(LeftHatX));
		}
	}
}


