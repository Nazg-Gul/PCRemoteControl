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

#include "app_network.h"
#include "app_control.h"

#include "chip_configuration.h"
#include "eeprom.h"
#include "enc28j60.h"
#include "net.h"
#include "spi.h"

#include <stdio.h>
#include <string.h>

static uint8_t my_macaddr[6] = {0};
static uint8_t my_ip[4] = {0};
static char baseurl[] = "/";

#define BUFFER_SIZE 700
static unsigned char buf[BUFFER_SIZE + 1];
#define STR_BUFFER_SIZE 22
static char strbuf[STR_BUFFER_SIZE + 1];

/* The returned value is stored in the global var strbuf. */
static uint8_t find_key_val(char *str, char *key) {
  uint8_t found = 0;
  uint8_t i = 0;
  char *kp;
  kp = key;
  while (*str &&  *str != ' ' && found == 0) {
    if (*str == *kp) {
      kp++;
      if (*kp == '\0') {
        str++;
        kp = key;
        if (*str == '=') {
          found = 1;
        }
      }
    } else {
      kp = key;
    }
    str++;
  }
  if (found == 1) {
    /* copy the value to a buffer and terminate it with '\0' */
    while (*str &&  *str != ' ' && *str != '&' && i < STR_BUFFER_SIZE) {
      strbuf[i] = *str;
      i++;
      str++;
    }
    strbuf[i] = '\0';
  }
  return found;
}

static int8_t analyse_cmd(char *str) {
  int8_t r = -1;
  if (find_key_val(str, (char*)"cmd")) {
    if (*strbuf < 0x3a && *strbuf > 0x2f) {
      /* Is a ASCII number, return it. */
      r = (*strbuf-0x30);
    }
  }
  return r;
}

static uint16_t print_webpage_pc(int pc, uint16_t plen)
{
  uint8_t counter;

  /* PC name */
  const char *name = APP_control_get_pc_name_ptr(pc);
  plen = NET_fill_tcp_data_p(buf, plen, "<span>Name: ");
  plen = NET_fill_tcp_data_p(buf, plen, name);
  plen = NET_fill_tcp_data_p(buf, plen, "</span>");

  /* PC status */
  uint8_t status = APP_control_get_pc_status(pc);
  counter = 0;
  plen = NET_fill_tcp_data_p(buf, plen, "<div>Status: ");
  if (status & PC_STATUS_ON) {
      plen = NET_fill_tcp_data_p(buf, plen, "<span style=\"color: green\">ON</span>");
      ++counter;
  }
  else {
      plen = NET_fill_tcp_data_p(buf, plen, "<span style=\"color: red\">OFF</span>");
      ++counter;
  }
  if (status & PC_STATUS_WILL_PRESS) {
    if (counter != 0) {
      plen = NET_fill_tcp_data_p(buf, plen, ", ");
    }
    plen = NET_fill_tcp_data_p(buf, plen, "WILL_PRESS_BUTTON");
    ++counter;
  }
  if (status & PC_STATUS_PRESSED) {
    if (counter != 0) {
      plen = NET_fill_tcp_data_p(buf, plen, ", ");
    }
    plen = NET_fill_tcp_data_p(buf, plen, "BUTTON_PRESSED");
    ++counter;
  }
  plen = NET_fill_tcp_data_p(buf, plen, "</div>");

  /* PC Control buttons */
  sprintf(strbuf, "%d", pc);

  plen = NET_fill_tcp_data_p(buf, plen, "<div><a href=\"/press/");
  plen = NET_fill_tcp_data_p(buf, plen, strbuf);
  plen = NET_fill_tcp_data_p(buf, plen, "\">Press Button</a>");
  plen = NET_fill_tcp_data_p(buf, plen, "&nbsp;&nbsp;&nbsp;&nbsp;");
  plen = NET_fill_tcp_data_p(buf, plen, "<a href=\"/hold/");
  plen = NET_fill_tcp_data_p(buf, plen, strbuf);
  plen = NET_fill_tcp_data_p(buf, plen, "\">Hold Button</a></div>");
  return plen;
}

static uint16_t print_webpage(uint8_t *buf, uint8_t on_off) {
  int i = 0;
  uint16_t plen;

  plen = NET_fill_tcp_data_p(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
  plen = NET_fill_tcp_data_p(buf, plen, "<html><body>");
  plen = NET_fill_tcp_data_p(buf, plen, "<h1>Welcome 2 PCRemoteControl</h1>");
  plen = NET_fill_tcp_data_p(buf, plen, "<h2>Computer #1</h2>");
  plen = print_webpage_pc(0, plen);
  plen = NET_fill_tcp_data_p(buf, plen, "<h2>Computer #1</h2>");
  plen = print_webpage_pc(1, plen);
  plen = NET_fill_tcp_data_p(buf, plen, "</html></body>");
  return plen;
}

void APP_network_init(void) {
  /* Read settings from EEPROM. */
  my_ip[0] = EEPROM_Read(EEPROM_IP_ADDR + 0);
  my_ip[1] = EEPROM_Read(EEPROM_IP_ADDR + 1);
  my_ip[2] = EEPROM_Read(EEPROM_IP_ADDR + 2);
  my_ip[3] = EEPROM_Read(EEPROM_IP_ADDR + 3);
  my_macaddr[0] = EEPROM_Read(EEPROM_MAC_ADDR + 0);
  my_macaddr[1] = EEPROM_Read(EEPROM_MAC_ADDR + 1);
  my_macaddr[2] = EEPROM_Read(EEPROM_MAC_ADDR + 2);
  my_macaddr[3] = EEPROM_Read(EEPROM_MAC_ADDR + 3);
  my_macaddr[4] = EEPROM_Read(EEPROM_MAC_ADDR + 4);
  my_macaddr[5] = EEPROM_Read(EEPROM_MAC_ADDR + 5);

  LATBbits.LB4 = 0;  /* Reset the module. */
  LATBbits.LB5 = 0;
  __delay_ms(10);

  SPI_Init();
  LATBbits.LB4 = 1;  /* Resume the module. */
  LATBbits.LB5 = 1;

  ENC28J60_Init(my_macaddr);
  ENC28J60_ClkOut(2);
  __delay_ms(10);

  /* Debug blink: keep both LEDs on for a bit. */
  APP_network_debug_blink();

  NET_init(my_macaddr, my_ip, 80);
}

void APP_network_debug_blink(void)
{
  uint8_t a;
  ENC28J60_PhyWrite(PHLCON, 0x990);
  for (a = 0; a < 100; ++a)  __delay_ms(10);
  ENC28J60_PhyWrite(PHLCON, 0x880);
  for (a = 0; a < 100; ++a)  __delay_ms(10);
  ENC28J60_PhyWrite(PHLCON, 0x990);
  for (a = 0; a < 100; ++a)  __delay_ms(10);
  /* LEDA=links status, LEDB=receive/transmit. */
  ENC28J60_PhyWrite(PHLCON, 0x476);
}

void APP_network_loop(void) {
  uint16_t plen, dat_p;
  int8_t cmd;
  uint8_t on_off = 1;

  plen = ENC28J60_PacketReceive(BUFFER_SIZE, buf);
  /* plen will be unequal to zero if there is a valid packet
   * (without crc error)
   */
  if (plen != 0) {
    /* arp is broadcast if unknown but a host may also verify the mac address by
     * sending it to a unicast address.
     */
    if (NET_eth_type_is_arp_and_my_ip(buf, plen)) {
      NET_make_arp_answer_from_request(buf);
      return;
    }

    /* Check if ip packets are for us. */
    if (NET_eth_type_is_ip_and_my_ip(buf, plen) == 0) {
      return;
    }

    if (buf[IP_PROTO_P] == IP_PROTO_ICMP_V &&
        buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
    {
      NET_make_echo_reply_from_request(buf, plen);
      return;
    }

    /* tcp port www start, compare only the lower byte. */
    if (buf[IP_PROTO_P] == IP_PROTO_TCP_V &&
        buf[TCP_DST_PORT_H_P] == 0 &&
        buf[TCP_DST_PORT_L_P] == 80)
    {
      if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) {
        /* NET_make_tcp_synack_from_syn does already send the syn, ack. */
        NET_make_tcp_synack_from_syn(buf);
        return;
      }
      if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V) {
        NET_init_len_info(buf);  /* Init some data structures. */
        dat_p = NET_get_tcp_data_pointer();
        if (dat_p == 0) {  /* we can possibly have no data, just ack. */
          if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V) {
            NET_make_tcp_ack_from_any(buf);
          }
          return;
        }
        if (strncmp("GET ", (char *)&(buf[dat_p]), 4) != 0) {
          /* head, post and other methods for possible status codes see:
           *   http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
           */
          plen = NET_fill_tcp_data_p(buf,
                                     0,
                                     "HTTP/1.0 200 OK\r\n"
                                     "Content-Type: text/html\r\n\r\n"
                                     "<h1>200 OK</h1>");
          goto SENDTCP;
        }
        if (strncmp("/ ", (char *)&(buf[dat_p + 4]), 2) == 0) {
          plen = print_webpage(buf, on_off);
          goto SENDTCP;
        }
        else if (strncmp("/press/", (char *)&(buf[dat_p + 4]), 7) == 0) {
          uint8_t pc = buf[dat_p + 11] - '0';
          APP_control_switch_press(pc, false);
          plen = NET_fill_tcp_data_p(buf,
                                     0,
                                     "HTTP/1.1 302 Found\r\n"
                                     "Location: /\r\n\r\n");
          goto SENDTCP;
        }
        else if (strncmp("/hold/", (char *)&(buf[dat_p + 4]), 6) == 0) {
          uint8_t pc = buf[dat_p + 10] - '0';
          APP_control_switch_press(pc, true);
          plen = NET_fill_tcp_data_p(buf,
                                     0,
                                     "HTTP/1.1 302 Found\r\n"
                                     "Location: /\r\n\r\n");
          goto SENDTCP;
        }
        cmd = analyse_cmd((char *)&(buf[dat_p + 5]));
        if (cmd == 2) {
          on_off = 1;
          //LED2_IO = 1;
        } else if (cmd == 3) {
          on_off = 0;
          //LED2_IO = 0;
        }
        plen = print_webpage(buf, on_off);
SENDTCP:
        NET_make_tcp_ack_from_any(buf);  /* Send ack for http get. */
        NET_make_tcp_ack_with_data(buf, plen);  /* send data. */
      }
    }
  }
}

void APP_network_set_ip(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3) {
  EEPROM_Write(EEPROM_IP_ADDR + 0, ip0);
  EEPROM_Write(EEPROM_IP_ADDR + 1, ip1);
  EEPROM_Write(EEPROM_IP_ADDR + 2, ip2);
  EEPROM_Write(EEPROM_IP_ADDR + 3, ip3);
}

void APP_network_set_mac(uint8_t mac0,
                         uint8_t mac1,
                         uint8_t mac2,
                         uint8_t mac3,
                         uint8_t mac4,
                         uint8_t mac5) {
  EEPROM_Write(EEPROM_MAC_ADDR + 0, mac0);
  EEPROM_Write(EEPROM_MAC_ADDR + 1, mac1);
  EEPROM_Write(EEPROM_MAC_ADDR + 2, mac2);
  EEPROM_Write(EEPROM_MAC_ADDR + 3, mac3);
  EEPROM_Write(EEPROM_MAC_ADDR + 4, mac4);
  EEPROM_Write(EEPROM_MAC_ADDR + 5, mac5);
}

void APP_network_get_ip(uint8_t *ip0, uint8_t *ip1, uint8_t *ip2, uint8_t *ip3) {
  *ip0 = EEPROM_Read(EEPROM_IP_ADDR + 0);
  *ip1 = EEPROM_Read(EEPROM_IP_ADDR + 1);
  *ip2 = EEPROM_Read(EEPROM_IP_ADDR + 2);
  *ip3 = EEPROM_Read(EEPROM_IP_ADDR + 3);
}

void APP_network_get_mac(uint8_t *mac0,
                         uint8_t *mac1,
                         uint8_t *mac2,
                         uint8_t *mac3,
                         uint8_t *mac4,
                         uint8_t *mac5) {
  *mac0 = EEPROM_Read(EEPROM_MAC_ADDR + 0);
  *mac1 = EEPROM_Read(EEPROM_MAC_ADDR + 1);
  *mac2 = EEPROM_Read(EEPROM_MAC_ADDR + 2);
  *mac3 = EEPROM_Read(EEPROM_MAC_ADDR + 3);
  *mac4 = EEPROM_Read(EEPROM_MAC_ADDR + 4);
  *mac5 = EEPROM_Read(EEPROM_MAC_ADDR + 5);
}
