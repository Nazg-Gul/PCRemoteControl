/* Copyright (C) 2014 Sergey Sharybin <sergey@blender.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <system.h>
#include <system_config.h>
#include <usb/inc/usb.h>
#include <usb/inc/usb_device_hid.h>

#include "app_device_custom_hid.h"

void main(void) {
  SYSTEM_Initialize();

  USBDeviceInit();
  USBDeviceAttach();

  while (1) {
    SYSTEM_Tasks();

#if defined(USB_POLLING)
    /* Interrupt or polling method.  If using polling, must call
     * this function periodically.  This function will take care
     * of processing and responding to SETUP transactions
     * (such as during the enumeration process when you first
     * plug in).  USB hosts require that USB devices should accept
     * and process SETUP packets in a timely fashion.  Therefore,
     * when using polling, this function should be called
     * regularly (such as once every 1.8ms or faster** [see
     * inline code comments in usb_device.c for explanation when
     * "or faster" applies])  In most cases, the USBDeviceTasks()
     * function does not take very long to execute (ex: <100
     * instruction cycles) before it returns.
     */
    USBDeviceTasks();
#endif

    /* If the USB device isn't configured yet, we can't really do anything
     * else since we don't have a host to talk to.  So jump back to the
     * top of the while loop.
     */
    if (USBGetDeviceState() < CONFIGURED_STATE) {
      continue;
    }

    /* If we are currently suspended, then we need to see if we need to
     * issue a remote wakeup.  In either case, we shouldn't process any
     * keyboard commands since we aren't currently communicating to the host
     * thus just continue back to the start of the while loop.
     */
    if (USBIsDeviceSuspended() == true) {
      continue;
    }

    /* Application specific USB tasks. */
    APP_DeviceCustomHIDTasks();
  }
}

bool USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, uint16_t size)
{
  switch ((int)event) {
    case EVENT_TRANSFER:
      break;

    case EVENT_SOF:
      break;

    case EVENT_SUSPEND:
      break;

    case EVENT_RESUME:
      break;

    case EVENT_CONFIGURED:
      /* When the device is configured, we can (re)initialize the app code. */
      APP_DeviceCustomHIDInitialize();
      break;

    case EVENT_SET_DESCRIPTOR:
      break;

    case EVENT_EP0_REQUEST:
        /* We have received a non-standard USB request.  The HID driver
         * needs to check to see if the request was for it.
         */
        USBCheckHIDRequest();
        break;

    case EVENT_BUS_ERROR:
        break;

    case EVENT_TRANSFER_TERMINATED:
        break;

    default:
        break;
  }
  return true;
}
