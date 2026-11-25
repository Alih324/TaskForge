#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "controller.h"

void controller_print_identity(void)
{
    printf("===== TaskForge Controller =====\nController PID: %ld\nController PPID: %ld\n",
           (long)getpid(), (long)getppid());
}

int controller_init(TaskRegistry *registry)
{
    memset(registry, 0, sizeof(*registry));
    registry->next_id = 1;
    return 0;
}

void controller_destroy(TaskRegistry *registry)
{
    free(registry->items);
    memset(registry, 0, sizeof(*registry));
}

int controller_submit(TaskRegistry *registry, TaskType type, long start, long end)
{
    if ((type != TASK_SUM && type != TASK_PRIME_COUNT) || start > end) {
        errno = EINVAL;
        return -1;
    }
    if (registry->next_id <= 0 || registry->next_id == INT_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    if (registry->count == registry->capacity) {
        size_t capacity = registry->capacity == 0 ? 8 : registry->capacity * 2;
        if (capacity < registry->capacity || capacity > SIZE_MAX / sizeof(*registry->items)) {
            errno = ENOMEM;
            return -1;
        }
        Task *items = realloc(registry->items, capacity * sizeof(*items));
        if (items == NULL) return -1;
        registry->items = items;
        registry->capacity = capacity;
    }
    Task task = {0};
    task.id = registry->next_id++;
    task.type = type;
    task.start = start;
    task.end = end;
    task.status = TASK_PENDING;
    registry->items[registry->count++] = task;
    return task.id;
}

static char *worker_executable_path(const char *argv0)
{
    char executable[PATH_MAX + 1];
    ssize_t length = readlink("/proc/self/exe", executable, PATH_MAX);
    if (length >= 0) {
        executable[length] = '\0';
    } else {
        char *resolved = realpath(argv0, NULL);
        if (resolved == NULL) return NULL;
        if (strlen(resolved) >= sizeof(executable)) {
            free(resolved);
            errno = ENAMETOOLONG;
            return NULL;
        }
        strcpy(executable, resolved);
        free(resolved);
    }

    char *slash = strrchr(executable, '/');
    if (slash == NULL) {
        errno = EINVAL;
        return NULL;
    }
    *slash = '\0';
    const char suffix[] = "/taskforge_worker";
    size_t needed = strlen(executable) + sizeof(suffix);
    char *path = malloc(needed);
    if (path == NULL) return NULL;
    (void)snprintf(path, needed, "%s%s", executable, suffix);
    return path;
}

int controller_run_pending(TaskRegistry *registry)
{
    size_t pending = 0;
    for (size_t i = 0; i < registry->count; ++i)
        if (registry->items[i].status == TASK_PENDING) ++pending;
    if (pending == 0) {
        puts("There are no pending tasks to run.");
        return 0;
    }
    char *worker_path = worker_executable_path("./build/taskforge");
    if (worker_path == NULL) {
        perror("locating taskforge_worker");
        return -1;
    }

    registry->last_submitted = pending;
    registry->last_succeeded = 0;
    registry->last_failed = 0;
    ++registry->batch_id;
    printf("\n===== Starting %zu task(s) =====\n", pending);
    for (size_t i = 0; i < registry->count; ++i) {
        Task *task = &registry->items[i];
        if (task->status != TASK_PENDING) continue;

        char id[32], start[64], end[64];
        (void)snprintf(id, sizeof(id), "%d", task->id);
        (void)snprintf(start, sizeof(start), "%ld", task->start);
        (void)snprintf(end, sizeof(end), "%ld", task->end);
        const char *kind = task->type == TASK_SUM ? "sum" : "prime";
        task->batch_id = registry->batch_id;
        fflush(stdout);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            task->status = TASK_FAILED;
            ++registry->last_failed;
            continue;
        }
        if (pid == 0) {
            char *const args[] = {worker_path, id, (char *)kind, start, end, NULL};
            execvp(worker_path, args);
            perror("execvp taskforge_worker");
            _exit(127);
        }

        task->pid = pid;
        task->status = TASK_RUNNING;
        printf("Controller created Worker PID %ld for task %d (%s).\n",
               (long)pid, task->id, task_type_name(task->type));
        fflush(stdout);
    }

    for (size_t i = 0; i < registry->count; ++i) {
        Task *task = &registry->items[i];
        if (task->batch_id != registry->batch_id || task->status != TASK_RUNNING) continue;

        int status;
        pid_t result;
        do {
            result = waitpid(task->pid, &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            task->status = TASK_FAILED;
            ++registry->last_failed;
            continue;
        }
        if (WIFEXITED(status)) {
            task->exit_code = WEXITSTATUS(status);
            if (task->exit_code == 0) {
                task->status = TASK_SUCCEEDED;
                ++registry->last_succeeded;
            } else {
                task->status = TASK_FAILED;
                ++registry->last_failed;
                printf("Worker PID %ld for task %d exited with status %d.\n",
                       (long)task->pid, task->id, task->exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            task->status = TASK_FAILED;
            task->term_signal = WTERMSIG(status);
            ++registry->last_failed;
            printf("Worker PID %ld for task %d was terminated by signal %d.\n",
                   (long)task->pid, task->id, task->term_signal);
        }
    }

    puts("\n===== Execution Summary =====");
    printf("Submitted: %zu | Succeeded: %zu | Failed: %zu\n",
           registry->last_submitted, registry->last_succeeded, registry->last_failed);
    for (size_t i = 0; i < registry->count; ++i) {
        const Task *task = &registry->items[i];
        if (task->batch_id != registry->batch_id) continue;
        if (task->status == TASK_FAILED && task->pid == 0)
            printf("Task %d | No Worker created | FAILED\n", task->id);
        else if (task->status == TASK_SUCCEEDED || task->status == TASK_FAILED)
            printf("Task %d | PID %ld | %s\n", task->id, (long)task->pid,
                   task_status_name(task->status));
    }
    free(worker_path);
    return registry->last_failed == 0 ? 0 : -1;
}
