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

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libusb.h>

// #define VERSION "0.1.0"
#define VENDOR_ID 0x04d8
#define PRODUCT_ID 0x003f

#define COMMAND_TEST             0x80
#define COMMAND_SWITCH_PRESS     0x81
#define COMMAND_CFG_SET_AUTOBOOT 0x82
#define COMMAND_CFG_SET_IP       0x83
#define COMMAND_CFG_SET_MAC      0x84
#define COMMAND_CFG_SET_PC_NAME  0x85
#define COMMAND_CFG_GET_AUTOBOOT 0x86
#define COMMAND_CFG_GET_IP       0x87
#define COMMAND_CFG_GET_MAC      0x88
#define COMMAND_GET_STATUS       0x89
#define COMMAND_CFG_GET_PC_NAME  0x90

#define NUM_PCS 2

namespace {

const int PACKET_INT_LEN = 64;
const int INTERFACE = 0;
const int ENDPOINT_INT_IN = 0x81; /* endpoint 0x81 address for IN */
const int ENDPOINT_INT_OUT = 0x01; /* endpoint 1 address for OUT */
const int TIMEOUT = 5000; /* timeout in ms */

static libusb_device_handle *devh = NULL;
static libusb_context *ctx = NULL;

int find_lvr_hidusb(void) {
  devh = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
  return devh ? 0 : -EIO;
}

int send_buffer(unsigned char buffer[PACKET_INT_LEN]) {
  int transferred;
  int r = libusb_interrupt_transfer(devh,
                                    ENDPOINT_INT_OUT,
                                    buffer,
                                    PACKET_INT_LEN,
                                    &transferred,
                                    TIMEOUT);
  if (r < 0) {
    fprintf(stderr, "Interrupt write error %s\n", libusb_error_name(r));
    return r;
  }
  return 0;
}

int send_command(int command,
                 int arg,
                 const char *data,
                 int len) {
  unsigned char buffer[PACKET_INT_LEN] = {0};
  buffer[0] = command;
  buffer[1] = arg;
  memcpy(buffer + 2, data, len);
  send_buffer(buffer);
  return 0;
}

int send_command(int command,
                 int arg1 = 0,
                 int arg2 = 0,
                 int arg3 = 0,
                 int arg4 = 0,
                 int arg5 = 0,
                 int arg6 = 0) {
  unsigned char buffer[PACKET_INT_LEN] = {0};
  buffer[0] = command;
  buffer[1] = arg1;
  buffer[2] = arg2;
  buffer[3] = arg3;
  buffer[4] = arg4;
  buffer[5] = arg5;
  buffer[6] = arg6;
  return send_buffer(buffer);
}

int read_answer(unsigned char *buffer) {
  int r, transferred;
  r = libusb_interrupt_transfer(devh,
                                ENDPOINT_INT_IN,
                                buffer,
                                PACKET_INT_LEN,
                                &transferred,
                                TIMEOUT);
  if (r < 0) {
    fprintf(stderr, "Interrupt read error %s\n", libusb_error_name(r));
    return r;
  }
  return 0;
}

void send_test_command(void) {
  send_command(COMMAND_TEST);
}

void send_press_command(int pc, bool force) {
  send_command(COMMAND_SWITCH_PRESS, pc, force ? 1 : 0);
}

void send_set_ip_command(const unsigned char ip[4]) {
  send_command(COMMAND_CFG_SET_IP, ip[0], ip[1], ip[2], ip[3]);
}

void send_set_mac_command(const unsigned char mac[6]) {
  send_command(COMMAND_CFG_SET_MAC,
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void send_set_autoboot_command(int pc, bool enabled) {
  send_command(COMMAND_CFG_SET_AUTOBOOT, pc, enabled ? 1 : 0);
}

void send_set_name_command(int pc, const char *name) {
  send_command(COMMAND_CFG_SET_PC_NAME, pc, name, strlen(name));
}

void send_get_ip_command(void) {
  send_command(COMMAND_CFG_GET_IP);
}

void retrieve_ip(unsigned char ip[4]) {
  unsigned char buffer[64];
  read_answer(buffer);
  memcpy(ip, buffer, 4);
}

void send_get_mac_command(void) {
  send_command(COMMAND_CFG_GET_MAC);
}

void retrieve_mac(unsigned char mac[6]) {
  unsigned char buffer[64];
  read_answer(buffer);
  memcpy(mac, buffer, 6);
}

void send_get_status_command(void) {
  send_command(COMMAND_GET_STATUS);
}

void retrieve_status(unsigned char buffer[64]) {
  read_answer(buffer);
}

void send_get_autoboot_command(int pc) {
  send_command(COMMAND_CFG_GET_AUTOBOOT, pc);
}

bool retrieve_autoboot(void) {
  unsigned char buffer[64];
  read_answer(buffer);
  return buffer[0] != 0;
}

void send_get_name_command(int pc) {
  send_command(COMMAND_CFG_GET_PC_NAME, pc);
}

void retrieve_name(unsigned char name[64]) {
  read_answer(name);
}

bool check_pc_valid(int pc) {
  return pc >= 0 && pc <= NUM_PCS;
}

bool parse_press_command(int argc, char **argv) {
  if ((argc != 3 && argc != 4) ||
      (argc == 4 && strcmp(argv[3], "force") != 0)) {
    printf("Usage: %s press <pc> [force]\n", argv[0]);
    return false;
  }
  int pc = atoi(argv[2]);
  if (!check_pc_valid(pc)) {
    fprintf(stderr, "Invalid PC number\n");
    return false;
  }
  bool force = argc == 4;
  send_press_command(pc, force);
  return true;
}

void parse_ip_addr(const char *value, unsigned char ip[4]) {
  sscanf(value, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
}

void parse_mac_addr(const char *value, unsigned char mac[6]) {
  int tmac[6];
  sscanf(value, "%x:%x:%x:%x:%x:%x",
         &tmac[0], &tmac[1], &tmac[2], &tmac[3], &tmac[4], &tmac[5]);
  mac[0] = tmac[0];
  mac[1] = tmac[1];
  mac[2] = tmac[2];
  mac[3] = tmac[3];
  mac[4] = tmac[4];
  mac[5] = tmac[5];
}

bool parse_set_global_command(int argc, char **argv) {
  /* Number of arguments has been already checked by callee. */
  const char *variable = argv[2];
  const char *value = argv[3];
  if (strcmp(variable, "ip") == 0) {
    unsigned char ip[4];
    parse_ip_addr(value, ip);
    send_set_ip_command(ip);
  } else if (strcmp(variable, "mac") == 0) {
    unsigned char mac[6];
    parse_mac_addr(value, mac);
    send_set_mac_command(mac);
  } else {
    printf("Unknown variable %s. "
           "Supported variables are: ip, mac.\n", variable);
    return false;
  }
  return true;
}

bool parse_set_pc_command(int argc, char **argv) {
  /* Number of arguments has been already checked by callee. */
  int pc = atoi(argv[2]);
  if (!check_pc_valid(pc)) {
    fprintf(stderr, "Invalid PC number\n");
    return false;
  }
  const char *variable = argv[3];
  const char *value = argv[4];
  if (strcmp(variable, "autoboot") == 0) {
    if (strcmp(value, "on") != 0 && strcmp(value, "off") != 0) {
      printf("Unsupported value %s. Supported values: on, off\n", value);
      return false;
    }
    bool autoboot = strcmp(value, "on") == 0;
    send_set_autoboot_command(pc, autoboot);
  } else if (strcmp(variable, "name") == 0) {
    send_set_name_command(pc, value);
  } else {
    printf("Unknown variable %s. "
           "Supported variables are: autoboot, name.\n", variable);
    return false;
  }
  return true;
}

bool parse_set_command(int argc, char **argv) {
  if (argc == 4) {
    return parse_set_global_command(argc, argv);
  } else if (argc == 5) {
    return parse_set_pc_command(argc, argv);
  }
  printf("Usage: %s set [<pc>] <variable> <value>\n", argv[0]);
  return false;
}

void retrieve_and_print_ip(void) {
  unsigned char ip[4];
  send_get_ip_command();
  retrieve_ip(ip);
  printf("IP address: %d:%d:%d:%d\n",
         ip[0], ip[1], ip[2], ip[3]);
}

void retrieve_and_print_mac(void) {
  unsigned char mac[6];
  send_get_mac_command();
  retrieve_mac(mac);
  printf("Mac address: %x:%x:%x:%x:%x:%x\n",
         mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);
}

bool parse_get_global_command(int argc, char **argv) {
  /* Number of arguments has been already checked by callee. */
  const char *variable = argv[2];
  if (strcmp(variable, "ip") == 0) {
    retrieve_and_print_ip();
  } else if (strcmp(variable, "mac") == 0) {
    retrieve_and_print_mac();
  } else if (strcmp(variable, "status") == 0) {
    retrieve_and_print_ip();
    retrieve_and_print_mac();
    unsigned char buffer[64];
    send_get_status_command();
    retrieve_status(buffer);
    printf("Number of computers: %d\n", buffer[0]);
    for (int i = 0; i < buffer[0]; ++i) {
      unsigned char name[64];
      send_get_name_command(i);
      retrieve_name(name);
      printf("  Computer %d:\n", i);
      printf("    Name: %s\n",  name);
      printf("    Status: %d\n",  buffer[i + 1]);
    }
  } else {
    printf("Unknown variable %s. "
           "Supported variables are: ip, mac, status.\n", variable);
    return false;
  }
  return true;
}

bool parse_get_pc_command(int argc, char **argv) {
  /* Number of arguments has been already checked by callee. */
  int pc = atoi(argv[2]);
  if (!check_pc_valid(pc)) {
    fprintf(stderr, "Invalid PC number\n");
    return false;
  }
  const char *variable = argv[3];
  if (strcmp(variable, "autoboot") == 0) {
    send_get_autoboot_command(pc);
    bool enabled = retrieve_autoboot();
    printf("Computer %d: Autoboot is %s\n",
           pc, enabled ? "disabled" : "enabled");
  } else if (strcmp(variable, "name") == 0) {
    unsigned char name[64];
    send_get_name_command(pc);
    retrieve_name(name);
    printf("Computer name: %s\n", name);
  } else {
    printf("Unknown variable %s. "
           "Supported variables are: autoboot, name.\n", variable);
    return false;
  }
  return true;
}

bool parse_get_command(int argc, char **argv) {
  if (argc == 3) {
    return parse_get_global_command(argc, argv);
  } else if (argc == 4) {
    return parse_get_pc_command(argc, argv);
  }
  printf("Usage: %s set [<pc>] <variable> <value>\n", argv[0]);
  return false;
}

void print_usage(const char *argv0) {
  printf("Usage: %s test|"
         "press <pc> <force>|"
         "set <variable> [<pc>] <value>|"
         "get <variable> [<pc>]\n", argv0);
};

}  /* namespace */

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  int r = libusb_init(&ctx);
  if (r < 0) {
    fprintf(stderr, "Failed to initialise libusb\n");
    return EXIT_FAILURE;
  }

  /* Set verbosity level to 3, as suggested in the documentation. */
  libusb_set_debug(ctx, 3);

  r = find_lvr_hidusb();
  if (r < 0) {
    fprintf(stderr, "Could not find/open LVR Generic HID device\n");
    return EXIT_FAILURE;
  }

  printf("Successfully find the LVR Generic HID device\n");

  libusb_detach_kernel_driver(devh, 0);

#if 0
  r = libusb_set_configuration(devh, 1);
  if (r < 0) {
    fprintf(stderr, "libusb_set_configuration error %d\n", r);
    goto out;
  }

  printf("Successfully set usb configuration 1\n");
#endif

  r = libusb_claim_interface(devh, 0);
  if (r < 0) {
    fprintf(stderr, "libusb_claim_interface error %d\n", r);
    goto out;
  }

  printf("Successfully claimed interface\n");

  if (!strcmp(argv[1], "test")) {
    send_test_command();
  } else if (!strcmp(argv[1], "press")) {
    parse_press_command(argc, argv);
  } else if (!strcmp(argv[1], "set")) {
    parse_set_command(argc, argv);
  } else if (!strcmp(argv[1], "get")) {
    parse_get_command(argc, argv);
  } else {
    print_usage(argv[0]);
  }

  libusb_release_interface(devh, 0);

 out:
  /* libusb_reset_device(devh); */
  libusb_close(devh);
  libusb_exit(ctx);
  return r == 0 ? EXIT_SUCCESS : -EXIT_FAILURE;
}
