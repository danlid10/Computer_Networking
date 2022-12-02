#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") 

#define MAX_FILENAME 100
#define MSG_SIZE 1000000


int get_file_name(char file_name[]);
void flip_bit(char buffer[], int n_bit);
char get_bit(char buffer[], int n_bit);
int hamming_decode(char buffer[], int size, char uh_buffer[]);
int is2pow(int num);
void set_bit(char buffer[], int n_bit, int val);


// get file name from the user, return 0 if user entered "quit"
int get_file_name(char file_name[])
{
	printf("enter file name:\n");
	fgets(file_name, MAX_FILENAME, stdin);
	file_name[strcspn(file_name, "\r\n")] = '\0'; // remove '\n' and '\r'
	return strcmp(file_name, "quit");
}

void flip_bit(char buffer[], int n_bit)
{
	char offset = 7 - (n_bit % 8);
	char mask = 1 << offset;
	buffer[n_bit / 8] ^= mask;	// invert bit
}

char get_bit(char buffer[], int n_bit)
{
	char offset = 7 - (n_bit % 8);
	return (buffer[n_bit / 8] >> offset) & 1;
}

void set_bit(char buffer[], int n_bit, int val)
{
	char offset = 7 - (n_bit % 8);
	char mask = 1 << offset;
	if (val)
		buffer[n_bit / 8] |= mask;	// set 1
	else
		buffer[n_bit / 8] &= ~mask;	// set 0
}

// return 1 if num is a power of 2, else return 0
int is2pow(int num)
{
	if (num <= 0)
		return 0;
	return !(num & (num - 1));
}

int hamming_decode(char buffer[], int size, char uh_buffer[])
{
	int block, offset, uh_offset, check_idx, wrong_bit, bits_corrected = 0;
	int Check[5] = { 0 };

	for (block = 0; block < (8 * size) / 31; block++)
	{
		// Initialize parities to 0
		for (check_idx = 0; check_idx < 5; check_idx++)
			Check[check_idx] = 0;

		// Calculate parities
		for (offset = 0; offset < 31; offset++)
			for (check_idx = 0; check_idx < 5; check_idx++)
				if ((offset + 1) & (1 << check_idx))
					Check[check_idx] ^= get_bit(buffer, offset + 31 * block);

		// Error detection 
		wrong_bit = 0;
		for (check_idx = 0; check_idx < 5; check_idx++)
			wrong_bit += Check[check_idx] << check_idx;

		// Error correction
		if (wrong_bit)
		{
			flip_bit(buffer, wrong_bit - 1 + 31 * block);
			bits_corrected++;
		}
	
		// Set data bits to the new buffer
		for (uh_offset = 0, offset = 0; offset < 31; offset++)
			if (!is2pow(offset + 1))
			{
				set_bit(uh_buffer, uh_offset + 26 * block, get_bit(buffer, offset + 31 * block));
				uh_offset++;
			}
			
	}

	return bits_corrected;
}

int main(int argc, char* argv[])
{
	int status, received_bytes, size, bits_corrected;
	char file_name[MAX_FILENAME], buffer[MSG_SIZE];
	char* uh_buffer;
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN server;
	FILE* fp;

	// Check command line argumets
	if (argc != 3)
	{
		fprintf(stderr, "Incorrect command line aruments\n");
		exit(1);
	}

	// Initialize Winsock
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
	{
		fprintf(stderr, "WSAStartup failed: %d\n", status);
		exit(1);
	}

	// Server settings
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_port = htons(atoi(argv[2]));

	// Receive data and write to file
	while (get_file_name(file_name))
	{

		// Create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			fprintf(stderr, "Socket creation failed\n");
			exit(1);
		}

		// Connect to the server
		status = connect(sock, (SOCKADDR_IN*)&server, sizeof(server));
		if (status != 0)
		{
			fprintf(stderr, "Server connection failed\n");
			exit(1);
		}

		// Receive data form the server
		received_bytes = recv(sock, buffer, MSG_SIZE, 0);
		if (received_bytes < 0)
		{
			fprintf(stderr, "receive failed\n");
			exit(1);
		}
		printf("received: %d bytes\n", received_bytes);

		// Hamming decoding
		size = received_bytes * (26.0 / 31.0);
		uh_buffer = (char*)malloc(sizeof(char) * size);
		if (uh_buffer == NULL)
		{
			fprintf(stderr, "Memory alloaction failed\n");
			exit(1);
		}
		bits_corrected = hamming_decode(buffer, received_bytes, uh_buffer);

		// Write data to file
		fp = fopen(file_name, "wb");
		if (fp == NULL)
		{
			fprintf(stderr, "File opening failed\n");
			exit(1);
		}
		fwrite(uh_buffer, sizeof(char), size, fp);
		fclose(fp);

		printf("wrote: %d bytes\n", size);
		printf("corrected %d errors\n", bits_corrected);

		free(uh_buffer);

		closesocket(sock);

	}
	WSACleanup();

	return 0;

}
