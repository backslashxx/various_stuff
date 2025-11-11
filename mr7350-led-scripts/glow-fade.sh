#!/bin/ash

red=/sys/class/leds/pca963x\:red/brightness
green=/sys/class/leds/pca963x\:green/brightness
blue=/sys/class/leds/pca963x\:blue/brightness 

reset() {
echo 0 > $red
echo 0 > $green
echo 0 > $blue
}
echo 255 > $red
echo 255 > $green
echo 255 > $blue
sleep 1

i=1
led=$red
while true
do
	reset
	[ $i = 1 ] && led=$red
	[ $i = 2 ] && led=$green
	[ $i = 3 ] && led=$blue
	a=1
	while [ $a -lt 255 ] 
	do
		a=$((a << 1))
		x=$((a-1))
		echo $x > $led
		sleep 0.1
	done
	
	a=256
	while [ $a -gt 2 ]
	do
		a=$((a >> 1))
		x=$((a-1))
		echo $x > $led
		sleep 0.1
	done
	
	
		
	i=$((i+1))

	[ $i = 4 ] && i=1
	
	
done

