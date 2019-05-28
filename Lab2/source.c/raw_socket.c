#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <netinet/in.h>

// typedef int int32;
// typedef unsigned int u_int32;
// typedef unsigned char u_char;
// typedef unsigned short u_short;

#define BUFFER_MAX 2048


#define BLUE "\033[34m"
#define NONE "\033[0m"
#define LIGHTBLUE "\033[36m"
#define RED "\033[31m"
#define LIGHTRED "\033[35m"
#define YELLOW "\033[33m"
#define GREEN "\033[32m"

#define IP_TYPE 0x800
#define ARP_TYPE 0x806
#define RARP_TYPE 0x8035

#define DIVIDE printf("--------------------------------------------------\n");

#define __LITTLE_ENDIAN_BIFIELD


struct Mac_Frame_Header
{
    char dst_mac_addr[6];
    char src_mac_addr[6];
    unsigned short type;
}__attribute__((packed)) Mac_Frame_Header;

struct IP_Header
{
#ifdef __LITTLE_ENDIAN_BIFIELD
    __u_char ip_len:4, ip_ver:4;
#else
    __u_char ip_ver:4, ip_len:4;
#endif

    // unsigned short  ip_tos;
    // unsigned int ip_total_len;
    // unsigned int ip_id;
    // unsigned int ip_flags;
    // unsigned char  ip_ttl;
    // unsigned char  ip_protocol;
    // unsigned int ip_chksum;
    unsigned char service_field;
    unsigned short total_len;
    unsigned short id;
    unsigned short flags;
    unsigned char time_to_live;
    unsigned char protocol;
    unsigned short checksum;
    u_int32_t ip_src;
    u_int32_t ip_dest;
}__attribute__((packed)) IP_Header;

struct ICMP
{
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
}__attribute__((packed)) ICMP;

struct ARP
{
    unsigned short hardware_type;
    unsigned short protocol_type;
    unsigned char hardware_size;
    unsigned char protocol_size;
    unsigned short opcode;
    unsigned char sender_mac[6];
    unsigned int sender_ip;
    unsigned char target_mac[6];
    unsigned int target_ip;
}__attribute__((packed)) ARP;

int print_Ethernet(struct Mac_Frame_Header *header);

int print_IP(char *header);

int print_ICMP(char *header);

int print_ARP(char *header);

int print_IGMP(char *header);

int main(int argc, char *argv[])
{
    int sock_fd;
    int proto;
    int n_read;
    char buffer[BUFFER_MAX];
    char *eth_head;
    char *ip_head;
    // char *tcp_head;
    // char *udp_head;
    char *icmp_head;
    unsigned char *p;

    struct Mac_Frame_Header *mac_header;
    int frame_type = 0;

    printf(">>>>>  " LIGHTBLUE "Coding by HELLORPG" NONE "\n");

    if ((sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
    {
        printf("error create raw socket\n");
        return -1;
    }

    while(1)
    {
        n_read = recvfrom(sock_fd, buffer, 2048, 0, NULL, NULL);

        if (n_read < 42) // why is 42? maybe too short?
        {
            printf("error when recv msg \n");
            return -1;
        }

        // eth_head = buffer;
        // p = eth_head;

        // printf("MAC address: %.2x:%02x:%02x:%02x:%02x:%02x ==> %.2x:%02x:%02x:%02x:%02x:%02x\n",
        // p[6],p[7],p[8],p[9],p[10],p[11],
        // p[0],p[1],p[2],p[3],p[4],p[5]);

        // ip_head = eth_head+14;
        // p = ip_head+12;

        // printf("IP:%d.%d.%d.%d==> %d.%d.%d.%d\n",
        // p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);

        // proto = (ip_head + 9)[0];
        // p = ip_head +12;

        mac_header = (struct Mac_Frame_Header*)buffer;
        

        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

        frame_type = print_Ethernet(mac_header);
        // return the type in Ethernet II

        switch(frame_type) // to decide how long and how to translate the data
        {
            // next header is buffer + 14
            case IP_TYPE: proto = print_IP(buffer + 14); break;
            case ARP_TYPE: proto = print_ARP(buffer + 14); break;
            case RARP_TYPE: break;
        }



        printf("Protocol: ");

        switch(proto)
        {
        case IPPROTO_ICMP:printf(YELLOW "icmp\n" NONE);break;
        case IPPROTO_IGMP:printf(YELLOW "igmp\n" NONE);break;
        case IPPROTO_IPIP:printf(YELLOW "ipip\n" NONE);break;
        case -77: printf(YELLOW "arp\n" NONE); break;
        // case IPPROTO_TCP:printf("tcp\n");break; // Don't need.
        // case IPPROTO_UDP:printf("udp\n");break; // Don't need.
        default:printf(RED "Pls query yourself" NONE "\n");
        }
    }
    return -1;
}

int print_Ethernet(struct Mac_Frame_Header *header)
{

    int ret = 0;

    printf(BLUE "Ethernet II, Src: " NONE "%.2x:%02x:%02x:%02x:%02x:%02x", 
    header->src_mac_addr[0] & 0xff, header->src_mac_addr[1] & 0xff, header->src_mac_addr[2] & 0xff, header->src_mac_addr[3] & 0xff, header->src_mac_addr[4] & 0xff, header->src_mac_addr[5] & 0xff);
    printf(BLUE " , Dst: " NONE "%.2x:%02x:%02x:%02x:%02x:%02x\n",
    header->dst_mac_addr[0] & 0xff, header->dst_mac_addr[1] & 0xff, header->dst_mac_addr[2] & 0xff, header->dst_mac_addr[3] & 0xff, header->dst_mac_addr[4] & 0xff, header->dst_mac_addr[5] & 0xff);

    printf("    " BLUE "Src Mac Address: " NONE "%.2x:%02x:%02x:%02x:%02x:%02x\n", 
    header->src_mac_addr[0] & 0xff, header->src_mac_addr[1] & 0xff, header->src_mac_addr[2] & 0xff, header->src_mac_addr[3] & 0xff, header->src_mac_addr[4] & 0xff, header->src_mac_addr[5] & 0xff);
    printf("    " BLUE "Src Mac Address: " NONE "%.2x:%02x:%02x:%02x:%02x:%02x\n",
    header->dst_mac_addr[0] & 0xff, header->dst_mac_addr[1] & 0xff, header->dst_mac_addr[2] & 0xff, header->dst_mac_addr[3] & 0xff, header->dst_mac_addr[4] & 0xff, header->dst_mac_addr[5] & 0xff);
    printf("    " BLUE "Type: " NONE "%.4x", ntohs(header->type));

    switch(ntohs(header->type))
    {
        case IP_TYPE: printf(" (IP)"); ret = 0x800; break;
        case ARP_TYPE: printf(" (ARP)"); ret = 0x806; break;
        case RARP_TYPE: printf(" (RARP)"); ret = 0x8035; break;
        default: printf(RED "(CAN'T TRANSLATE)" NONE); ret = -1; break;
    }
    printf("\n");

    return ret;
}

int print_IP(char *header)
{
    struct IP_Header *ip_header = (struct IP_Header*)header;

    DIVIDE

    char *src_ip, *dst_ip;

    src_ip = (char*)&ip_header->ip_src;
    dst_ip = (char*)&ip_header->ip_dest;

    printf(LIGHTBLUE "Internet Protocol Version %d, ", ip_header->ip_ver);
    printf("Src: " NONE "%d.%d.%d.%d" LIGHTBLUE ", Dst: " NONE "%d.%d.%d.%d\n",
    src_ip[0] & 0xff, src_ip[1] & 0xff, src_ip[2] & 0xff, src_ip[3] & 0xff, 
    dst_ip[0] & 0xff, dst_ip[1] & 0xff, dst_ip[2] & 0xff, dst_ip[3] & 0xff);

    printf(LIGHTBLUE "    Version: " NONE "%d\n", ip_header->ip_ver & 0xf);
    printf(LIGHTBLUE "    Header Length: " NONE "%d bytes (%d)\n", (ip_header->ip_len & 0xf) * 4, ip_header->ip_len & 0xf);
    printf(LIGHTBLUE "    Differentiated Service Field: " NONE "0x%.2x\n", ip_header->service_field & 0xf);
    printf(LIGHTBLUE "    Total Length: " NONE "%d\n", ntohs(ip_header->total_len) & 0xffff);
    printf(LIGHTBLUE "    Identification: " NONE "0x%.4x (%d)\n", ntohs(ip_header->id) & 0xffff, ntohs(ip_header->id) & 0xffff);
    printf(LIGHTBLUE "    Flags: " NONE "0x%.4x\n", ntohs(ip_header->flags) & 0xffff);
    printf(LIGHTBLUE "    Time to live: " NONE "%d\n", ip_header->time_to_live & 0xff);
    printf(LIGHTBLUE "    Protocal: " NONE);
    switch(ip_header->protocol)
    {
        case 1: printf("ICMP ");break;
        default: printf(RED "CAN'T TRANSLATE " NONE);break;
    }
    printf("(%d)\n", ip_header->protocol & 0xff);
    printf(LIGHTBLUE "    Header checksum: " NONE "0x%.4x\n", ntohs(ip_header->checksum) & 0xffff);
    printf(LIGHTBLUE "    Source: " NONE "%d.%d.%d.%d\n" LIGHTBLUE "    Destination: " NONE "%d.%d.%d.%d\n",
    src_ip[0] & 0xff, src_ip[1] & 0xff, src_ip[2] & 0xff, src_ip[3] & 0xff, 
    dst_ip[0] & 0xff, dst_ip[1] & 0xff, dst_ip[2] & 0xff, dst_ip[3] & 0xff);
    
    switch(ip_header->protocol)
    {
        case 1: print_ICMP(header + (ip_header->ip_len & 0xf) * 4); break; // ICMP
        case 2: print_IGMP(header + (ip_header->ip_len & 0xf) * 4); break; // IGMP
        case 94: break; // IPIP
        default: break;
    }

    return ip_header->protocol & 0xff;
}

int print_ICMP(char *header)
{
    struct ICMP *icmp_header = (struct ICMP*)header;
    DIVIDE
    printf(GREEN "Internet Control Message Protocol\n" NONE);
    printf(GREEN "    Type: " NONE "%d\n", icmp_header->type & 0xff);
    printf(GREEN "    Code: " NONE "%d\n", icmp_header->code & 0xff);
    printf(GREEN "    Checksum: " NONE "0x%.4x\n", ntohs(icmp_header->checksum) & 0xffff);
    return 0;
}

int print_ARP(char *header)
{
    struct ARP *arp = (struct ARP*)header;
    unsigned char *s_ip = (char*)&arp->sender_ip, *t_ip = (char*)&arp->target_ip;

    DIVIDE
    printf(GREEN "Address Resolution Protocol\n" NONE);
    printf(GREEN "    Hardware type: " NONE);
    switch(arp->hardware_type)
    {
        case 1: printf("Ethernet "); break;
        default: break;
    }
    printf("(%d)\n", arp->hardware_type & 0xffff);
    printf(GREEN "    Protocol type: " NONE);
    switch(ntohs(arp->protocol_type))
    {
        case 0x800: printf("IPv4 "); break;
        default: break;
    }
    printf("(0x%.4x)\n", ntohs(arp->protocol_type) & 0xffff);

    printf(GREEN "    Hardware size: " NONE "%d\n", arp->hardware_size & 0xff);
    printf(GREEN "    Protocol size: " NONE "%d\n", arp->protocol_size & 0xff);
    printf(GREEN "    Opcode: " NONE);
    switch(ntohs(arp->opcode))
    {
        case 1: printf("request "); break;
        case 2: printf("answer "); break;
        default: break;
    }
    printf("(%d)\n", ntohs(arp->opcode));

    printf(GREEN "    Sender MAC address: " NONE);
    printf("%.2x:%02x:%02x:%02x:%02x:%02x\n", 
    arp->sender_mac[0] & 0xff, arp->sender_mac[1] & 0xff, arp->sender_mac[2] & 0xff, arp->sender_mac[3] & 0xff, arp->sender_mac[4] & 0xff, arp->sender_mac[5] & 0xff);
    
    printf(GREEN "    Sender IP address: " NONE "%d.%d.%d.%d\n",
    s_ip[0] & 0xff, s_ip[1] & 0xff, s_ip[2] & 0xff, s_ip[3] & 0xff);

    printf(GREEN "    Target MAC address: " NONE);
    printf("%.2x:%02x:%02x:%02x:%02x:%02x\n", 
    arp->target_mac[0] & 0xff, arp->target_mac[1] & 0xff, arp->target_mac[2] & 0xff, arp->target_mac[3] & 0xff, arp->target_mac[4] & 0xff, arp->target_mac[5] & 0xff);
    
    printf(GREEN "    Target IP address: " NONE "%d.%d.%d.%d\n",
    t_ip[0] & 0xff, t_ip[1] & 0xff, t_ip[2] & 0xff, t_ip[3] & 0xff);
    
    return -77;
}

int print_IGMP(char *buffer)
{

    return 0;
}