#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 192.168.3.13 netmask 255.255.255.0
sudo route add default gw 192.168.3.1


