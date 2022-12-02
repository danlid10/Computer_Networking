#ifndef CHANNEL_H
#define CHANNEL_H

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

// Ports settings 
#define DEBUG 0
#if DEBUG	// Fixed values for debugging
#define SENDER_PORT	 6342		
#define RECEIVER_PORT 6343	
#else	// 0 to let OS choosethe  ports
#define SENDER_PORT	 0		
#define RECEIVER_PORT 0
#endif



SOCKET create_listen_socket(char IP_address[], int port, SOCKADDR_IN* client, int* chosen_port);
int continue_input();
void flip_bit(char buffer[], int n_bit);
int add_random_nosie(char buffer[], int msg_size, int n);
int add_deterministic_noise(char buffer[], int msg_size, int n);
void getIPaddress(char IPaddr[]);
int receive_data(SOCKET listen_sock, char buffer[]);
int send_data(SOCKET listen_sock, char buffer[], int msg_size);
int add_noise(char buffer[], int msg_size, char noise_type[], int n);


#endif