# I have to run this shell script on ubuntu14.04 to add enumerate usb devices for ftdi to uart
sudo /sbin/modprobe ftdi_sio product=0x0000 vendor=0x0403
echo '0403 0000' | sudo tee --append /sys/bus/usb-serial/drivers/ftdi_sio/new_id
sudo udevadm control --reload
ls /dev/tty*
