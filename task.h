#ifndef TASK_H
#define TASK_H

typedef enum {
    SUM,
    PRIME_COUNT
} TaskType;

typedef struct {
    int id;
    TaskType type;
    long start;
    long end;
} Task;

#endif