#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
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
#include <stdlib.h>
#include <stdbool.h>
// All of #include

// #define GRPDEBUG
#define GRPPRINT
#define MAX_ROUTE_SIZE 2 // pc1 -> pc2 pc2 -> pc1, in GUDIE, it named MAX_ROUTE_INFO
#define MAX_ARP_SIZE 4
#define MAX_DEVICE_SIZE 4 // in GUIDE, it named MAX_DIVICE
#define MAX_IP_SIZE 2
#define BUFFER_MAX 4096
#define SIZE 64

#define APR_FILE_NAME "arp.txt"
#define ROUTE_FILE_NAME "route.txt"
#define IP_FILE_NAME "ip.txt"


struct eth_header  
{  
    unsigned char dstmac[6]; // des mac  
    unsigned char srcmac[6]; // src mac 
    unsigned short eth_type; // type  
};


struct route_item
{
    char destination[16];
    char gateway[16];
    char netmask[16];
    // char interface_name[5];
    char interface[16]; // just like interfac_name: ens33
}route_info[MAX_ROUTE_SIZE]; // about route info
int route_item_index = 0;

// struct route_item{
// 	char destination[16];
// 	char gateway[16];
// 	char interface_name[5];
// 	char interface[16];
// }route_info[MAX_ROUTE_INFO];

struct arp_table_item
{
    char ip_addr[16];
    char mac_addr[18];
}arp_table[MAX_ARP_SIZE]; // about arp info
int arp_item_index = 0;

struct device_item
{
    char interface[14];
    char mac_addr[18];
}device[MAX_DEVICE_SIZE];
int device_index = 0;


int sock_fd;
struct sockaddr_in dest;
/*
struct sockaddr_in 
{
    short int sin_family;                      /* Address family 
    unsigned short int sin_port;       /* Port number 
    struct in_addr sin_addr;              /* Internet address 
    unsigned char sin_zero[8];         /* Same size as struct sockaddr 
};
struct in_addr 
{
    unsigned long s_addr;
};
typedef struct in_addr {
union {
            struct{
                        unsigned char s_b1,
                        s_b2,
                        s_b3,
                        s_b4;
                        } S_un_b;
           struct {
                        unsigned short s_w1,
                        s_w2;
                        } S_un_w;
            unsigned long S_addr;
          } S_un;
} IN_ADDR;

sin_family指代协议族，在socket编程中只能是AF_INET
sin_port存储端口号（使用网络字节顺序）
sin_addr存储IP地址，使用in_addr这个数据结构
sin_zero是为了让sockaddr与sockaddr_in两个数据结构保持大小相同而保留的空字节。
s_addr按照网络字节顺序存储IP地址

sockaddr_in和sockaddr是并列的结构，指向sockaddr_in的结构体的指针也可以指向
sockadd的结构体，并代替它。也就是说，你可以使用sockaddr_in建立你所需要的信息,
在最后用进行类型转换就可以了bzero((char*)&mysock,sizeof(mysock));//初始化
mysock结构体名
mysock.sa_family=AF_INET;
mysock.sin_addr.s_addr=inet_addr("192.168.0.1");
……
等到要做转换的时候用：
（struct sockaddr*）mysock
*/
// https://blog.csdn.net/renchunlin66/article/details/52351751

struct sockaddr_ll sll;
/*
struct sockaddr_ll
{
   unsigned short int sll_family; /* 一般为AF_PACKET 
   unsigned short int sll_protocol; /* 上层协议 
   int sll_ifindex; /* 接口类型 
   unsigned short int sll_hatype; /* 报头类型 
   unsigned char sll_pkttype; /* 包类型 
   unsigned char sll_halen; /* 地址长度 
   unsigned char sll_addr[8]; /* MAC地址 
};
*/
// http://www.rrxjs.com/article/52


// unsigned short checksum(unsigned short *data, int temp);
uint16_t checksum(unsigned char* buffer, int bytes);
int ctoi(char c); // translate char(0x hex) to int
void char2mac(char *c, unsigned char *mac); // translate char(string) to mac address
void set_ip_addr(char *ip); // set ip to [struct sockaddr_in dest]
void init_arp();
void init_route();
int find_route(struct in_addr ip_addr); // return the index of the route
void get_arp_mac(struct in_addr ip_addr, unsigned char *mac); // get the ip, and return the mac addr in *mac
bool receive_packet(char *buffer); // if success, return ture
bool translate_buffer(char *buffer);
bool check_packet(char *buffer, int n_read);
bool find_mac(unsigned char *mac);
bool router();
bool send_packet(char *final);



int main()
{
    init_arp();
    init_route();
    
    bool flag = router();
    if (!flag)
        printf("PING ERROR\n");
    

    return 0;
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

int ctoi(char c)
{
    assert((c >= '0' && c <= '9') || (c >= 'a' && c <='f'));
    // whether c is correct

    if (c >= '0' && c <= '9')
        return c - '0';
    else
        return c - 'a' + 10;
}

void char2mac(char *c, unsigned char *mac)
{
#ifdef GRPDEBUG
    printf("char2mac : [%s]\n", c);
#endif
    for (int i = 0; i < 6; ++i)
    {
        mac[i] = ctoi(c[3 * i]) * 16 + ctoi(c[3 * i + 1]);
    }
    return;
}

void set_ip_addr(char *ip)
{
    memset(&dest, 0, sizeof(dest)); // init all 0
    struct hostent *hp = gethostbyname(ip); // if it is IP, it won't return NULL
    memcpy(&(dest.sin_addr), hp->h_addr_list[0], hp->h_length);
    unsigned char *str = hp->h_addr_list[0];
    dest.sin_family = hp->h_addrtype;

    return;
}

void init_arp()
{
    FILE *fd = fopen(APR_FILE_NAME, "r");
    assert(fd != NULL); // whether the file is opened successfully
	
	for(int i = 0; i < MAX_ARP_SIZE; ++i)
		fscanf(fd, "%s%s", &arp_table[i].ip_addr, &arp_table[i].mac_addr);

	fclose(fd);

    return;
}

void init_route()
{
    FILE *fd = fopen(ROUTE_FILE_NAME, "r");
    assert(fd != NULL);
	
	for(int i = 0; i < MAX_ROUTE_SIZE; ++i)
		fscanf(fd, "%s%s%s%s", &route_info[i].destination, &route_info[i].gateway, &route_info[i].netmask, &route_info[i].interface);
	fclose(fd);

    return;
}

int find_route(struct in_addr ip_addr)
{
    for(int i = 0; i < MAX_ROUTE_SIZE; ++i)
    {
        if((inet_addr(route_info[i].destination) & inet_addr(route_info[i].netmask)) == (ip_addr.s_addr & inet_addr(route_info[i].netmask)))
        {
            // printf("%x\n", ip_addr.s_addr);
            return i;
        }
    }
}

void get_arp_mac(struct in_addr ip_addr, unsigned char *mac)
{
    for(int i = 0; i < MAX_ARP_SIZE; ++i)
    {
        if(inet_addr(arp_table[i].ip_addr) == ip_addr.s_addr)
        {
            char2mac(arp_table[i].mac_addr, mac);
            break;
        }
    }

    return;
}

// bool receive_packet(char *buffer)
// {
//     while(true)
//     {

//     }

//     return true;
// }

bool translate_buffer(char *buffer)
{
    struct eth_header *eth_head = buffer;
    struct ip* ip_head = buffer + 14;
    /*
    struct ip {
    unsigned int   ip_hl:4; /* both fields are 4 bits 
    unsigned int   ip_v:4;
    uint8_t        ip_tos;
    uint16_t       ip_len;
    uint16_t       ip_id;
    uint16_t       ip_off;
    uint8_t        ip_ttl;
    uint8_t        ip_p;
    uint16_t       ip_sum;
    struct in_addr ip_src;
    struct in_addr ip_dst;
    };
    */
    // https://blog.csdn.net/changgongliu/article/details/78189534
    /*
    struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8    ihl:4,
        version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
    __u8    version:4,
        ihl:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
    __u8    tos;
    __be16  tot_len;
    __be16  id;
    __be16  frag_off;
    __u8    ttl;
    __u8    protocol;
    __sum16 check;
    __be32  saddr;
    __be32  daddr;
    /*The options start here. 
    };
    */
    // https://blog.csdn.net/dreamInTheWorld/article/details/77159101

    --ip_head->ip_ttl;
    ip_head->ip_sum = checksum((unsigned char *)ip_head, 20);

    unsigned char dest_addr[16];
    strcpy(dest_addr, inet_ntoa(ip_head->ip_dst));
    set_ip_addr((char *)dest_addr);

    struct in_addr dest_ip;
    dest_ip.s_addr = ip_head->ip_dst.s_addr;
    int i = find_route(dest_ip);

    if (i >= MAX_ROUTE_SIZE)
    {
        printf("translate ERROR\n");
        return false;
    }

    struct in_addr route_next_addr; // next point
    route_next_addr.s_addr = inet_addr(route_info[i].gateway);
    get_arp_mac(route_next_addr, eth_head->dstmac);

    /*
    struct sockaddr_ll
    {
    unsigned short int sll_family; /* 一般为AF_PACKET 
    unsigned short int sll_protocol; /* 上层协议 
    int sll_ifindex; /* 接口类型 
    unsigned short int sll_hatype; /* 报头类型 
    unsigned char sll_pkttype; /* 包类型 
    unsigned char sll_halen; /* 地址长度 
    unsigned char sll_addr[8]; /* MAC地址 
    };
    */
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_IP);
    struct ifreq my_ifr;
    /*
    struct ifreq 
    {
    #define IFHWADDRLEN 6
    union
    {
    char ifrn_name[IFNAMSIZ];  /* if name, e.g. "en0" 
    } ifr_ifrn;
    
    union {
    struct sockaddr ifru_addr;
    struct sockaddr ifru_dstaddr;
    struct sockaddr ifru_broadaddr;
    struct sockaddr ifru_netmask;
    struct  sockaddr ifru_hwaddr;
    short ifru_flags;
    int ifru_ivalue;
    int ifru_mtu;
    struct  ifmap ifru_map;
    char ifru_slave[IFNAMSIZ]; /* Just fits the size 
    char ifru_newname[IFNAMSIZ];
    void __user * ifru_data;
    struct if_settings ifru_settings;
    } ifr_ifru;
    };
    #define ifr_name ifr_ifrn.ifrn_name /* interface name  
    #define ifr_hwaddr ifr_ifru.ifru_hwaddr /* MAC address   
    #define ifr_addr ifr_ifru.ifru_addr /* address  
    #define ifr_dstaddr ifr_ifru.ifru_dstaddr /* other end of p-p lnk 
    #define ifr_broadaddr ifr_ifru.ifru_broadaddr /* broadcast address 
    #define ifr_netmask ifr_ifru.ifru_netmask /* interface net mask 
    #define ifr_flags ifr_ifru.ifru_flags /* flags  
    #define ifr_metric ifr_ifru.ifru_ivalue /* metric  
    #define ifr_mtu  ifr_ifru.ifru_mtu /* mtu   
    #define ifr_map  ifr_ifru.ifru_map /* device map  
    #define ifr_slave ifr_ifru.ifru_slave /* slave device  
    #define ifr_data ifr_ifru.ifru_data /* for use by interface 
    #define ifr_ifindex ifr_ifru.ifru_ivalue /* interface index 
    #define ifr_bandwidth ifr_ifru.ifru_ivalue    /* link bandwidth 
    #define ifr_qlen ifr_ifru.ifru_ivalue /* Queue length  
    #define ifr_newname ifr_ifru.ifru_newname /* New name  
    #define ifr_settings ifr_ifru.ifru_settings /* Device/proto settings
    */
    // https://blog.csdn.net/zhu114wei/article/details/6927513

    memset(&my_ifr, 0, sizeof(my_ifr)); // init all ifreq
    strcpy(my_ifr.ifr_name, route_info[i].interface); // interface name like "ens33"
    ioctl(sock_fd, SIOCGIFINDEX, &my_ifr);
    // Linux 下 可以使用ioctl()函数 以及 结构体 struct ifreq  结构体struct ifconf来获取网络接口的各种信息。
    sll.sll_ifindex = my_ifr.ifr_ifindex;
    memcpy(sll.sll_addr, eth_head->dstmac, 6);
    sll.sll_halen = 6;

    return true;
}

bool find_mac(unsigned char *mac)
{
    FILE *fd = fopen(IP_FILE_NAME,"r");	
    assert(fd != NULL);

	int i = 0;
	for(; i < MAX_IP_SIZE; ++i)
	{
		char temp[16];
		fscanf(fd,"%s",&temp);
		fscanf(fd,"%s",&temp);		
		int j = 0;
		for(; j < MAX_ARP_SIZE; ++j)
		{
			if(strncmp(arp_table[j].ip_addr, temp,16) == 0)
			{
				unsigned char mac_temp[6];
				memset(mac_temp,0,6);
				char2mac(arp_table[j].mac_addr, mac_temp);		
				
				if(mac_temp[0]==mac[0] && mac_temp[1]==mac[1] && mac_temp[2]==mac[2]
                   && mac_temp[3]==mac[3] && mac_temp[4]==mac[4] && mac_temp[5]==mac[5])
				{
					fclose(fd);
					return true;
				}
				break;
			}
		}
	}
	
	fclose(fd);
	return false;
}

bool check_packet(char *buffer, int n_read)
{
    struct eth_header *eth_head = buffer;
    if(find_mac(eth_head->dstmac) == false)
        return false;
    if(eth_head->eth_type != 0x0008)
        return false;
    
    struct ip *ip_head = buffer + 14;
    if(!((ip_head->ip_v == 4) && (ip_head->ip_ttl >= 0) && (ip_head->ip_p == 1)))
		return false;	
	int ip_head_len = ip_head->ip_hl * 4;
	struct icmp* icmp_head = buffer+ip_head_len+14;
	int len = n_read - ip_head_len;
	if(len < 8)
	{
		printf("ICMP length error\n");
		return false;
	}
	return true;
}

bool receive_packet(char *buffer)
{
    while(true)
	{
#ifdef GRPPRINT
        printf("<A RECEIVE PACKET TIME>\n");
#endif
		struct sockaddr_ll from;
		int fromlen = sizeof(from);
		int n_read = recvfrom(sock_fd, buffer, BUFFER_MAX, 0, (struct sockaddr *)&from, &fromlen);
		if(n_read < 0)
		{
			if(errno == EINTR)
                continue;
            else
			    printf("number error\n");
			return false;
		}
		//solve_packet(buffer,n_read);
		if(check_packet(buffer,n_read))
		{
			if(translate_buffer(buffer)) 
                break;
		}
	}
	return true;
}

bool send_packet(char *final)
{	
	if(sendto(sock_fd, final, SIZE + 34, 0, (struct sockaddr *)&sll, sizeof(sll)) < 0)
		return false;
	return true;
}

bool router()
{
#ifdef GRPPRINT
    printf("<START ROUTER>\n");
#endif
    if((sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) == -1)
    {
        printf("ERROR create raw socket\n");
        printf("create socket error...%s\n", strerror(errno));
     //return conn_fd;
        return false;
    }
	while(true)
	{
#ifdef GRPPRINT
    printf("<A ROUTER TIME>\n");
#endif
		char buffer[BUFFER_MAX];
		if(!receive_packet(buffer))
		{
			printf("receive error\n");
			return false;
		}	
		if(!send_packet(buffer))
		{
			printf("send error\n");
			return false;
		}
	}
	return true;
}