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

BTD::BTD() :
connectToWii(false),
pairWithWii(false),
connectToHIDDevice(false),
pairWithHIDDevice(false),
bAddress(0), 
qNextPollTime(0), // Reset NextPollTime
pollInterval(0),
bPollEnable(true) // Don't start polling before dongle is connected
{
	btService = NULL;
	//Initialize(); // Set all variables, endpoint structs etc. to default values
}

void BTD::Initialize() {
	btService->Reset(); // Reset all Bluetooth services
	connectToWii = false;
	incomingWii = false;
	connectToHIDDevice = false;
	incomingHIDDevice = false;
	incomingPS4 = false;
	bAddress = 0; // Clear device address
	qNextPollTime = 0; // Reset next poll time
	pollInterval = 3;
	bPollEnable = true; // Don't start polling before dongle is connected
	hci_state = HCI_INIT_STATE;
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
			D_PrintHex(hcibuf[2], 0x80);
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
			D_PrintHex(hcibuf[2], 0x80);
			#endif
			for(uint8_t i = 0; i < hcibuf[2]; i++) {
				uint8_t offset = 8 * hcibuf[2] + 3 * i;

				for(uint8_t j = 0; j < 3; j++)
				classOfDevice[j] = hcibuf[j + 4 + offset];

				#ifdef EXTRADEBUG
				Notify(PSTR("\r\nClass of device: "), 0x80);
				D_PrintHex(classOfDevice[2], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex(classOfDevice[1], 0x80);
				Notify(PSTR(" "), 0x80);
				D_PrintHex(classOfDevice[0], 0x80);
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
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nConnection Failed: "), 0x80);
			D_PrintHex(hcibuf[2], 0x80);
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
			#ifdef DEBUG_UART_HOST
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
		D_PrintHex(classOfDevice[2], 0x80);
		Notify(PSTR(" "), 0x80);
		D_PrintHex(classOfDevice[1], 0x80);
		Notify(PSTR(" "), 0x80);
		D_PrintHex(classOfDevice[0], 0x80);
		#endif
		hci_set_flag(HCI_FLAG_INCOMING_REQUEST);    //hci_set_flag(HCI_FLAG_INCOMING_REQUEST);
		hci_state = HCI_CONNECT_IN_STATE;
		break;

		case EV_PIN_CODE_REQUEST:
		if(pairWithWii) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nPairing with Wiimote"), 0x80);
			#endif
			hci_pin_code_request_reply();
			} else if(btdPin != NULL) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nBluetooth pin is set too: "), 0x80);
			Notify(btdPin, 0x80);
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
			D_PrintHex (hcibuf[2], 0x80);
			#endif
			hci_disconnect(hci_handle);
			hci_state = HCI_DISCONNECT_STATE;
		}
		break;
		/* We will just ignore the following events */
		case EV_NUM_COMPLETE_PKT:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nNum Complete Packet"), 0x80);
		#endif
		break;
		
		case EV_ROLE_CHANGED:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nRole Changed"), 0x80);
		#endif
		break;
		
		case EV_PAGE_SCAN_REP_MODE:
		if(HCI_FLAG_PSCAN_REP){
			hci_set_flag(HCI_FLAG_PSCAN_REP);
		}
		break;
		
		case EV_LOOPBACK_COMMAND:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nLoopBack Command"), 0x80);
		#endif
		break;
		case EV_DATA_BUFFER_OVERFLOW:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nData Buffer Overflow Event"), 0x80);
		#endif
		break;
		case EV_CHANGE_CONNECTION_LINK:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nChange Connection Link Event"), 0x80);
		#endif
		break;
		case EV_MAX_SLOTS_CHANGE:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nMax slots changed event"), 0x80);
		#endif
		break;
		case EV_QOS_SETUP_COMPLETE:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nQOS Setup Complete Event"), 0x80);
		#endif
		break;
		case EV_LINK_KEY_NOTIFICATION:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nLink key notification event"), 0x80);
		#endif
		break;
		case EV_ENCRYPTION_CHANGE:
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nEncryption Change Event"), 0x80);
		#endif
		break;
		case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:
		if(!hcibuf[2]){
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nRead remote version complete"), 0x80);
		#endif
		hci_set_flag(HCI_FLAG_REMOTE_VER_COMPLETE);
		}
		else{
			Notify(PSTR("\r\nRead remote version failed"), 0x80);
		}
		break;
		case EV_CHANGE_PACKET_TYPE_COMPLETE:
		if(!hcibuf[2]){
		hci_set_flag(HCI_FLAG_CHANGE_PTYPE_COMPLETE);
		#ifdef DEBUG_UART_HOST
		Notify(PSTR("\r\nChange Packet type successful"), 0x80);
		#endif
		}
		else{
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nChange Packet type failed"), 0x80);
			#endif
		}
		#ifdef EXTRADEBUG
		default:
		if(hcibuf[0] != 0x00) {
			Notify(PSTR("\r\nUnmanaged HCI Event: "), 0x80);
			D_PrintHex(hcibuf[0], 0x80);
		}
		break;
		#endif
	} // Switch

}

void BTD::HCI_task() {
	
	switch(hci_state) {
		case HCI_INIT_STATE:
		hci_counter++;
		if(hci_counter > hci_num_reset_loops) { // wait until we have looped x times to clear any old events
			hci_reset();
			hci_state = HCI_RESET_STATE;
			hci_counter = 0;
		}
		break;

		case HCI_RESET_STATE:
		hci_counter++;
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
			hci_counter = 0;
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nHCI Reset complete"), 0x80);
			#endif
			hci_state = HCI_CLASS_STATE;
			hci_write_class_of_device();
			} else if(hci_counter > hci_num_reset_loops) {
			hci_num_reset_loops *= 10;
			if(hci_num_reset_loops > 2000)
			hci_num_reset_loops = 2000;
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nNo response to HCI Reset"), 0x80);
			#endif
			hci_state = HCI_INIT_STATE;
			hci_counter = 0;
		}
		break;

		case HCI_WRITE_EN_STATE:
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)){
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nWrite access enabled"), 0x80);
			#endif
			hci_write_class_of_device();
			hci_state = HCI_CLASS_STATE;
		}
		else{
			hci_write_class_of_device();
			hci_state = HCI_CLASS_STATE;
		}
		break;

		case HCI_CLASS_STATE:
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nWrite class of device"), 0x80);
			#endif
			hci_state = HCI_BDADDR_STATE;
			hci_read_bdaddr();
		}
		break;

		case HCI_BDADDR_STATE:
		if(hci_check_flag(HCI_FLAG_READ_BDADDR)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nLocal Bluetooth Address: "), 0x80);
			for(int8_t i = 5; i > 0; i--) {
				D_PrintHex((my_bdaddr[i]), 0x80);
				Notify(PSTR(":"), 0x80);
			}
			D_PrintHex((my_bdaddr[0]), 0x80);
			#endif
			hci_read_local_version_information();
			hci_state = HCI_LOCAL_VERSION_STATE;
		}
		break;

		case HCI_LOCAL_VERSION_STATE: // The local version is used by the PS3BT class
		if(hci_check_flag(HCI_FLAG_READ_VERSION)) {
			if(btdName != NULL) {
				hci_set_local_name(btdName);
				hci_state = HCI_SET_NAME_STATE;
			} else
			hci_state = HCI_CHECK_DEVICE_SERVICE;
		}
		break;

		case HCI_SET_NAME_STATE:
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nThe name is set to: "), 0x80);
			printStringCRNL1(btdName);
			#endif
			hci_write_scan_enable();
			hci_state = HCI_CHECK_DEVICE_SERVICE;
			
		}
		break;
		
		case HCI_CHECK_DEVICE_SERVICE:
		if(pairWithHIDDevice || pairWithWii) { // Check if it should try to connect to a Wiimote
			#ifdef DEBUG_UART_HOST
			if(pairWithWii)
			Notify(PSTR("\r\nStarting inquiry\r\nPress 1 & 2 on the Wiimote\r\nOr press the SYNC button if you are using a Wii U Pro Controller or a Wii Balance Board"), 0x80);
			else
			Notify(PSTR("\r\nPlease enable discovery of your device"), 0x80);
			#endif
			hci_inquiry();
			hci_state = HCI_INQUIRY_STATE;
		} else
		hci_state = HCI_SCANNING_STATE; // Don't try to connect to a Wiimote
		break;

		case HCI_INQUIRY_STATE:
		if(hci_check_flag(HCI_FLAG_DEVICE_FOUND)) {
			hci_inquiry_cancel(); // Stop inquiry
			#ifdef DEBUG_UART_HOST
			if(pairWithWii)
			Notify(PSTR("\r\nWiimote found"), 0x80);
			else
			Notify(PSTR("\r\nHID device found"), 0x80);

			Notify(PSTR("\r\nNow just create the instance like so:"), 0x80);
			if(pairWithWii)
			Notify(PSTR("\r\nWII Wii(&Btd);"), 0x80);
			else
			Notify(PSTR("\r\nBTHID bthid(&Btd);"), 0x80);

			Notify(PSTR("\r\nAnd then press any button on the "), 0x80);
			if(pairWithWii)
			Notify(PSTR("Wiimote"), 0x80);
			else
			Notify(PSTR("device"), 0x80);
			#endif
			if(checkRemoteName) {
				hci_remote_name(); // We need to know the name to distinguish between the Wiimote, the new Wiimote with Motion Plus inside, a Wii U Pro Controller and a Wii Balance Board
				hci_state = HCI_REMOTE_NAME_STATE;
			} else
			hci_state = HCI_CONNECT_DEVICE_STATE;
		}
		break;

		case HCI_CONNECT_DEVICE_STATE:
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			if(pairWithWii)
			Notify(PSTR("\r\nConnecting to Wiimote"), 0x80);
			else
			Notify(PSTR("\r\nConnecting to HID device"), 0x80);
			#endif
			checkRemoteName = false;
			hci_connect();
			hci_state = HCI_CONNECTED_DEVICE_STATE;
		}
		break;

		case HCI_CONNECTED_DEVICE_STATE:
		if(hci_check_flag(HCI_FLAG_CONNECT_EVENT)) {
			if(hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
				#ifdef DEBUG_UART_HOST
				if(pairWithWii)
				Notify(PSTR("\r\nConnected to Wiimote"), 0x80);
				else
				Notify(PSTR("\r\nConnected to HID device"), 0x80);
				#endif
				hci_authentication_request(); // This will start the pairing with the Wiimote
				hci_state = HCI_SCANNING_STATE;
				} else {
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nTrying to connect one more time..."), 0x80);
				#endif
				hci_connect(); // Try to connect one more time
			}
		}
		break;

		case HCI_SCANNING_STATE:
		if(!connectToWii && !pairWithWii && !connectToHIDDevice && !pairWithHIDDevice) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nWait For Incoming Connection Request"), 0x80);
			#endif
			hci_write_scan_enable();
			waitingForConnection = true;
			hci_state = HCI_CONNECT_IN_STATE;
		}
		break;

		case HCI_CONNECT_IN_STATE:
		if(hci_check_flag(HCI_FLAG_INCOMING_REQUEST)) {
			waitingForConnection = false;
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nIncoming Connection Request"), 0x80);
			#endif
			hci_remote_name();
			hci_state = HCI_REMOTE_NAME_STATE;
		} else if(hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE))
		hci_state = HCI_DISCONNECT_STATE;
		break;

		case HCI_REMOTE_NAME_STATE:
		if(hci_check_flag(HCI_FLAG_REMOTE_NAME_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nRemote Name: "), 0x80);
			for(uint8_t i = 0; i < strlen(remote_name); i++)
			NotifyS(remote_name[i], 0x80);
			#endif
			if(strncmp((const char*)remote_name, "Nintendo", 8) == 0) {
				incomingWii = true;
				motionPlusInside = false;
				wiiUProController = false;
				pairWiiUsingSync = false;
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nWiimote is connecting"), 0x80);
				#endif
				if(strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-TR", 22) == 0) {
					#ifdef DEBUG_UART_HOST
					Notify(PSTR(" with Motion Plus Inside"), 0x80);
					#endif
					motionPlusInside = true;
					} else if(strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-UC", 22) == 0) {
					#ifdef DEBUG_UART_HOST
					Notify(PSTR(" - Wii U Pro Controller"), 0x80);
					#endif
					wiiUProController = motionPlusInside = pairWiiUsingSync = true;
					} else if(strncmp((const char*)remote_name, "Nintendo RVL-WBC-01", 19) == 0) {
					#ifdef DEBUG_UART_HOST
					Notify(PSTR(" - Wii Balance Board"), 0x80);
					#endif
					pairWiiUsingSync = true;
				}
			}
			if(classOfDevice[2] == 0 && classOfDevice[1] == 0x25 && classOfDevice[0] == 0x08 && strncmp((const char*)remote_name, "Wireless Controller", 19) == 0) {
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nPS4 controller is connecting"), 0x80);
				#endif
				incomingPS4 = true;
			}
			if(pairWithWii && checkRemoteName)
			hci_state = HCI_CONNECT_DEVICE_STATE;
			else {
				hci_write_authentication_enable();
				hci_state = HCI_WRITE_AUTHENTICATION_STATE;
			}
		}
		break;

		case HCI_WRITE_AUTHENTICATION_STATE:
		if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)){
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nWrite authentication Enabled"), 0x80);
			#endif
			hci_accept_connection();
			hci_state = HCI_CONNECTED_STATE;
		}
		break;

		case HCI_CONNECTED_STATE:
		if(hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nConnected to Device: "), 0x80);
			for(int8_t i = 5; i > 0; i--) {
				D_PrintHex((disc_bdaddr[i]), 0x80);
				Notify(PSTR(":"), 0x80);
			}
			D_PrintHex((disc_bdaddr[0]), 0x80);
			#endif
			if(incomingPS4)
			connectToHIDDevice = true; // We should always connect to the PS4 controller

			// Clear these flags for a new connection
			l2capConnectionClaimed = false;
			sdpConnectionClaimed = false;
			rfcommConnectionClaimed = false;
			hci_event_flag = 0;
			hci_state = HCI_PSCAN_REP_STATE;
		}
		break;
		
		case HCI_PSCAN_REP_STATE:
		if(hci_check_flag(HCI_FLAG_PSCAN_REP)){
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nPage Scan Repetition Mode Change Event"),0x80);
				#endif
		}
		//hci_read_remote_version();
		hci_state = HCI_DONE_STATE;
		break;
		
		case HCI_READ_VERSION:
			if(hci_check_flag(HCI_FLAG_REMOTE_VER_COMPLETE)){
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nReceived Remote Version"),0x80);
				#endif
			hci_state = HCI_READ_REMOTE_NAME;
			hci_remote_name();
			}
		break;
		
		case HCI_READ_REMOTE_NAME:
		if(hci_check_flag(HCI_FLAG_REMOTE_NAME_COMPLETE)){
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nReceived Remote Name"),0x80);
			#endif
		hci_change_packet_type();
		hci_state = HCI_CHANGE_PACKET_TYPE;
		}
		break;
		
		case HCI_CHANGE_PACKET_TYPE:
			if(hci_check_flag(HCI_FLAG_CHANGE_PTYPE_COMPLETE)){
				#ifdef DEBUG_UART_HOST
				Notify(PSTR("\r\nPacket type changed"),0x80);
				#endif
			}
			hci_state = HCI_DONE_STATE;
		break;
		
		case HCI_WRITE_LINK_POLICY:
		break;
		
		case HCI_DONE_STATE:
		hci_counter++;
		if(hci_counter > 1000) { // Wait until we have looped 1000 times to make sure that the L2CAP connection has been started
			hci_counter = 0;
			hci_state = HCI_SCANNING_STATE;
		}
		break;

		case HCI_DISCONNECT_STATE:
		if(hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE)) {
			#ifdef DEBUG_UART_HOST
			Notify(PSTR("\r\nHCI Disconnected from Device"), 0x80);
			#endif
			hci_event_flag = 0; // Clear all flags

			// Reset all buffers
			memset(hcibuf, 0, BULK_MAXPKTSIZE);
			memset(l2capinbuf, 0, BULK_MAXPKTSIZE);

			connectToWii = incomingWii = pairWithWii = false;
			connectToHIDDevice = incomingHIDDevice = pairWithHIDDevice = checkRemoteName = false;
			incomingPS4 = false;

			hci_state = HCI_SCANNING_STATE;
		}
		break;
		default:
		break;
	}
}

void BTD::ACL_event_task() {
	uint16_t length = BULK_MAXPKTSIZE;
		if(length > 0) { // Check if any data was read
			btService->ACLData(aclbuf);
			btService->Run();
		}
}

uint8_t BTD::BPoll() {
	if(((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) || EVENT_RECEIVED || ACL_DATA_RECEIVED) { // Don't poll if shorter than polling interval
		if(EVENT_RECEIVED){
			EVENT_RECEIVED = 0;
			HCI_event_task();
			HCI_task();
		}
		if(ACL_DATA_RECEIVED)
		ACL_DATA_RECEIVED = 0;
		ACL_event_task();
	}
	qNextPollTime = (uint32_t)millis() + pollInterval; // Set new poll time
	//printStringCRNL1("Polling");
	return 0;
}

/************************************************************/
/*                    HCI Commands                        */

/************************************************************/
void BTD::HCI_Command(uint8_t *data, uint16_t nbytes) {
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
        hcibuf[3] = 0x00; // Robot
        hcibuf[4] = 0x01; // Toy
        hcibuf[5] = 0x00;

        HCI_Command(hcibuf, 6);
}

void BTD::hci_change_packet_type() {
	hcibuf[0] = 0x0f; // HCI OCF = 0F
	hcibuf[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf[2] = 0x04; // parameter length 4
	hcibuf[3] = 0x0c;
	hcibuf[4] = 0x00;
	hcibuf[5] = 0x18;
	hcibuf[6] = 0xcc;
	HCI_Command(hcibuf, 7);
}

void BTD::hci_read_remote_version(){
	 hcibuf[0] = 0x1d; // HCI OCF = 1D
	 hcibuf[1] = 0x01 << 2; // HCI OGF = 3
	 hcibuf[2] = 0x02; // parameter length = 3
	 hcibuf[3] = 0x0c; 
	 hcibuf[4] = 0x00; 

	 HCI_Command(hcibuf, 5);
}

void BTD::hci_write_link_policy(){
	hcibuf[0] = 0x0d; // HCI OCF = 0D
	hcibuf[1] = 0x02 << 2; // HCI OGF = 1
	hcibuf[2] = 0x04; // parameter length 4
	hcibuf[3] = 0x0c;
	hcibuf[4] = 0x00;
	hcibuf[5] = 0x0f;
	hcibuf[6] = 0x00;
	HCI_Command(hcibuf, 7);
}

void BTD::hci_write_link_timeout(){
	hcibuf[0] = 0x37; // HCI OCF = 37
	hcibuf[1] = 0x03 << 2; // HCI OGF = 1
	hcibuf[2] = 0x04; // parameter length 4
	hcibuf[3] = 0x0c;
	hcibuf[4] = 0x00;
	hcibuf[5] = 0x80;
	hcibuf[6] = 0x3e;
	HCI_Command(hcibuf, 7);
}

void BTD::hci_set_AFH() {
	hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE | HCI_FLAG_CONNECT_EVENT);
	hcibuf[0] = 0x05;
	hcibuf[1] = 0x01 << 2; // HCI OGF = 1
	hcibuf[2] = 0x0D; // parameter Total Length = 13
	hcibuf[9] = 0x18; // DM1 or DH1 may be used
	hcibuf[10] = 0xCC; // DM3, DH3, DM5, DH5 may be used
	hcibuf[11] = 0x01; // Page repetition mode R1
	hcibuf[12] = 0x00; // Reserved
	hcibuf[13] = 0x00; // Clock offset
	hcibuf[14] = 0x00; // Invalid clock offset
	hcibuf[15] = 0x00; // Do not allow role switch

	HCI_Command(hcibuf, 16);
}

void BTD::hci_write_authentication_enable(){
	hcibuf[0] = 0x20; // HCI OCF = 20
	hcibuf[1] = 0x03 << 2; // HCI OGF = 3
	hcibuf[2] = 0x01; // parameter length = 1
	hcibuf[3] = 0x00;

	HCI_Command(hcibuf, 4);
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
		USART0_Transmit(0x02);	//HCI ACL Data Header
		for(uint16_t i = 0; i < (nbytes + 8); i++){
			USART0_Transmit(buf[i]);
		}
#ifdef DEBUG_USB_HOST
                Notify(PSTR("\r\nError sending L2CAP message: 0x"), 0x80);
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
