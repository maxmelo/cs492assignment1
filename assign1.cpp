#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <queue>
#include <ctime>
#include <vector>
#include <map>
#include <climits>

struct product {
    int id;
    clock_t timestamp;
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

int produced_total;
int consumed_total;

std::queue<product> queue;

pthread_mutex_t queue_mutex;
pthread_cond_t notFull, notEmpty;

int rand_inc;

clock_t start_time;
std::map<int, clock_t> wait_map;

std::vector<int> wait_times;
std::vector<int> turn_times;

//Fibonacci implementation to simulate consumption in consumer threads
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
    //Only run while the amount consumed doesn't exceed the limit defined in the commandline arguments
    while (consumed_total < product_limit) {
        struct product p;

        // mutex lets us give one thread access to the queue
        pthread_mutex_lock(&queue_mutex);

        // check if queue is empty
        while (queue.size() == 0) {
            pthread_cond_wait(&notEmpty, &queue_mutex);
        }

        if (scheduling_algo == 0) { //First-come first-serve scheduling
            p = queue.front();
            queue.pop();        
            wait_times.push_back(clock() - (wait_map.find(p.id)->second));

            //Simulate consumption
            for (int i = 0; i < p.life; i++) { fn(10); }

            std::cout << "Product with ID " << p.id << " consumed" << std::endl;
            turn_times.push_back(clock() - p.timestamp);

            consumed_total++;
        } else if (scheduling_algo == 1) { //Round-robin scheduling
            if (queue.front().life >= quantum_value) {
                //Replace the product at the front of the queue with a copy of the same product with quantum_value less life
                p = queue.front();
                p.life -= quantum_value;
                queue.front() = p;

                //Simulate consumption "quantum_value" times
                for (int i = 0; i < quantum_value; i++) { fn(10); }
            } else {
                //Remove from the queue
                queue.pop();

                wait_times.push_back(clock() - (wait_map.find(p.id)->second));

                //Simulate consumption "life" times
                for (int i = 0; i < p.life; i++) { fn(10); }

                //Console output for consumption
                std::cout << "Product with ID " << p.id << " consumed" << std::endl;
                turn_times.push_back(clock() - p.timestamp);

                consumed_total++;
            }
        }

        //Sleep thread for 100ms
        usleep(100000);

        // signal that queue is not full
        pthread_cond_signal(&notFull);

        pthread_mutex_unlock(&queue_mutex);
    }

    pthread_exit(NULL);
}

void* producer(void *q) {
    //Only run while the amount produced doesn't exceed the limit defined in the commandline argument
    while (produced_total < product_limit) {
        //Mutex so only one thread can access the queue at once
        pthread_mutex_lock(&queue_mutex);

        //Generate a new product with random life and timestamp in seconds since January 1st 1970
        std::srand(rand_seed + rand_inc);
        int rand_val = std::rand() % 1024;
        struct product p = { .id = produced_total, .timestamp = clock(), .life = rand_val };
        rand_inc++;

        //Condition variable for full queue. This will need to get uncommented when the consumer code is done
        while (queue.size() >= queue_size) {
            pthread_cond_wait(&notFull, &queue_mutex);
        }

        //Push new product to queue and update our produced counter
        queue.push(p);
        wait_map.insert(std::pair<int,clock_t>(p.id, clock()));

        produced_total++;

        pthread_cond_signal(&notEmpty);

        //Console output for production
        std::cout << "Product with ID " << p.id << " produced at " << p.timestamp << std::endl;

        pthread_mutex_unlock(&queue_mutex);

        //Sleep thread for 100ms
        usleep(100000);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    //pthread_t **producer_threads, **consumer_threads;
    int error, producer_count, consumer_count;
    int retval[1];
    float metrics_totaltime, metrics_turnmax, metrics_turnmin, metrics_turnavg, metrics_waitmax, metrics_waitmin, metrics_waitavg, metrics_prodthru, metrics_consthru;

    metrics_turnmax = std::numeric_limits<int>::min();
    metrics_waitmax = std::numeric_limits<int>::min();
    metrics_turnmin = std::numeric_limits<int>::max();
    metrics_waitmin = std::numeric_limits<int>::max();
    metrics_turnavg = 0;
    metrics_waitavg = 0;
    metrics_prodthru = 0;
    metrics_consthru = 0;

    produced_total = 0;
    consumed_total = 0;
    rand_inc = 0;

    //initialize mutex for producers
    pthread_mutex_init(&queue_mutex, NULL);


    //Parse command-line arguments
    if (argc == 2) { producer_thread_number = atoi(argv[1]); } else { producer_thread_number = 4; }
    if (argc >= 3) { consumer_thread_number = atoi(argv[2]); } else { consumer_thread_number = 4; }
    if (argc >= 4) { product_limit = atoi(argv[3]); } else { product_limit = 30; }
    if (argc >= 5) { queue_size = atoi(argv[4]); } else { queue_size = 10; }
    if (argc >= 6) { scheduling_algo = atoi(argv[5]); } else { scheduling_algo = 1; }
    if (argc >= 7) { quantum_value = atoi(argv[6]); } else { quantum_value = 100; }
    if (argc >= 8) { rand_seed = atoi(argv[7]); } else { rand_seed = 10; }

    //Generate producers and consumers based on the amount given in the command line arguments
    pthread_t** consumer_threads = new pthread_t*[consumer_thread_number];
    pthread_t** producer_threads = new pthread_t*[producer_thread_number];

    //Loop the number of times given in the commandline arguments and create consumer pthreads. Print error if the creation returns a non-zero error code.
    for (int i = 0; i < consumer_thread_number; i++) {
        consumer_threads[i] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(consumer_threads[i], NULL, (void*(*)(void*))(&consumer), (void*)i);
        if (error != 0) printf("Error number: %i\n", error);
    }

    //Loop the number of times given in the commandline arguments and create producer pthreads. Print error if the creation returns a non-zero error code.
    for (int i = 0; i < producer_thread_number; i++) {
        producer_threads[i] = (pthread_t*) calloc(1, sizeof(pthread_t));
        error = pthread_create(producer_threads[i], NULL, (void*(*)(void*))(&producer), (void*)i);
        if (error != 0) printf("Error number: %i\n", error);
    }

    //Initialize full/empty condition variables for use in our producer/consumer threads
    pthread_cond_init(&notFull, NULL);
    pthread_cond_init(&notEmpty, NULL);
    
    start_time = clock();

    //At end of calling function, wait for all threads to complete
    for (int i = 0; i < consumer_thread_number; i++) { pthread_join (*(consumer_threads[i]), (void**)(&retval)); }
    for (int i = 0; i < producer_thread_number; i++) { pthread_join (*(producer_threads[i]), (void**)(&retval)); }


    /*
        Time metrics calculations and printing
    */

    for(std::vector<int>::iterator it = turn_times.begin(); it != turn_times.end(); ++it) {
        int value = *it;

        if (value > metrics_turnmax) { metrics_turnmax = value; }
        if (value < metrics_turnmin) { metrics_turnmin = value; }

        metrics_turnavg += value;
    }
    metrics_turnavg /= turn_times.size();

    for(std::vector<int>::iterator it = wait_times.begin(); it != wait_times.end(); ++it) {
        int value = *it;

        if (value > metrics_waitmax) { metrics_waitmax = value; }
        if (value < metrics_waitmin) { metrics_waitmin = value; }

        metrics_waitavg += value;
    }
    metrics_waitavg /= wait_times.size();

    metrics_totaltime = clock() - start_time;

    metrics_prodthru = produced_total / metrics_totaltime * 60;
    metrics_consthru = consumed_total / metrics_totaltime * 60;

    printf("Total time for processing all products: %f\n", metrics_totaltime);
    printf("Turn around time    Min: %f   Max %f   Average %f\n", metrics_turnmin, metrics_turnmax, metrics_turnavg);
    printf("Wait time    Min: %f   Max %f   Average %f\n", metrics_waitmin, metrics_waitmax, metrics_waitavg);
    printf("Producer throughput: %f\n", metrics_prodthru);
    printf("Consumer throughput: %f\n", metrics_consthru);
    pthread_exit(0);
}