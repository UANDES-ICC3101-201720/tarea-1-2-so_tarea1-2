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
typedef struct list
{
    UINT num;
    struct list *next;
} list;
typedef struct list_container
{
    list *less;
    list *bigger;
    list *next;
} list_container;

typedef struct data
{
    UINT *arr;
    int low;
    int high;
    UINT pivot;
    list_container *lists;
} data;

void swap(UINT *a, UINT *b)
{
    UINT t = *a;
    *a = *b;
    *b = t;
}

int append(UINT value, list *initial, list *next)
{
    next = initial;

    if (next->num == -1)
    {
        next->num = value;
        return 0;
    }
    if (next != NULL)
    {
        while (next->next != NULL)
        {
            next = next->next;
        }
    }

    next->next = malloc(sizeof(list));
    next = next->next;
    next->num = value;
    return 0;
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

int Partition(UINT *arr, int low, int high, UINT pivot, list *less, list *bigger, list *next)
{
    int i = (low - 1);
    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] <= pivot)
        {
            i++;
            append(arr[j], less, next);
        }
        else
        {
            append(arr[j], bigger, next);
        }
    }
    return (i + 1);
}
void quickSort(void *d)
{
    data *info = d;
    if (info->low < info->high)
    {
        pthread_mutex_lock(&lock);
        Partition(info->arr, info->low, info->high, info->pivot, info->lists->less, info->lists->bigger, info->lists->next);
        pthread_mutex_unlock(&lock);
        pthread_cond_broadcast(&cond);
    }
}
// TODO: implement
void parallel_quicksort(UINT *arr, long int size, int low, int high, int con)
{
    int max_threads = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t m_tid[max_threads];
    int parts = high / max_threads;
    int counter = low;
    int piv = rand() % size;
    list_container *l_c = malloc(sizeof(list_container));
    list *less = malloc(sizeof(list));
    less->num = -1;
    list *bigger = malloc(sizeof(list));
    bigger->num = -1;
    l_c->bigger = bigger;
    l_c->less = less;
    l_c->next = NULL;
    for (int i = 0; i < max_threads; i++)
    {
        data *info = malloc(sizeof(data));
        info->low = counter;
        info->high = size * (i + 1) / max_threads;
        info->arr = arr;
        info->pivot = arr[piv];
        info->lists = l_c;
        if (counter > size * (i + 1) / max_threads)
        {
            free(info);
            continue;
        }
        if (pthread_create(&m_tid[i], NULL, (void *)quickSort, info))
        {
            free(info);
        }
        counter += parts;
    }
    for (int i = 0; i < max_threads; i++)
    {
        pthread_join(m_tid[i], NULL);
    }
    short int help = 0;
    int biggy_start = 0;
    /*list *next = less;
    while (next != NULL)
    {
        fprintf(stderr, "[l]%u\n", next->num);
        next = next->next;
    }
    next = bigger;
    while (next != NULL)
    {
        fprintf(stderr, "[b]%u\n", next->num);
        next = next->next;
    }
    */
    for (UINT *pv = arr; pv < arr + size; pv++)
    {
        if (help == 0)
        {
            *pv = less->num;

            if (less->next == NULL)
            {
                help = 1;
            }
            else
            {
                less = less->next;
            }
            biggy_start++;
        }
        else
        {

            *pv = bigger->num;
            if (bigger->next == NULL)
            {
                break;
            }
            else
            {
                bigger = bigger->next;
            }
        }
    }

    free(less);
    free(bigger);
    if (biggy_start < high && biggy_start > low)
    {
        fprintf(stderr, "%i , %i, %li\n", low, biggy_start, size);
        parallel_quicksort(arr, size, low, biggy_start - 1, con);
        parallel_quicksort(arr, size, biggy_start + 1, high, con);
    }
}

int main(int argc, char **argv)
{
    printf("[quicksort] Starting up...\n");

    /* Get the number of4 CPU cores available */
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
    //quicksort(readbuf, 0, numvalues);
    parallel_quicksort(readbuf, numvalues, 0, numvalues, 0);
    printf("\n\nS: ");
    for (UINT *pv = readbuf; pv < readbuf + numvalues; pv++)
    {
        printf("%u ", *pv);
    }
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