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
#include "espbot_cfgfile.hpp"
#include "espbot_cron.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_http_routes.hpp"
#include "espbot_json.hpp"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "espbot_http_server.hpp"
#include "drivers.hpp"
#include "app.hpp"
#include "app_relay.hpp"
#include "app_command.hpp"
#include "app_event_codes.h"

static void get_api_command(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("get_api_command");
    char *msg = command_list_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void post_api_command(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
    ALL("post_api_command");
    JSONP new_command(parsed_req->req_content, parsed_req->content_len);
    char name[32];
    os_memset(name, 0, 32);
    new_command.getStr(f_str("name"), name, 32);
    bool enabled = (bool)new_command.getInt(f_str("enabled"));
    enum command_t type = (enum command_t)new_command.getInt(f_str("type"));
    struct date exe_time;
    exe_time.minutes = new_command.getInt(f_str("min"));
    exe_time.hours = new_command.getInt(f_str("hour"));
    exe_time.day_of_week = new_command.getInt(f_str("dow"));
    exe_time.day_of_month = new_command.getInt(f_str("dom"));
    exe_time.month = new_command.getInt(f_str("month"));
    enum contact_pin relay_id = (enum contact_pin)new_command.getInt(f_str("relay_id"));
    uint32 duration = (uint32)new_command.getInt(f_str("duration"));
    if (new_command.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // input validation
    if ((type < switch_open) || (type > pulse_close))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("type value out of range"), false);
        return;
    }
    if (duration < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("duration has negatie value"), false);
        return;
    }
    if ((relay_id < d1) || (relay_id > d8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay_id value out of range"), false);
        return;
    }
    if ((exe_time.minutes > 59) && (exe_time.minutes != 0xff))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("minutes value out of range"), false);
        return;
    }
    if ((exe_time.hours > 23) && (exe_time.hours != 0xff))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("hours value out of range"), false);
        return;
    }
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
    if ((exe_time.day_of_week < 0) || (exe_time.day_of_week > 7))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dow value out of range"), false);
        return;
    }
    if (exe_time.day_of_week == 0)
        exe_time.day_of_week = CRON_STAR;
    // create new command
    int id = command_create(enabled, name, type, &exe_time, relay_id, duration);
    if (id < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error creating new command"), false);
        return;
    }
    int res = command_list_save();
    if (res != CFG_ok)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot save changes to flash"), false);
        return;
    }
    char *msg = command_json_stringify(id);
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
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
    res = command_list_save();
    if (res != CFG_ok)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot save changes to flash"), false);
        return;
    }
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"command deleted\"}"), false);
    mem_mon_stack();
}

static void get_api_command_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
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
    char *msg = command_json_stringify(id);
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void put_api_command_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    // {"id":,"enabled":,"name":"","type":,"duration":,"relay_id":,"min":,"hour":,"dom":,"month":,"dow":}
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
    JSONP new_command(parsed_req->req_content, parsed_req->content_len);
    char name[32];
    os_memset(name, 0, 32);
    new_command.getStr(f_str("name"), name, 32);
    bool enabled = (bool)new_command.getInt(f_str("enabled"));
    enum command_t type = (enum command_t)new_command.getInt(f_str("type"));
    struct date exe_time;
    exe_time.minutes = new_command.getInt(f_str("min"));
    exe_time.hours = new_command.getInt(f_str("hour"));
    exe_time.day_of_week = new_command.getInt(f_str("dow"));
    exe_time.day_of_month = new_command.getInt(f_str("dom"));
    exe_time.month = new_command.getInt(f_str("month"));
    enum contact_pin relay_id = (enum contact_pin)new_command.getInt(f_str("relay_id"));
    uint32 duration = (uint32)new_command.getInt(f_str("duration"));
    if (new_command.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // input validation
    if ((type < switch_open) || (type > pulse_close))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("type value out of range"), false);
        return;
    }
    if (duration < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("duration has negatie value"), false);
        return;
    }
    if ((relay_id < d1) || (relay_id > d8))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("relay_id value out of range"), false);
        return;
    }
    if ((exe_time.minutes > 59) && (exe_time.minutes != 0xff))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("minutes value out of range"), false);
        return;
    }
    if ((exe_time.hours > 23) && (exe_time.hours != 0xff))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("hours value out of range"), false);
        return;
    }
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
    if ((exe_time.day_of_week < 0) || (exe_time.day_of_week > 7))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("dow value out of range"), false);
        return;
    }
    if (exe_time.day_of_week == 0)
        exe_time.day_of_week = CRON_STAR;
    // update commmand
    int cmd_id = command_update(id, enabled, name, type, &exe_time, relay_id, duration);
    if (cmd_id < 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error updating command"), false);
        return;
    }
    int res = command_list_save();
    if (res != CFG_ok)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot save changes to flash"), false);
        return;
    }
    char *msg = command_json_stringify(cmd_id);
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void get_api_info(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    char *msg = app_info_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void get_api_relay(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("get_api_relay");
    char *msg = relay_list_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void get_api_relay_idx(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
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
    char *msg = relay_json_stringify(id);
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
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
    JSONP relay_idx(parsed_req->req_content, parsed_req->content_len);
    struct relay tmp_relay;
    // this properties cannot be changed
    // tmp_relay.pin = (enum contact_pin)relay_idx.getInt(f_str("pin"));
    // tmp_relay.reserved = (bool)relay_idx.getInt(f_str("reserved"));
    os_memset(tmp_relay.name, 0, 32);
    relay_idx.getStr(f_str("name"), tmp_relay.name, 32);
    tmp_relay.logic = (enum contact_logic)relay_idx.getInt(f_str("logic"));
    tmp_relay.status_at_boot = (enum contact_status)relay_idx.getInt(f_str("status_at_boot"));
    if (relay_idx.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // input validation
    if ((tmp_relay.logic < logic_undefined) || (tmp_relay.logic > pin_high_relay_closed))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("logic value out of range"), false);
        return;
    }
    if ((tmp_relay.status_at_boot < status_undefined) || (tmp_relay.status_at_boot > closed))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("status_at_boot out of range"), false);
        return;
    }
    int res = conf_relay((enum contact_pin)id, tmp_relay.name, tmp_relay.logic, tmp_relay.status_at_boot);
    if (res)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Error updating relay"), false);
        return;
    }
    res = relay_list_save();
    if (res != CFG_ok)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot save list to flash"), false);
        return;
    }
    char *msg = relay_json_stringify(id);
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
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