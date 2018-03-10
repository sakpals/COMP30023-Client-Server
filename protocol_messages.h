#ifndef PROTOCOL_MESSAGES_H_
#define PROTOCOL_MESSAGES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include "uint256.h"
#include "sha256.h"


#define HEADER_LEN 4
#define ERRO_REASON_LEN 40
#define MAX_CLIENTS 100

typedef struct work{
	uint32_t difficulty;
	BYTE *seed;
	uint64_t start;
	uint8_t worker_count;
	BYTE *target;
	int client_fd;
	struct work *next;
	char *str_difficulty;
	char *str_seed;
	char *str_start;
} work_msg_t;

char *create_response_msg(char *curr_msg, char *add, int *size); 
char *read_message(char *msg, work_msg_t **worker_queue, int i);
int process_solution(char *msg);
BYTE *convert_to_binary(uint32_t num);
BYTE *map_difficulty(uint32_t difficulty);
uint32_t two_power_of(int num); // might not need this either!!!
BYTE *convert_to_hex(BYTE *binary_num); // probs don't need this at all!!!!
BYTE *convert_to_hex2(uint32_t num);
char *convert_to_hex3(uint64_t num);
BYTE *get_seed(char *token);
BYTE *concatenate(BYTE *seed, uint64_t solution);
BYTE *get_solution(uint64_t solution);
void remove_work(work_msg_t **worker_queue, int client_fd);
//uint32_t ntohl(uint32_t netlong);


void parse_work(work_msg_t **worker_queue, char *msg, int i);
int process_work(work_msg_t *work);

#endif