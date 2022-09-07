#include "xmodemserver.h"

#ifndef PORT 
    #define PORT 55496
#endif

int main() {

	char temp;

	// set up server socket
	int server_soc = socket(AF_INET, SOCK_STREAM, 0);
	if (server_soc == -1) {
		perror("server: server socket");
		exit(1);
	}

	int yes = 1;
    if((setsockopt(server_soc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
        exit(1);
    }

	// bind the socket to an address, and make it listen
	struct sockaddr_in server_a;
	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	server_a.sin_addr.s_addr = INADDR_ANY;
	memset(&(server_a.sin_zero), 0, 8);

	if (bind(server_soc, (struct sockaddr *)(&server_a), sizeof(server_a)) == -1) {
		perror("server: bind");
		exit(1);
	}

	if (listen(server_soc, 5) < 0) {
		perror("listen");
		exit(1);
	}

	// set up fd_sets for select
	fd_set curr_sockets, temp_sockets;
	int largest_fd = server_soc;

	FD_ZERO(&curr_sockets);
	FD_SET(server_soc, &curr_sockets);

	// handling multiple clients with select
	struct client *last_client = NULL;
	struct client *head_client = NULL;
	while (1) {
		temp_sockets = curr_sockets;
		if (select(largest_fd + 1, &temp_sockets, NULL, NULL, NULL) < 0) { // select is destructive
			perror("server: select");
			exit(1);
		}
		int i = largest_fd;

		for (; i > -1; i--) {
			if (FD_ISSET(i, &temp_sockets)) {
				if (i == server_soc) { // if the server_soc has detected a new connection
					printf("serversoc has detected a new connection\n");
					struct sockaddr_in client_a;
					client_a.sin_family = AF_INET;
					socklen_t s_ca = sizeof(client_a);
					int client_soc = accept(server_soc, (struct sockaddr *)&client_a, &s_ca);

					// set up new struct client, and update last_client head_client
					struct client new;
					new.fd = client_soc;
					new.inbuf = 0;
					new.state = initial;
					new.next = NULL;
					if (last_client == NULL) {
						last_client = &new;
						head_client = &new;
					} else {
						last_client -> next = &new;
						last_client = &new;
					}
					FD_SET(new.fd, &curr_sockets);
					if (new.fd > largest_fd) {
						largest_fd = new.fd;
					}

				} else { // if one of the client sockets has sent something
					// find the struct client with the fd i
					struct client *cur = head_client;
					struct client *a = NULL;
					while(cur != NULL) {
						if (cur -> fd == i) {
							a = cur;
							break;
						} else {
							cur = cur -> next;
						}
					}
					if (a == NULL) {
						fscanf(stderr, "server: could not find client socket");
						exit(1);
					}

					// read from client socket into buf. 
					int r = read_from_socket(a -> fd, a -> buf, &(a -> inbuf));
					
					if (r == -1) {
						fscanf(stderr, "server: could not read from client socket");
						exit(1);
					} else if (r == 1) {
						fscanf(stderr, "server: client socket has been closed");
						exit(1);
					} else if (r == 2) { // no network new line found, keep going

					} else if (r == 0) { // network new line found, keep going

					}
					

					// do operation based on state
					while(1){
					if (a -> state == initial) {
						char *name;
						if (get_message(&name, a -> buf, &(a -> inbuf)) != 0) {
							fscanf(stdout, "server: get_message did not find network newline");
							break;
						} else {
							a -> fp = open_file_in_dir(name, DIRECTORY);
							free(name);
							temp = 'C';
							if (write(a -> fd, &temp, 1) < 0) {
								perror("server: initial write");
								exit(1);
							}
							a -> state = pre_block;
							a -> current_block = 1;
							a -> blocksize = 0;
						}

					} 
					if (a -> state == pre_block) {
						int cont = 0;
						for (int i = 0; i < a -> inbuf; i++) {
							if ((a -> buf)[i] == EOT) {
								// send ACK, transition to finish
								temp = ACK;
								if (write(a -> fd, &temp, 1) < 0) {
									perror("server: pre_block write");
									exit(1);
								}
								a -> state = finished;
								cont = 1;
								break;

							} else if ((a -> buf)[i] == SOH) { // expects to read 132 bytes
								
								// get rid of everything before SOH, including SOH
								memmove(a -> buf, (a -> buf) + (i + 1), (a -> inbuf) -= (i + 1));
								// transition to get_block, 
								a -> state = get_block;
								a -> blocksize = 132;
								cont = 1;
								break;

							} else if ((a -> buf)[i] == STX) { // expects to read 1028 bytes

								// get rid of everything before SOH, including SOH
								memmove(a -> buf, (a -> buf) + (i + 1), (a -> inbuf) -= (i + 1));
								// transition to get_block, 
								a -> state = get_block;
								a -> blocksize = 1028;
								cont = 1;
								break;
							}
						}
						if (! cont){
							break;
						}

					} 
					if (a -> state == get_block) {
						
						if (r != 2) {
							fscanf(stderr, "server: block read contains network newline");
							exit(1);
						}

						if (a -> inbuf == a -> blocksize) {
							a -> state = check_block;
						} else if(a -> inbuf > a -> blocksize) {
							fscanf(stderr, "server: client's inbuf is greater than client's blocksize");
							a -> state = check_block;
							// should this be an error?
						} else { // not enough bytes sent
							break;
						}

					}
					if (a -> state == check_block) {
	
						unsigned char block = (a -> buf[0]);

						int cond_inverse = (block + (a -> buf)[1] == 255);
						int cond_prev = (block + 1 == a -> current_block); 
						int cond_blockn = (block == (a -> current_block));

						char bbuf[a -> blocksize - 4]; // the payload
						memcpy(bbuf, a -> buf + 2, a -> blocksize - 4);
						unsigned short calculated_crc = crc_message(XMODEM_KEY, (unsigned char *)bbuf, a -> blocksize - 4);
						unsigned short sent_crc = a -> buf[a -> blocksize - 2];
						sent_crc = sent_crc << 8;
						sent_crc += a -> buf[a -> blocksize - 1];

						int cond_crc = (calculated_crc == sent_crc);

						if (!cond_inverse) { // check if the block number and its inverse don't match
							fscanf(stderr, "server: block number and inverse does not match");
							exit(1);
						} else if(cond_prev) { // check if the block number is the previous block number
							temp = ACK;
							if (write(a -> fd, &temp, 1) < 0) {
								perror("server: pre_block write ACK failed");
								exit(1);
							}
						} else if (!cond_blockn) { // check if the block number is not the right one
							fscanf(stderr, "server: block number does not match");
							exit(1);
						} else if (!cond_crc) { // check if the crc does not match
							fscanf(stderr, "server: crc does not match");
							temp = NAK;
							if (write(a -> fd, &temp, 1) < 0) {
								perror("server: check_block write NAK");
								exit(1);
							}
							memcpy(a -> buf, a -> buf + (a -> blocksize), a -> inbuf - (a -> blocksize));
							a -> inbuf -= a -> blocksize;
							a -> state = pre_block;
							a -> blocksize = 0;
						} else {
							if (fwrite(bbuf, 1, a -> blocksize - 4, a -> fp) != a -> blocksize - 4){
								fscanf(stderr, "server: incorrect amount of bytes written to fp");
								exit(1);
							}
							block++;
							if (block > 255) {
								block = 0;
							}
							a -> current_block = block;
							temp = ACK;
							if (write(a -> fd, &temp, 1) < 0) {
								perror("server: check_block write ACK");
								exit(1);
							}
							memcpy(a -> buf, a -> buf + (a -> blocksize), a -> inbuf - (a -> blocksize));
							a -> inbuf -= a -> blocksize;
							a -> state = pre_block;
							a -> blocksize = 0;
						}

					}
					if (a -> state == finished) {
						fclose(a -> fp);

						// remove from linked list
						if (a == head_client) {
							head_client = a -> next;
							last_client = NULL;
							struct client *c1 = head_client;
							while (c1 != NULL) {
								if (c1 -> next == NULL) {
									last_client = c1;
								}
								c1 = c1 -> next;
							}
						} else {
							int found = 0;
							struct client *prev = head_client;
							struct client *curr = head_client -> next;
							while (curr != NULL) {
								if (curr == a) {
									prev -> next = curr -> next;
									found = 1;
									break;
								}
							}
							last_client = NULL;
							struct client *c2 = head_client;
							while (c2 != NULL) {
								if (c2 -> next == NULL) {
									last_client = c2;
								}
								c2 = c2 -> next;
							}


							if (!found) {
								fscanf(stderr, "server: finished cannot find a");
								exit(1);
							}
						}

						// remove from set
						FD_CLR(a -> fd, &curr_sockets);

						break;
					}}

				}
			}
		}
	}
}



