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
    setThingsUp();
	//printStringCRNL1("Waiting for Wake Event");
	for(char i = 0; i < 3 ;i++){
		while(!EVENT_RECEIVED);
			EVENT_RECEIVED = 0;
	}
	printStringCRNL1("HCI Wake Event Received");
	BTD b;
	PS3BT PSB(&b);
	b.Initialize();
	_delay_us(100);
	//b.Notify(PSTR("Hello"),01);
	b.hci_state = HCI_INIT_STATE;
	b.HCI_task();
    while (1) 
    {
		if(EVENT_RECEIVED){
			b.HCI_event_task();
			b.HCI_task();
		}
	}
}



