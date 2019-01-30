#!/bin/sh
IFS="\n"
while true 
do 
	uptime_ts=`cat /proc/uptime | awk '{ print $1}'`
	#echo $uptime_ts
	dmesg |grep "unregister_netdevice: waiting for eth0 to become free. Usage count" | awk -v uptime_ts=$uptime_ts 'BEGIN {
	    now_ts = systime();
	    start_ts = now_ts - uptime_ts;
	    #print "system start time seconds:", start_ts;
	    #print "system start time:", strftime("[%Y/%m/%d %H:%M:%S]", start_ts);
	 }
	{
	    print strftime("%Y/%m/%d %H:%M:%S", start_ts + substr($1, 2, length($1) - 2)) 
	}'|tail -n 300| while read line
	do
		if [ $(date +%s -d `echo ${line}`) -gt $(date +%s -d '- 3 Minute') ]
		then
			echo `date` "start to free netdev"
		        insmod /usr/local/free_netdev/free_netdev.ko 
			sleep 3
			rmmod /usr/local/free_netdev/free_netdev.ko	   		 
			break
		fi
	done
	echo `date` "netdev is normal"
	sleep 300
done
