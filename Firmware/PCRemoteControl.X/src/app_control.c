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

#include <string.h>

#include "app_control.h"

#include "app_network.h"

#include "eeprom.h"
#include "io_mapping.h"
#include "system.h"

enum {
  SW_STATE_NONE = 0,
  SW_STATE_NEED_PRESS = 1,
  SW_STATE_NEED_FORCE_PRESS = 2,
  SW_STATE_PRESSED = 3,
};

#define NUM_PCS 2
static uint8_t switch_stat[NUM_PCS] = {SW_STATE_NONE, SW_STATE_NONE};
static uint8_t switch_counter[NUM_PCS] = {0};
static bool need_update_status = false;
static bool initialized = false;
static uint8_t init_cycles = 0;
static int led_counter[NUM_PCS] = {0};
static char names[NUM_PCS][PC_MAX_NAME] = {{0}, {0}};

static void control_switch_set(uint8_t pc, uint8_t state) {
  if(pc == 0) {
    PC1_SW_OUT = state;
  } else /*if(pc == 1)*/ {
    PC2_SW_OUT = state;
  }
}

static void control_get_pc_name_eeprom(uint8_t pc, char name[PC_MAX_NAME]) {
  if(pc == 0) {
    EEPROM_ReadString(EEPROM_PC1_NAME, name, PC_MAX_NAME);
  } else /*if(pc == 1)*/ {
    EEPROM_ReadString(EEPROM_PC2_NAME, name, PC_MAX_NAME);
  }
}

void APP_control_init(void) {
  uint8_t i;

  PC1_LED_TRIS = 1;
  PC1_SW_TRIS = 0;
  PC2_LED_TRIS = 1;
  PC2_SW_TRIS = 0;

  for(i = 0; i < NUM_PCS; ++i) {
    if(APP_control_is_autoboot_enabled(i)) {
      switch_stat[i] = SW_STATE_NEED_PRESS;
    } else {
      switch_stat[i] = SW_STATE_NONE;
    }
    control_get_pc_name_eeprom(i, names[i]);
  }

  need_update_status = true;
  initialized = false;
  init_cycles = 16;

  /* Set up timers. */
  T0CONbits.T08BIT = 0;   /* 16 bit. */
  T0CONbits.T0CS = 0;     /* Internal clock. */
  T0CONbits.PSA = 0;      /* Prescaler enabled. */
  T0CONbits.T0PS = 0b101; /* 1:64 prescaler value */
  INTCONbits.T0IF = 0;    /* Clear the flag */
  INTCONbits.T0IE = 1;    /* Enable the interrupt */
  T0CONbits.TMR0ON = 1;
}

void APP_control_interrupts(void) {
  if(INTCONbits.TMR0IE && INTCONbits.T0IF) {
    INTCONbits.T0IF = 0;
    need_update_status = true;
  }
}

void APP_control_loop(void) {
  uint8_t i;
  if(need_update_status) {
    if(initialized) {
      for(i = 0; i < NUM_PCS; ++i) {
        switch(switch_stat[i]) {
          case SW_STATE_NEED_PRESS:
            control_switch_set(i, 1);
            switch_stat[i] = SW_STATE_PRESSED;
            switch_counter[i] = 1;
            break;
          case SW_STATE_NEED_FORCE_PRESS:
            control_switch_set(i, 1);
            switch_stat[i] = SW_STATE_PRESSED;
            switch_counter[i] = 16;
            break;
          case SW_STATE_PRESSED:
            --switch_counter[i];
            if(switch_counter[i] == 0) {
              switch_stat[i] = SW_STATE_NONE;
              control_switch_set(i, 0);
            }
            break;
        }
      }
    } else {
      --init_cycles;
      if(init_cycles == 0) {
        initialized = true;
      }
    }
    need_update_status = false;
  }
  if(!initialized) {
    for(i = 0; i < NUM_PCS; ++i) {
      if(APP_control_is_pc_on(i)) {
        if(led_counter[i] == 4096) {
            switch_stat[i] = SW_STATE_NONE;
            APP_network_debug_blink();
        } else {
          ++led_counter[i];
        }
      }
    }
  }
}

uint8_t APP_control_num_pcs(void) {
  return NUM_PCS;
}

bool APP_control_is_pc_on(uint8_t pc) {
  if(pc == 0) {
    return PC1_LED_IN == 0 ? true : false;
  } else /*if(pc == 1)*/ {
    return PC2_LED_IN == 0 ? true : false;
  }
}

bool APP_control_is_autoboot_enabled(uint8_t pc) {
  return EEPROM_Read(EEPROM_AUTOBOOT_ADDR + pc) != 0;
}

void APP_control_set_autoboot_enabled(uint8_t pc, bool enabled) {
  EEPROM_Write(EEPROM_AUTOBOOT_ADDR + pc, enabled ? 1 : 0);
}

uint8_t APP_control_get_pc_status(uint8_t pc) {
  int status = 0;
  if(APP_control_is_pc_on(pc)) {
    status |= PC_STATUS_ON;
  }
  switch(switch_stat[pc]) {
    case SW_STATE_NEED_PRESS:
    case SW_STATE_NEED_FORCE_PRESS:
      status |= PC_STATUS_WILL_PRESS;
      break;
    case SW_STATE_PRESSED:
      status |= PC_STATUS_PRESSED;
      break;
  }
  return status;
}

void APP_control_switch_press(uint8_t pc, bool force) {
  if(initialized) {
    switch_stat[pc] = force ? SW_STATE_NEED_FORCE_PRESS : SW_STATE_NEED_PRESS;
  }
}

void APP_control_set_pc_name(uint8_t pc, const char name[PC_MAX_NAME]) {
  if(pc == 0) {
    EEPROM_WriteString(EEPROM_PC1_NAME, name, PC_MAX_NAME);
  } else /*if(pc == 1)*/ {
    EEPROM_WriteString(EEPROM_PC2_NAME, name, PC_MAX_NAME);
  }
  memcpy(names[pc], name, PC_MAX_NAME);
}

void APP_control_get_pc_name(uint8_t pc, char name[PC_MAX_NAME]) {
  memcpy(name, names[pc], PC_MAX_NAME);
}

const char* APP_control_get_pc_name_ptr(uint8_t pc) {
  return names[pc];
}
