#include "helper.h"

/* Concatenate dirname and filename, and return an open
 * file pointer to file filename opened for writing in the
 * directory dirname (located in the current working directory)
 */
FILE *open_file_in_dir(char *filename, char *dirname) {
    char buffer[MAXBUFFER];
    strncpy(buffer, "./", MAXBUFFER);
    strncat(buffer, dirname, MAXBUFFER - strlen(buffer) - 1);

    // create the directory dirname; fail silently if directory exists
    if(mkdir(buffer, 0700) == -1) {
        if(errno != EEXIST) {
            perror("mkdir");
            exit(1);
        }
    }
    strncat(buffer, "/", MAXBUFFER - strlen(buffer));
    strncat(buffer, filename, MAXBUFFER - strlen(buffer));

    return fopen(buffer, "wb");
}

// The codes below are taken from piazza @182: find_network_newline, read_from_socket, get_message

/*
 * Search the first inbuf characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int inbuf) {
    for (int i = 0; i < inbuf - 1; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return i + 2;
        }
    }
    return -1;
}

/* 
 * Reads from socket sock_fd into buffer *buf containing *inbuf bytes
 * of data. Updates *inbuf after reading from socket.
 *
 * Return -1 if read error or maximum message size is exceeded.
 * Return 0 upon receipt of CRLF-terminated message.
 * Return 1 if socket has been closed.
 * Return 2 upon receipt of partial (non-CRLF-terminated) message.
 */
int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    int num_read = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf);
    if (num_read == 0) { // if client disconnected
        return 1;
    }
    else if (num_read == -1) {
        return -1;
    }
    *inbuf += num_read;
    for (int i = 0; i <= *inbuf-2; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return 0;
        }
    }
    if (*inbuf == BUF_SIZE) {
        return -1;
    }
    return 2;
}

/*
 * Search src for a network newline, and copy complete message
 * into a newly-allocated NULL-terminated string **dst.
 * Remove the complete message from the *src buffer by moving
 * the remaining content of the buffer to the front.
 *
 * Return 0 on success, 1 on error.
 */
int get_message(char **dst, char *src, int *inbuf) {
    int msg_len = find_network_newline(src, *inbuf);
    if (msg_len == -1) return 1;

    *dst = malloc(BUF_SIZE);
    if (*dst == NULL) {
        perror("malloc");
        return 1;
    }

    memmove(*dst, src, msg_len - 2);
    (*dst)[msg_len-2] = '\0';
    memmove(src, src + msg_len, BUF_SIZE - msg_len);
    *inbuf -= msg_len;
    return 0;
}



/* A simple test for open_file_in_dir */
/*
int main(int argc, char *argv[]) {
    char *dir = "filestore";

    FILE *fp;
 
    if((fp = open_file_in_dir(argv[1], dir)) == NULL) {
        perror("open file");
        exit(1);
    }
    fprintf(fp, "Trial\n");
    fclose(fp);

}
*/
