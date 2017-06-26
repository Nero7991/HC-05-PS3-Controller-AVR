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

#include "BTD.h"


extern volatile uint8_t EVENT_RECEIVED;
int main(void)
{
    //Init_PWM_T0(1);
	//Init_PWM_T1(1);
    //Init_PWM_T2(1);
	Init_PWM_T3(1);
	Init_T4(0x01);
	//fanSpeed(10,10,10,10);
	USART1_Init(25);		//Initialize USART1 with baud 19200
	USART0_Init(25);		//Initialize USART0 with baud 19200
	for(int i = 0; i<3;i++){
		printStringCRNL1("Waiting");
		while(!EVENT_RECEIVED){
			
		}
		EVENT_RECEIVED = 0;
		printStringCRNL1("Ahead");
		}
	//_delay_ms(1);
	
	BTD b;
	b.hci_reset();
	while (!EVENT_RECEIVED);
	EVENT_RECEIVED = 0;
	b.hci_set_local_name("Oren");
	while(!EVENT_RECEIVED);
	EVENT_RECEIVED = 0;
	b.hci_write_scan_enable();
	
    while (1) 
    {
		//relayUSART();
		//printStringCRNL0("AT");
    }
}



