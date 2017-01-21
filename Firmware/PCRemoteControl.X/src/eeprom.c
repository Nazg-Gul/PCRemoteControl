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

#include "eeprom.h"
#include "system.h"

__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);
__EEPROM_DATA(0, 0, 0, 0, 0, 0, 0, 0);

void EEPROM_Write(int addr, char data) {
  EEADR = addr;
  EEDATA = data;
  EECON1bits.EEPGD = 0;  /* Access data EEPROM memory. */
  EECON1bits.CFGS = 0;   /* Access Flash program or data EEPROM memory. */
  EECON1bits.WREN = 1;   /* Allows write cycles to Flash program/data EEPROM. */
  INTCONbits.GIE = 0;    /* Disable all interrupts. */
  EECON2 = 0x55;
  EECON2 = 0xAA;
  EECON1bits.WR = 1;     /* WR Control bit initiates write operation. */
  INTCONbits.GIE = 1;
  while(!PIR2bits.EEIF);
  PIR2bits.EEIF = 0;
  EECON1bits.WREN = 0;   /* Disable Writing to EEPROM. */
}

unsigned char EEPROM_Read(int addr) {
  EEADR = addr;
  EECON1bits.EEPGD = 0;  /* Access data EEPROM memory. */
  EECON1bits.CFGS = 0;   /* Access Flash program or data EEPROM memory. */
  EECON1bits.RD = 1;     /* EEPROM Read Enable Bit. */
  return EEDATA;
}

void EEPROM_WriteString(int addr, const char *data, int len) {
  int i;
  for (i = 0; i < len; ++i) {
    EEPROM_Write(addr + i, data[i]);
  }
}

void EEPROM_ReadString(int addr, char *data, int len) {
  int i;
  for (i = 0; i < len; ++i) {
    data[i] = EEPROM_Read(addr + i);
  }
}
