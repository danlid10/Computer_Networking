#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") 

// To avoid structure padding
#pragma pack(1)

#define MAX_DOMAIN_NAME 1000
#define DNS_PORT 53
#define TIMEOUT_MS 2000
#define MAX_PACKET 65535

// Structures
typedef struct
{
	/*
	flags:  | QR | Opcode | AA | TC | RD | RA | Z | RCODE |
	bits:	| 1  |    4   | 1  | 1  | 1  | 1  | 3 |   4   |
	*/
	unsigned short id;
	unsigned short flags;   
	unsigned short qdcount; 
	unsigned short ancount; 
	unsigned short nscount; 
	unsigned short arcount; 
} DNS_header;


typedef struct
{
	unsigned short qtype;		
	unsigned short qclass;		
} DNS_qinfo;

typedef struct
{
	unsigned short rname;
	unsigned short rtype;
	unsigned short rclass;
	unsigned int ttl;
	unsigned short rdlength;
} DNS_rinfo;

// Globals
SOCKET DNS_sock;
SOCKADDR_IN DNS_sock_addrin;
unsigned short query_ID = 0;

// Functions
int get_domain_name(char* domain_name);
void remove_leading_spaces(char* str);
int check_domain_name(char* domain_name);
int chek_IPv4_addr(char* IPaddr);
int domain2DNS(char* domain_name, char* DNS_domain_name);
void DNS2domain(char* domain_name, char* DNS_domain_name);
SOCKET create_UDP_socket(SOCKADDR_IN* server, char* serverIPaddress, int serverPort);
void create_DNS_header(DNS_header* header, unsigned short id, unsigned short flags, unsigned short qdcount, unsigned short ancount, unsigned short nscount, unsigned short arcount);
void create_DNS_qinfo(DNS_qinfo* qinfo, unsigned short qtype, unsigned short qclass);
void ntoh_DNS_header(DNS_header* header);
void ntoh_DNS_rinfo(DNS_rinfo* rinfo);
HOSTENT* dnsQuery(char* domain_name);


// https://cabulous.medium.com/dns-message-how-to-read-query-and-response-message-cfebcb4fe817