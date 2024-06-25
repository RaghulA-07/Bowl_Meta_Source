DESCRIPTION = "Simple cloud endpoint application"
LICENSE = "CLOSED"

#SRC_URI = "file://bleproxy.c"
SRC_URI = "file://*"

S = "${WORKDIR}"

S = "${WORKDIR}"

do_compile() {
    echo "compiling ble proxy app..."
    make
}

do_install() {
        echo "installing cloud endpoint binary...."
        install -d ${D}${bindir}
#        install -m 0755 ${WORKDIR}/build/cloudapp ${D}${bindir}
	install -m 0755 bleproxy ${D}${bindir}
}

FILES_${PN} += "${bindir}/bleproxy"

