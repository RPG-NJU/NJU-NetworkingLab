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
#include <assert.h>
#include <stdbool.h>

#define GRPPRINT


#define MAX_ARP_SIZE 2
#define SIZE 64
#define BUFFER_MAX 4096

#define APR_FILE_NAME "arp.txt"
#define ROUTE_FILE_NAME "route.txt"
#define IP_FILE_NAME "ip.txt"
#define GATEWAY_FILE_NAME "gateway.txt"

typedef struct _eth_hdr  
{  
    unsigned char dstmac[6]; // dest mac addr   
    unsigned char srcmac[6]; // src mac addr   
    unsigned short eth_type; // eth type   
}eth_hdr;

struct arp_table_item{
	char ip_addr[16];
	char mac_addr[18];
}arp_table[MAX_ARP_SIZE];

struct sockaddr_in dest;
struct sockaddr_ll sll;
pid_t pid;
int sock_fd;
float timeout = 1000;

int ping(char *addr,int timeout);
void set_ip_addr(char *addr);
struct timeval pack_icmp(int seq,char *data);
int send_packet(int seq,struct timeval *time);
uint16_t checksum(unsigned char* buffer, int bytes);
int receive_packet(struct timeval* start);
bool check_packet(char *buffer,struct timeval rece_time,int n_read);
float time_sub(struct timeval *old_time,struct timeval *new_time);
struct in_addr getip();
struct in_addr getgateway();
void init_mac();
void get_mac(unsigned char *mac,struct in_addr ip);
void pack_eth(char *eth);
void pack_ip(char *ipt,int num);

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("<Please input destination>\n");
        exit(-1);
    }

	init_mac();
	int i=0;
    int result = ping(argv[1],3000);

    if(result ==-1)
        printf("ping error\n");

    return 0;
}

int ping(char *addr,int timeout)
{
    if((sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) 
    // 底层包访问
    // RAW类型，提供原始网络协议访问
    // IP
    {
        printf("error create raw socket\n");
        return -1;
    }

	set_ip_addr(addr);
	pid = getpid();

	int seq = 1;
	while(1)
	{
		struct timeval start;
#ifdef GRPPRINT
        printf("<PING STRAT A TIME>\n");
#endif

		if(send_packet(seq, &start) < 0)
		{
			printf("send error\n");
			return -1;
		}

		//printf("send successfully\n");

		if(receive_packet(&start) < 0)
		{
			printf("receive error\n");
			return -2;
		}

		//printf("receive successfulle\n");		
	
		seq++;
		sleep(1);
	}

	return 1;
}

void set_ip_addr(char *ip)
{
    memset(&dest, 0, sizeof(dest)); // init all 0
    struct hostent *hp = gethostbyname(ip); // if it is IP, it won't return NULL
    memcpy(&(dest.sin_addr), hp->h_addr_list[0], hp->h_length);
    unsigned char *str = hp->h_addr_list[0];
    printf("PING %s(%s): %d bytes data in ICMP packets.\n",ip,inet_ntoa(dest.sin_addr),SIZE);
    dest.sin_family = hp->h_addrtype;

    return;
}

int send_packet(int seq,struct timeval *time)
{
	unsigned char final[34 + SIZE]; // all data in this
	unsigned char *eth = final;
	pack_eth(eth);
	unsigned char *ip_head = final+14;
	pack_ip(ip_head, seq);
	unsigned char *data = final + 34;
	*time = pack_icmp(seq, data);

	if(sendto(sock_fd, final, SIZE+34, 0, (struct sockaddr *)&sll, sizeof(sll)) < 0)
		return -1;
	return 1;
}
struct in_addr getip()
{
	struct in_addr addr;
	FILE *fd = fopen(IP_FILE_NAME,"r");
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
	FILE *fd = fopen(GATEWAY_FILE_NAME,"r");
	char temp[20];
	fscanf(fd,"%s",&temp);
	addr.s_addr = inet_addr(temp);
	fclose(fd);
	return addr;
}

void init_mac()
{
	FILE *fd = fopen(APR_FILE_NAME,"r");
	int i = 0;
	for(;i<MAX_ARP_SIZE;i++)
		fscanf(fd,"%s%s",&arp_table[i].ip_addr,&arp_table[i].mac_addr);
	fclose(fd);
}

int ctoi(char c)
{
	assert((c >= '0' && c <= '9') || (c >= 'a' && c <='f'));
    // whether c is correct

    if (c >= '0' && c <= '9')
        return c - '0';
    else
        return c - 'a' + 10;
}

void char2mac(char *mac,char *src)
{
	#ifdef GRPDEBUG
    printf("char2mac : [%s]\n", src);
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

void pack_ip(char *ipt,int num)
{
	struct ip* ip_head = ipt;
	ip_head->ip_v = 4;
	ip_head->ip_hl = 5;
	ip_head->ip_tos = 0x00;
	ip_head->ip_len = (20+SIZE)*256;	
	ip_head->ip_id = num;
	ip_head->ip_off = 0;
	ip_head->ip_ttl=64;
	ip_head->ip_p = 1;
	ip_head->ip_sum = 0;
	ip_head->ip_src=getip();
	ip_head->ip_dst= dest.sin_addr;
	ip_head->ip_sum = checksum((unsigned short *)ipt,20);
}

struct timeval pack_icmp(int seq,char *data)
/*
struct timeval
{
__time_t tv_sec;        /* Seconds. 
__suseconds_t tv_usec;  /* Microseconds. 
};
*/
{
	struct icmp*icmp_head = data;
	icmp_head->icmp_type = ICMP_ECHO;
	icmp_head->icmp_code = 0;
	icmp_head->icmp_cksum = 0;
	icmp_head->icmp_seq = seq;
	icmp_head->icmp_id = pid;
	struct timeval *time =icmp_head->icmp_data;
	gettimeofday(time,NULL);

	for(int i = 8 + sizeof(struct timeval); i < SIZE; ++i)
		data[i] = 0xc;

	icmp_head->icmp_cksum = checksum((unsigned char *)data,SIZE);
	return (*time);
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

int receive_packet(struct timeval *start)
{
	while(1)
	{
		struct sockaddr_ll from;
		int fromlen = sizeof(from);
		char buffer[BUFFER_MAX];
		int n_read = recvfrom(sock_fd,buffer,BUFFER_MAX,0,(struct sockaddr *)&from,&fromlen);
		if(n_read <0)
		{
			if(errno==EINTR)continue;
			printf("number error\n");
			return -1;
		}		
		struct timeval rece_time;
		 gettimeofday(&rece_time,NULL);
		
		if(check_packet(buffer,rece_time,n_read))
			break;
	}
	return 1;
}

bool check_packet(char *buffer, struct timeval rece_time, int n_read)
{
	eth_hdr *eth_head = buffer;
	char mac_addr[6];
	struct in_addr ip_addr=getip();
	get_mac(mac_addr,ip_addr);
	if(strncmp(eth_head->dstmac,mac_addr,6)!=0)
		return false;
	if(eth_head->eth_type!=0x0008)
		return false;
	struct ip *ip_head = buffer+14;
	int ip_head_len = ip_head->ip_hl<<2;
	struct icmp* icmp_head = buffer+ip_head_len+14;
	int len = n_read - ip_head_len-14;
	if(len<8)
	{
		printf("length error\n");
		return false;
	}
	if((icmp_head->icmp_type == ICMP_ECHOREPLY)&&(icmp_head->icmp_id == pid))
	{
		struct timeval *send = (struct timeval *)icmp_head->icmp_data;
		double time = time_sub(&rece_time, send);
		printf("%d byte from %s: icmp_seq=%u ttl=%d delay=%.10fms\n",
               len, inet_ntoa(dest.sin_addr), icmp_head->icmp_seq, ip_head->ip_ttl, time);
        }
	else
	{
		return false;
	}
	return true;
}
float time_sub(struct timeval *old_time,struct timeval *new_time)
{
    float x = 0.0;
    // printf("%d\n", old_time->tv_sec - new_time->tv_sec);
    x = (float)((new_time->tv_sec - old_time->tv_sec) * 1000) + ((float)(new_time->tv_usec - old_time->tv_usec) / 1000);
	return x;
}
