function get_code_str(code) { var code_str = [];
code_str[parseInt("1000", 16)] = "APP_INFO_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("1010", 16)] = "RELAY_LIST_RESTORE_ERROR";
code_str[parseInt("1011", 16)] = "RELAY_LIST_TRUNCATED";
code_str[parseInt("1012", 16)] = "RELAY_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("1023", 16)] = "RELAY_LIST_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("1020", 16)] = "COMMAND_LIST_RESTORE_ERROR";
code_str[parseInt("1021", 16)] = "COMMAND_LIST_TRUNCATED";
code_str[parseInt("1022", 16)] = "COMMAND_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("1023", 16)] = "COMMAND_LIST_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("5000", 16)] = "DHT_HEAP_EXHAUSTED";
code_str[parseInt("5001", 16)] = "DHT_READING_TIMEOUT";
code_str[parseInt("5002", 16)] = "DHT_READING_CHECKSUM_ERR";
code_str[parseInt("5003", 16)] = "DHT_READ_HEAP_EXHAUSTED";
code_str[parseInt("5004", 16)] = "MAX6675_THERMOCOUPLE_DISCONNECTED";
code_str[parseInt("5005", 16)] = "MAX6675_HEAP_EXHAUSTED";
code_str[parseInt("0010", 16)] = "FILE_TO_JSON_FS_NOT_AVAILABLE";
code_str[parseInt("0011", 16)] = "FILE_TO_JSON_FILE_NOT_FOUND";
code_str[parseInt("0012", 16)] = "FILE_TO_JSON_CANNOT_OPEN_FILE";
code_str[parseInt("0013", 16)] = "FILE_TO_JSON_HEAP_EXHAUSTED";
code_str[parseInt("0014", 16)] = "FILE_TO_JSON_CANNOT_PARSE_JSON";
code_str[parseInt("0015", 16)] = "FILE_TO_JSON_PAIR_NOT_FOUND";
code_str[parseInt("0016", 16)] = "CFGFILE_HEAP_EXHAUSTED";
code_str[parseInt("0020", 16)] = "GPIO_GETNUM_WRONG_INDEX";
code_str[parseInt("0021", 16)] = "GPIO_CONFIG_WRONG_INDEX";
code_str[parseInt("0022", 16)] = "GPIO_CONFIG_WRONG_TYPE";
code_str[parseInt("0023", 16)] = "GPIO_UNCONFIG_WRONG_INDEX";
code_str[parseInt("0024", 16)] = "GPIO_GET_CONFIG_WRONG_INDEX";
code_str[parseInt("0025", 16)] = "GPIO_READ_WRONG_INDEX";
code_str[parseInt("0026", 16)] = "GPIO_SET_WRONG_INDEX";
code_str[parseInt("0027", 16)] = "GPIO_SET_WRONG_LEVEL";
code_str[parseInt("0028", 16)] = "GPIO_SET_CANNOT_CHANGE";
code_str[parseInt("0029", 16)] = "GPIO_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("002A", 16)] = "GPIO_STATE_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0030", 16)] = "DIAG_RESTORE_CFG_ERROR";
code_str[parseInt("0031", 16)] = "DIAG_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0032", 16)] = "DIAG_INIT_DEFAULT_CFG";
code_str[parseInt("0038", 16)] = "MDNS_START";
code_str[parseInt("0039", 16)] = "MDNS_STOP";
code_str[parseInt("003A", 16)] = "MDNS_RESTORE_CFG_ERROR";
code_str[parseInt("003B", 16)] = "MDNS_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0040", 16)] = "MEM_MON_HEAP_EXHAUSTED";
code_str[parseInt("0041", 16)] = "MEM_MON_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0042", 16)] = "MEM_DUMP_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0043", 16)] = "MEM_DUMP_HEX_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0044", 16)] = "MEM_LAST_RESET_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0045", 16)] = "UTILS_CANNOT_PARSE_IP";
code_str[parseInt("0048", 16)] = "ESPBOT_INIT_DEFAULT_CFG";
code_str[parseInt("0049", 16)] = "ESPOT_SET_NAME_TRUNCATED";
code_str[parseInt("004A", 16)] = "ESPBOT_RESTORE_CFG_ERROR";
code_str[parseInt("004B", 16)] = "ESPBOT_SAVED_CFG_NOT_UPDATED_INCOMPLETE";
code_str[parseInt("004C", 16)] = "ESPBOT_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0050", 16)] = "WIFI_CONNECTED";
code_str[parseInt("0051", 16)] = "WIFI_DISCONNECTED";
code_str[parseInt("0052", 16)] = "WIFI_AUTHMODE_CHANGE";
code_str[parseInt("0053", 16)] = "WIFI_DHCP_TIMEOUT";
code_str[parseInt("0054", 16)] = "WIFI_GOT_IP";
code_str[parseInt("0055", 16)] = "WIFI_OPMODE_CHANGED";
code_str[parseInt("0056", 16)] = "WIFI_STA_CONNECTED";
code_str[parseInt("0057", 16)] = "WIFI_STA_DISCONNECTED";
code_str[parseInt("0058", 16)] = "WIFI_CONNECT_NO_SSID_AVAILABLE";
code_str[parseInt("0059", 16)] = "WIFI_AP_SET_CH_OOR";
code_str[parseInt("005A", 16)] = "WIFI_CFG_RESTORE_ERROR";
code_str[parseInt("005B", 16)] = "WIFI_CFG_RESTORE_NO_SSID_FOUND";
code_str[parseInt("005C", 16)] = "WIFI_CFG_RESTORE_NO_PWD_FOUND";
code_str[parseInt("005D", 16)] = "WIFI_CFG_RESTORE_AP_DEFAULT_PWD";
code_str[parseInt("005E", 16)] = "WIFI_CFG_RESTORE_AP_CUSTOM_PWD";
code_str[parseInt("005F", 16)] = "WIFI_CFG_RESTORE_AP_CH";
code_str[parseInt("0060", 16)] = "WIFI_CFG_RESTORE_AP_CH_OOR";
code_str[parseInt("0061", 16)] = "WIFI_INIT_CFG_DEFAULT_CFG";
code_str[parseInt("0062", 16)] = "WIFI_TRUNCATING_STRING_TO_31_CHAR";
code_str[parseInt("0063", 16)] = "WIFI_TRUNCATING_STRING_TO_63_CHAR";
code_str[parseInt("0064", 16)] = "WIFI_AP_LIST_HEAP_EXHAUSTED";
code_str[parseInt("0065", 16)] = "WIFI_AP_LIST_CANNOT_COMPLETE_SCAN";
code_str[parseInt("0066", 16)] = "WIFI_SETAP_ERROR";
code_str[parseInt("0067", 16)] = "WIFI_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0068", 16)] = "WIFI_STATUS_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0069", 16)] = "WIFI_SCAN_RESULTS_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0070", 16)] = "HTTP_CLEAR_BUSY_SENDING_DATA";
code_str[parseInt("0071", 16)] = "HTTP_DELETED_PENDING_RESPONSE";
code_str[parseInt("0072", 16)] = "HTTP_JSON_ERROR_MSG_HEAP_EXHAUSTED";
code_str[parseInt("0073", 16)] = "HTTP_SEND_BUFFER_SEND_QUEUE_FULL";
code_str[parseInt("0074", 16)] = "HTTP_SEND_BUFFER_HEAP_EXHAUSTED";
code_str[parseInt("0075", 16)] = "HTTP_SEND_BUFFER_ERROR";
code_str[parseInt("0076", 16)] = "HTTP_RESPONSE_HEAP_EXHAUSTED";
code_str[parseInt("0077", 16)] = "HTTP_SEND_REMAINING_MSG_RES_QUEUE_FULL";
code_str[parseInt("0078", 16)] = "HTTP_SEND_REMAINING_MSG_HEAP_EXHAUSTED";
code_str[parseInt("0079", 16)] = "HTTP_SEND_RES_QUEUE_FULL";
code_str[parseInt("007A", 16)] = "HTTP_SEND_HEAP_EXHAUSTED";
code_str[parseInt("007B", 16)] = "HTTP_FORMAT_HEADER_HEAP_EXHAUSTED";
code_str[parseInt("007C", 16)] = "HTTP_PARSE_REQUEST_CANNOT_PARSE_EMPTY_MSG";
code_str[parseInt("007D", 16)] = "HTTP_PARSE_REQUEST_HEAP_EXHAUSTED";
code_str[parseInt("007E", 16)] = "HTTP_PARSE_REQUEST_CANNOT_FIND_HTTP_TOKEN";
code_str[parseInt("007F", 16)] = "HTTP_PARSE_REQUEST_CANNOT_FIND_ACC_CTRL_REQ_HEADERS";
code_str[parseInt("0080", 16)] = "HTTP_PARSE_REQUEST_CANNOT_FIND_ORIGIN";
code_str[parseInt("0081", 16)] = "HTTP_PARSE_REQUEST_CANNOT_FIND_CONTENT_START";
code_str[parseInt("0082", 16)] = "HTTP_PARSE_REQUEST_CANNOT_FIND_CONTENT_LEN";
code_str[parseInt("0083", 16)] = "HTTP_SAVE_PENDING_REQUEST_HEAP_EXHAUSTED";
code_str[parseInt("0084", 16)] = "HTTP_SAVE_PENDING_REQUEST_CANNOT_SAVE_REQ";
code_str[parseInt("0085", 16)] = "HTTP_CHECK_PENDING_REQUESTS_CANNOT_FIND_REQ";
code_str[parseInt("0086", 16)] = "HTTP_SAVE_PENDING_RESPONSE_HEAP_EXHAUSTED";
code_str[parseInt("0087", 16)] = "HTTP_SAVE_PENDING_RESPONSE_CANNOT_SAVE_RES";
code_str[parseInt("0088", 16)] = "HTTP_CHECK_PENDING_RESPONSE_CANNOT_FIND_RES";
code_str[parseInt("0089", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_PARSE_EMPTY_MSG";
code_str[parseInt("008A", 16)] = "HTTP_PARSE_RESPONSE_HEAP_EXHAUSTED";
code_str[parseInt("008B", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_HTTP_TOKEN";
code_str[parseInt("008C", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_CONTENT_LEN";
code_str[parseInt("008D", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_CONTENT_RANGE";
code_str[parseInt("008E", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_RANGE_START";
code_str[parseInt("008F", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_RANGE_END";
code_str[parseInt("0090", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_RANGE_SIZE";
code_str[parseInt("0091", 16)] = "HTTP_PARSE_RESPONSE_CANNOT_FIND_CONTENT_START";
code_str[parseInt("0092", 16)] = "HTTP_CHECK_PENDING_SEND_QUEUE_FULL";
code_str[parseInt("0093", 16)] = "HTTP_PUSH_PENDING_SEND_QUEUE_FULL";
code_str[parseInt("0094", 16)] = "HTTP_PUSH_PENDING_SEND_HEAP_EXHAUSTED";
code_str[parseInt("009D", 16)] = "HTTP_SVR_START";
code_str[parseInt("009E", 16)] = "HTTP_SVR_STOP";
code_str[parseInt("009F", 16)] = "HTTP_SVR_EMPTY_URL";
code_str[parseInt("00A1", 16)] = "ROUTES_SEND_REMAINING_MSG_HEAP_EXHAUSTED";
code_str[parseInt("00A2", 16)] = "ROUTES_SEND_REMAINING_MSG_PENDING_RES_QUEUE_FULL";
code_str[parseInt("00A3", 16)] = "ROUTES_RETURN_FILE_HEAP_EXHAUSTED";
code_str[parseInt("00A4", 16)] = "ROUTES_RETURN_FILE_PENDING_RES_QUEUE_FULL";
code_str[parseInt("00A5", 16)] = "ROUTES_PREFLIGHT_RESPONSE_HEAP_EXHAUSTED";
code_str[parseInt("00A6", 16)] = "ROUTES_GETHEXMEMDUMP_HEAP_EXHAUSTED";
code_str[parseInt("00A7", 16)] = "ROUTES_GETMEMDUMP_HEAP_EXHAUSTED";
code_str[parseInt("00A8", 16)] = "ROUTES_GETMEMINFO_HEAP_EXHAUSTED";
code_str[parseInt("00AB", 16)] = "ROUTES_GETFS_HEAP_EXHAUSTED";
code_str[parseInt("00AC", 16)] = "ROUTES_GETFILELIST_HEAP_EXHAUSTED";
code_str[parseInt("00B7", 16)] = "ROUTES_GETDIAGNOSTICEVENTS_HEAP_EXHAUSTED";
code_str[parseInt("00C2", 16)] = "ROUTES_GETLASTRESET_HEAP_EXHAUSTED";
code_str[parseInt("00C3", 16)] = "ROUTES_CONNECTWIFI_HEAP_EXHAUSTED";
code_str[parseInt("00C4", 16)] = "ROUTES_DISCONNECTWIFI_HEAP_EXHAUSTED";
code_str[parseInt("00C5", 16)] = "ROUTES_GETDIAGNOSTICEVENTS_NEXT_HEAP_EXHAUSTED";
code_str[parseInt("00C6", 16)] = "ROUTES_GETDIAGNOSTICEVENTS_NEXT_PENDING_RES_QUEUE_FULL";
code_str[parseInt("00C7", 16)] = "ROUTES_GETDIAGEVENTS_PENDING_RES_QUEUE_FULL";
code_str[parseInt("00D0", 16)] = "OTA_INIT_DEFAULT_CFG";
code_str[parseInt("00D1", 16)] = "OTA_PATH_TRUNCATED";
code_str[parseInt("00D2", 16)] = "OTA_CANNOT_COMPLETE";
code_str[parseInt("00D3", 16)] = "OTA_TIMER_FUNCTION_USERBIN_ID_UNKNOWN";
code_str[parseInt("00D4", 16)] = "OTA_CANNOT_START_UPGRADE";
code_str[parseInt("00D5", 16)] = "OTA_SUCCESSFULLY_COMPLETED";
code_str[parseInt("00D6", 16)] = "OTA_FAILURE";
code_str[parseInt("00D7", 16)] = "OTA_REBOOTING_AFTER_COMPLETION";
code_str[parseInt("00D8", 16)] = "OTA_START_UPGRADE_CALLED_WHILE_OTA_IN_PROGRESS";
code_str[parseInt("00D9", 16)] = "OTA_RESTORE_CFG_ERROR";
code_str[parseInt("00DA", 16)] = "OTA_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("00DB", 16)] = "OTA_CHECK_VERSION_UNEXPECTED_WEBCLIENT_STATUS";
code_str[parseInt("00DC", 16)] = "OTA_ASK_VERSION_UNEXPECTED_WEBCLIENT_STATUS";
code_str[parseInt("00DD", 16)] = "OTA_UP_TO_DATE";
code_str[parseInt("00DE", 16)] = "OTA_ENGINE_HEAP_EXHAUSTED";
code_str[parseInt("00DF", 16)] = "OTA_CHECK_VERSION_BAD_FORMAT";
code_str[parseInt("00F0", 16)] = "OTA_CHECK_VERSION_EMPTY_RES";
code_str[parseInt("0100", 16)] = "HTTP_CLT_SEND_REQ_CANNOT_SEND_REQ";
code_str[parseInt("0101", 16)] = "HTTP_CLT_ADD_CLIENT_ESPCONN_ASSOCIATION_HEAP_EXHAUSTED";
code_str[parseInt("0102", 16)] = "HTTP_CLT_ADD_CLIENT_ASSOCIATION_REG_ERROR";
code_str[parseInt("0105", 16)] = "HTTP_CLT_RECV_CANNOT_FIND_ESPCONN";
code_str[parseInt("0106", 16)] = "HTTP_CLT_DISCON_CANNOT_FIND_ESPCONN";
code_str[parseInt("0107", 16)] = "HTTP_CLT_CONNECTED_CANNOT_FIND_ESPCONN";
code_str[parseInt("0108", 16)] = "HTTP_CLT_CONNECT_CONN_FAILURE";
code_str[parseInt("0109", 16)] = "HTTP_CLT_SEND_REQ_HEAP_EXHAUSTED";
code_str[parseInt("0110", 16)] = "SPIFFS_INIT_CANNOT_MOUNT";
code_str[parseInt("0111", 16)] = "SPIFFS_INIT_FS_FORMATTED";
code_str[parseInt("0112", 16)] = "SPIFFS_INIT_CANNOT_FORMAT";
code_str[parseInt("0113", 16)] = "SPIFFS_INIT_FS_MOUNTED";
code_str[parseInt("0114", 16)] = "SPIFFS_INIT_FS_SIZE";
code_str[parseInt("0115", 16)] = "SPIFFS_INIT_FS_USED";
code_str[parseInt("0116", 16)] = "SPIFFS_GET_TOTAL_SIZE_FS_NOT_MOUNTED";
code_str[parseInt("0117", 16)] = "SPIFFS_GET_USED_SIZE_FS_NOT_MOUNTED";
code_str[parseInt("0118", 16)] = "SPIFFS_LIST_FS_NOT_MOUNTED";
code_str[parseInt("0119", 16)] = "SPIFFS_CHECK_FS_NOT_MOUNTED";
code_str[parseInt("011A", 16)] = "SPIFFS_CHECK_SUCCESSFULLY";
code_str[parseInt("011B", 16)] = "SPIFFS_CHECK_ERRORS";
code_str[parseInt("0120", 16)] = "ESPFILE_FS_NOT_MOUNTED";
code_str[parseInt("0121", 16)] = "ESPFILE_OPEN_ERROR";
code_str[parseInt("0122", 16)] = "ESPFILE_NAME_TRUNCATED";
code_str[parseInt("0123", 16)] = "ESPFILE_CLOSE_ERROR";
code_str[parseInt("0124", 16)] = "ESPFILE_N_READ_FS_NOT_MOUNTED";
code_str[parseInt("0125", 16)] = "ESPFILE_N_READ_READ_ERROR";
code_str[parseInt("0126", 16)] = "ESPFILE_N_READ_OFFSET_FS_NOT_MOUNTED";
code_str[parseInt("0127", 16)] = "ESPFILE_N_READ_SEEK_ERROR";
code_str[parseInt("0128", 16)] = "ESPFILE_N_READ_OFFSET_READ_ERROR";
code_str[parseInt("0129", 16)] = "ESPFILE_N_APPEND_FS_NOT_MOUNTED";
code_str[parseInt("012A", 16)] = "ESPFILE_N_APPEND_WRITE_ERROR";
code_str[parseInt("012B", 16)] = "ESPFILE_CLEAR_FS_NOT_MOUNTED";
code_str[parseInt("012C", 16)] = "ESPFILE_CLEAR_CLOSE_ERROR";
code_str[parseInt("012D", 16)] = "ESPFILE_CLEAR_OPEN_ERROR";
code_str[parseInt("012E", 16)] = "ESPFILE_REMOVE_FS_NOT_MOUNTED";
code_str[parseInt("012F", 16)] = "ESPFILE_REMOVE_ERROR";
code_str[parseInt("0130", 16)] = "ESPFILE_EXISTS_FS_NOT_MOUNTED";
code_str[parseInt("0131", 16)] = "ESPFILE_SIZE_FS_NOT_MOUNTED";
code_str[parseInt("0140", 16)] = "SPIFFS_FLASH_READ_OUT_OF_BOUNDARY";
code_str[parseInt("0141", 16)] = "SPIFFS_FLASH_READ_ERROR";
code_str[parseInt("0142", 16)] = "SPIFFS_FLASH_READ_TIMEOUT";
code_str[parseInt("0143", 16)] = "SPIFFS_FLASH_WRITE_OUT_OF_BOUNDARY";
code_str[parseInt("0144", 16)] = "SPIFFS_FLASH_WRITE_READ_ERROR";
code_str[parseInt("0145", 16)] = "SPIFFS_FLASH_WRITE_READ_TIMEOUT";
code_str[parseInt("0146", 16)] = "SPIFFS_FLASH_WRITE_WRITE_ERROR";
code_str[parseInt("0147", 16)] = "SPIFFS_FLASH_WRITE_WRITE_TIMEOUT";
code_str[parseInt("0148", 16)] = "SPIFFS_FLASH_ERASE_OUT_OF_BOUNDARY";
code_str[parseInt("0149", 16)] = "SPIFFS_FLASH_ERASE_ERROR";
code_str[parseInt("014A", 16)] = "SPIFFS_FLASH_ERASE_TIMEOUT";
code_str[parseInt("0150", 16)] = "TIMEDATE_RESTORE_CFG_ERROR";
code_str[parseInt("0151", 16)] = "TIMEDATE_CFG_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0152", 16)] = "TIMEDATE_STATE_STRINGIFY_HEAP_EXHAUSTED";
code_str[parseInt("0153", 16)] = "TIMEDATE_INIT_DEFAULT_CFG";
code_str[parseInt("0154", 16)] = "TIMEZONE_CHANGED";
code_str[parseInt("0155", 16)] = "TIMEDATE_CHANGED";
code_str[parseInt("0156", 16)] = "SNTP_START";
code_str[parseInt("0157", 16)] = "SNTP_STOP";
code_str[parseInt("0158", 16)] = "SNTP_CANNOT_SET_TIMEZONE";
code_str[parseInt("0160", 16)] = "MDNS_RESTORE_CFG_INCOMPLETE";
code_str[parseInt("0161", 16)] = "MDNS_SAVE_CFG_FS_NOT_AVAILABLE";
code_str[parseInt("0162", 16)] = "MDNS_SAVE_CFG_CANNOT_OPEN_FILE";
code_str[parseInt("0163", 16)] = "MDNS_INIT_DEFAULT_CFG";
code_str[parseInt("0170", 16)] = "CRON_RESTORE_CFG_ERROR";
code_str[parseInt("0171", 16)] = "CRON_INIT_DEFAULT_CFG";
code_str[parseInt("0172", 16)] = "CRON_START";
code_str[parseInt("0173", 16)] = "CRON_STOP";
code_str[parseInt("0174", 16)] = "CRON_ADD_JOB_CANNOT_COMPLETE";
code_str[parseInt("0175", 16)] = "CRON_ADD_JOB_HEAP_EXHAUSTED";
code_str[parseInt("0176", 16)] = "CRON_ENABLED";
code_str[parseInt("0177", 16)] = "CRON_DISABLED";
code_str[parseInt("0178", 16)] = "CRON_CFG_STRINGIFY_HEAP_EXHAUSTED";
return code_str[parseInt(code, 16)]; }
