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
#include "app_event_codes.h"

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
    fs_sprintf((msg.ref + os_strlen(msg.ref)),
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
}


bool app_http_routes(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("app_http_routes");

    if ((0 == os_strcmp(parsed_req->url, f_str("/api/info"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_info(ptr_espconn, parsed_req);
        return true;
    }
    return false;
}