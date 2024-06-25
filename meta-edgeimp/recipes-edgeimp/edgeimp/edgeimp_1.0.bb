SUMMARY = "A simple video pipeline edge impluse application"
LICENSE = "CLOSED"
PR = "r0"

SRC_URI = "file://Makefile \
	file://edge-impulse-sdk  \
	file://tflite  \
	file://tidl-rt \
	file://inc \
	file://source \
	file://tflite-model \
	file://utils \
	file://build-scripts \
	file://ingestion-sdk-c  \
	file://model-parameters \
	file://tensorflow-lite \
	file://third_party \
"

inherit pkgconfig
DEPENDS = "opencv"
RDEPENDS_${PN} = "libopencv-ml libopencv-objdetect libopencv-stitching  libopencv-calib3d libopencv-features2d libopencv-highgui libopencv-videoio libopencv-imgcodecs libopencv-video libopencv-photo libopencv-imgproc libopencv-flann libopencv-core"
FILES_${PN} = "${bindir}"

S = "${WORKDIR}"

TARGET_LDFLAGS += "-L${PWD}/tmp/sysroots-components/aarch64/opencv/usr/lib64 -Wl,-R${PWD}/tmp/sysroots-components/aarch64/opencv/usr/lib64/libtest"

do_compiles() {
	echo ${PWD}
	echo ${LDFLAGS}
	export APP_CAMERA=1
	export USE_FULL_TFLITE=1
	export TARGET_RENESAS_RZV2L=1
	make -j4
}

do_installs() {
        install -d ${D}${bindir}
        install -m 0755 ${WORKDIR}/edgeimp ${D}${bindir}
}

