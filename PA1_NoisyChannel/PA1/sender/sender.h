#ifndef SENDER_H
#define SENDER_H

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
void hamming_encode(char buffer[], int size, char hammed_buffer[]);


#endif