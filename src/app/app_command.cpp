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
#include "app_command.hpp"

static struct command command_list[MAX_COMMAND_COUNT];

void command_function(void *);

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
  // FIXME: replace with flash values
  struct relay *cur_relay = get_relay(d5);

  command_list[0].id = 0;
  command_list[0].enabled = true;
  command_list[0].type = pulse_close;
  command_list[0].exe_time.minutes = 39;
  command_list[0].exe_time.hours = 10;
  command_list[0].exe_time.day_of_month = CRON_STAR;
  command_list[0].exe_time.month = CRON_STAR;
  command_list[0].exe_time.day_of_week = CRON_STAR;
  command_list[0].duration = 10000;
  command_list[0].output = cur_relay;
  command_list[0].job_id = cron_add_job(command_list[0].exe_time.minutes,
                                       command_list[0].exe_time.hours,
                                       command_list[0].exe_time.day_of_month,
                                       command_list[0].exe_time.month,
                                       command_list[0].exe_time.day_of_week,
                                       command_function,
                                       (void *)&command_list[0]);
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
  return id;
  // FIXME: save to flash
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
  // FIXME: save to flash
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
  // FIXME: save to flash
  return id;
}