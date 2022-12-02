#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") 

#define MSG_SIZE 1000000
#define MAX_INPUT 100
#define MAX_HOSTNAME 300
#define MAX_IP_ADDR 300
// Ports set up - 0 for OS chosen port
#define SENDER_PORT	0	// 6342		
#define RECEIVER_PORT 0	//	6343	


SOCKET create_listen_socket(char IP_address[], int port, SOCKADDR_IN* client, int* chosen_port);
int continue_input();
void flip_bit(char buffer[], int n_bit);
int add_random_nosie(char buffer[], int msg_size, int n);
int add_deterministic_noise(char buffer[], int msg_size, int n);
void getIPaddress(char IPaddr[]);
int receive_data(SOCKET listen_sock, char buffer[]);
int send_data(SOCKET listen_sock, char buffer[], int msg_size);
int add_noise(char buffer[], int msg_size, char noise_type[], int n);


// Initialize Winsock and get local IP address
void getIPaddress(char IP_address[])
{
	struct in_addr addr;
	struct hostent* hstnm;
	WSADATA wsaData;
	int status;
	char hostname[MAX_HOSTNAME];

	// Initialize Winsock
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
	{
		fprintf(stderr, "WSAStartup failed: %d\n", status);
		exit(1);
	}

	gethostname(hostname, MAX_HOSTNAME);
	hstnm = gethostbyname(hostname);
	addr.s_addr = *(u_long*)hstnm->h_addr_list[0];

	strncpy(IP_address, inet_ntoa(addr), MAX_IP_ADDR);
}

// Ask user if he wishes to continue, return 1 for yes, loops until gets yes or no
int continue_input()
{
	char cont[MAX_INPUT];
	int result = 0;
	do {
		printf("continue? (yes/no)\n");
		fgets(cont, MAX_INPUT, stdin);
		cont[strcspn(cont, "\r\n")] = '\0'; // remove '\n' and '\r'
	} while ((result = strcmp(cont, "yes")) && strcmp(cont, "no"));

	return !result;
}

void flip_bit(char buffer[], int n_bit)
{
	char offset = 7 - (n_bit % 8);
	char mask = 1 << offset;
	buffer[n_bit / 8] ^= mask;	// invert bit
}

int add_random_nosie(char buffer[], int msg_size, int n)
{
	int bit, flipped = 0;
	int den = 1 << 16;	// 2^16
	float prob = (float)n / den;
	float threshold = RAND_MAX * prob;
	for (bit = 0; bit < 8 * msg_size; bit++)
	{
		if (rand() < threshold)
		{
			flip_bit(buffer, bit);
			flipped++;
		}
	}
	return flipped;
}

int add_deterministic_noise(char buffer[], int msg_size, int n)
{
	int bit, flipped = 0;
	for (bit = 0; bit < 8 * msg_size; bit += n)
	{
		flip_bit(buffer, bit);
		flipped++;
	}
	return flipped;
}

SOCKET create_listen_socket(char IP_address[], int port, SOCKADDR_IN* client, int *chosen_port)
{
	SOCKET sock;
	int addr_len;

	// Client settings
	client->sin_family = AF_INET;
	client->sin_addr.s_addr = inet_addr(IP_address);
	client->sin_port = htons(port);

	// Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		fprintf(stderr, "Socket creation failed\n");
		exit(1);
	}

	// Bind and listen to client
	if (bind(sock, (SOCKADDR_IN*)client, sizeof(*client)) != 0)
	{
		fprintf(stderr, "bind failed\n");
		exit(1);
	}
	listen(sock, 1);

	// Set selected port to chosen_port
	if (port == 0)	// OS chosen port
	{
		addr_len = sizeof(*client);
		getsockname(sock, (SOCKADDR_IN*)client, &addr_len);
		*chosen_port = ntohs(client->sin_port);
	}
	else			// Constant port
		*chosen_port = port;
	
	return sock;
}

int receive_data(SOCKET listen_sock, char buffer[])
{
	int received_bytes;
	SOCKET sock;

	sock = accept(listen_sock, NULL, NULL);
	received_bytes = recv(sock, buffer, MSG_SIZE, 0);
	if (received_bytes < 0)
	{
		fprintf(stderr, "receive failed\n");
		exit(1);
	}
	closesocket(sock);

	return received_bytes;
}

int send_data(SOCKET listen_sock, char buffer[], int msg_size)
{
	int sent_bytes;
	SOCKET sock;

	sock = accept(listen_sock, NULL, NULL);
	sent_bytes = send(sock, buffer, msg_size, 0);
	if (sent_bytes < 0)
	{
		fprintf(stderr, "Send failed\n");
		exit(1);
	}
	closesocket(sock);

	return sent_bytes;
}

int add_noise(char buffer[], int msg_size, char noise_type[], int n)
{
	int flipped_bits;

	if (!strcmp(noise_type, "-r"))
		flipped_bits = add_random_nosie(buffer, msg_size, n);
	else if (!strcmp(noise_type, "-d"))
		flipped_bits = add_deterministic_noise(buffer, msg_size, n);
	else
		flipped_bits = 0;

	return flipped_bits;

}

int main(int argc, char* argv[])
{
	int sent_bytes, received_bytes, flipped_bits, sender_port, receiver_port;
	char buffer[MSG_SIZE], IP_address[MAX_IP_ADDR];
	SOCKET sender_listen_sock, receiver_listen_sock, sender_sock, receiver_sock;
	SOCKADDR_IN sender, receiver;

	// Check command line argumets
	if (argc != 3 && argc != 4)
	{
		fprintf(stderr, "Incorrect command line aruments\n");
		exit(1);
	}

	// Set random seed
	if (argc == 4)
		srand(atoi(argv[3]));

	// Initialize Winsock and get local IP address
	getIPaddress(IP_address);

	// Create listen sockets
	sender_listen_sock = create_listen_socket(IP_address, SENDER_PORT, &sender, &sender_port);
	printf("sender socket: IP address = %s port = %d\n", IP_address, sender_port);
	receiver_listen_sock = create_listen_socket(IP_address, RECEIVER_PORT, &receiver, &receiver_port);
	printf("receiver socket: IP address = %s port = %d\n", IP_address, receiver_port);

	// Receive data form sender, add noise and send data to receiver
	do {

		received_bytes = receive_data(sender_listen_sock, buffer);

		flipped_bits = add_noise(buffer, received_bytes, argv[1], atoi(argv[2]));
		
		sent_bytes = send_data(receiver_listen_sock, buffer, received_bytes);

		printf("transmitted %d bytes, flipped %d bits\n", sent_bytes, flipped_bits);

	} while (continue_input());

	closesocket(sender_listen_sock);
	closesocket(receiver_listen_sock);
	WSACleanup();

	return 0;

}