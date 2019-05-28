#! /bin/bash

# this is how to set the networks in lab1 for a router

sudo service network-manager stop
sudo ifconfig ens33 192.168.2.1 netmask 255.255.255.0
sudo ip route add 192.168.2.0/24 via 192.168.2.1
sudo ip route add 192.168.3.0/24 via 192.168.11.138

