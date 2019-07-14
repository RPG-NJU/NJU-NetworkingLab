#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 10.0.0.1 netmask 255.255.255.0
sudo ifconfig ens38 192.168.0.2 netmask 255.255.255.0
sudo route add default gw 192.168.0.1
sudo /etc/init.d/networking restart

