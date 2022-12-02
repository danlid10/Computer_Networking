#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") 

#define MAX_FILENAME 100

int get_file_name(char file_name[]);
char get_bit(char buffer[], int n_bit);
void set_bit(char buffer[], int n_bit, int val);
int is2pow(int num);
int get_file_size(FILE* fp);
void hamming_encode(char buffer[], int size, char h_buffer[]);


// get file name from the user, return 0 if user entered "quit"
int get_file_name(char file_name[])
{
	printf("enter file name:\n");
	fgets(file_name, MAX_FILENAME, stdin);
	file_name[strcspn(file_name, "\r\n")] = '\0'; // remove '\n' and '\r'
	return strcmp(file_name, "quit");
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
		buffer[n_bit / 8] |= mask ;	// set 1
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

int get_file_size(FILE* fp)
{
	int file_size;

	fseek(fp, 0, SEEK_END);		// get fp to end of file
	file_size = ftell(fp);		// get size of file
	rewind(fp, 0, SEEK_SET);	// get fp back to beginning of file

	return file_size;
}

void hamming_encode(char buffer[], int size, char h_buffer[])
{
	int block, offset, check_idx, h_offset;
	int Check[5] = { 0 };

	for (block = 0; block < (8 * size) / 26; block++)
	{
		// Initialize parities to 0
		for (check_idx = 0; check_idx < 5; check_idx++)
			Check[check_idx] = 0;

		// Set data bits to the new buffer
		for (h_offset = 0, offset = 0; h_offset < 31; h_offset++)
			if (!is2pow(h_offset + 1))
			{
				set_bit(h_buffer, h_offset + 31 * block, get_bit(buffer, offset + 26 * block));
				offset++;
			}
		
		// Calculate parities for the check bits
		for (h_offset = 0; h_offset < 31; h_offset++)
			for (check_idx = 0; check_idx < 5; check_idx++)
				if (((h_offset + 1) & (1 << check_idx)) && !is2pow(h_offset + 1))
					Check[check_idx] ^= get_bit(h_buffer, h_offset + 31 * block);

		// Set check bits to the new buffer
		for (h_offset = 0, check_idx = 0; h_offset < 31; h_offset++)
			if (is2pow(h_offset + 1))
			{
				set_bit(h_buffer, h_offset + 31 * block, Check[check_idx]);
				check_idx++;
			}
	}
}


int main(int argc, char* argv[])
{
	int status, file_size, sent_bytes, h_size;
	char file_name[MAX_FILENAME];
	char* buffer, * h_buffer;
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

	// Read file and send data
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

		// Read Data from file
		fp = fopen(file_name, "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "File opening failed\n");
			exit(1);
		}
		file_size = get_file_size(fp);		
		printf("file length: %d bytes\n", file_size);
		buffer = (char*)malloc(sizeof(char) * file_size);
		if (buffer == NULL)
		{
			fprintf(stderr, "Memory alloaction failed\n");
			exit(1);
		}
		fread(buffer, sizeof(char), file_size, fp);
		fclose(fp);

		// Hamming encoding
		h_size = file_size * (31.0 / 26.0);
		h_buffer = (char*)malloc(sizeof(char) * h_size);
		if (h_buffer == NULL)
		{
			fprintf(stderr, "Memory alloaction failed\n");
			exit(1);
		}
		hamming_encode(buffer, file_size, h_buffer);
		free(buffer);

		// Send data to the server
		sent_bytes = send(sock, h_buffer, h_size, 0);
		if (sent_bytes < 0)
		{
			fprintf(stderr, "Send failed\n");
			exit(1);
		}
		printf("sent: %d bytes\n", sent_bytes);

		free(h_buffer);
		closesocket(sock);

	}

	WSACleanup();

	return 0;

}