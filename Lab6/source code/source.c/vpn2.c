#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <assert.h>

struct route_item {
	struct in_addr daddr;
	struct in_addr gateway;
	struct in_addr mask;
	char interface[IFNAMSIZ];
}route_table[2];

#define BUFSIZE 1024
char temp_buf[BUFSIZE], send_buf[BUFSIZE];
const char *my_ip = "172.0.0.2";

void print_ip(uint32_t ip) {
	unsigned char b[4];
	b[0] = ip & 0xff;
	b[1] = (ip >> 8) & 0xff;
	b[2] = (ip >> 16) & 0xff;
	b[3] = (ip >> 24) & 0xff;
	printf("%d.%d.%d.%d\n", b[0], b[1], b[2], b[3]);
}

int main() {
	const char *addr1 = "172.0.0.2";
	const char *addr2 = "10.0.0.2";
	const char *gate1 = "0.0.0.0";
	const char *gate2 = "192.168.0.2";
	const char inte1[IFNAMSIZ] = "ens38";
	const char inte2[IFNAMSIZ] = "ens33";

	inet_aton(addr1, &(route_table[0].daddr));
	inet_aton(addr2, &(route_table[1].daddr));
	inet_aton(gate1, &(route_table[0].gateway));
	inet_aton(gate2, &(route_table[1].gateway));
	strncpy(route_table[0].interface, inte1, IFNAMSIZ);
	strncpy(route_table[1].interface, inte2, IFNAMSIZ);
	
	int recvfd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));

	int i;

	while(1) {
		struct sockaddr_ll sall;
		socklen_t addr_len = sizeof(sall);
		recvfrom(recvfd, temp_buf, BUFSIZE, 0, (struct sockaddr*)&sall, &addr_len);
		
		struct iphdr *hip = (struct iphdr*)temp_buf;
		
		if(sall.sll_protocol == htons(ETH_P_IP)) 
		{
			if(hip->daddr == inet_addr(my_ip)) 
			{//发到本路由器
				if(hip->protocol == IPPROTO_IPIP) 
				{
					
					print_ip(hip->saddr);
					print_ip(hip->daddr);
					
					hip = (struct iphdr*)(temp_buf + hip->ihl * 4);//取出后面
					
					print_ip(hip->saddr);
					print_ip(hip->daddr);
					memcpy(send_buf, hip, ntohs(hip->tot_len));
					
					int sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
					int on = 1;
					setsockopt(sendfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
					
					struct sockaddr_in forward;
					forward.sin_family = AF_INET;
					forward.sin_addr.s_addr = hip->daddr;
					sendto(sendfd, send_buf, ntohs(hip->tot_len), 0, (struct sockaddr*)&forward, sizeof(forward));
					close(sendfd);
				}
			}
			else {//目标地址不是本路由器
				for(i = 0; i < 2; ++i)
				{
					if(hip->daddr == route_table[i].daddr.s_addr) 
					{//查路由表
						struct in_addr vpn_addr = route_table[i].gateway;
						print_ip(hip->saddr);
						print_ip(hip->daddr);
						print_ip(vpn_addr.s_addr);

						memset(send_buf, 0, BUFSIZE);
						memcpy(send_buf, temp_buf, ntohs(hip->tot_len));
						int sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP);
												
						struct sockaddr_in forward;
						forward.sin_family = AF_INET;
						forward.sin_addr = vpn_addr;
						
						sendto(sendfd, send_buf, ntohs(hip->tot_len), 0, (struct sockaddr*)&forward, sizeof(forward));
						close(sendfd);
						break;
					}
				}
			}
		}
	}
	return 0;
}

