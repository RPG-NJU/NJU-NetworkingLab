# Networking Lab6 实验报告

### VPN设计、实现与分析



## 实验目的

本实验主要目的是设计和实现一个简单的虚拟专用网络的机制，并与已有的标准实现（如PPTP）进行对比，进而让学生进一步理解VPN的工作原理和内部实现细节。



## 数据结构说明

```c
struct route_item {
	struct in_addr dest_addr;
	struct in_addr gateway;
	struct in_addr netmask;
	char interface[IFNAMSIZ];
}route_table[2];
```

全局数据结构，用于存储路由表的信息，其中的每个变量的解释如下：

`dest_addr`为目的地址

`gateway`为下一跳地址

`netmask`为子网掩码

`interface`为端口名称



## 配置文件说明

使用`.sh`脚本配置每个虚拟机的网络：

#### 对于客户机

```bash
#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 10.0.0.2 netmask 255.255.255.0
sudo route add default gw 10.0.0.1
sudo /etc/init.d/networking restart
```

#### 对于VPN服务器

```c
#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 10.0.0.1 netmask 255.255.255.0
sudo ifconfig ens38 192.168.0.2 netmask 255.255.255.0
sudo route add default gw 192.168.0.1
sudo /etc/init.d/networking restart
```

#### 对于模拟网络的虚拟机

```c
#! /bin/bash

sudo service network-manager stop
sudo ifconfig ens33 192.168.0.1 netmask 255.255.255.0
sudo ifconfig ens38 172.0.0.1 netmask 255.255.255.0
sudo /etc/init.d/networking restart
```



## 程序设计思路以及运行流程

> 抓包
>
> > if 目的地址不是本路由器
> >
> > > 加装VPN服务的IP包头，通过IPIP协议调用sendto进行发送
> > >
> >
> > else (目的地址是本路由器)
> >
> > > 需要解包，将第一个IP头剥离，之后通过ICMP协议调用sendto进行发送



## 运行结果截图

![运行截图-ping](D:\OneDrive\ComputerScienceMajor\Computer Networking\Lab\Lab6\运行截图-ping.PNG)

如上为在`PC1`上ping之后的终端输出。



![运行截图-抓包](D:\OneDrive\ComputerScienceMajor\Computer Networking\Lab\Lab6\运行截图-抓包.PNG)

如上为在模拟网络的`Network`主机上的wireshark抓包结果。



## 代码个人创新以及思考

摒弃了`Lab4`中使用的从链路层开始封装发包的实验原理，转而采用了从网络层进行调整和发送，从而减少了大量的代码量，使用`IPIP`协议的方式实现VPN。



## 改程序的应用场景创新

可以用于小团体的私有服务器，如果团队成员位于不同的地区，可以通过VPN直接访问到该服务器或主机，从而达到从外部访问私有服务器或主机的目的。