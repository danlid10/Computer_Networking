#include "nsclient.h"

// Gets domain name from the user. Return 0  if user entered "quit", else return non zero value.
int get_domain_name(char* domain_name)
{
	printf("nsclient> ");
	fgets(domain_name, MAX_DOMAIN_NAME, stdin);
	domain_name[strcspn(domain_name, "\r\n")] = '\0'; // remove '\n' and '\r'
	remove_leading_spaces(domain_name);
	return strcmp(domain_name, "quit");
}

// Converts domain name to DNS format domain name, return the size of DNS domain name.
int domain2DNS(char* domain_name, char* DNS_domain_name)
{
	char* token;
	char str[MAX_DOMAIN_NAME];

	DNS_domain_name[0] = '\0';
	
	token = strtok(domain_name, ".");
	while (token)
	{
		sprintf(str, "%c%s", (int)strlen(token), token);
		strcat(DNS_domain_name, str);
		token = strtok(NULL, ".");
	}
	strcat(DNS_domain_name, "\0");

	return strlen(DNS_domain_name) + 1;

}

// Converts DNS format domain name to domain name.
void DNS2domain(char* domain_name, char* DNS_domain_name)
{
	int size;
	domain_name[0] = '\0';

	while (*DNS_domain_name)
	{
		size = *DNS_domain_name;
		strncpy(domain_name, DNS_domain_name + 1, size);
		domain_name[size] = '.';
		domain_name += size + 1;
		DNS_domain_name += size + 1;
	}
	domain_name[-1] = '\0';	// Replace last dot with NULL 
}

// Remove leading spaces from a string.
void remove_leading_spaces(char *str)
{
	char* str_cpy = str;
	while (isspace(*str_cpy))
		str_cpy++;

	if (str != str_cpy)
		strcpy(str, str_cpy);
}

// Check domain name format: return 1 and converts all alphabet characters to lowercase if valid, else return 0.
int check_domain_name(char* domain_name)
{
	int dot_count = 0;
	char prev_ch = 0;

	// Check length and first character
	if (strlen(domain_name) < 3 || *domain_name == '.')	
		return 0;

	while (*domain_name)
	{
		// Check for invalid characters 
		if (!isalnum(*domain_name) && *domain_name != '-' && *domain_name != '.')
			return 0;

		// Check dot sequence 
		if (*domain_name == '.')
		{ 
			if (prev_ch == '.')	
				return 0;
			dot_count++;
		}
		
		prev_ch = *domain_name;

		// Character to lowercase 
		*domain_name = tolower(*domain_name);

		domain_name++;
	}

	// Check last character and amount of dots
	if (prev_ch == '.' || dot_count == 0)
		return 0;

	return 1;

}

//  Check IPv4 address format: return non zero value if valid , else retrun 0.
int chek_IPv4_addr(char* IPaddr)
{
	SOCKADDR_IN sa;
	return inet_pton(AF_INET, IPaddr, &(sa.sin_addr));
}

// Initialize Winsock, create a UDP socket with given IP address and port and set receive timeout.
SOCKET create_UDP_socket(SOCKADDR_IN* server, char* serverIPaddress, int serverPort)
{
	int status, timeout, slen;
	WSADATA wsaData;
	SOCKET sock;

	// Initialize Winsock
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
	{
		printf("[ERROR] WSAStartup failed! Error code: %d\n", status);
		exit(1);
	}

	// DNS server parameters 
	server->sin_family = AF_INET;
	server->sin_addr.s_addr = inet_addr(serverIPaddress);
	server->sin_port = htons(serverPort);

	slen = sizeof(server);
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	// Set receive timeout
	timeout = TIMEOUT_MS;
	status = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	if (status == SOCKET_ERROR)
	{
		printf("[ERROR] setsockopt failed! Error code: %d\n", WSAGetLastError());
		exit(1);
	}

	return sock;

}

// Fill DNS_header with data converted from host to network.
void create_DNS_header(DNS_header* header, unsigned short id, unsigned short flags, unsigned short qdcount, unsigned short ancount, unsigned short nscount, unsigned short arcount)
{
	header->id = htons(id);
	header->flags = htons(flags);		
	header->qdcount = htons(qdcount);
	header->ancount = htons(ancount);
	header->nscount = htons(nscount);
	header->arcount = htons(arcount);
}

// Fill DNS_qinfo with data converted from host to network.
void create_DNS_qinfo(DNS_qinfo* qinfo, unsigned short qtype, unsigned short qclass)
{
	qinfo->qtype = htons(qtype);
	qinfo->qclass = htons(qclass);
}

// DNS_header: convert form network to host.
void ntoh_DNS_header(DNS_header* header)
{
	header->id = ntohs(header->id);
	header->flags = ntohs(header->flags);
	header->qdcount = ntohs(header->qdcount);
	header->ancount = ntohs(header->ancount);
	header->nscount = ntohs(header->nscount);
	header->arcount = ntohs(header->arcount);

}

// DNS_rinfo: convert form network to host.
void ntoh_DNS_rinfo(DNS_rinfo* rinfo)
{
	rinfo->rname = ntohs(rinfo->rname);
	rinfo->rtype = ntohs(rinfo->rtype);
	rinfo->rclass = ntohs(rinfo->rclass);
	rinfo->ttl = ntohl(rinfo->ttl);
	rinfo->rdlength = ntohs(rinfo->rdlength);

}

// Get the IP address of given domain name from a DNS server, return as HOSTENT pointer.
HOSTENT* dnsQuery(char* domain_name)
{
	int qname_len, slen, sentBytes, receivedBytes, query_Size, curr_ans, last_ans_offset, rdata_offset, rname_offset;
	unsigned char buffer[MAX_PACKET], * qname, * rdata;
	DNS_header* DNS = NULL;
	DNS_qinfo* qinfo = NULL;
	DNS_rinfo* answer = NULL;
	HOSTENT hstnt;

	// Set DNS header in the buffer
	DNS = (DNS_header*)&buffer;
	// flags = 0x100 -> RD = 1 -> Recursion Desired
	create_DNS_header(DNS, query_ID, 0x100, 1, 0, 0, 0);	
	// ID increment for the next query
	query_ID++;	

	// Set DNS question in the buffer
	qname = &buffer[sizeof(DNS_header)];
	qname_len = domain2DNS(domain_name, qname);
	qinfo = (DNS_qinfo*)&buffer[sizeof(DNS_header) + qname_len];
	// qtype = 1 -> A -> Host Address,  qclass = 1 -> IN -> The Internet
	create_DNS_qinfo(qinfo, 1, 1);

	query_Size = sizeof(DNS_header) + qname_len + sizeof(DNS_qinfo);

	slen = sizeof(DNS_sock_addrin);

	// Send query to the DNS server
	sentBytes = sendto(DNS_sock, buffer, query_Size, 0, (SOCKADDR_IN*)&DNS_sock_addrin, slen);
	if (sentBytes == SOCKET_ERROR)
	{
		printf("[ERROR] sendto failed! Error code: %d\n", WSAGetLastError());
		return NULL;
	}

	// Receive response from the DNS server
	receivedBytes = recvfrom(DNS_sock, buffer, MAX_PACKET, 0, (SOCKADDR_IN*)&DNS_sock_addrin, &slen);
	if (receivedBytes == SOCKET_ERROR)
	{
		printf("[ERROR] recvfrom failed! Error code: %d\n", WSAGetLastError());
		return NULL;
	}

	// Get the DNS header form the buffer
	DNS = (DNS_header*)&buffer;
	ntoh_DNS_header(DNS);

	// Check if received answers
	if (DNS->ancount == 0)
	{ 
		printf("[ERROR] NONEXISTENT: Domain name was not found.\n");
		return NULL;
	}


	// Get the last DNS answer form the buffer
	curr_ans = 1;
	last_ans_offset = query_Size;
	answer = (DNS_rinfo*)&buffer[query_Size];
	ntoh_DNS_rinfo(answer);
	while (curr_ans < DNS->ancount)
	{
		last_ans_offset += sizeof(DNS_rinfo) + answer->rdlength;
		answer = (DNS_rinfo*)&buffer[last_ans_offset];
		ntoh_DNS_rinfo(answer);
		curr_ans++;
	}

	// Get RDATA form the buffer
	rdata_offset = last_ans_offset + sizeof(DNS_rinfo);
	rdata = &buffer[rdata_offset];

	// Set HOSTENT with the IP address received from the DNS server
	hstnt.h_addrtype = AF_INET;
	hstnt.h_length = 4;
	hstnt.h_addr_list = rdata;

	return &hstnt;
}

int main(int argc, char** argv)
{
	int valid_domain_name;
	char domain_name[MAX_DOMAIN_NAME];
	HOSTENT* hstnt = NULL;
	IN_ADDR* addr;

	// Check command line arguments
	if (argc < 2)
	{
		printf("[ERROR] Incorrect command line arguments: missing argv[1].\n");
		exit(1);
	}
	if(!chek_IPv4_addr(argv[1]))
	{
		printf("[ERROR] Incorrect command line arguments: argv[1] is not a valid IPv4 address.\n");
		exit(1);
	}
	
	// Initialize Winsock and set the global socket
	DNS_sock = create_UDP_socket(&DNS_sock_addrin, argv[1], DNS_PORT);

	while (get_domain_name(domain_name))
	{
		valid_domain_name = check_domain_name(domain_name);
		if (!valid_domain_name)
		{
			hstnt = NULL;
			printf("[ERROR] BAD NAME: Invalid domain name.\n");
		}
		else
			hstnt = dnsQuery(domain_name);
		

		if (hstnt != NULL)
		{
			addr = (IN_ADDR*)hstnt->h_addr_list;
			printf("%s\n", inet_ntoa(*addr));
		}

	} 

	// Free the global socket and cleanup Winsock 
	closesocket(DNS_sock);
	WSACleanup();

	return 0;
}