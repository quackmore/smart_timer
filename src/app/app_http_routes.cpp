/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

// SDK includes
extern "C"
{
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "ip_addr.h"
}

#include "espbot.hpp"
#include "espbot_cron.hpp"
#include "espbot_debug.hpp"
#include "espbot_global.hpp"
#include "espbot_http_routes.hpp"
#include "espbot_json.hpp"
#include "espbot_logger.hpp"
#include "espbot_utils.hpp"
#include "espbot_webserver.hpp"
#include "library.hpp"
#include "app.hpp"
#include "app_relay.hpp"
#include "app_command.hpp"
#include "app_event_codes.h"

static void get_api_command(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"commands":[]}
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    ALL("get_api_command");
    int command_count = 0;
    int idx;
    struct command *command_ptr;
    for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
    {
        command_ptr = command_read(idx);
        if (command_ptr)
            command_count++;
    }
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    int str_len = 15 + (command_count * 88) + (command_count * (2 + 1 + 2 + 10 + 2 + 3 + 3 + 2 + 2 + 1)) + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_COMMAND_HEAP_EXHAUSTED, str_len);
        ERROR("get_api_command heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    // {"commands":[]}
    fs_sprintf(msg.ref, "{\"commands\":[");
    bool first_element = true;
    for (idx = 0; idx < MAX_COMMAND_COUNT; idx++)
    {
        // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
        command_ptr = command_read(idx);
        if (command_ptr == NULL)
            continue;
        if (first_element)
            first_element = false;
        else
            fs_sprintf(msg.ref + os_strlen(msg.ref), ",");
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "{\"id\":%d,\"enabled\":%d,\"type\":%d,",
                   command_ptr->id,
                   command_ptr->enabled,
                   command_ptr->type);
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
                   command_ptr->duration,
                   command_ptr->output->pin,
                   ((command_ptr->exe_time.minutes == CRON_STAR) ? (-1) : command_ptr->exe_time.minutes));
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
                   ((command_ptr->exe_time.hours == CRON_STAR) ? (-1) : command_ptr->exe_time.hours),
                   ((command_ptr->exe_time.day_of_month == CRON_STAR) ? (0) : command_ptr->exe_time.day_of_month),
                   ((command_ptr->exe_time.month == CRON_STAR) ? (0) : command_ptr->exe_time.month),
                   ((command_ptr->exe_time.day_of_week == CRON_STAR) ? (0) : command_ptr->exe_time.day_of_week));
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref), "]}");

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void post_api_command(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    ALL("post_api_command");
    Json_str new_command(parsed_req->req_content, parsed_req->content_len);
    if (new_command.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // enabled
    if (new_command.find_pair(f_str("enabled")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'enabled'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'enabled' does not have a INTEGER type"), false);
        return;
    }
    bool enabled = ((atoi(new_command.get_cur_pair_value()) == 0) ? false : true);
    // type
    if (new_command.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'type'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'type' does not have a INTEGER value"), false);
        return;
    }
    enum command_t type = (enum command_t)atoi(new_command.get_cur_pair_value());
    if ((type < switch_open) || (type > pulse_close))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("type value out of range"), false);
        return;
    }
    // duration
    if (new_command.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'duration'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'duration' does not have a INTEGER value"), false);
        return;
    }
    uint32 duration = atoi(new_command.get_cur_pair_value());
    if (duration < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("duration has negatie value"), false);
        return;
    }
    // relay_id
    if (new_command.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'relay_id'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'relay_id' does not have a INTEGER value"), false);
        return;
    }
    enum contact_pin relay_id = (enum contact_pin)atoi(new_command.get_cur_pair_value());
    if ((relay_id < d1) || (relay_id > d8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay_id value out of range"), false);
        return;
    }
    struct date exe_time;
    // min
    if (new_command.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'min'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'min' does not have a INTEGER value"), false);
        return;
    }
    exe_time.minutes = atoi(new_command.get_cur_pair_value());
    if ((exe_time.minutes < -1) || (exe_time.minutes > 59))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("minutes value out of range"), false);
        return;
    }
    if (exe_time.minutes < 0)
        exe_time.minutes = CRON_STAR;
    // min
    if (new_command.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'hour'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'hour' does not have a INTEGER value"), false);
        return;
    }
    exe_time.hours = atoi(new_command.get_cur_pair_value());
    if ((exe_time.hours < -1) || (exe_time.hours > 23))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("hours value out of range"), false);
        return;
    }
    if (exe_time.hours < 0)
        exe_time.hours = CRON_STAR;
    // dom
    if (new_command.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'dom'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'dom' does not have a INTEGER value"), false);
        return;
    }
    exe_time.day_of_month = atoi(new_command.get_cur_pair_value());
    // day of month check will be performed later
    // month
    if (new_command.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'month'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'month' does not have a INTEGER value"), false);
        return;
    }
    exe_time.month = atoi(new_command.get_cur_pair_value());
    if ((exe_time.month < 0) || (exe_time.month > 12))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("month value out of range"), false);
        return;
    }
    if (exe_time.month == 0)
        exe_time.month = CRON_STAR;
    if ((exe_time.day_of_month < 0) ||
        ((exe_time.month == 2) && (exe_time.day_of_month > 29)) ||
        (((exe_time.month == 1) || (exe_time.month == 3) || (exe_time.month == 5) || (exe_time.month == 7) || (exe_time.month == 8) || (exe_time.month == 10) || (exe_time.month == 12)) && (exe_time.day_of_month > 31)) ||
        (((exe_time.month == 4) || (exe_time.month == 6) || (exe_time.month == 9) || (exe_time.month == 11)) && (exe_time.day_of_month > 30)))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dom value out of range"), false);
        return;
    }
    if (exe_time.day_of_month == 0)
        exe_time.day_of_month = CRON_STAR;
    // dow
    if (new_command.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'dow'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'dow' does not have a INTEGER value"), false);
        return;
    }
    exe_time.day_of_week = atoi(new_command.get_cur_pair_value());
    if ((exe_time.day_of_week < 0) || (exe_time.day_of_week > 7))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dow value out of range"), false);
        return;
    }
    if (exe_time.day_of_week == 0)
        exe_time.day_of_week = CRON_STAR;
    int id = command_create(enabled, type, &exe_time, relay_id, duration);
    if (id < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error creating new command"), false);
        return;
    }
    int str_len = 88 + 1 +
                  2 +
                  1 +
                  2 +
                  10 +
                  1 +
                  3 +
                  3 +
                  2 +
                  2 +
                  1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_POST_COMMAND_HEAP_EXHAUSTED, str_len);
        ERROR("post_api_command heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"id\":%d,\"enabled\":%d,\"type\":%d,",
               id,
               enabled,
               type);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
               duration,
               relay_id,
               ((exe_time.minutes == CRON_STAR) ? (-1) : exe_time.minutes));
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
               ((exe_time.hours == CRON_STAR) ? (-1) : exe_time.hours),
               ((exe_time.day_of_month == CRON_STAR) ? (0) : exe_time.day_of_month),
               ((exe_time.month == CRON_STAR) ? (0) : exe_time.month),
               ((exe_time.day_of_week == CRON_STAR) ? (0) : exe_time.day_of_week));

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void del_api_command_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("del_api_command_idx");
    int id;
    char *id_str = parsed_req->url + os_strlen(f_str("/api/command/"));
    if (os_strlen(id_str) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No command ID provided"), false);
        return;
    }
    id = atoi(id_str);
    if ((id < 0) || (id > MAX_COMMAND_COUNT))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID out of range [0-29]"), false);
        return;
    }
    int res = command_delete(id);
    if (res < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID not found"), false);
        return;
    }

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"command deleted\"}"), false);
    espmem.stack_mon();
}

static void get_api_command_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    ALL("get_api_command_idx");
    int id;
    char *id_str = parsed_req->url + os_strlen(f_str("/api/command/"));
    if (os_strlen(id_str) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No command ID provided"), false);
        return;
    }
    id = atoi(id_str);
    if ((id < 0) || (id > MAX_COMMAND_COUNT))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID out of range [0-29]"), false);
        return;
    }
    struct command *command_ptr = command_read(id);
    if (command_ptr == NULL)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID not found"), false);
        return;
    }
    int str_len = 88 + 1 +
                  2 +
                  1 +
                  2 +
                  10 +
                  1 +
                  3 +
                  3 +
                  2 +
                  2 +
                  1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_COMMANDIDX_HEAP_EXHAUSTED, str_len);
        ERROR("get_api_command_idx heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"id\":%d,\"enabled\":%d,\"type\":%d,",
               command_ptr->id,
               command_ptr->enabled,
               command_ptr->type);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
               command_ptr->duration,
               command_ptr->output->pin,
               ((command_ptr->exe_time.minutes == CRON_STAR) ? (-1) : command_ptr->exe_time.minutes));
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
               ((command_ptr->exe_time.hours == CRON_STAR) ? (-1) : command_ptr->exe_time.hours),
               ((command_ptr->exe_time.day_of_month == CRON_STAR) ? (0) : command_ptr->exe_time.day_of_month),
               ((command_ptr->exe_time.month == CRON_STAR) ? (0) : command_ptr->exe_time.month),
               ((command_ptr->exe_time.day_of_week == CRON_STAR) ? (0) : command_ptr->exe_time.day_of_week));

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void put_api_command_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    ALL("put_api_command_idx");
    int id;
    char *id_str = parsed_req->url + os_strlen(f_str("/api/command/"));
    if (os_strlen(id_str) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No command ID provided"), false);
        return;
    }
    id = atoi(id_str);
    if ((id < 0) || (id > MAX_COMMAND_COUNT))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID out of range [0-29]"), false);
        return;
    }
    struct command *command_ptr = command_read(id);
    if (id < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("command ID not found"), false);
        return;
    }
    Json_str new_command(parsed_req->req_content, parsed_req->content_len);
    if (new_command.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // enabled
    if (new_command.find_pair(f_str("enabled")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'enabled'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'enabled' does not have a INTEGER type"), false);
        return;
    }
    bool enabled = ((atoi(new_command.get_cur_pair_value()) == 0) ? false : true);
    // type
    if (new_command.find_pair(f_str("type")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'type'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'type' does not have a INTEGER value"), false);
        return;
    }
    enum command_t type = (enum command_t)atoi(new_command.get_cur_pair_value());
    if ((type < switch_open) || (type > pulse_close))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("type value out of range"), false);
        return;
    }
    // duration
    if (new_command.find_pair(f_str("duration")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'duration'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'duration' does not have a INTEGER value"), false);
        return;
    }
    uint32 duration = atoi(new_command.get_cur_pair_value());
    if (duration < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("duration has negatie value"), false);
        return;
    }
    // relay_id
    if (new_command.find_pair(f_str("relay_id")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'relay_id'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'relay_id' does not have a INTEGER value"), false);
        return;
    }
    enum contact_pin relay_id = (enum contact_pin)atoi(new_command.get_cur_pair_value());
    if ((relay_id < d1) || (relay_id > d8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay_id value out of range"), false);
        return;
    }
    struct date exe_time;
    // min
    if (new_command.find_pair(f_str("min")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'min'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'min' does not have a INTEGER value"), false);
        return;
    }
    exe_time.minutes = atoi(new_command.get_cur_pair_value());
    if ((exe_time.minutes < -1) || (exe_time.minutes > 59))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("minutes value out of range"), false);
        return;
    }
    if (exe_time.minutes < 0)
        exe_time.minutes = CRON_STAR;
    // min
    if (new_command.find_pair(f_str("hour")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'hour'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'hour' does not have a INTEGER value"), false);
        return;
    }
    exe_time.hours = atoi(new_command.get_cur_pair_value());
    if ((exe_time.hours < -1) || (exe_time.hours > 23))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("hours value out of range"), false);
        return;
    }
    if (exe_time.hours < 0)
        exe_time.hours = CRON_STAR;
    // dom
    if (new_command.find_pair(f_str("dom")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'dom'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'dom' does not have a INTEGER value"), false);
        return;
    }
    exe_time.day_of_month = atoi(new_command.get_cur_pair_value());
    // day of month check will be performed later
    // month
    if (new_command.find_pair(f_str("month")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'month'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'month' does not have a INTEGER value"), false);
        return;
    }
    exe_time.month = atoi(new_command.get_cur_pair_value());
    if ((exe_time.month < 0) || (exe_time.month > 12))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("month value out of range"), false);
        return;
    }
    if (exe_time.month == 0)
        exe_time.month = CRON_STAR;
    if ((exe_time.day_of_month < 0) ||
        ((exe_time.month == 2) && (exe_time.day_of_month > 29)) ||
        (((exe_time.month == 1) || (exe_time.month == 3) || (exe_time.month == 5) || (exe_time.month == 7) || (exe_time.month == 8) || (exe_time.month == 10) || (exe_time.month == 12)) && (exe_time.day_of_month > 31)) ||
        (((exe_time.month == 4) || (exe_time.month == 6) || (exe_time.month == 9) || (exe_time.month == 11)) && (exe_time.day_of_month > 30)))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dom value out of range"), false);
        return;
    }
    if (exe_time.day_of_month == 0)
        exe_time.day_of_month = CRON_STAR;
    // dow
    if (new_command.find_pair(f_str("dow")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'dow'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'dow' does not have a INTEGER value"), false);
        return;
    }
    exe_time.day_of_week = atoi(new_command.get_cur_pair_value());
    if ((exe_time.day_of_week < 0) || (exe_time.day_of_week > 7))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dow value out of range"), false);
        return;
    }
    if (exe_time.day_of_week == 0)
        exe_time.day_of_week = CRON_STAR;
    int res = command_update(id, enabled, type, &exe_time, relay_id, duration);
    if (res < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error updating command"), false);
        return;
    }
    int str_len = 88 + 1 +
                  2 +
                  1 +
                  2 +
                  10 +
                  1 +
                  3 +
                  3 +
                  2 +
                  2 +
                  1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_PUT_COMMANDIDX_HEAP_EXHAUSTED, str_len);
        ERROR("put_api_command heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"id\":%d,\"enabled\":%d,\"type\":%d,",
               id,
               enabled,
               type);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"duration\":%d,\"relay_id\":%d,\"min\":%d,",
               duration,
               relay_id,
               ((exe_time.minutes == CRON_STAR) ? (-1) : exe_time.minutes));
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"hour\":%d,\"dom\":%d,\"month\":%d,\"dow\":%d}",
               ((exe_time.hours == CRON_STAR) ? (-1) : exe_time.hours),
               ((exe_time.day_of_month == CRON_STAR) ? (0) : exe_time.day_of_month),
               ((exe_time.month == CRON_STAR) ? (0) : exe_time.month),
               ((exe_time.day_of_week == CRON_STAR) ? (0) : exe_time.day_of_week));

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void get_api_info(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"device_name":"","chip_id":"","app_name":"","app_version":"","espbot_version":"","library_version":"","sdk_version":"","boot_version":""}
    ALL("get_api_info");
    int str_len = 138 +
                  os_strlen(espbot.get_name()) +
                  10 +
                  os_strlen(app_name) +
                  os_strlen(app_release) +
                  os_strlen(espbot.get_version()) +
                  os_strlen(library_release) +
                  os_strlen(system_get_sdk_version()) +
                  10 +
                  1;
    Heap_chunk msg(138 + str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_API_INFO_HEAP_EXHAUSTED, 155 + str_len);
        ERROR("get_api_info heap exhausted %d", 155 + str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"device_name\":\"%s\",\"chip_id\":\"%d\",\"app_name\":\"%s\",",
               espbot.get_name(),
               system_get_chip_id(),
               app_name);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"app_version\":\"%s\",\"espbot_version\":\"%s\",",
               app_release,
               espbot.get_version());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"library_version\":\"%s\",\"sdk_version\":\"%s\",",
               library_release,
               system_get_sdk_version());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"boot_version\":\"%d\"}",
               system_get_boot_version());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void get_api_relay(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"relays":[]}
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    ALL("get_api_relay");
    int str_len = 13 + (8 * 57) + (8 * (2 + 1 + 31 + 2 + 2)) + 1;

    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_RELAY_HEAP_EXHAUSTED, str_len);
        ERROR("get_api_relay heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    // {"relays":[]}
    fs_sprintf(msg.ref, "{\"relays\":[");
    int idx;
    struct relay *relay_ptr;
    bool first_element = true;
    for (idx = ((int)d1); idx <= ((int)d8); idx++)
    {
        // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
        relay_ptr = get_relay(idx);
        if (relay_ptr == NULL)
            continue;
        if (first_element)
            first_element = false;
        else
            fs_sprintf(msg.ref + os_strlen(msg.ref), ",");
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "{\"pin\":%d,\"reserved\":%d,",
                   relay_ptr->pin,
                   relay_ptr->reserved);
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "\"name\":\"%s\",",
                   relay_ptr->name);
        fs_sprintf(msg.ref + os_strlen(msg.ref),
                   "\"logic\":%d,\"status_at_boot\":%d}",
                   relay_ptr->logic,
                   relay_ptr->status_at_boot);
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref), "]}");

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void get_api_relay_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    ALL("get_api_relay_idx");
    int id;
    char *id_str = parsed_req->url + os_strlen(f_str("/api/relay/"));
    if (os_strlen(id_str) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No relay ID provided"), false);
        return;
    }
    id = atoi(id_str);
    if ((id < 1) || (id > 8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay ID out of range [1-8]"), false);
        return;
    }
    struct relay *relay_ptr;
    relay_ptr = get_relay(id);
    if (relay_ptr == NULL)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay ID not found"), false);
        return;
    }
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    int str_len = 57 + (2 + 1 + 31 + 2 + 2) + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_RELAY_IDX_HEAP_EXHAUSTED, str_len);
        ERROR("get_api_relay_idx heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "{\"pin\":%d,\"reserved\":%d,",
               relay_ptr->pin,
               relay_ptr->reserved);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"name\":\"%s\",",
               relay_ptr->name);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"logic\":%d,\"status_at_boot\":%d}",
               relay_ptr->logic,
               relay_ptr->status_at_boot);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

static void put_api_relay_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("put_api_relay_idx");
    int id;
    char *id_str = parsed_req->url + os_strlen(f_str("/api/relay/"));
    if (os_strlen(id_str) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No relay ID provided"), false);
        return;
    }
    id = atoi(id_str);
    if ((id < 1) || (id > 8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay ID out of range [1-8]"), false);
        return;
    }
    struct relay *relay_ptr;
    relay_ptr = get_relay(id);
    if (relay_ptr == NULL)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay ID not found"), false);
        return;
    }
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    Json_str new_command(parsed_req->req_content, parsed_req->content_len);
    if (new_command.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // name
    if (new_command.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'name'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'name' does not have a STRING type"), false);
        return;
    }
    char name[32];
    os_memset(name, 0, 32);
    os_strncpy(name,
               new_command.get_cur_pair_value(),
               ((new_command.get_cur_pair_string_len() > 31) ? 31 : new_command.get_cur_pair_value_len()));
    // logic
    if (new_command.find_pair(f_str("logic")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'logic'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'logic' does not have a INTEGER value"), false);
        return;
    }
    enum contact_logic logic = (enum contact_logic)atoi(new_command.get_cur_pair_value());
    if ((logic < logic_undefined) || (logic > pin_high_relay_closed))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("logic value out of range"), false);
        return;
    }
    // status_at_boot
    if (new_command.find_pair(f_str("status_at_boot")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'status_at_boot'"), false);
        return;
    }
    if (new_command.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'status_at_boot' does not have a INTEGER value"), false);
        return;
    }
    enum contact_status status_at_boot = (enum contact_status)atoi(new_command.get_cur_pair_value());
    if ((status_at_boot < status_undefined) || (status_at_boot > closed))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("status_at_boot out of range"), false);
        return;
    }
    int res = conf_relay((enum contact_pin)id, name, logic, status_at_boot);
    if (res)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error updating relay"), false);
        return;
    }
    // {"pin":,"reserved":,"name":"","logic":,"status_at_boot":}
    relay_ptr = get_relay(id);
    int str_len = 57 + (2 + 1 + 31 + 2 + 2) + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_PUT_RELAY_IDX_HEAP_EXHAUSTED, str_len);
        ERROR("put_api_relay_idx heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"pin\":%d,\"reserved\":%d,",
               relay_ptr->pin,
               relay_ptr->reserved);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"name\":\"%s\",",
               relay_ptr->name);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"logic\":%d,\"status_at_boot\":%d}",
               relay_ptr->logic,
               relay_ptr->status_at_boot);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
    espmem.stack_mon();
}

bool app_http_routes(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("app_http_routes");

    if ((0 == os_strcmp(parsed_req->url, f_str("/api/command"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_command(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/command"))) && (parsed_req->req_method == HTTP_POST))
    {
        post_api_command(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/command/"), os_strlen("/api/command/"))) && (parsed_req->req_method == HTTP_DELETE))
    {
        del_api_command_idx(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/command/"), os_strlen("/api/command/"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_command_idx(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/command/"), os_strlen("/api/command/"))) && (parsed_req->req_method == HTTP_PUT))
    {
        put_api_command_idx(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/info"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_info(ptr_espconn, parsed_req);
        return true;
    }

    if ((0 == os_strcmp(parsed_req->url, f_str("/api/relay"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_relay(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/relay/"), os_strlen("/api/relay/"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_relay_idx(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/relay/"), os_strlen("/api/relay/"))) && (parsed_req->req_method == HTTP_PUT))
    {
        put_api_relay_idx(ptr_espconn, parsed_req);
        return true;
    }
    return false;
}