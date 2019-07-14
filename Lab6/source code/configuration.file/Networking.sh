#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 192.168.0.1 netmask 255.255.255.0
sudo ifconfig ens38 172.0.0.1 netmask 255.255.255.0
sudo /etc/init.d/networking restart


