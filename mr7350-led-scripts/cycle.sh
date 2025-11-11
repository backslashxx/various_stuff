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

reset
	
a=1
b=0
c=0
	
x=1
y=1
z=1

brightness=0
while true
do

	sleep 0.1
	[ $a -lt 0 ] && a=1 
	[ $b -lt 0 ] && b=1 
	[ $c -lt 0 ] && c=1 
	
	#echo red $a
	brightness=$((a-1))
	[ $brightness -lt 0 ] && brightness=0
	echo $brightness > $red
	
	[ $x = 1 ] && a=$((a * 2))
	[ $x = 0 ] && a=$((a / 2))
	
	[ $a -ge 256 ] && x=0 && b=1 && y=1
	#echo green $b
	brightness=$((b-1))
	[ $brightness -lt 0 ] && brightness=0
	echo $brightness > $green
	
	
	[ $y = 1 ] && b=$((b * 2))
	[ $y = 0 ] && b=$((b / 2))
	
	[ $b -ge 256 ] && y=0 && c=1 && z=1
	
	#echo blue $c
	brightness=$((c-1))
	[ $brightness -lt 0 ] && brightness=0
	echo $brightness > $blue
	
	[ $z = 1 ] && c=$((c * 2))
	[ $z = 0 ] && c=$((c / 2))
	
	[ $c -ge 256 ] && z=0 && a=1 && x=1
	

	#echo ==

done

