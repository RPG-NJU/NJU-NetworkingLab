# Networking Lab5 实验报告

### 动态路由协议RIP，OSPF和BGP观察



## 实验目的

理解自洽系统（AS），观察RIP，OSPF以及BGP动态路由协议的实际运行过程。在网络拓扑结构变更的情况下观察路由表的动态变更，通过实验理解两种不同的路由算法（距离向量和链路状态）。

为之后的学习奠定基础，并且巩固所学的知识。



## 网络拓扑配置

| 节点名  | 虚拟设备名 |         IP          |    netmask    |
| :-----: | :--------: | :-----------------: | :-----------: |
| Router1 |  NW-G-01   | ens33: 192.168.2.1  | 255.255.255.0 |
|         |            | ens38: 192.168.14.1 | 255.255.255.0 |
| Router2 |  NW-G-02   | ens33: 192.168.2.2  | 255.255.255.0 |
|         |            | ens38: 192.168.3.1  | 255.255.255.0 |
| Router3 |  NW-G-03   | ens33: 192.168.3.2  | 255.255.255.0 |
|         |            | ens38: 192.168.4.1  | 255.255.255.0 |
| Router4 |  NW-G-04   | ens33: 192.168.4.2  | 255.255.255.0 |
|         |            | ens38: 192.168.13.1 | 255.255.255.0 |
|         |            | ens39: 192.168.14.2 | 255.255.255.0 |
| Router5 |  NW-G-05   | ens33: 192.168.11.1 | 255.255.255.0 |
|         |            | ens38: 192.168.13.2 | 255.255.255.0 |
| Router6 |  NW-G-06   | ens33: 192.168.11.2 | 255.255.255.0 |
|         |            | ens38: 192.168.12.1 | 255.255.255.0 |
| Router7 |  NW-G-07   | ens33: 192.168.12.2 | 255.255.255.0 |



## 路由配置文件

`在我的实验中Router的编号从1开始，所以Router1为实验要求中的Router0，以此类推`

#### Router1

###### zebra.conf

```wiki
! -*- zebra -*-
!
! zebra sample configuration file
!
! $Id: zebra.conf.sample,v 1.1 2002/12/13 20:15:30 paul Exp $
!
hostname Router
password zebra
enable password zebra
!
! Interface's description.
!
!interface lo
! description test of desc.
!
!interface sit0
! multicast

!
! Static default route sample.
!
!ip route 0.0.0.0/0 203.181.89.241
!

!log file zebra.log
```

###### ripd.conf

```wiki
!-*-rip-*-
hostname ripd
password zebra
router rip
        network ens33
        network ens38
log stdout
!
```



#### Router4

###### zebra.conf

```wiki
! -*- zebra -*-
!
! zebra sample configuration file
!
! $Id: zebra.conf.sample,v 1.1 2002/12/13 20:15:30 paul Exp $
!
hostname Router
password zebra
enable password zebra
!
! Interface's description.
!
!interface lo
! description test of desc.
!
!interface sit0
! multicast

!
! Static default route sample.
!
!ip route 0.0.0.0/0 203.181.89.241
!

!log file zebra.log
```

###### ripd.conf

```wiki
!-*-rip-*-
hostname ripd
password zebra
router rip
        network ens33
        network ens39
log stdout
!
```

###### bgpd.conf

```wiki
!-*-bgp-*-
hostname bgpd
password zebra
router bgp 1
        bgp router-id 192.168.13.1
        network 192.168.2.0/24
        network 192.168.3.0/24
        network 192.168.4.0/24
        network 192.168.14.0/24
        neighbor 192.168.13.2 remote-as 2
log stdout
!
```

#### Router5

###### zebra.conf

```wiki
! -*- zebra -*-
!
! zebra sample configuration file
!
! $Id: zebra.conf.sample,v 1.1 2002/12/13 20:15:30 paul Exp $
!
hostname Router
password zebra
enable password zebra
!
! Interface's description.
!
!interface lo
! description test of desc.
!
!interface sit0
! multicast

!
! Static default route sample.
!
!ip route 0.0.0.0/0 203.181.89.241
!

!log file zebra.log
```

###### ospfd.conf

```wiki
!-*-ospf-*-
hostname ospfd
password zebra
router ospf
        network 192.168.11.0/24 area 0
log stdout
!
```

###### bgpd.conf

```wiki
!-*-bgp-*-
hostname bgpd
password zebra
router bgp 2
        bgp router-id 192.168.13.2
        network 192.168.11.0/24
        network 192.168.12.0/24
        neighbor 192.168.13.1 remote-as 1
log stdout
!
```

#### Router7

###### zebra.conf

```wiki
! -*- zebra -*-
!
! zebra sample configuration file
!
! $Id: zebra.conf.sample,v 1.1 2002/12/13 20:15:30 paul Exp $
!
hostname Router
password zebra
enable password zebra
!
! Interface's description.
!
!interface lo
! description test of desc.
!
!interface sit0
! multicast

!
! Static default route sample.
!
!ip route 0.0.0.0/0 203.181.89.241
!

!log file zebra.log
```

###### ospfd.conf

```wiki
!-*-ospf-*-
hostname ospfd
password zebra
router ospf
        network 192.168.12.0/24 area 0
log stdout
!
```



## 数据包截图

#### Router4-ens38 BGP报文
![Router4 BGP报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router4%BGP报文.PNG)

#### Router4-ens33 RIP报文

![Router4 RIP报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router4%RIP报文.PNG)

#### Router5-ens33 OSPF报文

![Router5 OSPF报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router5%OSPF报文.PNG)



## 协议报文分析

#### RIP分析

![Router4 RIP报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router4%RIP报文.PNG)

从上至下进行分析：

`Command: Response(2)`代表为RIP响应信息，如果为1，则为RIP请求信息

`Version: RIPv2(2)`代表版本为RIPv2，第二版。

`Address Family: IP(2)`代表协议簇为TCP/IP协议，取值为2。

`IP Address: 192.168.2.0`代表了路由表项的目的网络地址。

`Netmask: 255.255.255.0`标志了子网掩码。

`Metric: 2`表示了从这里到目的网络地址所需要的跳数。



#### OSPF报文分析

![Router5 OSPF报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router5%OSPF报文.PNG)

从上至下进行分析：

`Version: 2`代表OSPF的版本为2。

`Massage Type: Hello Packet(1)`表示OSPF的不同类型，1:hello 2:dd 3:lsr 4:lsu 5:lsack。

`Source OSPF Router: 192.168.11.1`表示发包的源地址。

`Area ID: 0.0.0.0`代表始发该LSA的区域ID。

`Checksum`为校验和。

`Auth Type: Null`代表了验证类型，0:不可验证 1:明文验证 2:MD5验证。

`Network Mask`为子网掩码。

`Hello Interval`表示，如果发送hello报文的时间间隔不同，是不可以建立neighbor关系的。

`Router Priority`表示路由器的本地优先级。

`Router Dead Interval`表示失效时间，如果在规定时间内，没有收到来自neighbor的hello包，则视为失效。

`Designatd Router`表示路由器本地接口的IP地址。

`Active Neighbor`表示邻居路由器的IP地址。

`Backup Designated Router`表示了备份制定路由器地址。

#### BGP报文分析

![Router4 BGP报文](https://github.com/HELLORPG/NJU-NetworkingLab/edit/master/Lab5/Router4%BGP报文.PNG)

接收到的是BGP包中的`keepalive`存活证明类型报文。

这类报文只有BGP的固定头，默认每60秒发送一次，对方回复之后，会更新保活消息计时，如果联系对方三次都没有收到回复，则删除BGP neighbor。



## 观察动态路由

在网络拓扑变更之前，Router1如果想要到达其他的节点，必须按照Router1 -> Router2 -> Router3 -> Router4的顺序进行传递。

而网络拓扑变更之后，Router1中，到达Router4的路由表项为下一跳直接是Router4的IP地址。

这是因为，更新了网络拓扑结构之后，由于距离向量的更新机制，Router1得到了来自邻居Router4的信息，发现只需要进行一跳就可以到达，相比之前保存的三条距离，有缩短，所以更新了路由表项，改为下一跳为Router4。
