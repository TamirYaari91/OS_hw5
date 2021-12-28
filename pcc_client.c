#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>


unsigned int get_size(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) {
        perror("File Not Found.\n");
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    unsigned int res = ftell(fp);
    fclose(fp);
    return res;
}

int send_int(unsigned int num, int fd) {
    unsigned long conv = htonl(num);
    char *data = (char *) &conv;
    unsigned long left = sizeof(conv);
    unsigned long rc;
    do {
        rc = write(fd, data, left);
        if (rc < 0) {
            perror("write in send_int failed.\n");
            exit(1);
        }
        data += rc;
        left -= rc;
    } while (left > 0);
    return 0;
}

void send_file(FILE *fp, int sockfd) {
    long bytes_written;
    long bytes_written_total = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        bytes_written = write(sockfd, line, read);
        if (bytes_written == -1) {
            perror("Error in sending file.");
            exit(1);
        } else {
            bytes_written_total += bytes_written;
        }
    }
}

void receive_int(unsigned int *num, int fd) {
    unsigned int ret;
    char *data = (char *) &ret;
    long left = sizeof(ret);
    long rc;
    do {
        rc = read(fd, data, left);
        if (rc < 0) {
            perror("read in receive_int failed.\n");
            exit(1);
        }
        data += rc;
        left -= rc;
    } while (left > 0);
    *num = ntohl(ret);
}

int main(int argc, char *argv[]) {
    int sockfd;
    char *server_address_string;
    unsigned short server_port;
    char *ptr;
    FILE *fptr;
    char *filename;
    unsigned int N;
    unsigned int *C = malloc(sizeof(unsigned int));
    if (C == NULL) {
        perror("C malloc failed.\n");
        exit(1);
    }
    struct sockaddr_in serv_addr; // where we Want to get to

    if (argc != 4) {
        perror("Invalid number of arguments.");
        exit(1);
    }

    server_address_string = argv[1];
    server_port = (unsigned short) strtol(argv[2], &ptr, 10);
    filename = argv[3];

    if ((fptr = fopen(filename, "r")) == NULL) {
        perror("Error in file opening.\n");
        exit(1);
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in socket creation.\n");
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address_string, &serv_addr.sin_addr.s_addr);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error in connect.\n");
        exit(1);
    }

    N = get_size(filename); // calculate N
    send_int(N, sockfd); // send N to server
    send_file(fptr, sockfd); // send N bytes to server
    fclose(fptr);
    receive_int(C, sockfd); // get C from server
    close(sockfd);
    printf("# of printable characters: %u\n", *C);
    free(C);
    return 0;
}
