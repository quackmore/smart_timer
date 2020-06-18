# espbot_2.0

## Summary

Library built on Espressif NON-OS SDK with common functions for esp8266 apps.

## Features

+ configuration persistency
+ cron (linux style)
+ customizable logger over serial interface
+ customizable diagnostic events logger (in memory, available through http)
+ digital I/O provisioning and management
+ json
+ linked lists
+ mDns
+ macros for storing strings into flash and save on RAM memory
+ memory monitor (stack and heap)
+ OTA
+ queues
+ customizable profiler over serial interface
+ SPIFFS
+ time and date with or without SNTP
+ wifi management (chromecast style)
+ WEBCLIENT
+ customizable WEBSERVER

## Memory usage

+ about 30 kB of RAM are available to user application (while idle more than 43 kB are available but under stress conditions, for both SDK and ESPBOT, available memory got reduced by 10 kB and a little more)
+ ESPBOT uses 8 bytes of RTC memory, leaving 504 bytes available to user application

## REST APIs

Espbot REST apis are detailed by [espbot_api.yaml](api/espbot_api.yaml).  
Open the file with the [Swagger online editor](https://editor.swagger.io/) or (better) with Visual Studio [Swagger Viewer](https://marketplace.visualstudio.com/items?itemName=Arjun.swagger-viewer) extension for description and use.

## Using ESPBOT

### Building the binaries and flashing ESP8266

Required:

+ [Espressif NON-OS SDK] (<https://github.com/espressif/ESP8266_NONOS_SDK)> in a separate repository.
+ [esp-open-sdk toolchain] (<https://github.com/pfalcon/esp-open-sdk)> in a separate repository; build the bare Xtensa toolchain and leave ESP8266 SDK separate using:

      make STANDALONE=n

Build steps (linux)

+ Clone the repository.
+ Customize build variables according to your ESP8266 module and environment:

      cd <your path>/espbot_2.0
      ./gen_env.sh

      this will generate a env.sh file
      for instance a WEMOS D1 mini file will look like this:
      
      export SDK_DIR=<your path to ESP8266_NONOS_SDK>
      export COMPILE=gcc
      export BOOT=new
      export APP=1
      export SPI_SPEED=40
      export SPI_MODE=DIO
      export SPI_SIZE_MAP=4
      export COMPILE=gcc
      export COMPORT=<your COM port>
      export CC_DIR=<your path to compiler>
      export PATH=$PATH:<your path to compiler>
      export SDK_DIR=<your path to ESP8266_NONOS_SDK>
      export BOOT=new
      export APP=1
      export SPI_SPEED=40
      export FREQDIV=0
      export SPI_MODE=dio
      export MODE=2
      export SPI_SIZE_MAP=6
      export FLASH_SIZE=4096
      export LD_REF=2048
      export FLASH_OPTIONS=" write_flash -fm dio -fs 32m-c1 -ff 40m "
      export FLASH_INIT="0x3FB000 <your path to ESP8266_NONOS_SDK>/bin/blank.bin 0x3FC000 <your path to ESP8266_NONOS_SDK>/bin/esp_init_data_default_v08.bin 0x3FE000 <your path to ESP8266_NONOS_SDK>/blank.bin"

+ Building (commands available as tasks in case you are using Visual Studio)
  
  Clean project
  
      source ${workspaceFolder}/env.sh && make clean

  Building current user#.bin

      source ${workspaceFolder}/env.sh && make all

  Building user1.bin
  
      source ${workspaceFolder}/env.sh && make -e APP=1 all

  Building user2.bin
  
      source ${workspaceFolder}/env.sh && make -e APP=2 all

  Building both user1.bin and user2.bin
  
      source ${workspaceFolder}/env.sh && make -e APP=1 all && make -e APP=2 all

+ Flashing ESP8266 using esptool.py (checkout your distribution packages or [github repository](https://github.com/espressif/esptool)) (commands available as tasks in case you are using Visual Studio)
  
  Erase flash
  
      source ${workspaceFolder}/env.sh && make flash_erase

  Flash the bootloader
  
      source ${workspaceFolder}/env.sh && make flash_boot

  Flash init
  
      source ${workspaceFolder}/env.sh && make flash_init

  Flash current user#.bin
  
      source ${workspaceFolder}/env.sh && make flash

  Flash user1.bin
  
      source ${workspaceFolder}/env.sh && make -e APP=1 flash

  Flash user2.bin
  
      source ${workspaceFolder}/env.sh && make -e APP=2 flash

### FOTA example

Here is an example on how to use espbot FOTA using a [docker](https://www.docker.com/community-edition#/download) container as http server (thank you docker for existing).

    Start an http server using docker:
    $ docker run -d --name espbot-http-upgrade -p 80:80 -v <your espbot directory>/bin/upgrade/www:/usr/share/nginx/html:ro nginx:alpine

    Configure and command espbot with following curl examples or use the REST apis with any swagger viewer.
      
    Configure espbot:

    curl --location --request POST 'http://{{device_host}}/api/ota/cfg' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "host": "{{your host IP}}",
          "port": 80,
          "path": "/",
          "check_version": "false",
          "reboot_on_completion": "true"
      }'
      
    Start upgrade:

    curl --location --request POST 'http://{{device_host}}/api/ota' \
      --data-raw ''

## Integrating

To integrate espbot in your project as a library checkout src/app example source files for how to build your app and use the following files:

+ lib/libespbot.a
+ lib/libdriver.a
+ lib/libspiffs.a

To import the library source files use the following files:

+ src/driver
+ src/espbot
+ src/spiffs

Espot include files are:

+ driver_hw_timer.h
+ driver_uart_register.h
+ driver_uart.h
+ esp8266_io.h
+ espbot_config.hpp
+ espbot_cron.hpp
+ espbot_diagnostic.hpp
+ espbot_event_codes.h
+ espbot_global.hpp
+ espbot_gpio.hpp
+ espbot_http_routes.hpp
+ espbot_http.hpp
+ espbot_json.hpp
+ espbot_list.hpp
+ espbot_mdns.hpp
+ espbot_mem_macros.h
+ espbot_mem_mon.h
+ espbot_ota.hpp
+ espbot_profiler.hpp
+ espbot_queue.hpp
+ espbot_rtc_mem_map.h
+ espbot_timedate.hpp
+ espbot_utils.hpp
+ espbot_webclient.hpp
+ espbot_webserver.hpp
+ espbot_wifi.hpp
+ espbot.hpp
+ spiffs_config.h
+ spiffs_esp8266.hpp
+ spiffs_flash_functions.hpp
+ spiffs_nucleus.h
+ spiffs.h

## License

Espbot_2.0 comes with a [BEER-WARE] license.

Enjoy.
