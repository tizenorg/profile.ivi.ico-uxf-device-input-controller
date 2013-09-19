#!/bin/sh
#
# sample navi bus guide control

if [ -e /tmp/busguide_sw.off ] ; then
	rm -fr /tmp/busguide_sw.off
	/usr/local/bin/set_navi_info genr ON &
else
	touch /tmp/busguide_sw.off
	/usr/local/bin/set_navi_info genr OFF &
fi
