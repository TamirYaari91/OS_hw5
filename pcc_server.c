#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#define BUFFSIZE 1024
#define ARRSIZE 126-32+1


//unsigned int pcc_total[ARRSIZE] = {0};
unsigned int* pcc_total;
//unsigned int pcc_total_temp[ARRSIZE] = {0};
unsigned int* pcc_total_temp;
sig_atomic_t volatile keep_running = 1;
int listenfd = -1;


void receive_long(unsigned long *num, int fd) {
    unsigned long ret;
    char *data = (char *) &ret;
    long left = sizeof(ret);
    long rc;
    do {
        rc = read(fd, data, left);
        if (rc <= 0) {
            perror("error in receive_long.\n");
            if (rc < 0) {
                exit(1);
            }
        }
        data += rc;
        left -= rc;

    } while (left > 0);
    *num = ntohl(ret);
}

int receive_file(int connfd, unsigned long num_of_bytes_to_read) {
    unsigned char* buffer;
    long bytes_read;
    long total_bytes_read = 0;
    int i = 0;
    int char_value, char_value_ind;
    int pcc_counter = 0;

//    buffer = malloc(BUFFSIZE * sizeof (char));
    buffer = calloc(BUFFSIZE,sizeof (char));
    if (buffer == NULL) {
        perror("buffer malloc failed.\n");
        exit(1);
    }

    pcc_total_temp = malloc(ARRSIZE * sizeof (int));
    if (pcc_total_temp == NULL) {
        perror("pcc_total_temp malloc failed.\n");
        exit(1);
    }

    while (total_bytes_read < num_of_bytes_to_read) {
        bytes_read = read(connfd, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                exit(1);
            }
            break;
        }
        total_bytes_read += bytes_read;
        printf("buffer = %s\n",buffer);
        while (buffer[i] != '\0') { // while buffer is full
            char_value = buffer[i];
            if (32 <= char_value && char_value <= 126) { // buffer[i] is a printable character
                char_value_ind = char_value - 32;
                pcc_total_temp[char_value_ind]++;
                pcc_counter++;
            }
            i++;
        }
        i = 0;
    }
    free(buffer);
    free(pcc_total_temp);
    return pcc_counter;
}

int send_int(unsigned int num, int fd) {
    unsigned int conv = htonl(num);
    char *data = (char *) &conv;
    unsigned long left = sizeof(conv);
    unsigned long rc;
    do {
        rc = write(fd, data, left);
        if (rc <= 0) {
            perror("write in send_int failed.\n");
            if (rc < 0) {
                exit(1);
            }
        }
        data += rc;
        left -= rc;
    } while (left > 0);
    return 0;
}

void print_char_arr(unsigned int* arr) {
    int i;
    for (i = 0; i < ARRSIZE; i++) {
        printf("char '%c' : %u times\n", (i + 32), arr[i]);
    }
}

void update_stats(unsigned int* arr, const unsigned int* temp) {
    int i;
    for (i = 0; i < ARRSIZE; i++) {
        arr[i] += temp[i];
    }
}

void erase_temp(unsigned int* temp) {
    int i;
    for (i = 0; i < ARRSIZE; i++) {
        temp[i] = 0;
    }
}

void sig_handler(int _) {
    (void) _;
    keep_running = 0;
    close(listenfd);
}

int main(int argc, char *argv[]) {
    int connfd;
    int pcc_counter;
    unsigned short server_port;
    char *ptr;
    unsigned long *N = malloc(sizeof(unsigned long));
    if (N == NULL) {
        perror("N malloc failed.\n");
        exit(1);
    }
    pcc_total = malloc(ARRSIZE * sizeof (int));
    if (pcc_total == NULL) {
        perror("pcc_total malloc failed.\n");
        exit(1);
    }
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    if (argc != 2) {
        perror("Invalid number of arguments.\n");
        exit(1);
    }
    server_port = (unsigned short) strtol(argv[1], &ptr, 10);

    signal(SIGINT, &sig_handler);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, addrsize);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    int enable = 1; // set port for immediate re-use
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed.\n");
        exit(1);
    }

    if (0 != bind(listenfd, (struct sockaddr *) &serv_addr, addrsize)) {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        exit(1);
    }

    if (0 != listen(listenfd, 10)) {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        if (!(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) {
            exit(1);
        }
    }
    while (keep_running) {
        connfd = accept(listenfd, NULL, NULL);
        if (keep_running) {
            if (connfd < 0) {
                printf("\n Error : Accept Failed. %s \n", strerror(errno));
                if (!(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) {
                    exit(1);
                }
            }
            receive_long(N, connfd);
            pcc_counter = receive_file(connfd, *N);
            send_int(pcc_counter, connfd);
            update_stats(pcc_total,pcc_total_temp);
            erase_temp(pcc_total_temp);
        }
        close(connfd);
    }
    print_char_arr(pcc_total);
    free(N);
    free(pcc_total);
}
