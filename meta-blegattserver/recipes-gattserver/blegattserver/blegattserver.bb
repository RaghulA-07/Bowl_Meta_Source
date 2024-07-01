# recipes-gattserver/blegattserver/blegattserver.bb
SUMMARY = "BLE GATT Server"
DESCRIPTION = "A BLE GATT server implementation"
#LICENSE = "MIT"
#LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT"
LICENSE = "CLOSED"
 
SRC_URI = "file://CMakeLists.txt \
           file://binc/adapter.c \
           file://binc/adapter.h \
           file://binc/advertisement.c \
           file://binc/advertisement.h \
           file://binc/agent.c \
           file://binc/agent.h \
           file://binc/application.c \
           file://binc/application.h \
           file://binc/characteristic.c \
           file://binc/characteristic.h \
           file://binc/characteristic_internal.h \
           file://binc/descriptor.c \
           file://binc/descriptor.h \
           file://binc/descriptor_internal.h \
           file://binc/device.c \
           file://binc/device.h \
           file://binc/device_internal.h \
           file://binc/forward_decl.h \
           file://binc/logger.c \
           file://binc/logger.h \
           file://binc/parser.c \
           file://binc/parser.h \
           file://binc/service.c \
           file://binc/service.h \
           file://binc/service_internal.h \
           file://binc/utility.c \
           file://binc/utility.h \
           file://bleGattServer/CMakeLists.txt \
           file://bleGattServer/CommandParser.c \
           file://bleGattServer/CommandParser.h \
           file://bleGattServer/ble_commands.h \
           file://bleGattServer/bleGattServer.c"
 
S = "${WORKDIR}"

DEPENDS = "glib-2.0"

inherit cmake
 
do_compile() {
    cmake -S ${S} -B build
    cmake --build build
}
 
do_install() {
    install -d ${D}${bindir}
    install -m 0755 build/bleGattServer/bleGattServer ${D}${bindir}/bleGattServer
}

