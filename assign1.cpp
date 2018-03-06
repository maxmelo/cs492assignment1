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

int consumed;

int product_total;
std::queue<product> queue;

pthread_mutex_t queue_mutex;
pthread_cond_t notFull, notEmpty;

unsigned int fn(unsigned int n) {     
    if (n > 1) {
        return fn(n - 1) + fn(n - 2);
    } else if (n == 0 || n == 1) {
        return n;
    }

    std::cerr << "Error: invalid input\n";
    return 0;
}

void* consumer(void *q) {
    while (product_total < product_limit) {
        struct product p;

        // mutex lets us give one thread access to the queue
        pthread_mutex_lock(&queue_mutex);

        // check if queue is empty
        while (queue.size() == 0){
            pthread_cond_wait(&notEmpty, &queue_mutex);
        }

        if (scheduling_algo == 0) { //First-come first-serve scheduling
            p = queue.front();
            queue.pop();        
            
            //Simulate consumption
            for (int i = 0; i < p.life; i++) { fn(10); }

            std::cout << "Product with ID " << p.id << " consumed" << std::endl;
        } else if (scheduling_algo == 1) { //Round-robin scheduling
            p = queue.front();

            if (p.life >= quantum_value) {
                p.life = (p.life - quantum_value);
                for (int i = 0; i < quantum_value; i++) { fn(10); }
            } else {
                queue.pop();

                for (int i = 0; i < p.life; i++) { fn(10); }

                std::cout << "Product with ID " << p.id << " consumed" << std::endl;
            }
        }

        usleep(100000);

        // signal that queue is not full
        pthread_cond_signal(&notFull);

        pthread_mutex_unlock(&queue_mutex);
    }

    pthread_exit(NULL);
}

void* producer(void *q) {
    while (product_total < product_limit) {
        //Mutex so only one thread can access the queue at once
        pthread_mutex_lock(&queue_mutex);

        //Generate a new product
        std::srand(rand_seed);
        int rand_val = std::rand() % 1024;
        struct product p = { .id = product_total, .timestamp = std::time(nullptr), .life = rand_val };

        //Condition variable for full queue. This will need to get uncommented when the consumer code is done
        while (queue.size() >= queue_size) {
            pthread_cond_wait(&notFull, &queue_mutex);
        }

        queue.push(p);
        product_total++;

        pthread_cond_signal(&notEmpty);

        //Print product
        std::cout << "Product with ID " << p.id << " created at " << p.timestamp << std::endl;
        
        pthread_mutex_unlock(&queue_mutex);

        usleep(100000);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    //pthread_t **producer_threads, **consumer_threads;
    int error, producer_count, consumer_count;
    int retval[1];

    product_total = 0;

    //initialize mutex for producers
    pthread_mutex_init(&queue_mutex, NULL);


    //Parse command-line arguments
    if (argc == 2) { producer_thread_number = atoi(argv[1]); } else { producer_thread_number = 5; }
    if (argc >= 3) { consumer_thread_number = atoi(argv[2]); } else { consumer_thread_number = 5; }
    if (argc >= 4) { product_limit = atoi(argv[3]); } else { product_limit = 50; }
    if (argc >= 5) { queue_size = atoi(argv[4]); } else { queue_size = 5; }
    if (argc >= 6) { scheduling_algo = atoi(argv[5]); } else { scheduling_algo = 1; }
    if (argc >= 7) { quantum_value = atoi(argv[6]); } else { quantum_value = 580; }
    if (argc >= 8) { rand_seed = atoi(argv[7]); } else { rand_seed = 0; }

    //Generate producers and consumers based on the amount given in the command line arguments
    pthread_t** consumer_threads = new pthread_t*[consumer_thread_number];
    pthread_t** producer_threads = new pthread_t*[producer_thread_number];

    for (int i = 0; i < consumer_thread_number; i++) {
        consumer_threads[i] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(consumer_threads[i], NULL, (void*(*)(void*))(&consumer), (void*)i);
        if (error != 0) printf("Error number: %i\n", error);
    }
    for (int i = 0; i < producer_thread_number; i++) {
        producer_threads[i] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(producer_threads[i], NULL, (void*(*)(void*))(&producer), (void*)i);
        if (error != 0) printf("Error number: %i\n", error);
    }

    pthread_cond_init(&notFull, NULL);
    pthread_cond_init(&notEmpty, NULL);
    

    //At end of calling function, wait for all threads to complete
    for (int i = 0; i < consumer_thread_number; i++) { pthread_join (*(consumer_threads[i]), (void**)(&retval)); }
    for (int i = 0; i < producer_thread_number; i++) { pthread_join (*(producer_threads[i]), (void**)(&retval)); }

    pthread_exit(0);
}