#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "task.h"
#include "worker.h"

static int parse_long(const char *text, long *value)
{
    char *end;
    errno = 0;
    long parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0') return -1;
    *value = parsed;
    return 0;
}

static int is_prime(long number)
{
    if (number < 2) return 0;
    for (long divisor = 2; divisor <= number / divisor; ++divisor)
        if (number % divisor == 0) return 0;
    return 1;
}

static int calculate_sum(long start, long end, long *result)
{
    long sum = 0;
    for (long value = start;; ++value) {
        if ((value > 0 && sum > LONG_MAX - value) ||
            (value < 0 && sum < LONG_MIN - value)) return -1;
        sum += value;
        if (value == end) break;
    }
    *result = sum;
    return 0;
}

int run_worker(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: taskforge_worker TASK_ID sum|prime START END\n");
        return 2;
    }

    long id_value, start, end;
    if (parse_long(argv[1], &id_value) != 0 || id_value < 1 || id_value > INT_MAX ||
        parse_long(argv[3], &start) != 0 || parse_long(argv[4], &end) != 0 || start > end) {
        fputs("Worker: invalid task arguments or range.\n", stderr);
        return 2;
    }
    TaskType type;
    if (strcmp(argv[2], "sum") == 0) type = TASK_SUM;
    else if (strcmp(argv[2], "prime") == 0) type = TASK_PRIME_COUNT;
    else {
        fprintf(stderr, "Worker: unknown task type '%s'.\n", argv[2]);
        return 2;
    }

    printf("\n===== Worker for Task %ld =====\n", id_value);
    printf("Worker PID: %ld | Parent PID: %ld\n", (long)getpid(), (long)getppid());
    printf("Task: %s | Range: %ld..%ld\n", task_type_name(type), start, end);
    if (type == TASK_SUM) {
        long sum;
        if (calculate_sum(start, end, &sum) != 0) {
            fflush(stdout);
            fputs("Worker: sum exceeds the range of long.\n", stderr);
            return 1;
        }
        printf("Result: %ld\n", sum);
    } else {
        unsigned long count = 0;
        for (long value = start;; ++value) {
            if (is_prime(value)) ++count;
            if (value == end) break;
        }
        printf("Result: %lu prime number(s)\n", count);
    }
    puts("Worker completed successfully.");
    return 0;
}
