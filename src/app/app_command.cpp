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
#include "app_relay.hpp"
#include "app_command.hpp"
#include "app_event_codes.h"

static struct command command_list[MAX_COMMAND_COUNT];

static bool restore_command_list(void);
static void save_command_list(void);

void command_function(void *);

static int get_commands_count(void)
{
  int idx;
  int memory_commands_count = 0;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id >= 0)
      memory_commands_count++;
  }
  return memory_commands_count;
}

void app_command_init(void)
{

  int idx;
  // init commands
  // default init
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    command_list[idx].id = -1;
  }
  // actual init
  restore_command_list();
}

void command_function(void *param)
{
  TRACE("executing command function");
  struct command *command_p = (struct command *)param;
  if (command_p->enabled)
  {
    TRACE("command %d enabled, type %d", command_p->id, command_p->type);
    switch (command_p->type)
    {
    case switch_close:
      close_relay(command_p->output);
      break;
    case switch_open:
      open_relay(command_p->output);
      break;
    case pulse_close:
      command_p->output->pulse = command_p->duration;
      pulse_close_relay(command_p->output);
      break;
    case pulse_open:
      command_p->output->pulse = command_p->duration;
      pulse_open_relay(command_p->output);
      break;
    default:
      break;
    }
  }
}

static int get_first_free_id(void)
{
  int idx;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id < 0)
      return idx;
  }
  if (idx >= MAX_COMMAND_COUNT)
    return -1;
}

int command_create(bool enabled,
                   enum command_t type,
                   struct date *exe_time,
                   enum contact_pin pin,
                   uint32 duration)
{
  int id = get_first_free_id();
  if (id < 0)
    // there are no available ids
    return id;
  command_list[id].id = id;
  command_list[id].enabled = enabled;
  command_list[id].type = type;
  command_list[id].exe_time.minutes = exe_time->minutes;
  command_list[id].exe_time.hours = exe_time->hours;
  command_list[id].exe_time.day_of_month = exe_time->day_of_month;
  command_list[id].exe_time.month = exe_time->month;
  command_list[id].exe_time.day_of_week = exe_time->day_of_week;
  command_list[id].output = get_relay(pin);
  command_list[id].duration = duration;
  if (enabled)
  {
    command_list[id].job_id = cron_add_job(command_list[id].exe_time.minutes,
                                           command_list[id].exe_time.hours,
                                           command_list[id].exe_time.day_of_month,
                                           command_list[id].exe_time.month,
                                           command_list[id].exe_time.day_of_week,
                                           command_function,
                                           (void *)&command_list[id]);
  }
  save_command_list();
  return id;
}

struct command *command_read(int id)
{
  int idx;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id == id)
      return &command_list[idx];
  }
  return NULL;
}

int command_update(int id,
                   bool enabled,
                   enum command_t type,
                   struct date *exe_time,
                   enum contact_pin pin,
                   uint32 duration)
{
  int idx;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id == id)
      break;
  }
  // id not found
  if (idx >= MAX_COMMAND_COUNT)
    return -1;

  // if the command was enabled stop the cron job
  if (command_list[idx].enabled)
  {
    cron_del_job(command_list[idx].job_id);
  }
  command_list[idx].enabled = enabled;
  command_list[idx].type = type;
  command_list[idx].exe_time.minutes = exe_time->minutes;
  command_list[idx].exe_time.hours = exe_time->hours;
  command_list[idx].exe_time.day_of_month = exe_time->day_of_month;
  command_list[idx].exe_time.month = exe_time->month;
  command_list[idx].exe_time.day_of_week = exe_time->day_of_week;
  command_list[idx].output = get_relay(pin);
  command_list[idx].duration = duration;
  if (enabled)
  {
    command_list[idx].job_id = cron_add_job(command_list[idx].exe_time.minutes,
                                            command_list[idx].exe_time.hours,
                                            command_list[idx].exe_time.day_of_month,
                                            command_list[idx].exe_time.month,
                                            command_list[idx].exe_time.day_of_week,
                                            command_function,
                                            (void *)&command_list[idx]);
  }
  save_command_list();
  return command_list[idx].id;
}

int command_delete(int id)
{
  int idx;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id == id)
      break;
  }
  // id not found
  if (idx >= MAX_COMMAND_COUNT)
    return -1;

  // if the command was enabled stop the cron job
  if (command_list[idx].enabled)
  {
    cron_del_job(command_list[idx].job_id);
  }
  // delete the command
  command_list[idx].id = -1;
  save_command_list();
  return id;
}

// PERSISTENCY 
// {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}

#define COMMAND_FILENAME f_str("command.cfg")

static bool restore_command_list(void)
{
  ALL("restore_command_list");
  if (!espfs.is_available())
  {
    esp_diag.error(RESTORE_COMMAND_LIST_FS_NOT_AVAILABLE);
    ERROR("restore_command_list FS not available");
    return false;
  }
  File_to_json cfgfile(COMMAND_FILENAME);
  if (!cfgfile.exists())
    return false;

  // command_count
  if (cfgfile.find_string(f_str("command_count")))
  {
    esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE);
    ERROR("restore_command_list cannot find \"command_count\"");
    return false;
  }
  int commands_count = atoi(cfgfile.get_value());
  // commands
  if (cfgfile.find_string(f_str("commands")))
  {
    esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE);
    ERROR("restore_command_list cannot find \"commands\"");
    return false;
  }
  Json_array_str commands_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((commands_array.syntax_check() != JSON_SINTAX_OK) || (commands_array.size() != commands_count))
  {
    esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE);
    ERROR("restore_command_list incorrect array sintax");
    return false;
  }
  int idx;
  int id;
  int command_id;
  for (idx = 0; idx < commands_count; idx++)
  {
    if ((commands_array.get_elem(idx) == NULL) || (commands_array.get_elem_type(idx) != JSON_OBJECT))
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    Json_str commands_x(commands_array.get_elem(idx), commands_array.get_elem_len(idx));
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    // id
    if (commands_x.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    id = atoi(commands_x.get_cur_pair_value());
    if ((id < 0) || (id > MAX_COMMAND_COUNT))
      return false;
    command_id = id;
    command_list[command_id].id = id;
    // enabled
    if (commands_x.find_pair(f_str("enabled")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].enabled = atoi(commands_x.get_cur_pair_value());
    // type
    if (commands_x.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].type = (enum command_t)atoi(commands_x.get_cur_pair_value());
    // duration
    if (commands_x.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].duration = atoi(commands_x.get_cur_pair_value());
    // relay_id
    if (commands_x.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].output = get_relay(atoi(commands_x.get_cur_pair_value()));
    // min
    if (commands_x.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].exe_time.minutes = atoi(commands_x.get_cur_pair_value());
    // hour
    if (commands_x.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].exe_time.hours = atoi(commands_x.get_cur_pair_value());
    // dom
    if (commands_x.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].exe_time.day_of_month = atoi(commands_x.get_cur_pair_value());
    // month
    if (commands_x.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].exe_time.month = atoi(commands_x.get_cur_pair_value());
    // dow
    if (commands_x.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    command_list[command_id].exe_time.day_of_week = atoi(commands_x.get_cur_pair_value());
    // finally add the cron job
    if (command_list[command_id].enabled)
    {
      command_list[command_id].job_id = cron_add_job(command_list[command_id].exe_time.minutes,
                                                     command_list[command_id].exe_time.hours,
                                                     command_list[command_id].exe_time.day_of_month,
                                                     command_list[command_id].exe_time.month,
                                                     command_list[command_id].exe_time.day_of_week,
                                                     command_function,
                                                     (void *)&command_list[command_id]);
    }
  }
  espmem.stack_mon();
  return true;
}

static bool saved_command_list_not_updated(void)
{
  ALL("saved_command_list_not_updated");
  if (!espfs.is_available())
  {
    esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_FS_NOT_AVAILABLE);
    ERROR("saved_command_list_not_updated FS not available");
    return true;
  }
  File_to_json cfgfile(COMMAND_FILENAME);
  if (!cfgfile.exists())
    return true;

  // command_count
  if (cfgfile.find_string(f_str("command_count")))
  {
    esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_command_list_not_updated cannot find \"command_count\"");
    return true;
  }
  int commands_count = atoi(cfgfile.get_value());
  // count commands in memory
  int memory_commands_count = get_commands_count();
  if (commands_count != memory_commands_count)
    return true;
  // commands
  if (cfgfile.find_string(f_str("commands")))
  {
    esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_command_list_not_updated cannot find \"commands\"");
    return true;
  }
  Json_array_str commands_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((commands_array.syntax_check() != JSON_SINTAX_OK) || (commands_array.size() != 8))
  {
    esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_command_list_not_updated incorrect array sintax");
    return true;
  }
  int idx;
  int id;
  int command_id;
  for (idx = 0; idx < commands_count; idx++)
  {
    if ((commands_array.get_elem(idx) == NULL) || (commands_array.get_elem_type(idx) != JSON_OBJECT))
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    Json_str commands_x(commands_array.get_elem(idx), commands_array.get_elem_len(idx));
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    // id
    if (commands_x.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    id = atoi(commands_x.get_cur_pair_value());
    if ((id < 0) || (id > MAX_COMMAND_COUNT))
      return true;
    command_id = id;
    if (command_list[command_id].id != id)
      return true;
    // enabled
    if (commands_x.find_pair(f_str("enabled")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].enabled != atoi(commands_x.get_cur_pair_value()))
      return true;
    // type
    if (commands_x.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].type != ((enum command_t)atoi(commands_x.get_cur_pair_value())))
      return true;
    // duration
    if (commands_x.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].duration != atoi(commands_x.get_cur_pair_value()))
      return true;
    // relay_id
    if (commands_x.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    struct relay *relay_ptr = get_relay(atoi(commands_x.get_cur_pair_value()));
    if (command_list[command_id].output != relay_ptr)
      return true;
    // min
    if (commands_x.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.minutes != atoi(commands_x.get_cur_pair_value()))
      return true;
    // hour
    if (commands_x.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.hours != atoi(commands_x.get_cur_pair_value()))
      return true;
    // dom
    if (commands_x.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.day_of_month != atoi(commands_x.get_cur_pair_value()))
      return true;
    // month
    if (commands_x.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.month != atoi(commands_x.get_cur_pair_value()))
      return true;
    // dow
    if (commands_x.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      esp_diag.error(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.day_of_week != atoi(commands_x.get_cur_pair_value()))
      return true;
  }
  espmem.stack_mon();
  return false;
}

static void remove_command_list(void)
{
  ALL("remove_command_list");
  if (!espfs.is_available())
  {
    esp_diag.error(REMOVE_COMMAND_LIST_FS_NOT_AVAILABLE);
    ERROR("remove_command_list FS not available");
    return;
  }
  if (Ffile::exists(&espfs, (char *)COMMAND_FILENAME))
  {
    Ffile cfgfile(&espfs, (char *)COMMAND_FILENAME);
    cfgfile.remove();
  }
}

static void save_command_list(void)
{
  ALL("save_command_list");
  if (saved_command_list_not_updated())
    remove_command_list();
  else
    return;
  if (!espfs.is_available())
  {
    esp_diag.error(SAVE_COMMAND_LIST_FS_NOT_AVAILABLE);
    ERROR("save_command_list FS not available");
    return;
  }
  Ffile cfgfile(&espfs, (char *)COMMAND_FILENAME);
  if (!cfgfile.is_available())
  {
    esp_diag.error(SAVE_COMMAND_LIST_CANNOT_OPEN_FILE);
    ERROR("save_command_list cannot open %s", COMMAND_FILENAME);
    return;
  }
  TRACE("Writing command list to flash...");
  {
    int memory_commands_count = get_commands_count();
    //  {"command_count":,"commands":[
    char buffer[(30 + 1 + 2)];
    fs_sprintf(buffer, "{\"command_count\":%d,\"commands\":[", memory_commands_count);
    cfgfile.n_append(buffer, os_strlen(buffer));
    espmem.stack_mon();
  }
  int idx;
  bool first_time = true;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    if (command_list[idx].id < 0)
      continue;
    // ,{"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    char buffer[(89 + 1 +
                 2 +
                 1 +
                 2 +
                 10 +
                 1 +
                 3 +
                 3 +
                 2 +
                 2 +
                 1)];
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
               "{\"id\":%d,\"enabled\":%d,\"type\":%d,\"duration\":%d,",
               command_list[idx].id,
               command_list[idx].enabled,
               command_list[idx].type,
               command_list[idx].duration);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"relay_id\":%d,\"min\":%d,\"hour\":%d,",
               command_list[idx].output->pin,
               command_list[idx].exe_time.minutes,
               command_list[idx].exe_time.hours);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"dom\":%d,\"month\":%d,\"dow\":%d}",
               command_list[idx].exe_time.day_of_month,
               command_list[idx].exe_time.month,
               command_list[idx].exe_time.day_of_week);
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