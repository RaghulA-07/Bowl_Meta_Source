cmake_minimum_required(VERSION 3.16)
project(bluez_inc C)
 
# Specify the compiler
#set(CMAKE_C_COMPILER "/usr/bin/aarch64-poky-linux/aarch64-poky-linux-gcc")
 
set(CMAKE_C_STANDARD 99)
 
# Show all the warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -Wno-unused-function -Wno-unused-parameter")
 
include(FindPkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
 
# Add source files from binc
set(BINC_SOURCES
    binc/adapter.c
    binc/advertisement.c
    binc/agent.c
    binc/application.c
    binc/characteristic.c
    binc/descriptor.c
    binc/device.c
    binc/logger.c
    binc/parser.c
    binc/service.c
    binc/utility.c
)
 
set(BINC_HEADERS
    binc/adapter.h
    binc/advertisement.h
    binc/agent.h
    binc/application.h
    binc/characteristic.h
    binc/characteristic_internal.h
    binc/descriptor.h
    binc/descriptor_internal.h
    binc/device.h
    binc/device_internal.h
    binc/forward_decl.h
    binc/logger.h
    binc/parser.h
    binc/service.h
    binc/service_internal.h
    binc/utility.h
)
 
# Create a static library for binc sources
add_library(Binc STATIC ${BINC_SOURCES} ${BINC_HEADERS})
target_include_directories(Binc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/binc)
target_link_libraries(Binc ${GLIB_LIBRARIES} m)
 
# Add executable for bleGattServer
add_subdirectory(bleGattServer)
