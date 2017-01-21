/* Copyright (C) 2014 Sergey Sharybin <sergey.vfx@gmail.com>
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

#ifndef __APP_NETWORK__
#define __APP_NETWORK__

#include <stdint.h>

void APP_network_init(void);
void APP_network_debug_blink(void);
void APP_network_loop(void);

void APP_network_set_ip(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
void APP_network_set_mac(uint8_t mac0,
                         uint8_t mac1,
                         uint8_t mac2,
                         uint8_t mac3,
                         uint8_t mac4,
                         uint8_t mac5);
void APP_network_get_ip(uint8_t *ip0, uint8_t *ip1, uint8_t *ip2, uint8_t *ip3);
void APP_network_get_mac(uint8_t *mac0,
                         uint8_t *mac1,
                         uint8_t *mac2,
                         uint8_t *mac3,
                         uint8_t *mac4,
                         uint8_t *mac5);

#endif  /* __APP_NETWORK__ */
