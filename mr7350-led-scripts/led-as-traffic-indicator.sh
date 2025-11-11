#!/bin/ash
interface=br-lan        # interface to monitor
bandwidth=100           # bandwidth to hit red in Megabits, 1000 for GbE, 100 for FE, 5 for your lousy internet connection :D
naptime=1               # fuck sleep here, if I can do 0.01 I will, well you can but you need to recalculate ceiling and install coreutils-sleep

red=/sys/class/leds/pca963x\:red/brightness
green=/sys/class/leds/pca963x\:green/brightness
blue=/sys/class/leds/pca963x\:blue/brightness

ceiling=$(( naptime * bandwidth ))

#reset leds
echo 0 > $red
echo 0 > $blue
echo 0 > $green

while true
do
        read -r bytes_now < "/sys/class/net/$interface/statistics/rx_bytes"
        bytes_delta="$(( (bytes_now - bytes_prev) / (1000 * ceiling)  ))"

	# explanation, we just assume that 255 and 250 is same shit, for faster math
	# ahemm, I feel like Quake 3's fast inverse square root
        level=$(( bytes_delta * 2 ))

        [ $level -gt 255 ] && level=255
        [ $level -lt 0 ] && level=0
        echo $level > $red

        antilevel=$((255 - level))

        echo $antilevel > $green

        sleep $naptime
        bytes_prev="${bytes_now}"
done
