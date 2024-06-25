DESCRIPTION = "Simple helloworld application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://msg-queue-send.c"

S = "${WORKDIR}"

do_compile() {
	${CC} msg-queue-send.c ${LDFLAGS} -o sendermsg
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 sendermsg ${D}${bindir}
}
