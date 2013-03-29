#!/bin/bash
cd /mnt/FLASH_DRIVE
out=$(gphoto2 --capture-image-and-download --quiet)
echo $out
