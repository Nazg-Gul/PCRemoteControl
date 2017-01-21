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

#ifndef __APP_DEVICE_CUSTOM_HID__
#define __APP_DEVICE_CUSTOM_HID__

#define MANUFACTURER_STRING_DESCRIPTOR \
  'V','o','i','d','n','e','s','s'

#define PRODUCT_STRING_DESCRIPTOR \
  'P','C','R','e','m','o','t','e','C','o','n','t','r','o','l'

/* Initializes the Custom HID code */
void APP_DeviceCustomHIDInitialize(void);

/* Keep the Custom HID code running. */
void APP_DeviceCustomHIDTasks();

#endif
