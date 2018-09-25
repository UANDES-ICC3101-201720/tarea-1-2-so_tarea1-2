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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct quickData
{
    UINT *arr;
    int left;
    int right;
} quickData;

void swap(UINT *a, UINT *b)
{
    UINT t = *a;
    *a = *b;
    *b = t;
}

int partition(UINT *arr, int low, int high)
{
    UINT pivot = arr[high];
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
int Partition(UINT *array, int left, int right, int pivot)
{
    int pivotValue = array[pivot];
    swap(&array[pivot], &array[right]);
    int storeIndex = left;
    for (int i = left; i < right; i++)
    {
        if (array[i] <= pivotValue)
        {
            swap(&array[i], &array[storeIndex]);
            storeIndex++;
        }
    }
    swap(&array[storeIndex], &array[right]);
    return storeIndex;
}
void quicksort(UINT *arr, int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high);
        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

void parallel_quicksort(UINT *array, int left, int right);
void *quicksort_thread(void *init)
{
    quickData *start = init;
    parallel_quicksort(start->arr, start->left, start->right);
    return NULL;
}
void parallel_quicksort(UINT *array, int left, int right)
{
    if (right > left)
    {
        int pivotIndex = left + (right - left) / 2;
        pivotIndex = Partition(array, left, right, pivotIndex);
        quickData arg = {array, left, pivotIndex - 1};
        pthread_t thread;
        pthread_create(&thread, NULL, quicksort_thread, &arg);
        parallel_quicksort(array, pivotIndex + 1, right);
        pthread_join(thread, NULL);
    }
}

int main(int argc, char **argv)
{
    int size;
    char experiments[2] = "";
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

    /* DEMO: request two sets of unsorted random numbers to datagen */

    /* Issue the END command to datagen */
    for (int l = 0; l < size; ++l)
    {
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

        UINT readvalues = 0;
        size_t numvalues = pow(10, atoi(experiments));
        size_t readbytes = 0;
        UINT *readbuf = malloc(sizeof(UINT) * numvalues);
        /* T value 3 hardcoded just for testing. */
        char begin[20] = "BEGIN U ";
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

        /* code */
        printf("E%i: ", l + 1);
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
        {
            printf("%u ", *pv);
        }
        //quicksort(readbuf, 0, numvalues);
        parallel_quicksort(readbuf, 0, numvalues - 1);

        printf("\n\nS%i: ", l + 1);
        for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
        {
            printf("%u ", *pv);
        }
        printf("\n\n");
        free(readbuf);
        rc = strlen(DATAGEN_END_CMD);
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
        close(fd);
    }

    exit(0);
}