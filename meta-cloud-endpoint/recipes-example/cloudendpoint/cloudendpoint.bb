DESCRIPTION = "Simple cloud endpoint application"
LICENSE = "CLOSED"

#SRC_URI = "file://cloud.c"
SRC_URI = "file://*"

S = "${WORKDIR}"

#inherit pkgconfig

#DEPENDS = "libcurl-dev"
#RDEPENDS_${PN} = ""

#DEPENDS += "libcurl-dev"
DEPENDS += "curl"

S = "${WORKDIR}"

do_compile() {
    echo "compiling cloud endpoint app..."
    make
}

do_install() {
        echo "installing cloud endpoint binary...."
        install -d ${D}${bindir}
#        install -m 0755 ${WORKDIR}/build/cloudapp ${D}${bindir}
	install -m 0755 cloud ${D}${bindir}
}

FILES_${PN} += "${bindir}/cloud"

