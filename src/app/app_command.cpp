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
    command_list[idx].job_id = cmd_idx.getInt(f_str("job_id"));
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
    tmp_cmd.job_id = cmd_idx.getInt(f_str("job_id"));
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
  // out of boundary index
  if ((idx < 0) || (idx >= MAX_COMMAND_COUNT))
  {
    *msg = 0;
    return msg;
  }
  // empty command
  if (command_list[idx].id < 0)
  {
    fs_sprintf(msg, "{}");
    return msg;
  }
  // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
  fs_sprintf(msg,
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

/*
#define COMMAND_FILENAME f_str("command.cfg")

static bool restore_command_list(void)
{
  ALL("restore_command_list");
  if (!espfs.is_available())
  {
    dia_error_evnt(RESTORE_COMMAND_LIST_FS_NOT_AVAILABLE);
    ERROR("restore_command_list FS not available");
    return false;
  }
  File_to_json cfgfile(COMMAND_FILENAME);
  if (!cfgfile.exists())
    return false;

  // command_count
  if (cfgfile.find_string(f_str("command_count")))
  {
    dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE);
    ERROR("restore_command_list cannot find \"command_count\"");
    return false;
  }
  int commands_count = atoi(cfgfile.get_value());
  // commands
  if (cfgfile.find_string(f_str("commands")))
  {
    dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE);
    ERROR("restore_command_list cannot find \"commands\"");
    return false;
  }
  Json_array_str commands_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((commands_array.syntax_check() != JSON_SINTAX_OK) || (commands_array.size() != commands_count))
  {
    dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE);
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
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return false;
    }
    Json_str commands_x(commands_array.get_elem(idx), commands_array.get_elem_len(idx));
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    // id
    if (commands_x.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] id not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] id not INTEGER", idx);
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
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] enabled not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] enabled not INTEGER", idx);
      return false;
    }
    command_list[command_id].enabled = atoi(commands_x.get_cur_pair_value());
    // name
    if (commands_x.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] name not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_STRING)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] name not STRING", idx);
      return false;
    }
    os_memset(command_list[command_id].name, 0, 32);
    os_strncpy(command_list[command_id].name,
               commands_x.get_cur_pair_value(),
               ((commands_x.get_cur_pair_value_len() > 31) ? 31 : commands_x.get_cur_pair_value_len()));
    // type
    if (commands_x.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] type not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] type not INTEGER", idx);
      return false;
    }
    command_list[command_id].type = (enum command_t)atoi(commands_x.get_cur_pair_value());
    // duration
    if (commands_x.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] duration not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] duration not INTEGER", idx);
      return false;
    }
    command_list[command_id].duration = atoi(commands_x.get_cur_pair_value());
    // relay_id
    if (commands_x.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] relay_id not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] relay_id not INTEGER", idx);
      return false;
    }
    command_list[command_id].output = get_relay(atoi(commands_x.get_cur_pair_value()));
    // min
    if (commands_x.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] min not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] min not INTEGER", idx);
      return false;
    }
    command_list[command_id].exe_time.minutes = atoi(commands_x.get_cur_pair_value());
    // hour
    if (commands_x.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] hour not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] hor not INTEGER", idx);
      return false;
    }
    command_list[command_id].exe_time.hours = atoi(commands_x.get_cur_pair_value());
    // dom
    if (commands_x.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] dom not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] dom not INTEGER", idx);
      return false;
    }
    command_list[command_id].exe_time.day_of_month = atoi(commands_x.get_cur_pair_value());
    // month
    if (commands_x.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] month not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] month not INTEGER", idx);
      return false;
    }
    command_list[command_id].exe_time.month = atoi(commands_x.get_cur_pair_value());
    // dow
    if (commands_x.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] dow not found", idx);
      return false;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(RESTORE_COMMAND_LIST_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] dow not INTEGER", idx);
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
    dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_FS_NOT_AVAILABLE);
    ERROR("saved_command_list_not_updated FS not available");
    return true;
  }
  File_to_json cfgfile(COMMAND_FILENAME);
  if (!cfgfile.exists())
    return true;

  // command_count
  if (cfgfile.find_string(f_str("command_count")))
  {
    dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
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
    dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
    ERROR("saved_command_list_not_updated cannot find \"commands\"");
    return true;
  }
  Json_array_str commands_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
  if ((commands_array.syntax_check() != JSON_SINTAX_OK) || (commands_array.size() != commands_count))
  {
    dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE);
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
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    Json_str commands_x(commands_array.get_elem(idx), commands_array.get_elem_len(idx));
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    // id
    if (commands_x.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
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
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].enabled != atoi(commands_x.get_cur_pair_value()))
      return true;
    // name
    if (commands_x.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_STRING)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    char command_name[32];
    os_memset(command_name, 0, 32);
    os_strncpy(command_name,
               commands_x.get_cur_pair_value(),
               ((commands_x.get_cur_pair_value_len() > 31) ? 31 : commands_x.get_cur_pair_value_len()));
    if (os_strcmp(command_list[command_id].name, command_name))
      return true;
    // type
    if (commands_x.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].type != ((enum command_t)atoi(commands_x.get_cur_pair_value())))
      return true;
    // duration
    if (commands_x.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].duration != atoi(commands_x.get_cur_pair_value()))
      return true;
    // relay_id
    if (commands_x.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    struct relay *relay_ptr = get_relay(atoi(commands_x.get_cur_pair_value()));
    if (command_list[command_id].output != relay_ptr)
      return true;
    // min
    if (commands_x.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.minutes != atoi(commands_x.get_cur_pair_value()))
      return true;
    // hour
    if (commands_x.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.hours != atoi(commands_x.get_cur_pair_value()))
      return true;
    // dom
    if (commands_x.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.day_of_month != atoi(commands_x.get_cur_pair_value()))
      return true;
    // month
    if (commands_x.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (command_list[command_id].exe_time.month != atoi(commands_x.get_cur_pair_value()))
      return true;
    // dow
    if (commands_x.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
      ERROR("restore_command_list array[%d] bad sintax", idx);
      return true;
    }
    if (commands_x.get_cur_pair_value_type() != JSON_INTEGER)
    {
      dia_error_evnt(SAVED_COMMAND_LIST_NOT_UPDATED_INCOMPLETE, idx);
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
    dia_error_evnt(REMOVE_COMMAND_LIST_FS_NOT_AVAILABLE);
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
    dia_error_evnt(SAVE_COMMAND_LIST_FS_NOT_AVAILABLE);
    ERROR("save_command_list FS not available");
    return;
  }
  Ffile cfgfile(&espfs, (char *)COMMAND_FILENAME);
  if (!cfgfile.is_available())
  {
    dia_error_evnt(SAVE_COMMAND_LIST_CANNOT_OPEN_FILE);
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
    system_soft_wdt_feed();
    if (command_list[idx].id < 0)
      continue;
    // ,{"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    char buffer[(99 + 1 +
                 2 +
                 1 +
                 32 +
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
               "{\"id\":%d,\"enabled\":%d,\"name\":\"%s\",",
               command_list[idx].id,
               command_list[idx].enabled,
               command_list[idx].name);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"type\":%d,\"duration\":%d,",
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
*/