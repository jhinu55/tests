#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_STRING_LEN 64
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

static int parse_client_id(const char *text, int *client_id) {
    char *end;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < 0 || value > 9) return 0;
    *client_id = (int)value;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s CLIENT_ID\n", argv[0]);
        return 2;
    }
    int client_id;
    if (!parse_client_id(argv[1], &client_id)) return 2;

    char queue_name[64];
    snprintf(queue_name, sizeof(queue_name), "/osdoc_client_%d", client_id);
    mq_unlink(queue_name);
    struct mq_attr attributes = {.mq_maxmsg = 10, .mq_msgsize = sizeof(response_t)};
    mqd_t responses = mq_open(queue_name, O_CREAT | O_RDONLY, 0600, &attributes);
    mqd_t requests = mq_open(SERVER_QUEUE, O_WRONLY);
    if (responses == (mqd_t)-1 || requests == (mqd_t)-1) {
        perror("queue");
        return 1;
    }

    FILE *input = fopen("input.txt", "r");
    if (!input) {
        perror("input.txt");
        return 1;
    }
    char command[16];
    while (fscanf(input, " C%d %15s", &client_id, command) == 2) {
        request_t request = {.client_id = client_id};
        if (strcmp(command, "READ") == 0) {
            request.operation = OP_READ;
            if (fscanf(input, "%d %d", &request.line, &request.word) != 2) break;
        } else if (strcmp(command, "WRITE") == 0) {
            request.operation = OP_WRITE;
            if (fscanf(input, "%d %d %64s %d", &request.line, &request.word,
                       request.value, &request.duration_ms) != 4) break;
        } else if (strcmp(command, "SLEEP") == 0) {
            int milliseconds;
            if (fscanf(input, "%d", &milliseconds) != 1) break;
            usleep((useconds_t)milliseconds * 1000);
            continue;
        } else {
            char ignored[256];
            fgets(ignored, sizeof(ignored), input);
            continue;
        }
        if (mq_send(requests, (const char *)&request, sizeof(request), 0) < 0) break;
        response_t response;
        if (mq_receive(responses, (char *)&response, sizeof(response), NULL) < 0) break;
        printf("%s %d %d: %s\n", response.ok ? "OK" : "ERROR",
               response.line, response.word, response.value);
    }
    fclose(input);
    mq_close(requests);
    mq_close(responses);
    mq_unlink(queue_name);
    return 0;
}
