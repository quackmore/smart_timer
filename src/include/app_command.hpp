/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __APP_COMMAND_HPP__
#define __APP_COMMAND_HPP__

#define MAX_COMMAND_COUNT 30

enum command_t
{
  switch_open = 1,
  switch_close,
  pulse_open,
  pulse_close
};

struct command
{
  int id;
  int job_id;
  bool enabled;
  enum command_t type;
  struct date exe_time; // minutes
  struct relay *output;
  uint32 duration; // milliseconds
};

void app_command_init(void);

void command_function(void *);

// returns command id
// or -1 on error
int command_create(bool enabled,
                  enum command_t type,
                  struct date *exe_time,
                  enum contact_pin pin,
                  uint32 duration = 0);
// returns NULL when id not found
struct command *command_read(int id);
// returns command id
// or -1 on error
int command_update(int id,
                  bool enabled,
                  enum command_t type,
                  struct date *exe_time,
                  enum contact_pin pin,
                  uint32 duration = 0);
// returns command id
// or -1 on error
int command_delete(int id);

#endif