# HC_05_PS3_AVR
All the files wer made for ATmega328PB
This repo includes files that implement HCI Transport Layer over UART to inferface with a PS3 Controller uisng firmware modified HC-05. This is library is modified version of the USB_HOST_SHIELD 2.0 which already implements most of the stuff needed .
HC-05 was used in HCI mode by first flashing with RN-42 firmware and then using PSKEY's to modify its behaviour.
Following PSKEY's were modified.
1. PSKEY_VM_DISABLE -> 0x0001
2. PSKEY_ONCHIP_HCI_CLIENT -> 0x0000
3. PSKEY_HOST_INTERFACE -> UART H4
4. PSKEY_UART_BAUDRATE -> Same as USART on ATmega328PB

Progress:
All the HCI commands functions work.
When a complete event is received, EVENT_RECEIVED flag is set and EVENT is available in hcibuf. 

Refer to the main.cpp file for an example of how to use this library

Contact : Oren 
orencollaco97@gmail.com
