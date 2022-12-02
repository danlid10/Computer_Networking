#ifndef RECEIVER_H
#define RECEIVER_H

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
int hamming_decode(char buffer[], int size, char dehammed_buffer[]);
int is2pow(int num);
void set_bit(char buffer[], int n_bit, int val);

#endif