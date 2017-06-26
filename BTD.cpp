/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "BTD.h"
extern 
// To enable serial debugging see "settings.h"
//#define EXTRADEBUG // Uncomment to get even more debugging data
uint8_t BTD::min(uint8_t a, uint8_t b){
	return a<b?a:b;
}
void BTD::HCI_event_task() {
	
	switch(hcibuf[0]) { // Switch on event type
		case EV_COMMAND_COMPLETE:
		if(!hcibuf[5]) { // Check if command succeeded
			hci_set_flag(HCI_FLAG_CMD_COMPLETE); // Set command complete flag
			if((hcibuf[3] == 0x01) && (hcibuf[4] == 0x10)) { // Parameters from read local version information
				hci_version = hcibuf[6]; // Used to check if it supports 2.0+EDR - see http://www.bluetooth.org/Technical/AssignedNumbers/hci.htm
				hci_set_flag(HCI_FLAG_READ_VERSION);
				} else if((hcibuf[3] == 0x09) && (hcibuf[4] == 0x10)) { // Parameters from read local bluetooth address
				for(uint8_t i = 0; i < 6; i++)
				my_bdaddr[i] = hcibuf[6 + i];
				hci_set_flag(HCI_FLAG_READ_BDADDR);
			}
		}
		break;

		case EV_COMMAND_STATUS:
		if(hcibuf[2]) { // Show status on serial if not OK
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nHCI Command Failed: "), 0x80);
			D_PrintHex<uint8_t > (hcibuf[2], 0x80);
			#endif
		}
		break;

		case EV_INQUIRY_COMPLETE:
		if(inquiry_counter >= 5 && (pairWithWii || pairWithHIDDevice)) {
			inquiry_counter = 0;
			#ifdef DEBUG_UART_HOST
			if(pairWithWii)
			Notify(PSTR("\r\nCouldn't find Wiimote"), 0x80);
			else
			Notify(PSTR("\r\nCouldn't find HID device"), 0x80);
			#endif
			connectToWii = false;
			pairWithWii = false;
			connectToHIDDevice = false;
			pairWithHIDDevice = false;
			hci_state = HCI_SCANNING_STATE;
		}
		inquiry_counter++;
		break;

		case EV_INQUIRY_RESULT:
		if(hcibuf[2]) { // Check that there is more than zero responses
			#ifdef EXTRADEBUG
			Notify(PSTR("\r\nNumber of responses: "), 0x80);
			Notify(hcibuf[2], 0x80);
			#endif
			for(uint8_t i = 0; i < hcibuf[2]; i++) {
				uint8_t offset = 8 * hcibuf[2] + 3 * i;

				for(uint8_t j = 0; j < 3; j++)
				classOfDevice[j] = hcibuf[j + 4 + offset];

				#ifdef EXTRADEBUG
				Notify(PSTR("\r\nClass of device: "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[2], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[1], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex<uint8_t > (classOfDevice[0], 0x80);
				#endif

				if(pairWithWii && classOfDevice[2] == 0x00 && (classOfDevice[1] & 0x05) && (classOfDevice[0] & 0x0C)) { // See http://wiibrew.org/wiki/Wiimote#SDP_information
					checkRemoteName = true; // Check remote name to distinguish between the different controllers

					for(uint8_t j = 0; j < 6; j++)
					disc_bdaddr[j] = hcibuf[j + 3 + 6 * i];

					hci_set_flag(HCI_FLAG_DEVICE_FOUND);
					break;
					} else if(pairWithHIDDevice && (classOfDevice[1] & 0x05) && (classOfDevice[0] & 0xC8)) { // Check if it is a mouse, keyboard or a gamepad - see: http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
					#ifdef DEBUG_UART_HOST
					if(classOfDevice[0] & 0x80)
					Notify(PSTR("\r\nMouse found"), 0x80);
					if(classOfDevice[0] & 0x40)
					Notify(PSTR("\r\nKeyboard found"), 0x80);
					if(classOfDevice[0] & 0x08)
					Notify(PSTR("\r\nGamepad found"), 0x80);
					#endif

					for(uint8_t j = 0; j < 6; j++)
					disc_bdaddr[j] = hcibuf[j + 3 + 6 * i];

					hci_set_flag(HCI_FLAG_DEVICE_FOUND);
					break;
				}
			}
		}
		break;

		case EV_CONNECT_COMPLETE:
		hci_set_flag(HCI_FLAG_CONNECT_EVENT);
		if(!hcibuf[2]) { // Check if connected OK
			#ifdef EXTRADEBUG
			Notify(PSTR("\r\nConnection established"), 0x80);
			#endif
			hci_handle = hcibuf[3] | ((hcibuf[4] & 0x0F) << 8); // Store the handle for the ACL connection
			hci_set_flag(HCI_FLAG_CONNECT_COMPLETE); // Set connection complete flag
			} else {
			hci_state = HCI_CHECK_DEVICE_SERVICE;
			#ifdef DEBUG_USB_HOST
			Notify(PSTR("\r\nConnection Failed: "), 0x80);
			D_PrintHex<uint8_t > (hcibuf[2], 0x80);
			#endif
		}
		break;

		case EV_DISCONNECT_COMPLETE:
		if(!hcibuf[2]) { // Check if disconnected OK
			hci_set_flag(HCI_FLAG_DISCONNECT_COMPLETE); // Set disconnect command complete flag
			hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE); // Clear connection complete flag
		}
		break;

		case EV_REMOTE_NAME_COMPLETE:
		if(!hcibuf[2]) { // Check if reading is OK
			for(uint8_t i = 0; i < min(sizeof (remote_name), sizeof (hcibuf) - 9); i++) {
				remote_name[i] = hcibuf[9 + i];
				if(remote_name[i] == '\0') // End of string
				break;
			}
			// TODO: Altid sæt '\0' i remote name!
			hci_set_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
		}
		break;

		case EV_INCOMING_CONNECT:
		for(uint8_t i = 0; i < 6; i++)
		disc_bdaddr[i] = hcibuf[i + 2];

		for(uint8_t i = 0; i < 3; i++)
		classOfDevice[i] = hcibuf[i + 8];

		if((classOfDevice[1] & 0x05) && (classOfDevice[0] & 0xC8)) { // Check if it is a mouse, keyboard or a gamepad
			#ifdef DEBUG_USB_HOST
			if(classOfDevice[0] & 0x80)
			Notify(PSTR("\r\nMouse is connecting"), 0x80);
			if(classOfDevice[0] & 0x40)
			Notify(PSTR("\r\nKeyboard is connecting"), 0x80);
			if(classOfDevice[0] & 0x08)
			Notify(PSTR("\r\nGamepad is connecting"), 0x80);
			#endif
			incomingHIDDevice = true;
		}

		#ifdef EXTRADEBUG
		Notify(PSTR("\r\nClass of device: "), 0x80);
		D_PrintHex<uint8_t > (classOfDevice[2], 0x80);
		Notify(PSTR(" "), 0x80);
		D_PrintHex<uint8_t > (classOfDevice[1], 0x80);
		Notify(PSTR(" "), 0x80);
		D_PrintHex<uint8_t > (classOfDevice[0], 0x80);
		#endif
		hci_set_flag(HCI_FLAG_INCOMING_REQUEST);
		break;

		case EV_PIN_CODE_REQUEST:
		if(pairWithWii) {
			#ifdef DEBUG_USB_HOST
			Notify(PSTR("\r\nPairing with Wiimote"), 0x80);
			#endif
			hci_pin_code_request_reply();
			} else if(btdPin != NULL) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nBluetooth pin is set too: "), 0x80);
			NotifyStr(btdPin, 0x80);
			#endif
			hci_pin_code_request_reply();
			} else {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nNo pin was set"), 0x80);
			#endif
			hci_pin_code_negative_request_reply();
		}
		break;

		case EV_LINK_KEY_REQUEST:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nReceived Key Request"), 0x80);
		#endif
		hci_link_key_request_negative_reply();
		break;

		case EV_AUTHENTICATION_COMPLETE:
		if(!hcibuf[2]) { // Check if pairing was successful
			if(pairWithWii && !connectToWii) {
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nPairing successful with Wiimote"), 0x80);
				#endif
				connectToWii = true; // Used to indicate to the Wii service, that it should connect to this device
				} else if(pairWithHIDDevice && !connectToHIDDevice) {
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nPairing successful with HID device"), 0x80);
				#endif
				connectToHIDDevice = true; // Used to indicate to the BTHID service, that it should connect to this device
			}
			} else {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nPairing Failed: "), 0x80);
			D_PrintHex<uint8_t > (hcibuf[2], 0x80);
			#endif
			hci_disconnect(hci_handle);
			hci_state = HCI_DISCONNECT_STATE;
		}
		break;
		/* We will just ignore the following events */
		case EV_NUM_COMPLETE_PKT:
		case EV_ROLE_CHANGED:
		case EV_PAGE_SCAN_REP_MODE:
		case EV_LOOPBACK_COMMAND:
		case EV_DATA_BUFFER_OVERFLOW:
		case EV_CHANGE_CONNECTION_LINK:
		case EV_MAX_SLOTS_CHANGE:
		case EV_QOS_SETUP_COMPLETE:
		case EV_LINK_KEY_NOTIFICATION:
		case EV_ENCRYPTION_CHANGE:
		case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
		break;
		#ifdef EXTRADEBUG
		default:
		if(hcibuf[0] != 0x00) {
			Notify(PSTR("\r\nUnmanaged HCI Event: "), 0x80);
			D_PrintHex<uint8_t > (hcibuf[0], 0x80);
		}
		break;
		#endif
	} // Switch

#ifdef EXTRADEBUG
else {
	Notify(PSTR("\r\nHCI event error: "), 0x80);
}
#endif
}

void Notify(char* str,uint8_t num){
	printString1(str);
}

/************************************************************/
/*                    HCI Commands                        */

/************************************************************/
void BTD::HCI_Command(uint8_t* data, uint16_t nbytes) {
	hci_clear_flag(HCI_FLAG_CMD_COMPLETE);
	USART0_Transmit(0x01);  //HCI UART Event header
	for(uint8_t i = 0; i < nbytes; i++){
		USART0_Transmit(data[i]);
	}
}

void BTD::hci_reset() {
        hci_event_flag = 0; // Clear all the flags
        hcibuf[0] = 0x03; // HCI OCF = 3
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_write_scan_enable() {
        hci_clear_flag(HCI_FLAG_INCOMING_REQUEST);
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        if(btdName != NULL)
                hcibuf[3] = 0x03; // Inquiry Scan enabled. Page Scan enabled.
        else
                hcibuf[3] = 0x02; // Inquiry Scan disabled. Page Scan enabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_write_scan_disable() {
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        hcibuf[3] = 0x00; // Inquiry Scan disabled. Page Scan disabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_read_bdaddr() {
        hci_clear_flag(HCI_FLAG_READ_BDADDR);
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_read_local_version_information() {
        hci_clear_flag(HCI_FLAG_READ_VERSION);
        hcibuf[0] = 0x01; // HCI OCF = 1
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_accept_connection() {
        hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE);
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x07; // parameter length 7
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x00; // Switch role to master

        HCI_Command(hcibuf, 10);
}

void BTD::hci_remote_name() {
        hci_clear_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
        hcibuf[0] = 0x19; // HCI OCF = 19
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0A; // parameter length = 10
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x01; // Page Scan Repetition Mode
        hcibuf[10] = 0x00; // Reserved
        hcibuf[11] = 0x00; // Clock offset - low byte
        hcibuf[12] = 0x00; // Clock offset - high byte

        HCI_Command(hcibuf, 13);
}

void BTD::hci_set_local_name(const char* name) {
        hcibuf[0] = 0x13; // HCI OCF = 13
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = strlen(name) + 1; // parameter length = the length of the string + end byte
        uint8_t i;
        for(i = 0; i < strlen(name); i++)
                hcibuf[i + 3] = name[i];
        hcibuf[i + 3] = 0x00; // End of string

        HCI_Command(hcibuf, 4 + strlen(name));
}

void BTD::hci_inquiry() {
        hci_clear_flag(HCI_FLAG_DEVICE_FOUND);
        hcibuf[0] = 0x01;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x05; // Parameter Total Length = 5
        hcibuf[3] = 0x33; // LAP: Genera/Unlimited Inquiry Access Code (GIAC = 0x9E8B33) - see https://www.bluetooth.org/Technical/AssignedNumbers/baseband.htm
        hcibuf[4] = 0x8B;
        hcibuf[5] = 0x9E;
        hcibuf[6] = 0x30; // Inquiry time = 61.44 sec (maximum)
        hcibuf[7] = 0x0A; // 10 number of responses

        HCI_Command(hcibuf, 8);
}

void BTD::hci_inquiry_cancel() {
        hcibuf[0] = 0x02;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x00; // Parameter Total Length = 0

        HCI_Command(hcibuf, 3);
}

void BTD::hci_connect() {
        hci_connect(disc_bdaddr); // Use last discovered device
}

void BTD::hci_connect(uint8_t *bdaddr) {
        hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE | HCI_FLAG_CONNECT_EVENT);
        hcibuf[0] = 0x05;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0D; // parameter Total Length = 13
        hcibuf[3] = bdaddr[0]; // 6 octet bdaddr (LSB)
        hcibuf[4] = bdaddr[1];
        hcibuf[5] = bdaddr[2];
        hcibuf[6] = bdaddr[3];
        hcibuf[7] = bdaddr[4];
        hcibuf[8] = bdaddr[5];
        hcibuf[9] = 0x18; // DM1 or DH1 may be used
        hcibuf[10] = 0xCC; // DM3, DH3, DM5, DH5 may be used
        hcibuf[11] = 0x01; // Page repetition mode R1
        hcibuf[12] = 0x00; // Reserved
        hcibuf[13] = 0x00; // Clock offset
        hcibuf[14] = 0x00; // Invalid clock offset
        hcibuf[15] = 0x00; // Do not allow role switch

        HCI_Command(hcibuf, 16);
}

void BTD::hci_pin_code_request_reply() {
        hcibuf[0] = 0x0D; // HCI OCF = 0D
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x17; // parameter length 23
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        if(pairWithWii) {
                hcibuf[9] = 6; // Pin length is the length of the Bluetooth address
                if(pairWiiUsingSync) {
#ifdef DEBUG_USB_HOST
                        Notify(PSTR("\r\nPairing with Wii controller via SYNC"), 0x80);
#endif
                        for(uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = my_bdaddr[i]; // The pin is the Bluetooth dongles Bluetooth address backwards
                } else {
                        for(uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = disc_bdaddr[i]; // The pin is the Wiimote's Bluetooth address backwards
                }
                for(uint8_t i = 16; i < 26; i++)
                        hcibuf[i] = 0x00; // The rest should be 0
        } else {
                hcibuf[9] = strlen(btdPin); // Length of pin
                uint8_t i;
                for(i = 0; i < strlen(btdPin); i++) // The maximum size of the pin is 16
                        hcibuf[i + 10] = btdPin[i];
                for(; i < 16; i++)
                        hcibuf[i + 10] = 0x00; // The rest should be 0
        }

        HCI_Command(hcibuf, 26);
}

void BTD::hci_pin_code_negative_request_reply() {
        hcibuf[0] = 0x0E; // HCI OCF = 0E
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_link_key_request_negative_reply() {
        hcibuf[0] = 0x0C; // HCI OCF = 0C
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_authentication_request() {
        hcibuf[0] = 0x11; // HCI OCF = 11
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x02; // parameter length = 2
        hcibuf[3] = (uint8_t)(hci_handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((hci_handle >> 8) & 0x0F); //connection handle - high byte

        HCI_Command(hcibuf, 5);
}

void BTD::hci_disconnect(uint16_t handle) { // This is called by the different services
        hci_clear_flag(HCI_FLAG_DISCONNECT_COMPLETE);
        hcibuf[0] = 0x06; // HCI OCF = 6
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = (uint8_t)(handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); //connection handle - high byte
        hcibuf[5] = 0x13; // reason

        HCI_Command(hcibuf, 6);
}

void BTD::hci_write_class_of_device() { // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
        hcibuf[0] = 0x24; // HCI OCF = 24
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = 0x04; // Robot
        hcibuf[4] = 0x08; // Toy
        hcibuf[5] = 0x00;

        HCI_Command(hcibuf, 6);
}
/*******************************************************************
 *                                                                 *
 *                        HCI ACL Data Packet                      *
 *                                                                 *
 *   buf[0]          buf[1]          buf[2]          buf[3]
 *   0       4       8    11 12      16              24            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |      HCI Handle       |PB |BC |       Data Total Length       |   HCI ACL Data Packet
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[4]          buf[5]          buf[6]          buf[7]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |            Length             |          Channel ID           |   Basic L2CAP header
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[8]          buf[9]          buf[10]         buf[11]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |     Code      |  Identifier   |            Length             |   Control frame (C-frame)
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.   (signaling packet format)
 */
/************************************************************/
/*                    L2CAP Commands                        */

/************************************************************/
void BTD::L2CAP_Command(uint16_t handle, uint8_t* data, uint8_t nbytes, uint8_t channelLow, uint8_t channelHigh) {
        uint8_t buf[8 + nbytes];
        buf[0] = (uint8_t)(handle & 0xff); // HCI handle with PB,BC flag
        buf[1] = (uint8_t)(((handle >> 8) & 0x0f) | 0x20);
        buf[2] = (uint8_t)((4 + nbytes) & 0xff); // HCI ACL total data length
        buf[3] = (uint8_t)((4 + nbytes) >> 8);
        buf[4] = (uint8_t)(nbytes & 0xff); // L2CAP header: Length
        buf[5] = (uint8_t)(nbytes >> 8);
        buf[6] = channelLow;
        buf[7] = channelHigh;

        for(uint16_t i = 0; i < nbytes; i++) // L2CAP C-frame
                buf[8 + i] = data[i];
	 // This small delay prevents it from overflowing if it fails
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nError sending L2CAP message: 0x"), 0x80);
                D_PrintHex<uint8_t > (rcode, 0x80);
                Notify(PSTR(" - Channel ID: "), 0x80);
                D_PrintHex<uint8_t > (channelHigh, 0x80);
                Notify(PSTR(" "), 0x80);
                D_PrintHex<uint8_t > (channelLow, 0x80);
#endif
        
}

void BTD::l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t* scid, uint16_t psm) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = (uint8_t)(psm & 0xff); // PSM
        l2capoutbuf[5] = (uint8_t)(psm >> 8);
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];
        l2capoutbuf[8] = result; // Result: Pending or Success
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00; // No further information
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t* dcid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = 0x00; // Flags
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x01; // Config Opt: type = MTU (Maximum Transmission Unit) - Hint
        l2capoutbuf[9] = 0x02; // Config Opt: length
        l2capoutbuf[10] = 0xFF; // MTU
        l2capoutbuf[11] = 0xFF;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x0A; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = scid[0]; // Source CID
        l2capoutbuf[5] = scid[1];
        l2capoutbuf[6] = 0x00; // Flag
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x00; // Result
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x01; // Config
        l2capoutbuf[11] = 0x02;
        l2capoutbuf[12] = 0xA0;
        l2capoutbuf[13] = 0x02;

        L2CAP_Command(handle, l2capoutbuf, 14);
}

void BTD::l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh) {
        l2capoutbuf[0] = L2CAP_CMD_INFORMATION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = infoTypeLow;
        l2capoutbuf[5] = infoTypeHigh;
        l2capoutbuf[6] = 0x00; // Result = success
        l2capoutbuf[7] = 0x00; // Result = success
        l2capoutbuf[8] = 0x00;
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00;
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

/* PS3 Commands - only set Bluetooth address is implemented in this library */
void BTD::setBdaddr(uint8_t* bdaddr) {
        /* Set the internal Bluetooth address */
        uint8_t buf[8];
        buf[0] = 0x01;
        buf[1] = 0x00;

        for(uint8_t i = 0; i < 6; i++)
                buf[i + 2] = bdaddr[5 - i]; // Copy into buffer, has to be written reversed, so it is MSB first
}

void BTD::setMoveBdaddr(uint8_t* bdaddr) {
        /* Set the internal Bluetooth address */
        uint8_t buf[11];
        buf[0] = 0x05;
        buf[7] = 0x10;
        buf[8] = 0x01;
        buf[9] = 0x02;
        buf[10] = 0x12;

        for(uint8_t i = 0; i < 6; i++)
                buf[i + 1] = bdaddr[i];
}
