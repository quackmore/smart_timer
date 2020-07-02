/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __APP_RELAY_HPP__
#define __APP_RELAY_HPP__

#include "espbot_gpio.hpp"

enum contact_logic
{
  logic_undefined = 0,
  pin_low_relay_closed = 1,
  pin_high_relay_closed
};

enum contact_status
{
  status_undefined = 0,
  open = 1,
  closed
};

enum contact_pin
{
  d1 = ESPBOT_D1,
  d2 = ESPBOT_D2,
  d3 = ESPBOT_D3,
  d4 = ESPBOT_D4,
  d5 = ESPBOT_D5,
  d6 = ESPBOT_D6,
  d7 = ESPBOT_D7,
  d8 = ESPBOT_D8,
};

struct relay
{
  enum contact_pin pin;
  bool reserved;
  char name[32];
  os_timer_t timer;
  uint32 pulse;
  enum contact_logic logic;
  enum contact_status status_at_boot;
};

// {
//   "pin":,
//   "reserved":,
//   "name":"0123456789012345678901234567890",
//   "logic":,
//   "status_at_boot":
// }

void app_relay_init(void);

// conf_relay returns 0 for success, !0 for error
int conf_relay(enum contact_pin, char *, enum contact_logic, enum contact_status);
struct relay *get_relay(int pin);


void close_relay(struct relay *);
void open_relay(struct relay *);
void pulse_close_relay(struct relay *);
void pulse_open_relay(struct relay *);

#endif