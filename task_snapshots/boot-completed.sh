#!/bin/sh
# for Task Snapshots (Android 12+)
PATH=/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:$PATH

until [ -d "/sdcard/Android" ]; do sleep 1; done

[ -w "/mnt" ] && MNT_FOLDER="/mnt"
[ -w "/mnt/vendor" ] && ! busybox grep -q " /mnt/vendor " "/proc/mounts" && MNT_FOLDER="/mnt/vendor"

SNAPSHOT_NEW_PATH="$MNT_FOLDER/tmpfs_snapshots"
SNAPSHOT_ORIG_PATH="/data/system_ce/0/snapshots"

if [ ! -d "$SNAPSHOT_ORIG_PATH" ]; then
	exit 1
fi

mkdir -p "$SNAPSHOT_NEW_PATH"
chown system:system "$SNAPSHOT_NEW_PATH"
chmod 700 "$SNAPSHOT_NEW_PATH"

busybox chcon --reference="$SNAPSHOT_ORIG_PATH" "$SNAPSHOT_NEW_PATH"

rm -rf /data/system_ce/0/snapshots/*

mount --bind "$SNAPSHOT_NEW_PATH" "$SNAPSHOT_ORIG_PATH"

# EOF
