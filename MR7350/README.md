# mr7350-stock
mr7350 stock fw dump

mtd -r write kernel.bin /dev/mtd16
mtd -r write rootfs.bin /dev/mtd17

mtd -r write kernel.bin /dev/mtd14
mtd -r write rootfs.bin /dev/mtd15
