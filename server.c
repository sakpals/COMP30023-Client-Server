/* A simple server in the internet domain using TCP
The port number is passed as an argument 

An Extended version of the server program provided, using select() 
to handle concurrent clients
https://www.gnu.org/software/libc/manual/html_node/Server-Example.html
 To compile: gcc server.c -o server 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "protocol_messages.h"
void *increment_start(void *num);


int main(int argc, char **argv)
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[256];
	char *msg;
	int msg_size = 100;

	time_t t;

	

	int select_return = 0;
	struct timeval select_timer;

	select_timer.tv_sec = 0;
	select_timer.tv_usec = 2000;

	// make a worker queue (LL style)
	work_msg_t **worker_queue = malloc(sizeof(work_msg_t));
	*worker_queue = NULL;

	pthread_t thread1;
	int more;
	int count = 0;
	char read_buffer[1];
	char more_buffer[2];
	int size = 100;

	fd_set read_fds;
	fd_set all_fds;
	int num_fds;
	int yes = 1;
	
	FILE *fp = fopen("log.txt", "w");
	

	FD_ZERO(&read_fds);
	FD_ZERO(&all_fds);


	struct sockaddr_in serv_addr, cli_addr;
	int n;
	int i;

	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	 /* Create TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}

	
	num_fds = sockfd;
	FD_SET(sockfd, &all_fds);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	/* erases the data in the n bytes (second argument) of the memory
	starting at the location pointed to by s (first argument) 
	bzero(void *s, size_t n) */
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	
	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}
	
	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */
	
	/* 5 = defines the maximum length to which the queue of
       pending connections for sockfd may grow.*/
	listen(sockfd,5);
	

	clilen = sizeof(cli_addr);
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */

	/* accept() used with connection-based socket types, extracts first
	connection request from queue of pening connections for the listening
	socket, sockfd, creates a new connected socket and returns a file descriptor 
	referring to that socket. NOTE: not in the listening state */

	while(1) {
		FD_ZERO(&read_fds);
		
		read_fds = all_fds;

		// iterate through worker queue here
		
		work_msg_t *tmp = *worker_queue;
		if(tmp != NULL) {
			if(pthread_create(&thread1, NULL, increment_start, (void *)&(tmp->start)) != 0) {
				printf("pthread error!");
				//exit(1);
			}
			if (pthread_join(thread1, NULL) != 0) {
				printf("pthread join error!");
				//exit(1);
			}
	
			if(process_work(tmp) == 1) {

				msg = create_response_msg(msg, "", &msg_size);
				msg = create_response_msg(msg, "SOLN ", &msg_size);
				msg = create_response_msg(msg, tmp->str_difficulty, &msg_size);
				msg = create_response_msg(msg, " ", &msg_size);
				msg = create_response_msg(msg, tmp->str_seed, &msg_size);
				msg = create_response_msg(msg, " ", &msg_size);
				msg = create_response_msg(msg, tmp->str_start, &msg_size);
				//printf("final_nonce = %s\n", tmp->str_start);
				msg = create_response_msg(msg, "\n", &msg_size);

				// printing to LOG FILE
				t = time(NULL); // get the time
				fprintf(fp, "%s",ctime(&t));
				fprintf(fp, "(%d) 0.0.0.0: --  %s\n\n", tmp->client_fd, msg);
				fflush(fp);

				n  = write(tmp->client_fd, msg, strlen(msg));
				if (n < 0) {
					perror("ERROR writing to socket");
					//exit(1);
				}
				// remove the node from the queue
				//worker_queue = tmp->next;
				//free(tmp);
				
				*worker_queue = tmp->next;

				tmp =  *worker_queue;
			}
			
		}
		select_return = select(num_fds+1, &read_fds, NULL, NULL, &select_timer);
		if(select_return < 0) {
			perror("ERROR on select");
			//exit(1);
		}
		else if(select_return > 0) {

			for(i=0; i <= num_fds; i++) {
				if(FD_ISSET(i, &read_fds)) {
			
					/* check if another client requests for connection  */
					if(i==sockfd) {
						newsockfd = accept(	sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
					
						if (newsockfd < 0) {
							perror("ERROR on accept");
							//exit(1);
						}
						FD_SET(newsockfd, &all_fds);
		
						if(newsockfd > num_fds) {
							num_fds = newsockfd;
						}
					}
					else {
						more = 1;
						/* service an already-connected client socket that is ready
						for reading */
						// do the reading/writing
						bzero(buffer,256);
						bzero(read_buffer, 1);
						bzero(more_buffer, 2);

						char *response = (char *)malloc(sizeof(char)*size);
						response = create_response_msg(response, "", &size);

						while(more) {

							while(1) {
								n = read(i, read_buffer, 1);
								if(n<0) {
									perror("ERROR reading from socket");
								}
								if(n == 1) {
									if(read_buffer[0] == '\r') {
										buffer[count] = '\0';
										count ++;
										break;
									}
									buffer[count] = read_buffer[0];
									count++;
								}
								else {
									break;
								}
							}
							count = 0;
							// when the client disconnects
							if(n==0) {
								FD_CLR(i, &all_fds);
								more = 0;
								break;
							}

							// record in LOG FILE what is requested by client
							t = time(NULL); // get the time
							fprintf(fp, "%s",ctime(&t));
							fprintf(fp, "(%d) %" PRIu32": --  ", i, cli_addr.sin_addr.s_addr);
							fprintf(fp, "%s\r\n\n\n",buffer);
							

							msg = read_message(buffer, worker_queue, i);

							
							fflush(fp);
							response = create_response_msg(response, msg, &size);
							bzero(buffer, 256);

							n = read(i, more_buffer, 2);
							if(n == 2) {
								more = 1;
								buffer[count] = more_buffer[1];
								count++;
								bzero(more_buffer,2);

							}
							else if(n == 1) {
								// record in LOG FILE what is outputted by 0.0.0.0
								if(strcmp(response, "") != 0) {
									t = time(NULL); // get the time
									fprintf(fp, "%s",ctime(&t));
									fprintf(fp, "(%d) 0.0.0.0: --  %s\n\n", i, response);
									fflush(fp);
								}
								
								n = write(i, response, strlen(response));
								if(n<0) {
									perror("ERROR writing to socket");
									//exit(1);
								}
								break;
							}
						}
						
					}
				}
			}
		}
		else {
			continue;
		}
		
	}
	// free the worker queue here if its not empty
	fclose(fp);
	/* close socket */
	close(sockfd);
	
	return 0; 
}

void *increment_start(void *num) {
	uint64_t *start = (uint64_t *)num;
//	printf("hello from the work function!!\n");
//	printf("start = %llu\n", *start);
	//printf("start + 1 = %llu\n", *start +1 );
	*start = *start + 1;
	return NULL;
}
