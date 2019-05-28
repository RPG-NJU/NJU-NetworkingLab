#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <stdbool.h>

#define MAX_ARP_SIZE 2
#define size 64
#define BUFFER_MAX 4096

#define APR_FILE_NAME "arp.txt"
#define ROUTE_FILE_NAME "route.txt"
#define IP_FILE_NAME "ip.txt"
#define GATEWAY_FILE_NAME "gateway.txt"

typedef struct _eth_hdr  
{  
    unsigned char dstmac[6]; //目标mac地址   
    unsigned char srcmac[6]; //源mac地址   
    unsigned short eth_type; //以太网类型   
}eth_hdr;

struct arp_table_item{
	char ip_addr[16];
	char mac_addr[18];
}arp_table[MAX_ARP_SIZE];

struct sockaddr_in dest;
struct sockaddr_ll sll;
int sock_fd;

void set_ip_addr(char *ip);
int send_packet(char *final);
int reply();
struct in_addr getip();
struct in_addr getgateway();
void inti_mac();
void get_mac(unsigned char *mac,struct in_addr ip_addr);
uint16_t checksum(unsigned char* buffer, int bytes);
int receive_packet(char *buffer);
int translate_buffer(unsigned char *src_buf,unsigned char *dest_buf);
bool check_packet(char *buffer,int n_read);
int ctoi(char c);
void char2mac(char *mac,char *src);

int main()
{
	inti_mac();
    int result = reply();
    if(result <= 0)
        printf("error\n");
    return 0;
}

int reply()
{
    if((sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
    {
        printf("ERROR create raw socket\n");
        return -1;
    }

	while(1)
	{
		char src_buf[BUFFER_MAX];
		char dest_buf[BUFFER_MAX];
		if(receive_packet(src_buf) < 0)
		{
			printf("receive error\n");
			return -1;
		}		

		if(translate_buffer(src_buf,dest_buf)<0)
		{
			printf("change error\n");
			return -1;
		}	
		else if(send_packet(dest_buf)<0)
		{
			printf("send error\n");
			return -1;
		}

		//printf("reply\n");	
	}
	return 1;
}

void set_ip_addr(char *ip)
{
    memset(&dest, 0, sizeof(dest));
    struct hostent *hp = gethostbyname(ip);
    memcpy(&(dest.sin_addr), hp->h_addr, hp->h_length);
    unsigned char *str = hp->h_addr;
    dest.sin_family = hp->h_addrtype;
}

int send_packet(char *final)
{	
	
	if(sendto(sock_fd,final,size+34,0,(struct sockaddr *)&sll,sizeof(sll))<0)
		return -1;
	return 1;
}

struct in_addr getip()
{
	struct in_addr addr;
	FILE *fd = fopen(IP_FILE_NAME, "r");
	char temp[16];
	fscanf(fd,"%s",&temp);
	fscanf(fd,"%s",&temp);
	addr.s_addr = inet_addr(temp);
	fclose(fd);
	return addr;
}

struct in_addr getgateway()
{
	struct in_addr addr;
	FILE *fd = fopen(GATEWAY_FILE_NAME, "r");
	char temp[20];
	fscanf(fd,"%s",&temp);
	addr.s_addr = inet_addr(temp);
	fclose(fd);
	return addr;
}

void inti_mac()
{
	FILE *fd = fopen(APR_FILE_NAME, "r");
	int i = 0;
	for(;i<MAX_ARP_SIZE;i++)
		fscanf(fd,"%s%s",&arp_table[i].ip_addr,&arp_table[i].mac_addr);
	fclose(fd);
}

int ctoi(char c)
{
	if(c>='0'&&c<='9')
		return c-'0';
	else
		return c-'a'+10;
}

void char2mac(char *mac,char *src)
{
	#ifdef GRPDEBUG
    printf("char2mac : [%s]\n", c);
#endif
    for (int i = 0; i < 6; ++i)
    {
        mac[i] = ctoi(src[3 * i]) * 16 + ctoi(src[3 * i + 1]);
    }
    return;
}

void get_mac(unsigned char *mac,struct in_addr ip_addr)
{
	int i = 0;
	for(;i<MAX_ARP_SIZE;i++)
	{
		if(inet_addr(arp_table[i].ip_addr)==ip_addr.s_addr)
		{
			char2mac(mac,arp_table[i].mac_addr);
			break;
		}
	}
}

uint16_t checksum(unsigned char* buffer, int bytes)
{
    uint32_t checksum = 0;
    unsigned char* end = buffer + bytes;

    // odd bytes add last byte and reset end
    if (bytes % 2 == 1) {
        end = buffer + bytes - 1;
        checksum += (*end) << 8;
    }

    // add words of two bytes, one by one
    while (buffer < end) {
        checksum += buffer[0] << 8;
        checksum += buffer[1];
        buffer += 2;
    }

    // add carry if any
    uint32_t carray = checksum >> 16;
    while (carray) {
        checksum = (checksum & 0xffff) + carray;
        carray = checksum >> 16;
    }

    // negate it
    checksum = ~checksum;

    return checksum & 0xffff;
}

int receive_packet(char *buffer)
{
	while(1)
	{
		struct sockaddr_ll from;
		int fromlen = sizeof(from);
		int n_read = recvfrom(sock_fd,buffer,BUFFER_MAX,0,(struct sockaddr *)&from,&fromlen);
		if(n_read <0)
		{
			if(errno==EINTR)continue;
			printf("number error\n");
			return -1;
		}

		
		if(check_packet(buffer,n_read))
			break;
	}
	return 1;
}

void pack_eth(char *eth)
{
	eth_hdr* eth_head = eth;
	struct in_addr gtwy_addr=  getgateway();
	get_mac(eth_head->dstmac,gtwy_addr);

	sll.sll_family = AF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_ifindex = 2;
	memcpy(sll.sll_addr,eth_head->dstmac,6);
	sll.sll_halen = 6;
	struct in_addr ip_addr = getip();
	get_mac(eth_head->srcmac,ip_addr);
	eth_head->eth_type = 0x0008;
}

void pack_ip(char *ipt,char *src_buf)
{
	struct ip* ip_head_s = src_buf+14;
	struct ip* ip_head = ipt;
	ip_head->ip_v = 4;
	ip_head->ip_hl = 5;
	ip_head->ip_tos = 0x00;
	ip_head->ip_len = (20+size)*256;	
	ip_head->ip_id = ip_head_s->ip_id;
	ip_head->ip_off = 0;
	ip_head->ip_ttl=64;
	ip_head->ip_p = 1;
	ip_head->ip_sum = 0;
	ip_head->ip_src=getip();
	ip_head->ip_dst= dest.sin_addr;
	ip_head->ip_sum = checksum((unsigned char *)ipt,20);
}

struct timeval pack_icmp(char *data,char *src_buf)
{
	struct icmp*icmp_head_s = src_buf + 34;
	struct icmp*icmp_head = data;
	icmp_head->icmp_type = ICMP_ECHOREPLY;
	icmp_head->icmp_code = 0;
	icmp_head->icmp_cksum = 0;
	icmp_head->icmp_seq = icmp_head_s->icmp_seq;
	icmp_head->icmp_id = icmp_head_s->icmp_id ;
	struct timeval *time = icmp_head->icmp_data;

	gettimeofday(time,NULL);
	char *data_part = data+16;
	int i;
	for(i = 8 + sizeof(struct timeval); i < size; ++i)
		data[i] = 0xc;
	icmp_head->icmp_cksum = checksum((unsigned char *)data,size);
	return (*time);
}

int translate_buffer(unsigned char *src_buf,unsigned char *dest_buf)
{
	unsigned char *eth = dest_buf;
	pack_eth(eth);
	unsigned char *ipHead = dest_buf+14;
	pack_ip(ipHead,src_buf);
	unsigned char *data = dest_buf+34;
	pack_icmp(data,src_buf);
	return 1;
}

bool check_packet(char *buffer,int n_read)
{
	eth_hdr *eth_head = buffer;
	char mac_addr[6];
	struct in_addr ip_addr=getip();
	get_mac(mac_addr,ip_addr);
	if(strncmp(eth_head->dstmac, mac_addr, 6) != 0)
		return false;
	if(eth_head->eth_type!=0x0008)
		return false;

	struct ip* ip_head = buffer + 14;
	if(!((ip_head->ip_v == 4)&&(ip_head->ip_ttl >= 0) && (ip_head->ip_p == 1)))
		return false;

	set_ip_addr(inet_ntoa(ip_head->ip_src));
	int ip_head_len = ip_head->ip_hl * 4;
	struct icmp* icmp_head = buffer+ip_head_len+14;
	int len = n_read - ip_head_len;
	if(len < 8)
		return false;

	if(!(icmp_head->icmp_type == ICMP_ECHO))
		return false;
    
	return true;
}
