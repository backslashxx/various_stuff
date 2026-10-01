#!/system/bin/sh
PATH=/data/adb/ksu/bin:$PATH

resetprop ro.surface_flinger.supports_background_blur 0
resetprop ro.sf.blurs_are_expensive 1

# EOF
