SUMMARY = "ESP-Hosted solution provides a way to use ESP board as a communication processor i.e. host for Wi-Fi and Bluetooth/BLE connectivity"
HOMEPAGE = "https://github.com/espressif/esp-hosted/"

#LICENSE = "Apache-2.0"
#LIC_FILES_CHKSUM = "file://../LICENSE;md5=7f43e699e0a26fae98c2938092f008d2"
LICENSE = "CLOSED"
#LIC_FILES_CHKSUM = "file://dummy;md5=d41d8cd98f00b204e9800998ecf8427e"


SRC_URI = "gitsm://github.com/espressif/esp-hosted.git;branch=master"
#SRCREV = "release/v${PV}"
SRCREV = "${AUTOREV}"

#S = "${WORKDIR}/git/host"
S = "${WORKDIR}/git/esp_hosted_ng/host"
#S = "${WORKDIR}/git/host/linux/host_driver/esp32"

do_configure_prepend() {
sed -i 's/#define SPI_DATA_READY_PIN.*/#define SPI_DATA_READY_PIN 432/g' ${S}/spi/esp_spi.h
sed -i 's/#define HANDSHAKE_PIN.*/#define HANDSHAKE_PIN 434/g' ${S}/spi/esp_spi.h
sed -i 's/esp_board.bus_num = .*/esp_board.bus_num = 0;/g' ${S}/spi/esp_spi.c
sed -i 's/esp_board.chip_select = .*/esp_board.chip_select = 0;/g' ${S}/spi/esp_spi.c
sed -i 's/esp_board.mode = SPI_MODE_2;/esp_board.mode = SPI_MODE_1;/g' ${S}/spi/esp_spi.c

# Changing SPI initial clock speed
sed -i 's/#define SPI_INITIAL_CLK_MHZ.*/#define SPI_INITIAL_CLK_MHZ 1/g' ${S}/spi/esp_spi.c

sed -i 's/ARCH := .*/ARCH := arm64/g' ${S}/Makefile
sed -i 's|CROSS_COMPILE := /usr/bin/arm-linux-gnueabihf-|CROSS_COMPILE := aarch64-poky-linux-|g' ${S}/Makefile
}

inherit module

EXTRA_OEMAKE_append_task-install = " -C ${STAGING_KERNEL_DIR} M=${S}"
EXTRA_OEMAKE += "KERNEL=${STAGING_KERNEL_DIR}"
EXTRA_OEMAKE += "target=spi"

RPROVIDES_${PN} += "kernel-module-esp-hosted"
