/*
 ===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
 ===============================================================================
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BT_Stack/includes/config.h"
#include "BT_Stack/includes/btstack_memory.h"
#include "BT_Stack/includes/hci.h"
#include "BT_Stack/includes/hci_dump.h"
#include "BT_Stack/includes/run_loop.h"
#include "BT_Stack/includes/l2cap.h"
#include "BT_Stack/includes/att.h"

#include <cr_section_macros.h>
#include <NXP/crp.h>

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP;

#include <stdio.h>

// TODO: insert other definitions and declarations here

static uint8_t HCI_WORKING = 0;

// enable LE, setup ADV data
static void packet_handler( uint8_t packet_type,
		uint8_t *packet, uint16_t size) {
	bd_addr_t addr;
	uint8_t adv_data[31] = "\x02\x01\x05" "\x05\x09mbed" "\x03\x02\xf0\xff";

	 switch (packet_type) {

			case HCI_EVENT_PACKET:
				switch (packet[0]) {

	                case BTSTACK_EVENT_STATE:
						// bt stack activated, get started - set local name
						if (packet[2] == HCI_STATE_WORKING) {
						   printf("Working!\n");
						   //send gap_device init through
							hci_send_device_int();
							HCI_WORKING = 1;
						}
						break;

	                case HCI_EVENT_VENDOR_SPECIFIC:
	                	hci_send_cmd(&hci_read_local_supported_features);
	                	break;


					case HCI_EVENT_COMMAND_COMPLETE:
						if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)){
	    					bt_flip_addr(addr, &packet[6]);
						    printf("BD ADDR: %s\n", bd_addr_to_str(addr));

						}
						if (COMMAND_COMPLETE_EVENT(packet, hci_read_local_supported_features)){
						    printf("Local supported features: %04X%04X\n", READ_BT_32(packet, 10), READ_BT_32(packet, 6));
	                        hci_send_cmd(&hci_set_event_mask, 0xffffffff, 0x20001fff);
							break;
						}
						if (COMMAND_COMPLETE_EVENT(packet, hci_set_event_mask)){
	                        hci_send_cmd(&hci_write_le_host_supported, 1, 1);
							break;
						}
						if (COMMAND_COMPLETE_EVENT(packet, hci_write_le_host_supported)){
					        hci_send_cmd(&hci_le_set_event_mask, 0xffffffff, 0xffffffff);
	                    	break;
						}
						if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_event_mask)){
					        hci_send_cmd(&hci_le_read_buffer_size);
	                    	break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_buffer_size)){
						    printf("LE buffer size: %u, count %u\n", READ_BT_16(packet,6), packet[8]);
						   hci_send_cmd(&hci_le_read_supported_states);
						   break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_supported_states)){
						   hci_send_cmd(&hci_le_set_advertising_parameters,  0x0400, 0x0800, 0, 0, 0, &addr, 0x07, 0);
						   break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertising_parameters)){
						   hci_send_cmd(&hci_le_set_advertising_data, sizeof(adv_data), adv_data);
						   break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertising_data)){
						   hci_send_cmd(&hci_le_set_scan_response_data, 10, adv_data);
						   break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_scan_response_data)){
						   hci_send_cmd(&hci_le_set_advertise_enable, 1);
						   break;
						}
					    if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertise_enable)){
	                        hci_discoverable_control(1);
	                        break;
						}

				}
		}
	}

	int main(void) {
		//hardware config
		//clock
		//adc

		btstack_memory_init();
		run_loop_init(RUN_LOOP_EMBEDDED);

		// init HCI
		hci_transport_t * transport = hci_transport_h4_dma_instance();
		remote_device_db_t * remote_db =
				(remote_device_db_t *) &remote_device_db_memory;
		hci_init(transport, NULL, NULL, remote_db);

		// set up l2cap_le

		/*     __enable_interrupt(); */

		printf("Run...\n\n");

		// turn on!
		hci_power_control(HCI_POWER_ON);
		/*l2cap_init();
		l2cap_register_packet_handler(packet_handler);
		//*/
		hci_register_packet_handler(packet_handler);
		// go!

		run_loop_execute();

		//hci_register_packet_handler(packet_handler);
		return 0;
	}
