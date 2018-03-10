#include "protocol_messages.h"


char *read_message(char *msg, work_msg_t **worker_queue, int i){

	BYTE header[HEADER_LEN + 1];
	int size  = 100;
	unsigned long payload_length;

	
	char *return_msg = (char *)malloc(sizeof(char)*size);
	return_msg = create_response_msg(return_msg, "", &size);
	
	char *token = strtok(msg, "\r");
	while(token!= NULL) {

		header[4] = '\0';
		strncpy((char *)&header, msg,4);
		
		if(strcmp((const char *)&header, "PING") == 0) {
			payload_length = 0;	
			if((strlen(msg)-(HEADER_LEN)) != payload_length) {
				return_msg = create_response_msg(return_msg, "ERRO bad message!\r\n", &size);		
			} 
			else {
				return_msg = create_response_msg(return_msg, "PONG\r\n", &size);
			}	
		}
		else if(strcmp((const char *)&header, "PONG") == 0) {
			payload_length = 0;
			if((strlen(msg)-(HEADER_LEN)) != payload_length) {
				return_msg=create_response_msg(return_msg, "ERRO bad message!\r\n", &size);
				
			} 
			else {
				return_msg=create_response_msg(return_msg, "ERRO PONG message is strictly reserved for server responses!\r\n", &size);
			}
		}
		else if(strcmp((const char *)&header, "OKAY") == 0) {
			payload_length = 0;
			if((strlen(msg)-(HEADER_LEN)) != payload_length) {
				return_msg = create_response_msg(return_msg, "ERRO bad message!""ERRO bad message!\r\n", &size);
			} 
			else {
				return_msg = create_response_msg(return_msg, "ERRO it's not okay to send OKAY messages to server!\r\n", &size);
			}
		}
		else if(strcmp((const char *)&header, "ERRO") == 0) {
			payload_length = 40;
			// +1 for the space after ERRO
			if((strlen(msg)-(HEADER_LEN+1)) != payload_length) {
				return_msg =create_response_msg(return_msg, "ERRO bad message!\r\n", &size);
			} 
			else {
				return_msg = create_response_msg(return_msg, "ERRO you should not send 'ERRO' messages to server!\r\n", &size);
			}
		}
		else if(strcmp((const char *)&header, "SOLN") == 0) {
			payload_length = 90;
			// +1 for the space after SOLN 
			if((strlen(msg)-(HEADER_LEN+1)) != payload_length) {
				return_msg =create_response_msg(return_msg, "ERRO bad message!\r\n", &size);
				
			} 
			// return_msg should be OKAY if its a valid proof of work
			else if(process_solution(msg) == 1) {
				return_msg = create_response_msg(return_msg, "OKAY\r\n", &size);
			}
			//otherwise reply with ERRO  with a 40-byte string describing error
			else {
				return_msg = create_response_msg(return_msg, "ERRO not a valid proof of work!              \r\n", &size);
			}
		}
		else if(strcmp((const char *)&header, "WORK") == 0) {
			payload_length = 93;
			// + 1 for the space after WORK
			if((strlen(msg) -(HEADER_LEN+1)) != payload_length) {
				return_msg =create_response_msg(return_msg, "ERRO bad message!\r\n", &size);
			}
			else {
				parse_work(worker_queue, msg, i);

				/* ******* */
				int len = 0;
				work_msg_t *tmp = *worker_queue;
				while(tmp != NULL) {
					len ++;
					tmp = tmp->next;
				}
				printf("worker_queue size = %d\n", len);
				/* ******* */
			}	
			//NOTE: actual work() message is being handled in the server program
		}
		else if(strcmp((const char *)&header, "ABRT") == 0) {
			payload_length = 0;
			if((strlen(msg) - HEADER_LEN) != payload_length) {
				return_msg = create_response_msg(return_msg, "ERRO bad message!\r\n", &size);	
			}
			else {
				remove_work(worker_queue, i);

				/* ******* */
				int len = 0;
				work_msg_t *tmp = *worker_queue;
				while(tmp != NULL) {
					len ++;
					tmp = tmp->next;
				}
				printf("worker_queue size = %d\n", len);
				/* ******* */

				
				return_msg = create_response_msg(return_msg, "OKAY\r\n", &size);	
			}
		}
		else {
			return_msg =create_response_msg(return_msg, "ERRO server doesn't accept this message      \r\n", &size);
		}	
		token = strtok(NULL, "\r");

	}
	return return_msg;
}
// try to FREE THINGS!!
void remove_work(work_msg_t **worker_queue, int client_fd) {
	work_msg_t *tmp = *worker_queue;
	while(tmp != NULL) {
		// if its in the front of the queue
		if(*worker_queue == tmp && tmp->client_fd == client_fd) {
			*worker_queue = tmp->next; // free(tmp) ?
		}
		// if its the last or in the middle of the queue
		else if(tmp->next != NULL) {
			if(tmp->next->client_fd == client_fd) {
				// remove all pending work for this client 
				tmp->next = tmp->next->next; // free(tmp->next) ?
			}
		}
		tmp = tmp->next;
	}

}
// parses work msg and adds to queue
void parse_work(work_msg_t **worker_queue, char *msg, int client_fd) {
	BYTE *target;
	int i = 0;
	char *token = strtok(msg, " ");
	uint32_t difficulty;
	BYTE *seed;
	uint64_t start;
	uint8_t worker_count;

	char *str_difficulty = (char *)malloc(sizeof(char)*8);
	char *str_seed = (char *)malloc(sizeof(char)*64);

	while(token != NULL) {
		if(i == 1) {
			strcpy(str_difficulty, token);
			difficulty = strtoul(token, NULL, 16);
		}
		else if(i==2) {
			strcpy(str_seed, token);
			seed = get_seed(token);
		}
		else if(i==3) {
			start = strtoull(token, NULL, 16);
			
		}
		else if(i==4) {
			worker_count = (uint8_t)strtol(token, NULL, 16);
		}
		token = strtok(NULL, " ");
		i++;
	}
	target = map_difficulty(difficulty);

	work_msg_t *new_work = (work_msg_t *)malloc(sizeof(work_msg_t));
	new_work->difficulty = difficulty;
	new_work->seed = seed;
	new_work->start = start;
	new_work->worker_count = worker_count;
	new_work->target = target;
	new_work->client_fd = client_fd;
	new_work->str_difficulty = str_difficulty;
	new_work->str_seed = str_seed;
	new_work->next = NULL;


	// add this new work message to the end of the worker_queue
	work_msg_t *tmp = *worker_queue;
	if(tmp == NULL) {
		*worker_queue = new_work;
	}
	else {
		while( tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = new_work;
	}
}


int process_work(work_msg_t *work) {
	SHA256_CTX ctx;
	BYTE *x;
	char *str_start =  (char *)malloc(sizeof(char)*16);
	BYTE buf[SHA256_BLOCK_SIZE];
    BYTE second_buf[SHA256_BLOCK_SIZE];
    x = concatenate(work->seed, work->start);

    str_start = convert_to_hex3(work->start);

	work->str_start = str_start;
	//printf("in process Work: work->str_start ----> %s\n", work->str_start);
    sha256_init(&ctx);
    sha256_update(&ctx, x, 40);
    sha256_final(&ctx, buf);

    sha256_init(&ctx);
    sha256_update(&ctx, buf, 32);
    sha256_final(&ctx, second_buf);

   
    //free(str_start);
    return sha256_compare(work->target, second_buf);

}

int process_solution(char *msg) {
	char *token = strtok(msg, " ");
	int i=0,j=0;
	uint32_t difficulty;// = (uint32_t)strtol(hex, NULL, 16);
	BYTE *seed;
	uint64_t solution;
	BYTE *target;
    BYTE *x = (BYTE *)malloc(sizeof(BYTE)*40);
    SHA256_CTX ctx;
    //SHA256_CTX ctx2;
    BYTE buf[SHA256_BLOCK_SIZE];
    BYTE second_buf[SHA256_BLOCK_SIZE];

	// get the difficulty, seed and solution from the SOLN string
	while(token!= NULL) {
		if(i == 1) {
			difficulty = strtoul(token, NULL, 16);
			//printf("difficulty  = %u\n", difficulty);
		}
		else if(i==2) {
			seed = get_seed(token);

			//print_uint256(seed);
		}
		else if(i ==3){
           // printf("token = %s\n", token);
			solution = strtoull(token, NULL, 16);
			//printf("solution =  %" PRIu64 "\n", solution);
		}
		//printf("token = %s\n", token);
		token = strtok(NULL, " ");
		i++;
	}
    //printf("target = ");
	target = map_difficulty(difficulty);
   // print_uint256(target);
    //printf("\n");

    BYTE *hex_sol = get_solution(solution);
    for(i=0; i < 40; i++) {
        x[i] = 0;
    }

    for(i=0; i < 32; i++) {
        x[i] = seed[i];
    }
    for(i=32; i < 40; i++){
        x[i] = hex_sol[j];
        j++;
    }
  
    //x = concatenate(seed, solution);
   // printf("concat = ");
    //print_uint256(x);
   // printf("\n");

    sha256_init(&ctx);
    sha256_update(&ctx, x, 40);
    sha256_final(&ctx, buf);

    sha256_init(&ctx);
    sha256_update(&ctx, buf, 32);
    sha256_final(&ctx, second_buf);
/*
    printf("printing buffer for hash(x) \n");
    for(i = 0; i < SHA256_BLOCK_SIZE; i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");

    printf("printing buffer for hash(hash(x)) \n");
    for(i = 0; i < SHA256_BLOCK_SIZE; i++) {
        printf("%02x", second_buf[i]);
    }
    printf("\n");

	*/
	//printf("sha256_compare(target, second_buf) = %d\n", sha256_compare(target, second_buf));
	return sha256_compare(target, second_buf);
   // printf("sha256_compare = %d\n", sha256_compare(target, second_buf));
// need to free all the malloc'd variables!!!!

}

BYTE *concatenate(BYTE *seed, uint64_t solution) {
    int i,j=0;
    BYTE *hex_sol = get_solution(solution);
    BYTE *con = (BYTE *)malloc(sizeof(BYTE)*40);

    //initialise con
    for(i=0; i < 40; i++) {
        con[i] = 0;
    }

    for(i=0; i < 32; i++) {
        con[i] = seed[i];
    }
    for(i=32; i < 40; i++){
        con[i] = hex_sol[j];
        j++;
    }
    
   /* printf("printing con...\n");
    for(i = 0; i < 40; i++) {
        printf("%02x", con[i]);
    }
    printf("\n");*/
    return con;
}

BYTE *get_solution(uint64_t solution) {
   	//uint64_t solution = ntohl(get_solution);
  //  printf("solution = %llu\n", solution);
    char remainders[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
    int i = 7;
    BYTE rem_index;
    uint64_t quotient = solution;
    //printf("quotient = %llu\n",quotient);
    int num1;
    int num2;
    int num3;
    BYTE *hex_num = (BYTE *)malloc(sizeof(BYTE)*8);

  //  printf("in get_solution ===\n");
    while(quotient != 0 && i >= 0) {
        rem_index = quotient%16;
        num1 = remainders[rem_index];
        quotient /= 16;
         //printf("rem_index = %d quotient = %llu\n",rem_index, quotient);
        if(quotient != 0 && i >= 0) {
            rem_index = quotient%16;
            num2 = remainders[rem_index];
            quotient /= 16;
            //printf("num2 << 4 = %2d\n", num2);
            num3 = (num2<<4)|num1;
            //printf("%d", num3);
           // printf("i = %d, num1 = %x, num2 = %x, num3 = %x\n", i,num1, num2, num3);
            hex_num[i] = num3;

        }
        else {
            i--;
            break;
        }
        i--;
    }
    while(i>=0) {
        hex_num[i] = 0x00;
        i--;
    }
    /*
    printf("printing hex_sol for %" PRIu64 "\n", solution);
    for(i=0; i<8; i++) {
        printf("%02x", hex_num[i]);
    }
    printf("\n");*/
    return hex_num;
}

BYTE *get_seed(char *token) {
	int i, k = 31;
	int num1, num2, num3;
	char s[2];
	BYTE *seed = (BYTE *)malloc(sizeof(BYTE)*32);
	char remainders[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	char raw_seed[64+1];
	//raw_seed[64] = '\0';
	for(i = 64; i >= 1; i-= 2) {
		strncpy(raw_seed, token, i);
		//raw_seed[i] = '\0';
		s[1] = '\0';
		s[0] = raw_seed[i-1];
		//s = raw_seed[i-1];
		//printf("s = %s  ", s);

		num1 = remainders[strtol(s, NULL, 16)];
		//printf("strtol(&s, NULL, 16) = %ld\n", strtol(s, NULL, 16));

		//raw_seed[i-1] = '\0';
		s[1] = '\0';
		s[0] = raw_seed[i-2];
		//s = raw_seed[i-2];
		//printf("s = %s  ", s);
		num2 = remainders[strtol(s, NULL, 16)];


		num3 = (num2<<4)|num1;
	
		//printf("strtol(&s, NULL, 16) = %ld\n", strtol(s, NULL, 16));
		seed[k] = num3;
		
		k--;
		num1 = 0;
		num2 = 0;
		num3 = 0;

	}
	/*
	  for(i=0; i < 40; i++) {
        printf("seed[%d] = %d\n", i, seed[i]);
    }*/
	return seed;
}


// change return to BYTE *
BYTE *map_difficulty(uint32_t get_difficulty) {
    uint32_t difficulty = ntohl(get_difficulty);
	//uint32_t difficulty = get_difficulty;
	int i,j;
	BYTE *target = (BYTE *)malloc(sizeof(BYTE)*32);
	BYTE exp[32];
	BYTE two[32];

	uint256_init(target);
	uint256_init(exp);
	uint256_init(two);
	two[31] = 0x2;

	BYTE *binary_num = convert_to_binary(difficulty);

	BYTE *hex_beta;

	uint32_t alpha = 0;
	uint32_t beta = 0;
    j=7;
    // extract alpha assuming ordering is 01234... from array?? CHECK!
    for(i = 24; i <= 31; i ++) {

        if(binary_num[i] == 1) {
            alpha += two_power_of(j);
        
        }
        j--;
    }

    j= 23;
    // extract beta 
    for(i = 0; i <= 23; i++) {
        if(binary_num[i] == 1) {
            beta += two_power_of(j);
        }
        j--;
    }   
	// let alpha be the exponent 
	alpha = 8*(alpha-3);
	/*for(i=0; i<32; i++) {
		printf("%d", binary_num[i]);
	}
	printf("\n");
*/

	//printf("alpha = %d\n", alpha);
	//printf("beta = %d\n", beta);

	//printf("2^8*(alpha-3) is...\n");
	uint256_exp(exp, two, alpha);
	//print_uint256(exp);
	
	hex_beta = convert_to_hex2(beta);
	//printf("printing target ....\n");
	uint256_mul(target, hex_beta, exp);
	//print_uint256(target);
	return target;

}
uint32_t two_power_of(int num) {
	int i;
	uint32_t val  = 1;
	for(i = 1; i <= num; i++) {
		val *= 2;
	}
	return val;
}
char *convert_to_hex3(uint64_t num) {
	char numbers[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	char remainders[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	int i = 31, j = 15;
	BYTE rem_index;
	uint64_t quotient = num;
	int num1;
	int num2;
	int num3;
	char *str_start = (char *)malloc(sizeof(char)*16);

	BYTE *hex_num = (BYTE *)malloc(sizeof(BYTE)*32);
	while(quotient != 0 && i >= 0) {
		//printf("quotient = %llu\n", quotient);
		rem_index = quotient%16;

		num1 = remainders[rem_index];
		quotient /= 16;
		if(quotient != 0 && i >= 0) {
			rem_index = quotient%16;
			num2 = remainders[rem_index];
			quotient /= 16;

			num3 = (num2<<4)|num1;
			hex_num[i] = num3;
			if(j >= 0) {
				str_start[j] = numbers[num1];
				//printf("str_start[%d] = %c\n",j, str_start[j]);
				j --;
				str_start[j] = numbers[num2];
				//printf("str_start[%d] = %c\n",j, str_start[j]);
				j --;
			}

		}
		else {
			i--;
			break;
		}
		i--;
	}
	//printf("quotient = %llu\n", quotient);
	while(i>=0) {
		hex_num[i] = 0x00;
		if(j >= 0) {
			str_start[j] = numbers[0];
			//printf("str_start[%d] = %c\n",j, str_start[j]);
			j--;
		}
		i--;
	}
	
	//printf("printing hex for %" PRIu64 "\n", num);
	/*for(i=0; i<16; i++) {
		printf("%c",str_start[i]);
	}
	printf("\n");*/
	//printf("str_start = %s\n", str_start);
	return str_start;

}

BYTE *convert_to_hex2(uint32_t num) {
	char remainders[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	int i = 31;
	BYTE rem_index;
	uint32_t quotient = num;
	int num1;
	int num2;
	int num3;
	BYTE *hex_num = (BYTE *)malloc(sizeof(BYTE)*32);
	while(quotient != 0 && i >= 0) {
		rem_index = quotient%16;
		num1 = remainders[rem_index];
		quotient /= 16;
		if(quotient != 0 && i >= 0) {
			rem_index = quotient%16;
			num2 = remainders[rem_index];
			quotient /= 16;

			num3 = (num2<<4)|num1;
			hex_num[i] = num3;

		}
		else {
			i--;
			break;
		}
		i--;
	}
	while(i>=0) {
		hex_num[i] = 0x00;
		i--;
	}
	/*
	printf("printing hex for %d\n", num);
	for(i=0; i<32; i++) {
		printf("%d", hex_num[i]);
	}
	printf("\n");*/
	return hex_num;

}

BYTE *convert_to_hex(BYTE *binary_num) {
	int i=31, j, k =31;
	
	char remainders[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	int num1;
	int num2;
	int num3;

	BYTE *hex = (BYTE *)malloc(sizeof(BYTE)*32);
	int rem_index= 0;

	while(i >= 3) {
		for(j=i; j > i-4; j--) {
			if(binary_num[j] == 1) {
				rem_index += two_power_of(i-j);
			}

		}
		num1 = remainders[rem_index];
		i -=4;
		rem_index = 0;
		if(i >= 3) {
			for(j=i; j > i-4; j--) {
				if(binary_num[j] == 1) {
					rem_index += two_power_of(i-j);
				}

			}
			num2 = remainders[rem_index];
		}
		else {
			break;
		}
		num3 = (num2<<4)|num1;
		hex[k] = num3;
		k--;
		rem_index = 0;
		i-= 4;
	}
	

	return hex;
}


BYTE *convert_to_binary(uint32_t num) {
	int i = 31;
	BYTE remainder;
	uint32_t quotient = num;
	BYTE *binary_num = (BYTE *)malloc(sizeof(BYTE)*32);
	while(quotient != 0 && i >= 0) {
		remainder = quotient%2;
		quotient /= 2;
		
		binary_num[i] = remainder;
		
		i--;
	}
	while(i>=0) {
		binary_num[i] = 0;
		i--;
	}
    /*
	printf("printing binary for %d\n", num);
	for(i=0; i<32; i++) {
		printf("%d", binary_num[i]);
	}
	printf("\n");*/
	return binary_num;

}

char *create_response_msg(char *curr_msg, char *add, int *size) {
	if((int)(strlen(add)+strlen(curr_msg)) > *size) {
		*size = 2*(strlen(add)+strlen(curr_msg));
		curr_msg = realloc(curr_msg, *size);
		strcat(curr_msg,add);

	}
	else {
		strcat(curr_msg,add);
	}
	return curr_msg;
}

