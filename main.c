#define _GNU_SOURCE

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <time.h>

sem_t *items;
sem_t *freeSpace;
struct Buffer *buffer;

pthread_t *consumes = NULL;
int *consumeRun = NULL;
int consumeCount = 0;

pthread_t *produces = NULL;
int *produceRun = NULL;
int produceCount = 0;

struct Message
{
    uint8_t type;
    uint16_t hash;
    uint8_t size;
    char data[];
};

#define MESSAGE_MAX_SIZE (sizeof(struct Message) + 255)

int MessageSize(struct Message *message) { return sizeof(*message) + message->size; }

struct Message *readMessage(struct Buffer *buffer)
{
    struct Message *head = alloca(MESSAGE_MAX_SIZE);
    readBytes(buffer, sizeof(struct Message), (char *)head);
    readBytes(buffer, head->size, head->data);
    buffer->extracted++;
    printf("Extracted : %d\n", buffer->extracted);
    return memcpy(malloc(MessageSize(head)), head, MessageSize(head));
}

void sendMessage(struct Buffer *buffer, struct Message *message)
{
    sendBytes(buffer, MessageSize(message), (char *)message);
    buffer->added++;
    printf("Added : %d\n", buffer->added);
}

uint16_t xor (int length, char bytes[]) {
    uint16_t res = 0;
    for (int i = 0; i < length; i++)
    {
        res ^= bytes[i];
    }
    return res;
}

    struct Message *randomMessage()
{
    uint8_t size = rand() % 256;
    struct Message *message = malloc(sizeof(struct Message) + size);
    *message = (struct Message){
        .type = rand(),
        .hash = 0,
        .size = size,
    };
    for (int i = 0; i < size; i++)
        message->data[i] = rand();
    message->hash = xor(sizeof(struct Message) + size, (char *)message);
    return message;
}

void produce(void *arg)
{
    while ((*(int *)arg))
    {
        sem_wait(freeSpace);
        struct Message *message = randomMessage();
        sendMessage(buffer, message);
        printf(
            "Producer %5lu Sent message with type %02hX and hash %04hX\n",
            pthread_self(),
            message->type,
            message->hash);
        free(message);
        sem_post(items);
        sleep(1);
        pthread_testcancel();
    }
}

void consume(void *arg)
{
    while ((*(int *)arg))
    {
        sem_wait(items);
        struct Message *message = readMessage(buffer);
        printf(
            "Consumer %5lu Got  message with type %02hX and hash %04hX\n",
            pthread_self(),
            message->type,
            message->hash);
        free(message);
        sem_post(freeSpace);
        sleep(1);
        pthread_testcancel();
    }
}

void newProduce()
{
    produceCount++;
    produces = realloc(produces, produceCount * sizeof(pthread_t));
    produceRun = realloc(produceRun, produceCount * sizeof(int));
    produceRun[produceCount - 1] = 1;
    pthread_create(&produces[produceCount - 1], NULL, produce, &produceRun[produceCount - 1]);
    printf("New produce: %lu\n", produces[produceCount - 1]);
}

void killProduce()
{
    if (produceCount == 0)
        return;
    produceCount--;
    produceRun[produceCount] = 0;
    printf("Kill Produce TID: %lu\n", produces[produceCount]);
    pthread_cancel(produces[produceCount]);
    pthread_join(produces[produceCount], NULL);
}

void killAllProduce()
{

    while (produceCount)
        killProduce();
    free(produces);
    produces = NULL;
}

void newConsume()
{
    consumeCount++;
    consumes = realloc(consumes, consumeCount * sizeof(pthread_t));
    consumeRun = realloc(consumeRun, consumeCount * sizeof(int));
    consumeRun[consumeCount - 1] = 1;
    pthread_create(&consumes[consumeCount - 1], NULL, consume, &consumeRun[consumeCount - 1]);
    printf("New consume: %lu\n", consumes[consumeCount - 1]);
}

void killConsume()
{
    if (consumeCount == 0)
        return;
    consumeCount--;
    consumeRun[consumeCount] = 0;
    printf("Kill Consume TID: %lu\n", consumes[consumeCount]);
    pthread_cancel(consumes[consumeCount]);
    pthread_join(consumes[consumeCount], NULL);
}

void killAllConsume()
{
    while (consumeCount)
        killConsume();
    free(consumes);
    consumes = NULL;
}

int main()
{
    sem_unlink("/items");
    sem_unlink("/free_space");
    int queue = 5;
    items = sem_open("/items", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR, 0);
    freeSpace = sem_open("/free_space", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR, queue);
    int capacity = 1024;
    buffer = createBuffer(smalloc(sizeof(struct Buffer) + capacity), capacity);
    char opt[256];
    printf("Start program\n");
    while (scanf("%s", opt))
    {
        if (!strcmp(opt, "+"))
        {
            sem_post(freeSpace);
            printf("Queue: %d\n", ++queue);
        }
        if (!strcmp(opt, "-"))
        {
            sem_wait(freeSpace);
            printf("Queue: %d\n", --queue);
        }
        if (!strcmp(opt, "p"))
            newProduce(buffer);
        if (!strcmp(opt, "kp"))
            killProduce();
        if (!strcmp(opt, "kap"))
            killAllProduce();
        if (!strcmp(opt, "c"))
            newConsume(buffer);
        if (!strcmp(opt, "kc"))
            killConsume();
        if (!strcmp(opt, "kac"))
            killAllConsume();
        if (!strcmp(opt, "ka"))
        {
            killAllConsume();
            killAllProduce();
        }
        if (!strcmp(opt, "q"))
            break;
    }

    sem_unlink("/items");
    sem_unlink("/free_space");
    killAllConsume();
    killAllProduce();
    freeDesctruct(buffer);
    return 0;
}
