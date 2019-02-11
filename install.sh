#!/bin/sh
make && cp -r ../free_netdev /usr/local/ 
cp auto-free-netdev.sh /usr/local/bin/ && cp freenetdev.service /etc/systemd/system/
systemctl enable freenetdev.service && systemctl start freenetdev.service
