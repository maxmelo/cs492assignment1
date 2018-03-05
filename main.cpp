#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int producer_threads;
int consumer_threads;
int product_total;
int queue_size;
int scheduling_algo;
int quantum_value;
int rand_seed;

int main(int argc, char *argv[]) {
    pthread_t *thread1,	*thread2;
    int retval[1], error;

    //Create	first	thread
    thread1	=	(pthread_t*) calloc (1, sizeof(pthread_t));
    error	= pthread_create (thread1,	NULL,	(void*(*)(void*))(&consumer),	NULL);
    if	(error	!=	0) printf ("Error	number:	%i\n",	error);
    //	Create	second	thread
    thread2	=	(pthread_t*) calloc (1, sizeof(pthread_t));
    error	= pthread_create (thread2,	NULL,	(void*(*)(void*))(&producer),	NULL);
    if	(error	!=	0)printf ("Error	number:	%i\n",	error);

    
    
    
    if (argc <= 1) { producer_threads = atoi(argv[0]); } else { producer_threads = 1; }
    if (argc <= 2) { consumer_threads = atoi(argv[1]); } else { consumer_threads = 1; }
    if (argc <= 3) { product_total = atoi(argv[2]); } else { product_total = 3; }
    if (argc <= 4) { queue_size = atoi(argv[3]); } else { queue_size = 5; }
    if (argc <= 5) { scheduling_algo = atoi(argv[4]); } else { scheduling_algo = 0; }
    if (argc <= 6) { quantum_value = atoi(argv[5]); } else { quantum_value = 1; }
    if (argc <= 7) { rand_seed = atoi(argv[6]); } else { rand_seed = 0; }




    //	At	end	of	calling	function,	wait	for	all	threads	to	complete
    pthread_join (*thread1,	(void**)(&retval));
    pthread_join (*thread2,	(void**)(&retval));
}

void* consumer(void *q) {
    //Consumer code
}
void* producer(void *q) {
    //Producer code
}