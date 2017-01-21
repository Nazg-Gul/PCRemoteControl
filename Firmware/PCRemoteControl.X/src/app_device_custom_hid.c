/********************************************************************
 Adopted from demo application with the following license:

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

#include "app_device_custom_hid.h"

#include <string.h>
#include <system.h>
#include <usb/inc/usb.h>
#include <usb/inc/usb_device_hid.h>

#include "app_control.h"
#include "app_network.h"

/* Some processors have a limited range of RAM addresses where the USB module
 * is able to access.  The following section is for those devices.  This section
 * assigns the buffers that need to be used by the USB module into those
 * specific areas.
 */
#if defined(FIXED_ADDRESS_MEMORY)
#  if defined(COMPILER_MPLAB_C18)
#    pragma udata HID_CUSTOM_OUT_DATA_BUFFER = HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS
unsigned char ReceivedDataBuffer[64];
#    pragma udata HID_CUSTOM_IN_DATA_BUFFER = HID_CUSTOM_IN_DATA_BUFFER_ADDRESS
unsigned char ToSendDataBuffer[64];
#    pragma udata
#  else defined(__XC8)
unsigned char ReceivedDataBuffer[64] @ HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS;
unsigned char ToSendDataBuffer[64] @ HID_CUSTOM_IN_DATA_BUFFER_ADDRESS;
#  endif
#else
unsigned char ReceivedDataBuffer[64];
unsigned char ToSendDataBuffer[64];
#endif

volatile USB_HANDLE USBOutHandle;
volatile USB_HANDLE USBInHandle;

typedef enum {
  COMMAND_TEST             = 0x80,
  COMMAND_SWITCH_PRESS     = 0x81,
  COMMAND_CFG_SET_AUTOBOOT = 0x82,
  COMMAND_CFG_SET_IP       = 0x83,
  COMMAND_CFG_SET_MAC      = 0x84,
  COMMAND_CFG_SET_PC_NAME  = 0x85,
  COMMAND_CFG_GET_AUTOBOOT = 0x86,
  COMMAND_CFG_GET_IP       = 0x87,
  COMMAND_CFG_GET_MAC      = 0x88,
  COMMAND_CFG_GET_STATUS   = 0x89,
  COMMAND_CFG_GET_PC_NAME  = 0x90,
} CUSTOM_HID_COMMANDS;

/* Transmit the response to the host/ */
static void transmitResponse(void) {
  if (!HIDTxHandleBusy(USBInHandle)) {
    USBInHandle = HIDTxPacket(CUSTOM_DEVICE_HID_EP,
                              (uint8_t*)&ToSendDataBuffer[0], 64);
  }
}

/* Initializes the Custom HID code. */
void APP_DeviceCustomHIDInitialize(void) {
  /* Initialize the variable holding the handle for the last transmission. */
  USBInHandle = 0;

  /* Enable the HID endpoint. */
  USBEnableEndpoint(CUSTOM_DEVICE_HID_EP,
                    USB_IN_ENABLED |
                    USB_OUT_ENABLED |
                    USB_HANDSHAKE_ENABLED |
                    USB_DISALLOW_SETUP);

    /* Re-arm the OUT endpoint for the next packet */
  USBOutHandle =
    (volatile USB_HANDLE) HIDRxPacket(CUSTOM_DEVICE_HID_EP,
                                      (uint8_t*)&ReceivedDataBuffer, 64);
}

/* Keep the Custom HID running.
*
* The application should have been initialized and started via
* the APP_DeviceCustomHIDInitialize() function.
*
********************************************************************/
void APP_DeviceCustomHIDTasks(void) {
  /* Check if we have received an OUT data packet from the host. */
  if (HIDRxHandleBusy(USBOutHandle) == false) {
    int i, n, pc;
    /* We just received a packet of data from the USB host.
     * Check the first uint8_t of the packet to see what command the host
     * application software wants us to fulfill.
     */

     /* Look at the data the host sent, to see what kind of application specific
      * command it sent. */
    switch (ReceivedDataBuffer[0]) {
      case COMMAND_TEST:
        APP_network_debug_blink();
        break;
      case COMMAND_SWITCH_PRESS:
        pc = ReceivedDataBuffer[1];
        if(pc >= 0 && pc < APP_control_num_pcs()) {
            APP_control_switch_press(pc, ReceivedDataBuffer[2] != 0);
        }
        break;
      case COMMAND_CFG_SET_AUTOBOOT:
        pc = ReceivedDataBuffer[1];
        if(pc >= 0 && pc < APP_control_num_pcs()) {
          APP_control_set_autoboot_enabled(pc, ReceivedDataBuffer[2] != 0);
        }
        break;
      case COMMAND_CFG_SET_IP:
        APP_network_set_ip(ReceivedDataBuffer[1],
                           ReceivedDataBuffer[2],
                           ReceivedDataBuffer[3],
                           ReceivedDataBuffer[4]);
        break;
      case COMMAND_CFG_SET_MAC:
        APP_network_set_mac(ReceivedDataBuffer[1],
                            ReceivedDataBuffer[2],
                            ReceivedDataBuffer[3],
                            ReceivedDataBuffer[4],
                            ReceivedDataBuffer[5],
                            ReceivedDataBuffer[6]);
      case COMMAND_CFG_SET_PC_NAME:
        pc = ReceivedDataBuffer[1];
        if(pc >= 0 && pc < APP_control_num_pcs()) {
          APP_control_set_pc_name(pc, ReceivedDataBuffer + 2);
        }
        break;
      case COMMAND_CFG_GET_AUTOBOOT:
        pc = ReceivedDataBuffer[1];
        if(pc >= 0 && pc < APP_control_num_pcs()) {
          ToSendDataBuffer[0] =
                  APP_control_is_autoboot_enabled(pc);
          transmitResponse();
        }
        break;
      case COMMAND_CFG_GET_IP:
        APP_network_get_ip(&ToSendDataBuffer[0],
                           &ToSendDataBuffer[1],
                           &ToSendDataBuffer[2],
                           &ToSendDataBuffer[3]);
        transmitResponse();
        break;
      case COMMAND_CFG_GET_MAC:
        APP_network_get_mac(&ToSendDataBuffer[0],
                            &ToSendDataBuffer[1],
                            &ToSendDataBuffer[2],
                            &ToSendDataBuffer[3],
                            &ToSendDataBuffer[4],
                            &ToSendDataBuffer[5]);
        transmitResponse();
        break;
      case COMMAND_CFG_GET_STATUS:
        n = APP_control_num_pcs();
        ToSendDataBuffer[0] = n;
        for (i = 0; i < n; ++i) {
          ToSendDataBuffer[i + 1] = APP_control_get_pc_status(i);
        }
        transmitResponse();
        break;
      case COMMAND_CFG_GET_PC_NAME:
        pc = ReceivedDataBuffer[1];
        if(pc >= 0 && pc < APP_control_num_pcs()) {
          APP_control_get_pc_name(pc, ToSendDataBuffer);
        }
        transmitResponse();
        break;
    }
    /* Re-arm the OUT endpoint, so we can receive the next OUT data packet
     * that the host may try to send us.
     */
    USBOutHandle = HIDRxPacket(CUSTOM_DEVICE_HID_EP,
                               (uint8_t*)&ReceivedDataBuffer,
                               64);
  }
}
