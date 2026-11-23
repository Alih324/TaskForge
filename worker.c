#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "worker.h"
#include "task.h"

int is_prime(long number)
{
    if (number < 2) {
        return 0;
    }

    for (int i = 2; i <= number / 2; i++) {
        if (number % i == 0) {
            return 0;
        }
    }

    return 1;
}

int run_worker(int argc, char *argv[])
{
    Task task;

    if (argc != 3 || strcmp(argv[1], "--worker") != 0) {
        printf("Invalid worker arguments.\n");
        return 1;
    }

    if (strcmp(argv[2], "sum") == 0) {
        task.id = 1;
        task.type = SUM;
    }
    else if (strcmp(argv[2], "prime") == 0) {
        task.id = 2;
        task.type = PRIME_COUNT;
    }
    else {
        printf("Unknown task.\n");
        return 1;
    }

    task.start = 1;
    task.end = 100;

    printf("\n===== Worker %d =====\n", task.id);
    printf("Worker PID: %d\n", (int)getpid());
    printf("Parent PID: %d\n", (int)getppid());

    if (task.type == SUM) {

        long sum = 0;

        for (long i = task.start; i <= task.end; i++) {
            sum += i;
        }

        printf("Task: Sum\n");
        printf("Result: %ld\n", sum);
    }
    else if (task.type == PRIME_COUNT) {

        int count = 0;

        for (long i = task.start; i <= task.end; i++) {
            if (is_prime(i)) {
                count++;
            }
        }

        printf("Task: Prime Count\n");
        printf("Result: %d\n", count);
    }

    printf("Worker finished.\n");

    return 0;
}