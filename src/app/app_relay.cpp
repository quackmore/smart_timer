/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

extern "C"
{
#include "c_types.h"
}

#include "espbot_cron.hpp"
#include "espbot_global.hpp"
#include "app.hpp"
#include "app_relay.hpp"

static struct relay relays[8];

void close_relay(struct relay *relay_p)
{
  if (relay_p->logic == pin_low_relay_closed)
    esp_gpio.set(relay_p->pin, ESPBOT_LOW);
  if (relay_p->logic == pin_high_relay_closed)
    esp_gpio.set(relay_p->pin, ESPBOT_HIGH);
}

void open_relay(struct relay *relay_p)
{
  if (relay_p->logic == pin_low_relay_closed)
    esp_gpio.set(relay_p->pin, ESPBOT_HIGH);
  if (relay_p->logic == pin_high_relay_closed)
    esp_gpio.set(relay_p->pin, ESPBOT_LOW);
}

void pulse_close_relay(struct relay *relay_p)
{
  os_timer_setfn(&relay_p->timer, (ETSTimerFunc *)open_relay, (void *)relay_p);
  os_timer_arm(&relay_p->timer, relay_p->pulse, 0);
  close_relay(relay_p);
}

void pulse_open_relay(struct relay *relay_p)
{
  os_timer_setfn(&relay_p->timer, (ETSTimerFunc *)close_relay, (void *)relay_p);
  os_timer_arm(&relay_p->timer, relay_p->pulse, 0);
  open_relay(relay_p);
}

void app_relay_init(void)
{

  int idx;
  // init relays
  // default init
  for (idx = 0; idx < 8; idx++)
  {
    relays[idx].pin = (contact_pin)(idx + 1);
    relays[idx].reserved = false;
    os_memset(relays[idx].name, 0, 32);
    // timer is not initialized
    relays[idx].pulse = 0;
    relays[idx].logic = logic_undefined;
    relays[idx].status_at_boot = status_undefined;
  }
  // reserved GPIO
  // d4 new event led (espbot_diagnostic)
  struct relay *cur_relay = get_relay(d4);
  cur_relay->reserved = true;
  os_strcpy(cur_relay->name, f_str("new_event"));
  cur_relay->logic = pin_low_relay_closed;
  cur_relay->status_at_boot = open;

  // actual init
  // FIXME: replace with flash values
  cur_relay = get_relay(d5);
  os_strcpy(cur_relay->name, f_str("timer_plug"));
  cur_relay->logic = pin_low_relay_closed;
  cur_relay->status_at_boot = open;
  esp_gpio.config(cur_relay->pin, ESPBOT_GPIO_OUTPUT);
  esp_gpio.set(cur_relay->pin, ESPBOT_HIGH);
}

struct relay *get_relay(int pin)
{
  int idx;
  if ((pin < d1) || (pin > d8))
    return NULL;
  for (idx = 0; idx < 8; idx++)
  {
    if (relays[idx].pin == pin)
      return &relays[idx];
  }
  return NULL;
}

int conf_relay(enum contact_pin pin, char *name, enum contact_logic logic, enum contact_status boot_status)
{
  struct relay *cur_relay = get_relay(pin);
  if (cur_relay->reserved)
  {
    return 1;
  }
  else
  {
    os_memset(cur_relay->name, 0, 32);
    os_strncpy(cur_relay->name, name, 31);
    cur_relay->logic = logic;
    cur_relay->status_at_boot = boot_status;
    esp_gpio.config(cur_relay->pin, ESPBOT_GPIO_OUTPUT);
    if (cur_relay->status_at_boot == open)
      open_relay(cur_relay);
    else if (cur_relay->status_at_boot == closed)
      close_relay(cur_relay);
    // FIXME: save to flash
    return 0;
  }
}