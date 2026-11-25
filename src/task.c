#include "task.h"

const char *task_type_name(TaskType type)
{
    switch (type) {
    case TASK_SUM: return "Sum of range";
    case TASK_PRIME_COUNT: return "Prime count";
    }
    return "Unknown";
}

const char *task_status_name(TaskStatus status)
{
    switch (status) {
    case TASK_PENDING: return "PENDING";
    case TASK_RUNNING: return "RUNNING";
    case TASK_SUCCEEDED: return "SUCCEEDED";
    case TASK_FAILED: return "FAILED";
    }
    return "UNKNOWN";
}
