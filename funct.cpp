/*
 * funct.cpp
 *
 * Created: 6/25/2017 12:26:19 PM
 *  Author: orencollaco
 */ 

#include "func.h"

const unsigned char FadeLookUp[] PROGMEM =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
	0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27, 0x29, 0x2B, 0x2C,
	0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E, 0x40, 0x43, 0x45, 0x47, 0x4A, 0x4C, 0x4F,
	0x51, 0x54, 0x57, 0x59, 0x5C, 0x5F, 0x62, 0x64, 0x67, 0x6A, 0x6D, 0x70, 0x73, 0x76, 0x79, 0x7C,
	0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8E, 0x91, 0x94, 0x97, 0x9A, 0x9C, 0x9F, 0xA2, 0xA5, 0xA7, 0xAA,
	0xAD, 0xAF, 0xB2, 0xB4, 0xB7, 0xB9, 0xBB, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
	0xD0, 0xD2, 0xD3, 0xD5, 0xD7, 0xD8, 0xDA, 0xDB, 0xDD, 0xDE, 0xDF, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5,
	0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF1, 0xF2,
	0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8,
	0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC,
	0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,
0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF};

volatile uint64_t time;
volatile uint16_t Event_Length = 299,Buf_top = 299;
volatile uint8_t hcibuf[300],tx1buf[300];//,acibuf[500];
volatile uint8_t tx1_ph = 0,tx1_pl = 0;
volatile uint8_t HCI_AWAKE_RECEIVED,EVENT_RECEIVED = 0,ACL_DATA_RECEIVED = 0;

#define START 0x08

void relayUSART(){
	char temp;
	if(UCSR1A & (1<<RXC1)){
		temp = UDR1;
		USART0_Transmit(temp);
		USART1_Transmit(temp);
	}
	if(UCSR0A & (1<<RXC0)){
		USART1_Transmit(UDR0);
	}
}

void BT_Init(){
	printStringCRNL0("AT");
	for(int i = 1;i++;1 < 10);
}

void Init_T4(unsigned char prescale){
	prescale &= 0x07;
	prescale |= 0x10;
	TCCR4A = 0x00;
	TCCR4B = prescale;
	ICR1 = 8000;
	sei();
	TIMSK4 = 0x20;
}

void Init_PWM_T0(unsigned char prescale){
	TCCR0A = 0xA1;
	prescale &= 0x07;
	TCCR0B |= prescale;
	DDRD |= 0xE0;
	DDRB |= 0x01;
}

void Init_PWM_T1(unsigned char prescale){
	TCCR0A = 0xA1;
	prescale &= 0x07;
	TCCR0B |= prescale;
	DDRB |= 0x17;
}

void Init_PWM_T2(unsigned char prescale){
	TCCR2A = 0xA1;
	prescale &= 0x07;
	TCCR2B |= prescale;
	DDRD |= 0x08;
	DDRB |= 0x08;
	
}

void Init_PWM_T3(unsigned char prescale){
	TCCR3A = 0xA1;
	prescale &= 0x07;
	TCCR3B |= prescale;
	DDRD |= 0x04;
	PORTD |= 0X04;
}

void Init_CTC_T2(unsigned char prescale){
	TCCR2A = 0x52;
	prescale &= 0x07;
	TCCR2B |= prescale;
	DDRD |= 0x08;
	DDRB |= 0x08;
}



void fanSpeed(unsigned char a0, unsigned char a2, unsigned char b0, unsigned char b2){
	OCR0A = a0;
	OCR0B = b0;
	OCR1A = a2;
	OCR1B = b2;
}

void holdPosition(unsigned int x,unsigned int y){
	static unsigned char max = 0x30,min = 0;//,step = 5,a0,a2,b0,b2,ref = 0,dy,dx,k = 1;
	if(UCSR0A & (1<<RXC0)){
		char *p = receiveStringCRNL();
		if(strcmp(p,"=\r\n")==0 || strcmp(p,"=")==0)
		{
			max += 10;
		}
		if(strcmp(p,"-\r\n")==0 || strcmp(p,"-")==0)
		{
			max -= 10;
		}
	}
	unsigned int xt,yt;
	xt = (unsigned int)(I2C_Read(0x68,0x3F) << 8);
	xt |= (unsigned int)(I2C_Read(0x68,0x40));
	yt = (unsigned int)(I2C_Read(0x68,0x3D) << 8);
	yt |= (unsigned int)(I2C_Read(0x68,0x3E));
	xt += 0x3000;
	yt += 0x3000;
	x += 0x3000;
	y += 0x3000;
}

void transmitSensorData(){
	unsigned char i;
	printString1("\033");
	printString1(" Accelerometer : ");
	i = 0x3B;
	while( i < 0x41 ){
		
		printString1(hexToString(I2C_Read(0x68,i)));
		printChar(' ');
		i += 1;
	}
	printString1(" Gyroscope : ");
	i = 0x43;
	while( i < 0x49 ){
		printString1(hexToString(I2C_Read(0x68,i)));
		printChar(' ');
		i += 2;
	}
	printChar('\n');
}

char* receiveStringCRNL(){
	unsigned char i=0,flag;
	static char str[50];
	while(1){
		while ((!(UCSR0A & (1<<RXC0))) && (!(TIFR1 & (1<<ICF1))));
		if(!(UCSR0A & (1<<RXC0))){
			str[i] = 0;
			return str;
		}
		TIFR1 = (1<<ICF1);
		setTimer1(0,0);
		TCNT1 = 0;
		str[i] = UDR0;
		i += 1;
		}
}

void printStringNewline(char *p){
	USART0_Transmit('\n');
	while(*p != 0){
		USART0_Transmit(*p);
		p++;
	}
}

void printStringCRNL0(const char *p){
	while(*p != 0){
		USART0_Transmit(*p);
		p++;
	}
	USART0_Transmit('\r');
	USART0_Transmit('\n');
}

void printStringCRNL1(const char *p){
	USART1_Transmit('\r');
	USART1_Transmit('\n');
	while(*p != 0){
		USART1_Transmit(*p);
		p++;
	}
}

char* hexToString(unsigned char data){
	static char ascii[3],temp;
	temp = data;
	data = data>>4;
	ascii[0] = hexToASCII(data);
	temp &= 0x0F;
	ascii[1] = hexToASCII(temp);
	ascii[2] = '\0';
	return ascii;
}
	
unsigned char charToHex(char *p){
	unsigned char t;
		if(*p >= 30 && *p <= 39)
		{
			*p -= 0x30;
			t = (*p << 4);
		}
		p++;
		if(*p >= 30 && *p <= 39)
		{
			*p -= 0x30;
			t = *p;
		}
	
	return t;
}
	
void setTimer1(unsigned int count, unsigned char prescale){
	TCCR1A = 0x80;
	prescale &= 0x07;
	TCCR1B = 0x10;
	TCCR1B |= prescale;
	ICR1 = count;
	OCR1A = 0.03 * count;
	DDRB |= 0x02;
}
	
unsigned char hexToASCII(unsigned char data){
	if(data > 0x09){
		data -= 0x0A;
		data += 0x41;
		return data;
	}
	else{
		data += 0x30;
		return data;
	}
}

void printString1(const char *p){
	while(*p != 0){
		USART1_Transmit(*p);
		p++;
	}
}



void printChar(unsigned char data){
	USART1_Transmit(data);
}

void USART0_Init(unsigned int ubrr) {
/*Set baud rate */
UBRR0H = (unsigned char)(ubrr>>8); 
UBRR0L = (unsigned char)ubrr;
/*Enable receiver and transmitter */
UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
/* Set frame format : 8 data bits, 2 stop bits */
UCSR0C = (1 << USBS0) | (3 << UCSZ00);
DDRB |= 0x40;
DDRB &= ~0x80;
}

void USART0_Transmit(unsigned char data){
	/* Wait for empty transmit buffer */
	
	while(!(UCSR0A & (1 << UDRE0)));
	/* Put the data into the buffer, data 
	is sent serially once written */
	UDR0 = data;
}

unsigned char USART0_Receive() {
/* Wait for data to be received */
//setTimer1(3000,1);
while (!(UCSR0A & (1<<RXC0)));
setTimer1(0,0);
/* Get and return received data from buffer */ 
return UDR0;
}

void USART1_Init(unsigned int ubrr) {
/*Set baud rate */
UBRR1H = (unsigned char)(ubrr>>8); 
UBRR1L = (unsigned char)ubrr;
/*Enable receiver and transmitter */
UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << UDRIE1);
/* Set frame format : 8 data bits, 2 stop bits */
UCSR1C = (1 << USBS1) | (3 << UCSZ10);
}

void USART1_Disable(){
	UCSR1B = 0;
}

void USART1_Transmit(unsigned char data){
	/* Wait for empty transmit buffer */
	//while(!(UCSR1A & (1 << UDRE1)));
	/* Put the data into the buffer, data 
	is sent serially once written */
	//UDR1 = data;
	if((UCSR1A & (1 << UDRE1)) && (tx1_ph == 0)){
		UDR1 = data;
	}
	else{
		if(!(UCSR1B & (1<<UDRIE1)))
			UCSR1B |= (1 << UDRIE1);
		if(tx1_pl == tx1_ph){
			tx1_ph = 0;
			tx1_pl = 0;
		}
		if(tx1_ph < 255){
			tx1buf[tx1_ph] = data;
			tx1_ph += 1;
		}
	}
}

unsigned char USART1_Receive() {
/* Wait for data to be received */
//setTimer1(3000,1);
while (!(UCSR1A & (1<<RXC0)));
/* Get and return received data from buffer */ 
return UDR1;
}

void I2C_Init(unsigned char twbr, unsigned char twsr){
	/* Set bit rate register */
	TWBR0 = twbr;
	/* Mask status bits */
	twsr &= 0x03;
	TWSR0 |= twsr;
	/* Enable TWI Interface */
}
void ERROR(unsigned char twsr){
	USART0_Transmit(twsr);
	printStringNewline(" I2C  Communication Error ");
	I2C_Write(0x68,0x6B,0);
}

unsigned char I2C_Read(unsigned char daddr, unsigned char raddr){
	unsigned char data;
	/* Transmit Start condition */
	TWCR0 = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT); 
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != START)
		ERROR(TWSR0 & 0xF8);
	TWDR0 = daddr << 1; 
	/* Transmit device address & R/W(w) bit */
	TWCR0 = (1<<TWINT) | (1<<TWEN); 
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != MTA_SLAW_ACK) 
		ERROR(TWSR0 & 0xF8);
	TWDR0 = raddr;
	/* Transmit device register address */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != MTD_SLAW_ACK)
	ERROR(TWSR0 & 0xF8);
	/* Transmit Start condition */
	TWCR0 = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != RESTART)
	ERROR(TWSR0 & 0xF8);
	/* Received Mode(SLA+R) */
	TWDR0 = daddr << 1 | 0x01;   
	/* Transmit device address */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != MTDS_SLAR_ACK)
	ERROR(TWSR0 & 0xF8);
	/* Receive device data */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	setTimer1(5000,3);
	while((!(TWCR0 & (1<<TWINT))) && (!(TIFR1 & (1<<ICF1))));
	TIFR1 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	if ((TWSR0 & 0xF8) != MTDR_SLAR_NACK)
	ERROR(TWSR0 & 0xF8);
	data = TWDR0;
	/* Transmit STOP */
	TWCR0 = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	setTimer1(5000,3);
	while((TWCR0 & (1<<TWSTO)) && (!(TIFR1 & (1<<ICF1))));
	TIFR0 = (1<<ICF1);
	setTimer1(0,0);
	TCNT1 = 0;
	return data;
}
void I2C_Write(unsigned char daddr, unsigned char raddr, unsigned char data){
	/* Transmit Start condition */
	TWCR0 = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	while(!(TWCR0 & (1<<TWINT)));
	if ((TWSR0 & 0xF8) != START)
	ERROR(TWSR0 & 0xF8);
	TWDR0 = daddr << 1;
	/* Transmit device address & R/W(w) bit */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
	if ((TWSR0 & 0xF8) != MTA_SLAW_ACK)
	ERROR(TWSR0 & 0xF8);
	TWDR0 = raddr;
	/* Transmit device register address */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
	if ((TWSR0 & 0xF8) != MTD_SLAW_ACK)
	ERROR(TWSR0 & 0xF8);
	TWDR0 = data;
	/* Transmit data */
	TWCR0 = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR0 & (1<<TWINT)));
	if ((TWSR0 & 0xF8) != MTD_SLAW_ACK)
	ERROR(TWSR0 & 0xF8);
	/* Transmit STOP */
	TWCR0 = (1 << TWSTO) | (1<<TWINT) | (1<<TWEN);
	while((TWCR0 & (1<<TWSTO)));
}

void Notify(const char* str,uint8_t num){
	char i = 0;
	while(pgm_read_byte(str) != 0){
		USART1_Transmit(pgm_read_byte(str));
		str += 1;
	}
}

void D_PrintHex(uint8_t d,uint8_t num){
	printString1(hexToString(d));
}

void NotifyS(const char d, uint8_t num){
	USART1_Transmit(d);
}

void setThingsUp(){
	//Init_PWM_T0(1);
	//Init_PWM_T1(1);
	//Init_PWM_T2(1);
	Init_PWM_T3(1);
	Init_T4(0x05);
	//fanSpeed(10,10,10,10);
	USART1_Init(12);		//Initialize USART1 with baud 19200
	USART0_Init(25);		//Initialize USART0 with baud 19200
}

uint64_t millis(){
	return time;
}

void delay(uint16_t t){
	static uint64_t tp;
	tp = time + t;
	while(time != tp);
}

ISR(USART0_RX_vect){
	static uint8_t EVENT_HEADER_RECEIVED = 0,dtemp,Event_Head;
	static uint16_t count;
	dtemp = UDR0;
	#ifdef ULTRA_DEBUG
	USART1_Transmit(dtemp);
	#endif
	if(!EVENT_HEADER_RECEIVED && dtemp <= 0x04 && dtemp > 0x01){
		Event_Head = dtemp;
		EVENT_HEADER_RECEIVED = 1;
		EVENT_RECEIVED = 0;
		#ifdef UART_DEBUG
		switch(Event_Head){
			case HCI_EVENT_PACKET:
			printStringCRNL1("EHD");
			break;
			case HCI_ACL_DATA_PACKET:
			printStringCRNL1("AHD");
			break;
			case HCI_SCO_DATA_PACKET:
			printStringCRNL1("SHD");
			break;
		}
		#endif
		return;
	}
	
	if(EVENT_HEADER_RECEIVED && count < 300){
		switch(Event_Head){						//Switch on HCI Header type(Only UART)
			case HCI_ACL_DATA_PACKET:
			//aclbuf[count] = dtemp;
			if(count == 1 && count == 2){
				if(count == 1)
				Buf_top = dtemp << 8;
				if(count == 2)
				Buf_top = dtemp + 2;
				#ifdef UART_DEBUG
				printStringCRNL1("L = ");
				printString1(hexToString(dtemp));
				#endif
			}
			count += 1;
			if(count == Buf_top){
				Event_Length = Buf_top;
				Buf_top = 299;
				ACL_DATA_RECEIVED = 1;
				#ifdef UART_DEBUG
				//printStringCRNL1("EV1");
				#endif
				count = 0;
				EVENT_HEADER_RECEIVED = 0;
				#ifdef UART_DEBUG
				printStringCRNL1("DA");
				#endif
				return;
			}
			break;
			case HCI_SCO_DATA_PACKET:
			break;
			case HCI_EVENT_PACKET:
			hcibuf[count] = dtemp;
			if(count == 1){
				Buf_top = dtemp + 2;
				#ifdef UART_DEBUG
				printStringCRNL1("L = ");
				printString1(hexToString(dtemp));
				#endif
			}
			count += 1;
			if(count == Buf_top){
				Event_Length = Buf_top;
				Buf_top = 299;
				EVENT_RECEIVED = 1;
				#ifdef UART_DEBUG
				//printStringCRNL1("EV1");
				#endif
				count = 0;
				EVENT_HEADER_RECEIVED = 0;
				#ifdef UART_DEBUG
				printStringCRNL1("ET");
				#endif
				return;
			}
			break;
		}
	}
	if(count >= 300){
		printStringCRNL1("Buffer Overrun");
		
	}
}

ISR(USART1_UDRE_vect){
	//RTS ^= 1;
	//USART1_Transmit(tx1_ph);
	//USART1_Transmit(0xAA);
	//USART1_Transmit(tx1_pl);
	if(tx1_ph == 0){
		if(UCSR1B & (1 << UDRIE1))
		UCSR1B &= ~(1 << UDRIE1);
	}
	if(tx1_ph){
		UDR1 = tx1buf[tx1_pl];
		tx1_pl += 1;
		if(tx1_pl == tx1_ph){
			tx1_ph = 0;
			tx1_pl = 0;
		}
	}
}

ISR(TIMER4_CAPT_vect){
	//RTS ^= 1;
	static unsigned char i,up = 1,count = 0,d = 0;
	time += 1;	
	if(count > 30){
		count = 0;
		if( i < 224 && up){
			i += 1;
			OCR3B = pgm_read_byte(&(FadeLookUp[i]));
			if(d){
				OCR0A = OCR3B;
				OCR0B = 0;
				PORTB &= ~0x01;
				PORTD |= 0x80;
			}
			if(!d){
				OCR0B = OCR3B;
				OCR0A = 0;
				PORTB |= 0x01;
				PORTD &= ~0x80;
			}
		}
		if(i == 224 && up){
			up = 0;
		}
		if(i > 0 && !up){
			i -= 1;
			OCR3B = pgm_read_byte(&(FadeLookUp[i]));
			if(d){
				OCR0A = OCR3B;
				OCR0B = 0;
				PORTB &= ~0x01;
				PORTD |= 0x80;
			}
			if(!d){
				OCR0B = OCR3B;
				OCR0A = 0;
				PORTB |= 0x01;
				PORTD &= ~0x80;
			}
		}
		if(i == 0 && !up){
			up = 1;
			d ^= 1;
		}
	}
	count += 1;
}