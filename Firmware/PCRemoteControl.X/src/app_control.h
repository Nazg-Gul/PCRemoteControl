/* Copyright (C) 2015 Sergey Sharybin <sergey.vfx@gmail.com>
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

#ifndef __APP_CONTROL__
#define __APP_CONTROL__

#include <stdbool.h>
#include <stdint.h>

enum {
  PC_STATUS_ON         = (1 << 0),
  PC_STATUS_WILL_PRESS = (1 << 1),
  PC_STATUS_PRESSED =    (1 << 2),
};

#define PC_MAX_NAME 16

void APP_control_init(void);
void APP_control_interrupts(void);
void APP_control_loop(void);

uint8_t APP_control_num_pcs(void);
bool APP_control_is_pc_on(uint8_t pc);
bool APP_control_is_autoboot_enabled(uint8_t pc);
void APP_control_set_autoboot_enabled(uint8_t pc, bool enabled);
uint8_t APP_control_get_pc_status(uint8_t pc);
void APP_control_set_pc_name(uint8_t pc, const char name[PC_MAX_NAME]);
void APP_control_get_pc_name(uint8_t pc, char name[PC_MAX_NAME]);
const char* APP_control_get_pc_name_ptr(uint8_t pc);
void APP_control_switch_press(uint8_t pc, bool force);

#endif  /* __APP_CONTROL__ */
