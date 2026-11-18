#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define GRID_SIZE 10
#define MAX_STRING_LEN 64
#define MAX_CLIENTS 10
#define SERVER_QUEUE "/osdoc_server"

typedef enum { OP_READ = 1, OP_WRITE = 2 } operation_t;

typedef struct {
    int client_id;
    operation_t operation;
    int line;
    int word;
    int duration_ms;
    char value[MAX_STRING_LEN + 1];
} request_t;

typedef struct {
    int ok;
    int line;
    int word;
    char value[MAX_STRING_LEN + 1];
} response_t;

static volatile sig_atomic_t stopping;
static char document[GRID_SIZE][GRID_SIZE][MAX_STRING_LEN + 1];

static void stop_server(int signal_number) {
    (void)signal_number;
    stopping = 1;
}

static int valid_request(const request_t *request) {
    return request->client_id >= 0 && request->client_id < MAX_CLIENTS &&
           request->line >= 0 && request->line < GRID_SIZE &&
           request->word >= 0 && request->word < GRID_SIZE &&
           request->duration_ms >= 0 &&
           memchr(request->value, '\0', sizeof(request->value)) != NULL;
}

int main(void) {
    struct sigaction action = {0};
    action.sa_handler = stop_server;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);

    mq_unlink(SERVER_QUEUE);
    struct mq_attr attributes = {.mq_maxmsg = 10, .mq_msgsize = sizeof(request_t)};
    mqd_t queue = mq_open(SERVER_QUEUE, O_CREAT | O_RDONLY, 0600, &attributes);
    if (queue == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    fprintf(stdout, "Server: Started\n");
    request_t request;
    while (!stopping) {
        ssize_t received = mq_receive(queue, (char *)&request, sizeof(request), NULL);
        if (received < 0) {
            if (errno == EINTR) continue;
            perror("mq_receive");
            break;
        }
        if ((size_t)received != sizeof(request)) continue;

        char queue_name[64];
        snprintf(queue_name, sizeof(queue_name), "/osdoc_client_%d", request.client_id);
        mqd_t response_queue = mq_open(queue_name, O_WRONLY);
        if (response_queue == (mqd_t)-1) continue;

        response_t response = {.line = request.line, .word = request.word};
        if (valid_request(&request)) {
            if (request.operation == OP_WRITE) {
                memcpy(document[request.line][request.word], request.value,
                       sizeof(document[request.line][request.word]));
                response.ok = 1;
            } else if (request.operation == OP_READ) {
                memcpy(response.value, document[request.line][request.word],
                       sizeof(response.value));
                response.ok = 1;
            }
        }
        mq_send(response_queue, (const char *)&response, sizeof(response), 0);
        mq_close(response_queue);
    }

    mq_close(queue);
    mq_unlink(SERVER_QUEUE);
    return 0;
}
