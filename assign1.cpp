#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <queue>
#include <ctime>

struct product {
    int id;
    std::time_t timestamp;
    int life;
};


int producer_thread_number;
int consumer_thread_number;
int product_limit;
int queue_size;
int scheduling_algo;
int quantum_value;
int rand_seed;

int product_total;
std::queue<product> queue;

pthread_mutex_t produce_mutex;
pthread_cond_t notFull, notEmpty;


void* consumer(void *q) {
    //Consumer code
    return nullptr;
}
void* producer(void *q) {
    //Generate a new product
    std::srand(rand_seed);
    int rand_val = std::rand() % 1024;
    struct product p = { .id = product_total, .timestamp = std::time(nullptr), .life = rand_val };

    //Mutex so only one thread can access the queue at once
    pthread_mutex_lock(&produce_mutex);

    //Condition variable for full queue. This will need to get uncommented when the consumer code is done
    /*while (queue.size() >= queue_size) {
        pthread_cond_wait(&notFull, &produce_mutex);
    }*/

    queue.push(p);
    product_total++;

    //pthread_cond_signal(&notEmpty);

    //Print product
    std::cout << "Product with ID " << p.id << " created at " << p.timestamp << std::endl;
    
    pthread_mutex_unlock(&produce_mutex);

    usleep(100000);

    return nullptr;
}

pthread_t** create_producers(int amt) {
    int count, error;
    pthread_t* threads[amt];

    while (count < amt) {
        threads[count] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(threads[count], NULL, (void*(*)(void*))(&producer), NULL);
        if (error != 0) printf("Error number: %i\n", error);
    }

    return threads;
}
pthread_t** create_consumers(int amt) {
    int count, error;
    pthread_t* threads[amt];

    while (count < amt) {
        threads[count] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(threads[count], NULL, (void*(*)(void*))(&consumer), NULL);
        if (error != 0) printf("Error number: %i\n", error);
    }

    return threads;
}

int main(int argc, char *argv[]) {
    pthread_t **producer_threads, **consumer_threads;
    int error, producer_count, consumer_count;

    product_total = 0;

    pthread_mutex_init(&produce_mutex, NULL);


    if (argc == 2) { producer_thread_number = atoi(argv[1]); } else { producer_thread_number = 5; }
    if (argc <= 3) { consumer_thread_number = atoi(argv[2]); } else { producer_thread_number = 5; }
    if (argc <= 4) { product_limit = atoi(argv[3]); } else { product_limit = 50; }
    if (argc <= 5) { queue_size = atoi(argv[4]); } else { queue_size = 50; }
    if (argc <= 6) { scheduling_algo = atoi(argv[5]); } else { scheduling_algo = 0; }
    if (argc <= 7) { quantum_value = atoi(argv[6]); } else { quantum_value = 1; }
    if (argc <= 8) { rand_seed = atoi(argv[7]); } else { rand_seed = 0; }

    consumer_threads = create_consumers(consumer_thread_number);
    producer_threads = create_producers(producer_thread_number);

    pthread_cond_init(&notFull, NULL);
    pthread_cond_init(&notEmpty, NULL);
    
    //At end of calling function, wait for all threads to complete
    for (int i = 0; i < consumer_thread_number; i++) { pthread_join (*(consumer_threads[i]), NULL); }
    for (int i = 0; i < consumer_thread_number; i++) { pthread_join (*(producer_threads[i]), NULL); }

    return 1;
}