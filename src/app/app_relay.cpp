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

#include "espbot_config.hpp"
#include "espbot_cron.hpp"
#include "espbot_global.hpp"
#include "espbot_json.hpp"
#include "espbot_mem_macros.h"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_relay.hpp"

static struct relay relays[8];

static bool restore_relay_list(void);
static void save_relay_list(void);

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
  restore_relay_list();
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
    save_relay_list();
    return 0;
  }
}

// PERSISTENCY
// {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}

#define RELAY_FILENAME f_str("relay.cfg")

static bool restore_relay_list(void)
{
  ALL("restore_relay_list");
  if (!espfs.is_available())
  {
    esp_diag.error(RESTORE_RELAY_LIST_FS_NOT_AVAILABLE);
    ERROR("restore_relay_list FS not available");
    return false;
  }
  File_to_json cfgfile(RELAY_FILENAME);
  if (!cfgfile.exists())
    return false;

  // relay_count
  if (cfgfile.find_string(f_str("relay_count")))
  {
    esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE);
    ERROR("restore_relay_list cannot find \"relay_count\"");
    return false;
  }
  int relays_count = atoi(cfgfile.get_value());
  // relays
  if (cfgfile.find_string(f_str("relays")))
  {
    esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE);
    ERROR("restore_relay_list cannot find \"relays\"");
    return false;
  }
  Json_array_str relays_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((relays_array.syntax_check() != JSON_SINTAX_OK) || (relays_array.size() != relays_count))
  {
    esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE);
    ERROR("restore_relay_list incorrect array sintax");
    return false;
  }
  int idx;
  int pin;
  int relay_id;
  for (idx = 0; idx < relays_count; idx++)
  {
    if ((relays_array.get_elem(idx) == NULL) || (relays_array.get_elem_type(idx) != JSON_OBJECT))
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    Json_str relays_x(relays_array.get_elem(idx), relays_array.get_elem_len(idx));
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    // pin
    if (relays_x.find_pair(f_str("pin")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    pin = atoi(relays_x.get_cur_pair_value());
    if ((pin < 1) || (pin > 7))
      return false;
    relay_id = pin - 1;
    relays[relay_id].pin = (enum contact_pin)pin;
    // reserved
    if (relays_x.find_pair(f_str("reserved")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    relays[relay_id].reserved = atoi(relays_x.get_cur_pair_value());
    // name
    if (relays_x.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_STRING)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    os_memset(relays[relay_id].name, 0, 32);
    os_strncpy(relays[relay_id].name,
               relays_x.get_cur_pair_value(),
               ((relays_x.get_cur_pair_value_len() > 31) ? 31 : relays_x.get_cur_pair_value_len()));
    // logic
    if (relays_x.find_pair(f_str("logic")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    relays[relay_id].logic = (enum contact_logic)atoi(relays_x.get_cur_pair_value());
    // status_at_boot
    if (relays_x.find_pair(f_str("status_at_boot")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_RELAY_LIST_INCOMPLETE, idx);
      ERROR("restore_relay_list array[%d] bad sintax", idx);
      return false;
    }
    relays[relay_id].status_at_boot = (enum contact_status)atoi(relays_x.get_cur_pair_value());
  }
  espmem.stack_mon();
  return true;
}

static bool saved_relay_list_not_updated(void)
{
  ALL("saved_relay_list_not_updated");
  if (!espfs.is_available())
  {
    esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_FS_NOT_AVAILABLE);
    ERROR("saved_relay_list_not_updated FS not available");
    return true;
  }
  File_to_json cfgfile(RELAY_FILENAME);
  if (!cfgfile.exists())
    return true;

  // relay_count
  if (cfgfile.find_string(f_str("relay_count")))
  {
    esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_relay_list_not_updated cannot find \"relay_count\"");
    return true;
  }
  int relays_count = atoi(cfgfile.get_value());
  // relays
  if (cfgfile.find_string(f_str("relays")))
  {
    esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_relay_list_not_updated cannot find \"relays\"");
    return true;
  }
  Json_array_str relays_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((relays_array.syntax_check() != JSON_SINTAX_OK) || (relays_array.size() != 8))
  {
    esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_relay_list_not_updated incorrect array sintax");
    return true;
  }
  int idx;
  int pin;
  int relay_id;
  for (idx = 0; idx < relays_count; idx++)
  {
    if ((relays_array.get_elem(idx) == NULL) || (relays_array.get_elem_type(idx) != JSON_OBJECT))
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    Json_str relays_x(relays_array.get_elem(idx), relays_array.get_elem_len(idx));
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    // pin
    if (relays_x.find_pair(f_str("pin")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    pin = atoi(relays_x.get_cur_pair_value());
    if ((pin < 1) || (pin > 7))
      return true;
    relay_id = pin - 1;
    if (relays[relay_id].pin != pin)
      return true;
    // reserved
    if (relays_x.find_pair(f_str("reserved")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays[relay_id].reserved != atoi(relays_x.get_cur_pair_value()))
      return true;
    // name
    if (relays_x.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_STRING)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (os_strncmp(relays[relay_id].name, relays_x.get_cur_pair_value(), 31))
      return true;
    // logic
    if (relays_x.find_pair(f_str("logic")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays[relay_id].logic != ((enum contact_logic)atoi(relays_x.get_cur_pair_value())))
      return true;
    // status_at_boot
    if (relays_x.find_pair(f_str("status_at_boot")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_RELAY_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("saved_relay_list array[%d] bad sintax", idx);
      return true;
    }
    if (relays[relay_id].status_at_boot != ((enum contact_status)atoi(relays_x.get_cur_pair_value())))
      return true;
  }
  espmem.stack_mon();
  return false;
}

static void remove_relay_list(void)
{
  ALL("remove_relay_list");
  if (!espfs.is_available())
  {
    esp_diag.error(REMOVE_RELAY_LIST_FS_NOT_AVAILABLE);
    ERROR("remove_relay_list FS not available");
    return;
  }
  if (Ffile::exists(&espfs, (char *)RELAY_FILENAME))
  {
    Ffile cfgfile(&espfs, (char *)RELAY_FILENAME);
    cfgfile.remove();
  }
}

static void save_relay_list(void)
{
  ALL("save_relay_list");
  if (saved_relay_list_not_updated())
    remove_relay_list();
  else
    return;
  if (!espfs.is_available())
  {
    esp_diag.error(SAVE_RELAY_LIST_FS_NOT_AVAILABLE);
    ERROR("save_relay_list FS not available");
    return;
  }
  Ffile cfgfile(&espfs, (char *)RELAY_FILENAME);
  if (!cfgfile.is_available())
  {
    esp_diag.error(SAVE_RELAY_LIST_CANNOT_OPEN_FILE);
    ERROR("save_relay_list cannot open %s", RELAY_FILENAME);
    return;
  }
  TRACE("Writing relay list to flash...");
  {
    //  {"relay_count":,"relays":[
    char buffer[(26 + 1 + 2)];
    fs_sprintf(buffer, "{\"relay_count\":%d,\"relays\":[", 8);
    cfgfile.n_append(buffer, os_strlen(buffer));
    espmem.stack_mon();
  }
  int idx;
  bool first_time = true;
  for (idx = 0; idx < 8; idx++)
  {
    // ,{"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    char buffer[(58 + 1 + 2 + 1 + 31 + 1 + 1)];
    if (first_time)
    {
      first_time = false;
      fs_sprintf(buffer, "");
    }
    else
    {
      fs_sprintf(buffer, ",");
    }
    fs_sprintf(buffer + os_strlen(buffer),
               "{\"pin\":%d,\"reserved\":%d,",
               relays[idx].pin,
               relays[idx].reserved);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"name\":\"%s\",",
               relays[idx].name);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"logic\":%d,\"status_at_boot\":%d}",
               relays[idx].logic,
               relays[idx].status_at_boot);
    cfgfile.n_append(buffer, os_strlen(buffer));
    espmem.stack_mon();
  }
  {
    //  ]}
    char buffer[(2 + 1)];
    fs_sprintf(buffer, "]}");
    cfgfile.n_append(buffer, os_strlen(buffer));
    espmem.stack_mon();
  }
}