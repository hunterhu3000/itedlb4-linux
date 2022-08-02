#!/bin/sh

# Please replace [my_password] to your ubuntu sudo password,
# before using this script.

BUILD=$1
FIRMWARE=${BUILD}/zephyr/zephyr.bin
FLASHER=$HOME/itetool/ite
echo $1
echo ${FLASHER} ${FIRMWARE}
# flash binary
#echo "my_password" | sudo -S ~/itetool/ite -f build/zephyr/zephyr -s

retry=0
while [ $retry -lt 3 ]
do
	echo "$(date): pid=$$ retry=$retry, start" >> $HOME/itetool/flash_log.txt

	echo "my_password" | sudo -S ${FLASHER} -f ${FIRMWARE} > $HOME/itetool/flash_result.txt

	echo "$(date): pid=$$ retry=$retry, done" >> $HOME/itetool/flash_log.txt
	echo "" >> $HOME/itetool/flash_log.txt

	res=`grep -c "Verifying...     : 100%" $HOME/itetool/flash_result.txt`
	if [ $res != "1" ];
	then
		echo "****************"
		echo "*    Retry     *"
		echo "****************"
		echo "power off"
		echo "$(date): pid=$$ power off" >> $HOME/itetool/flash_log.txt
		sudo ykushcmd -d a 
        	sleep 3
		echo "power on"
		echo "$(date): pid=$$ power on" >> $HOME/itetool/flash_log.txt
		sudo ykushcmd -u a
		sleep 5
		retry=$((retry+1))
	else
		exit 0
	fi
done

# This is a way to power cycle the NUC. This is risky because we
# could not be able to start NUC again. Plesae use it unless the
# ITE board cannot be flashed anymore.
# 
# power off
# curl http://admin:1234@192.165.1.20/script?run019=run --noproxy "*" > out1
# 
# power on
# curl http://admin:1234@192.165.1.20/script?run017=run --noproxy "*" > out2
