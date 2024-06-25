#!/bin/sh

cru=$(cat /sys/class/video4linux/video*/name | grep "CRU")
csi2=$(cat /sys/class/video4linux/v4l-subdev*/name | grep "csi2")

# Available resolution of OV5645.
# Please choose one of a following resolution then comment out the rest.

ar0234_res=$1

#ar0234_res=1280x720
#ar0234_res=1920x1080
#ar0234_res=1920x1200


echo "$1"

if [ -z "$cru" ]
then
	echo "No CRU video device founds"
else
	media-ctl -d /dev/media0 -r
	if [ -z "$csi2" ]
	then
		echo "No MIPI CSI2 sub video device founds"
	else
		media-ctl -d /dev/media0 -l "'rzg2l_csi2 10830400.csi2':1 -> 'CRU output':0 [1]"
		media-ctl -d /dev/media0 -V "'rzg2l_csi2 10830400.csi2':1 [fmt:UYVY8_2X8/$ar0234_res field:none]"
		media-ctl -d /dev/media0 -V "'ar0234 0-0042':0 [fmt:UYVY8_2X8/$ar0234_res field:none]"
		echo "Link CRU/CSI2 to ar0234 0-0042 with format UYVY8_2X8 and resolution $ar0234_res"
	fi
fi

gst-launch-1.0 v4l2src device=/dev/video0 ! waylandsink

