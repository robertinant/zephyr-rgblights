# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

#set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
#  COMMAND foo "hello world"
#  )
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-rgblights)

if(DEFINED CONFIG_APP_USB)
       target_sources(app PRIVATE src/usb_uart.c)
endif()

test_sysbuild()
target_sources(app PRIVATE src/main.c src/led.c src/ble.c src/ble_services/ble_rgbled.c)
target_sources(app PRIVATE ${app_sources})
