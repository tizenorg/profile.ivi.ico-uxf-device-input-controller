#!/bin/sh
#
# set day/night mode

if [ -e /tmp/daynight_sw.day ] ; then
	rm -fr /tmp/daynight_sw.day
	/usr/local/bin/ico_set_vehicleinfo EXTERIOR=0
else
	touch /tmp/daynight_sw.day
	/usr/local/bin/ico_set_vehicleinfo EXTERIOR=1000
fi
