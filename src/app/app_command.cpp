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
#include "user_interface.h"
}

#include "espbot_cfgfile.hpp"
#include "espbot_cron.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_json.hpp"
#include "espbot_mem_macros.h"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_relay.hpp"
#include "app_command.hpp"
#include "app_event_codes.h"

static struct command command_list[MAX_COMMAND_COUNT];

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
                   char *name,
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
  os_memset(command_list[id].name, 0, 32);
  os_strncpy(command_list[id].name, name, 31);
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
  // command_list_save();
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
                   char *name,
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
  os_memset(command_list[id].name, 0, 32);
  os_strncpy(command_list[id].name, name, 31);
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
  // command_list_save();
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
  // command_list_save();
  return id;
}

// PERSISTENCY

#define COMMAND_LIST_FILENAME ((char *)f_str("command.cfg"))

// {"commands":[]}
// and each item:
// {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
// {
//   "commands": [
//     {
//       "id":3,
//       "enabled":1,
//       "name":"Switch on mobile router",
//       "type":2,
//       "duration":3600000,
//       "relay_id":5,
//       "min":0,
//       "hour":9,
//       "dom":255,
//       "month":255,
//       "dow":255
//     }
//   ]
// }

static int command_list_restore(void)
{
  ALL("command_list_restore");

  if (!Espfile::exists(COMMAND_LIST_FILENAME))
    return CFG_cantRestore;
  Cfgfile cfgfile(COMMAND_LIST_FILENAME);
  JSONP_ARRAY commands = cfgfile.getArray(f_str("commands"));
  if (cfgfile.getErr() != JSON_noerr)
  {
    dia_error_evnt(COMMAND_LIST_RESTORE_ERROR);
    ERROR("command_list_restore error");
    return CFG_error;
  }
  int idx;
  for (idx = 0; idx < commands.len(); idx++)
  {
    if (idx >= MAX_COMMAND_COUNT)
    {
      dia_warn_evnt(COMMAND_LIST_TRUNCATED, MAX_COMMAND_COUNT);
      WARN("command_list_restore truncated to %d commands", MAX_COMMAND_COUNT);
      break;
    }
    JSONP cmd_idx = commands.getObj(idx);
    if (commands.getErr() != JSON_noerr)
    {
      dia_error_evnt(COMMAND_LIST_RESTORE_ERROR);
      ERROR("command_list_restore error");
      return CFG_error;
    }
    command_list[idx].id = cmd_idx.getInt(f_str("id"));
    os_memset(command_list[idx].name, 0, 32);
    cmd_idx.getStr(f_str("name"), command_list[idx].name, 32);
    command_list[idx].enabled = (bool)cmd_idx.getInt(f_str("enabled"));
    command_list[idx].type = (enum command_t)cmd_idx.getInt(f_str("type"));
    command_list[idx].exe_time.minutes = cmd_idx.getInt(f_str("min"));
    command_list[idx].exe_time.hours = cmd_idx.getInt(f_str("hour"));
    command_list[idx].exe_time.day_of_week = cmd_idx.getInt(f_str("dow"));
    command_list[idx].exe_time.day_of_month = cmd_idx.getInt(f_str("dom"));
    command_list[idx].exe_time.month = cmd_idx.getInt(f_str("month"));
    command_list[idx].output = get_relay(cmd_idx.getInt(f_str("relay_id")));
    command_list[idx].duration = cmd_idx.getInt(f_str("duration"));
    if (cmd_idx.getErr() != JSON_noerr)
    {
      dia_error_evnt(COMMAND_LIST_RESTORE_ERROR);
      ERROR("command_list_restore error");
      return CFG_error;
    }
  }
  mem_mon_stack();
  return CFG_ok;
}

static int saved_command_list_updated(void)
{
  ALL("saved_command_list_updated");
  if (!Espfile::exists(COMMAND_LIST_FILENAME))
  {
    return CFG_notUpdated;
  }
  Cfgfile cfgfile(COMMAND_LIST_FILENAME);
  JSONP_ARRAY commands = cfgfile.getArray(f_str("commands"));
  mem_mon_stack();
  if (cfgfile.getErr() != JSON_noerr)
    return CFG_error;
  int idx;
  if( commands.len() != get_commands_count())
    return CFG_error;
  for (idx = 0; idx < commands.len(); idx++)
  {
    if (idx >= MAX_COMMAND_COUNT)
    {
      break;
    }
    JSONP cmd_idx = commands.getObj(idx);
    if (commands.getErr() != JSON_noerr)
    {
      // no need to raise an error, the cfg file will be overwritten
      return CFG_error;
    }
    struct command tmp_cmd;
    tmp_cmd.id = cmd_idx.getInt(f_str("id"));
    os_memset(tmp_cmd.name, 0, 32);
    cmd_idx.getStr(f_str("name"), tmp_cmd.name, 32);
    tmp_cmd.enabled = (bool)cmd_idx.getInt(f_str("enabled"));
    tmp_cmd.type = (enum command_t)cmd_idx.getInt(f_str("type"));
    tmp_cmd.exe_time.minutes = cmd_idx.getInt(f_str("min"));
    tmp_cmd.exe_time.hours = cmd_idx.getInt(f_str("hour"));
    tmp_cmd.exe_time.day_of_week = cmd_idx.getInt(f_str("dow"));
    tmp_cmd.exe_time.day_of_month = cmd_idx.getInt(f_str("dom"));
    tmp_cmd.exe_time.month = cmd_idx.getInt(f_str("month"));
    tmp_cmd.output = get_relay(cmd_idx.getInt(f_str("relay_id")));
    tmp_cmd.duration = cmd_idx.getInt(f_str("duration"));
    if (cmd_idx.getErr() != JSON_noerr)
    {
      // no need to raise an error, the cfg file will be overwritten
      return CFG_error;
    }
    if ((tmp_cmd.id != command_list[idx].id) ||
        (os_strncmp(tmp_cmd.name, command_list[idx].name, 32)) ||
        (tmp_cmd.job_id != command_list[idx].job_id) ||
        (tmp_cmd.enabled != (bool)command_list[idx].enabled) ||
        (tmp_cmd.type != (enum command_t)command_list[idx].type) ||
        (tmp_cmd.exe_time.minutes != command_list[idx].exe_time.minutes) ||
        (tmp_cmd.exe_time.hours != command_list[idx].exe_time.hours) ||
        (tmp_cmd.exe_time.day_of_week != command_list[idx].exe_time.day_of_week) ||
        (tmp_cmd.exe_time.day_of_month != command_list[idx].exe_time.day_of_month) ||
        (tmp_cmd.exe_time.month != command_list[idx].exe_time.month) ||
        (tmp_cmd.output != command_list[idx].output) ||
        (tmp_cmd.duration != command_list[idx].duration))
    {
      return CFG_notUpdated;
    }
  }
  return CFG_ok;
}

char *command_json_stringify(int idx, char *dest, int len)
{
  int msg_len = 98 + 2 + 1 + 32 + 2 + 10 + 2 + 3 + 3 + 2 + 2 + 1 + 1;
  char *msg;
  if (dest == NULL)
  {
    msg = new char[msg_len];
    if (msg == NULL)
    {
      dia_error_evnt(COMMAND_STRINGIFY_HEAP_EXHAUSTED, msg_len);
      ERROR("command_list_json_stringify heap exhausted [%d]", msg_len);
      return NULL;
    }
  }
  else
  {
    msg = dest;
    if (len < msg_len)
    {
      *msg = 0;
      return msg;
    }
  }
  struct command *cmd_ptr = command_read(idx);
  // not found
  if (cmd_ptr == NULL)
  {
    fs_sprintf(msg, "{}");
    return msg;
  }
  // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
  fs_sprintf(msg,
             "{\"id\":%d,\"enabled\":%d,\"name\":\"%s\",\"type\":%d,",
             cmd_ptr->id,
             cmd_ptr->enabled,
             cmd_ptr->name,
             cmd_ptr->type);
  fs_sprintf(msg + os_strlen(msg),
             "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
             cmd_ptr->duration,
             cmd_ptr->output->pin,
             ((cmd_ptr->exe_time.minutes == CRON_STAR) ? (-1) : cmd_ptr->exe_time.minutes));
  fs_sprintf(msg + os_strlen(msg),
             "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
             ((cmd_ptr->exe_time.hours == CRON_STAR) ? (-1) : cmd_ptr->exe_time.hours),
             ((cmd_ptr->exe_time.day_of_month == CRON_STAR) ? (0) : cmd_ptr->exe_time.day_of_month),
             ((cmd_ptr->exe_time.month == CRON_STAR) ? (0) : cmd_ptr->exe_time.month),
             ((cmd_ptr->exe_time.day_of_week == CRON_STAR) ? (0) : cmd_ptr->exe_time.day_of_week));
  mem_mon_stack();
  return msg;
}

char *command_list_json_stringify(char *dest, int len)
{
  int msg_len = 15 + (get_commands_count() * 98) + (get_commands_count() * (2 + 1 + 32 + 2 + 10 + 2 + 3 + 3 + 2 + 2 + 1)) + 1;
  char *msg;
  if (dest == NULL)
  {
    msg = new char[msg_len];
    if (msg == NULL)
    {
      dia_error_evnt(COMMAND_LIST_STRINGIFY_HEAP_EXHAUSTED, msg_len);
      ERROR("command_list_json_stringify heap exhausted [%d]", msg_len);
      return NULL;
    }
  }
  else
  {
    msg = dest;
    if (len < msg_len)
    {
      *msg = 0;
      return msg;
    }
  }
  fs_sprintf(msg, "{\"commands\":[");
  bool first_element = true;
  int idx;
  for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
  {
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    if (command_list[idx].id < 0)
      continue;
    if (first_element)
      first_element = false;
    else
      fs_sprintf(msg + os_strlen(msg), ",");
    fs_sprintf(msg + os_strlen(msg),
               "{\"id\":%d,\"enabled\":%d,\"name\":\"%s\",\"type\":%d,",
               command_list[idx].id,
               command_list[idx].enabled,
               command_list[idx].name,
               command_list[idx].type);
    fs_sprintf(msg + os_strlen(msg),
               "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
               command_list[idx].duration,
               command_list[idx].output->pin,
               ((command_list[idx].exe_time.minutes == CRON_STAR) ? (-1) : command_list[idx].exe_time.minutes));
    fs_sprintf(msg + os_strlen(msg),
               "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
               ((command_list[idx].exe_time.hours == CRON_STAR) ? (-1) : command_list[idx].exe_time.hours),
               ((command_list[idx].exe_time.day_of_month == CRON_STAR) ? (0) : command_list[idx].exe_time.day_of_month),
               ((command_list[idx].exe_time.month == CRON_STAR) ? (0) : command_list[idx].exe_time.month),
               ((command_list[idx].exe_time.day_of_week == CRON_STAR) ? (0) : command_list[idx].exe_time.day_of_week));
  }
  fs_sprintf(msg + os_strlen(msg), "]}");
  mem_mon_stack();
  return msg;
}

int command_list_save(void)
{
  ALL("command_list_save");
  if (saved_command_list_updated() == CFG_ok)
    return CFG_ok;
  Cfgfile cfgfile(COMMAND_LIST_FILENAME);
  if (cfgfile.clear() != SPIFFS_OK)
    return CFG_error;
  char *str = command_list_json_stringify();
  if (str == NULL)
    return CFG_error;
  int res = cfgfile.n_append(str, os_strlen(str));
  delete str;
  if (res < SPIFFS_OK)
    return CFG_error;
  mem_mon_stack();
  return CFG_ok;
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
  if (command_list_restore() != CFG_ok)
    // in case of errors reset to default
    for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
    {
      command_list[idx].id = -1;
    }
}