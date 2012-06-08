#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

struct execution_data {
	int num_threads;
	unsigned long long int num_bytes;
	unsigned long long int num_passes;
	uint64_t *bytes;
};

struct thread_data {
	pthread_t thread;
	int local_id;
	struct execution_data *execution_data;
	unsigned long long int total;
};

static void *thread_main(void *data) {
	struct thread_data *thread_data = (struct thread_data *)data;
	int local_id = thread_data->local_id;
	int num_threads = thread_data->execution_data->num_threads;
	uint64_t *bytes = thread_data->execution_data->bytes;
	unsigned long long int num_bytes = thread_data->execution_data->num_bytes;
	unsigned long long int num_passes = thread_data->execution_data->num_passes;
	unsigned long long int i, start, end, total, pass;

	start = num_bytes / num_threads / 8 * local_id;
	end = num_bytes / num_threads / 8 * (local_id + 1);
	total = 0;
	for(pass = 0; pass < num_passes; pass++) {
		for(i = start; i < end - 1; i += 2) {
			total += __builtin_popcountll(bytes[i]);
			total += __builtin_popcountll(bytes[i+1]);
		}
		for(; i < end; i++) {
			total += __builtin_popcountll(bytes[i]);
		}
	}

	thread_data->total = total;
	return NULL;
}

struct thread_data *create_threads(int num_threads, struct execution_data *execution_data) {
	int i;	
	struct thread_data *threads;
	
	threads = malloc(sizeof(threads[0]) * num_threads);
	if(threads == NULL) {
		printf("Unable to malloc space for threads\n");
		exit(EXIT_FAILURE);
	}	

	for(i = 0; i < num_threads; i++) {
		threads[i].local_id = i;
		threads[i].execution_data = execution_data;
		if(pthread_create(&(threads[i].thread), NULL, thread_main, &threads[i]) != 0) {
			printf("Unable to spawn threads\n");
			exit(EXIT_FAILURE);
		}
	}
	
	return threads;
}

static void join_threads(int num_threads, struct thread_data *threads) {
	int i;

	for(i = 0; i < num_threads; i++) {
		if(pthread_join(threads[i].thread, NULL) != 0) {
			printf("Unable to join threads\n");
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv) {
	char *argp;
	int num_threads;
	long long int i, num_bytes, num_passes;
	unsigned long long int factor, total;
	struct thread_data *threads;
	struct execution_data *execution_data;
	struct timeval start, end;
	time_t elapsed_time_us;

	if(argc != 4) {
		printf("usage: %s #threads #mem_size_bytes #passes\n", argv[0]);
		return EXIT_FAILURE;
	}

	errno = 0;
	num_threads = strtol(argv[1], &argp, 0);
	if(errno != 0) {
		perror("Illegal number of threads");
		return EXIT_FAILURE;
	} else if (argv[1] == argp || num_threads < 1) {
		printf("Illegal number of threads\n");
		return EXIT_FAILURE;
	}

	num_bytes = strtol(argv[2], &argp, 0);
	if(errno != 0) {
		perror("Illegal number of bytes");
		return EXIT_FAILURE;
	} else if (argv[2] == argp || num_bytes < 1 || (argp[0] != '\0' && argp[1] != '\0')) {
		printf("Illegal number of bytes\n");
		return EXIT_FAILURE;
	}
	switch(argp[0]) {
		case '\0':
			factor = 0;
			break;
		case 'k': case 'K':
			factor = 1;
			break;
		case 'm': case 'M':
			factor = 2;
			break;
		case 'g': case 'G':
			factor = 3;
			break;
		case 't': case 'T':
			factor = 4;
			break;
		case 'p': case 'P':
			factor = 5;
			break;
		case 'e': case 'E':
			factor = 6;
			break;
		case 'z': case 'Z':
			factor = 7;
			break;
		case 'y': case 'Y':
			 factor = 8;
			 break;
		default:
			printf("Illegal number of bytes\n");
			exit(EXIT_FAILURE);
	}
	num_bytes *= ((unsigned long long int)1 << (factor * 10));
	if(num_bytes % ((unsigned long long int)8 * num_threads) != 0) {
		printf("Number of bytes must be a multiple of (8 * #threads)\n");
		return EXIT_FAILURE;
	}

	num_passes = strtol(argv[3], &argp, 0);
	if(errno != 0) {
		perror("Illegal number of passes");
		return EXIT_FAILURE;
	} else if (argv[3] == argp || num_passes < 1) {
		printf("Illegal number of passes\n");
		return EXIT_FAILURE;
	}

	execution_data = malloc(sizeof(execution_data[0]));
	if(execution_data == NULL) {
		printf("Unable to malloc space for thread data\n");
		return EXIT_FAILURE;
	}
	execution_data->num_threads = num_threads;
	execution_data->num_bytes = num_bytes;
	execution_data->num_passes = num_passes;
	execution_data->bytes = malloc(num_bytes);
	if(execution_data->bytes == NULL) {
		printf("Unable to malloc space for actual bytes to popcnt\n");
		return EXIT_FAILURE;
	}
	for(i = 0; i < num_bytes / 8; i++) {
		execution_data->bytes[i] = i;
	}

	gettimeofday(&start, NULL);
	threads = create_threads(num_threads, execution_data);

	join_threads(num_threads, threads);
	total = 0;
	for(i = 0; i < num_threads; i++) {
		total += threads[i].total;
	}
	gettimeofday(&end, NULL);

	free(threads);
	free(execution_data);

	printf("Total: %llu\n", total);

	elapsed_time_us = end.tv_usec - start.tv_usec;
	elapsed_time_us += 1000000l * (end.tv_sec - start.tv_sec);
	printf("Execution time: %.3fs\n", elapsed_time_us / 1e6);
	
	return EXIT_SUCCESS;
}

