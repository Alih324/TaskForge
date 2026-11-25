#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <sys/types.h>

typedef enum {
    TASK_SUM,
    TASK_PRIME_COUNT
} TaskType;

typedef enum {
    TASK_PENDING,
    TASK_RUNNING,
    TASK_SUCCEEDED,
    TASK_FAILED
} TaskStatus;

typedef struct {
    int id;
    TaskType type;
    long start;
    long end;
    pid_t pid;
    TaskStatus status;
    int exit_code;
    int term_signal;
    size_t batch_id;
} Task;

typedef struct {
    Task *items;
    size_t count;
    size_t capacity;
    int next_id;
    size_t last_submitted;
    size_t last_succeeded;
    size_t last_failed;
    size_t batch_id;
} TaskRegistry;

const char *task_type_name(TaskType type);
const char *task_status_name(TaskStatus status);

#endif
