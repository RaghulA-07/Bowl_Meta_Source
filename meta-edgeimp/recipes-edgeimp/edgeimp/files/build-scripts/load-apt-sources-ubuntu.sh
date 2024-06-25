#!/bin/bash
set -e

if [ -z "$APT_PIN_DATE" ]; then
    echo "Missing APT_PIN_DATE, should match a tag from http://debmirror.acc2.edgeimpulse.com/"
    exit 1
fi

DIST_NAME=$(cat /etc/lsb-release | grep CODENAME | cut -d'=' -f2)
UNAME=`uname -m`

if [ "$UNAME" == "aarch64" ]; then
    printf "\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-updates main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-updates universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME multiverse \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-updates multiverse \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-security main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-security universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/ports.ubuntu.com/ $DIST_NAME-security multiverse \n\
" > /etc/apt/sources.list
else
    printf "\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-updates main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-updates universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME multiverse \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-updates multiverse \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-security main restricted \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-security universe \n\
deb [trusted=yes] http://debmirror.acc2.edgeimpulse.com/$APT_PIN_DATE/archive.ubuntu.com/ubuntu/ $DIST_NAME-security multiverse \n\
" > /etc/apt/sources.list
fi
