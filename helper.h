#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define MAXBUFFER 1024
#define BUF_SIZE 2048


FILE *open_file_in_dir(char *filename, char *dirname);
int find_network_newline(const char *buf, int inbuf);
int read_from_socket(int sock_fd, char *buf, int *inbuf);
int get_message(char **dst, char *src, int *inbuf);

