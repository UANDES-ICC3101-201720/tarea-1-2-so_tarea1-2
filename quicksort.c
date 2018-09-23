#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include "types.h"
#include "const.h"
#include "util.h"

typedef struct data
{
    UINT *arr;
} data;

void swap(UINT *a, UINT *b)
{
    UINT t = *a;
    *a = *b;
    *b = t;
}

int partition(UINT *arr, int low, int high)
{
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] <= pivot)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

// TODO: implement
void quicksort(UINT *arr, int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}
// TODO: implement
int parallel_quicksort(UINT *A, int low, int high)
{
    return 0;
}

int main(int argc, char **argv)
{
    printf("[quicksort] Starting up...\n");

    /* Get the number of CPU cores available */
    printf("[quicksort] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));
    int size;
    char experiments[2] = "";
    /* TODO: parse arguments with getopt */
    int option = 0;
    while ((option = getopt(argc, argv, "T:E:")) != -1)
    {
        switch (option)
        {
        case 'T':

            strcpy(experiments, optarg);
            continue;
        case 'E':
            size = atoi(optarg);
            continue;
        default:
            printf("Parameters wrong");
            exit(1);
        }
    }
    /* TODO: start datagen here as a child process. */
    pid_t i = fork();
    if (i == 0)
    {
        if (execv("./datagen", argv) < 0)
        {
            printf("Doesnt works");
            exit(1);
        }
    }
    else if (i > 0)
    {
    }
    else
    {
        perror("fork failed");
        exit(3);
    }

    /* Create the domain socket to talk to datagen. */

    struct sockaddr_un addr;
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("[quicksort] Socket error.\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

    while (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("[quicksort] connect error.\n");
        close(fd);
        exit(-1);
    }

    /* DEMO: request two sets of unsorted random numbers to datagen */
    UINT readvalues = 0;
    size_t numvalues = pow(10, atoi(experiments));
    size_t readbytes = 0;

    UINT *readbuf = malloc(sizeof(UINT) * numvalues);
    for (int i = 0; i < 2; i++)
    {
        /* T value 3 hardcoded just for testing. */
        char begin[20] = "BEGIN S ";
        strcat(begin, experiments);
        int rc = strlen(begin);

        /* Request the random number stream to datagen */
        if (write(fd, begin, strlen(begin)) != rc)
        {
            if (rc > 0)
                fprintf(stderr, "[quicksort] partial write.\n");
            else
            {
                perror("[quicksort] write error.\n");
                exit(-1);
            }
        }

        /* validate the response */
        char respbuf[10];
        read(fd, respbuf, strlen(DATAGEN_OK_RESPONSE));
        respbuf[strlen(DATAGEN_OK_RESPONSE)] = '\0';

        if (strcmp(respbuf, DATAGEN_OK_RESPONSE))
        {
            perror("[quicksort] Response from datagen failed.\n");
            close(fd);
            exit(-1);
        }

        while (readvalues < numvalues)
        {
            /* read the bytestream */
            readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
            readvalues += readbytes / 4;
        }
    }
    printf("E: ");
    for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
    {
        printf("%u ", *pv);
    }
    quicksort(readbuf, 0, numvalues);
    printf("\n\nS: ");
    for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
    {
        printf("%u ", *pv);
    }
    free(readbuf);
    /* Issue the END command to datagen */
    int rc = strlen(DATAGEN_END_CMD);
    if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) != rc)
    {
        if (rc > 0)
            fprintf(stderr, "[quicksort] partial write.\n");
        else
        {
            perror("[quicksort] write error.\n");
            close(fd);
            exit(-1);
        }
    }
    for (int i = 0; i < size; ++i)
    {
        /* code */
    }

    close(fd);
    exit(0);
}