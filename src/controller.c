#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "controller.h"

#define WORKER_PATH "./build/taskforge_worker"

void controller_print_identity(void)
{
    printf("===== TaskForge Controller =====\n");
    printf("Controller PID: %ld\n", (long)getpid());
    printf("Controller PPID: %ld\n", (long)getppid());
}

int controller_submit(TaskRegistry *registry, TaskType type, long start, long end)
{
    if ((type != TASK_SUM && type != TASK_PRIME_COUNT) || start > end) {
        errno = EINVAL;
        return -1;
    }

    if (registry->count >= MAX_TASKS) {
        errno = ENOSPC;
        return -1;
    }

    Task *task = &registry->items[registry->count];

    task->id = (int)registry->count + 1;
    task->type = type;
    task->start = start;
    task->end = end;
    task->pid = 0;
    task->status = TASK_PENDING;
    task->exit_code = 0;
    task->term_signal = 0;
    task->batch_id = 0;

    registry->count++;
    return task->id;
}

int controller_run_pending(TaskRegistry *registry)
{
    size_t pending = 0;

    for (size_t i = 0; i < registry->count; i++) {
        if (registry->items[i].status == TASK_PENDING) {
            pending++;
        }
    }

    if (pending == 0) {
        puts("There are no pending tasks to run.");
        return 0;
    }

    registry->last_submitted = pending;
    registry->last_succeeded = 0;
    registry->last_failed = 0;
    registry->batch_id++;

    char worker_path[] = WORKER_PATH;

    printf("\n===== Starting %zu task(s) =====\n", pending);
    fflush(stdout);

    /* Start every Worker before waiting, so they can run concurrently. */
    for (size_t i = 0; i < registry->count; i++) {
        Task *task = &registry->items[i];

        if (task->status != TASK_PENDING) {
            continue;
        }

        char id[20];
        char start[40];
        char end[40];
        char *type;

        snprintf(id, sizeof(id), "%d", task->id);
        snprintf(start, sizeof(start), "%ld", task->start);
        snprintf(end, sizeof(end), "%ld", task->end);

        if (task->type == TASK_SUM) {
            type = "sum";
        } else {
            type = "prime";
        }

        task->batch_id = registry->batch_id;

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            task->status = TASK_FAILED;
            registry->last_failed++;
            continue;
        }

        if (pid == 0) {
            char *args[] = {worker_path, id, type, start, end, NULL};

            execvp(worker_path, args);
            perror("execvp");
            _exit(127);
        }

        task->pid = pid;
        task->status = TASK_RUNNING;

        printf("Controller created Worker PID %ld for task %d.\n",
               (long)pid, task->id);
        fflush(stdout);
    }

    /* Wait for each Worker and collect its exit status. */
    for (size_t i = 0; i < registry->count; i++) {
        Task *task = &registry->items[i];

        if (task->batch_id != registry->batch_id ||
            task->status != TASK_RUNNING) {
            continue;
        }

        int status;
        pid_t result;

        do {
            result = waitpid(task->pid, &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            task->status = TASK_FAILED;
            registry->last_failed++;
            continue;
        }

        if (WIFEXITED(status)) {
            task->exit_code = WEXITSTATUS(status);

            if (task->exit_code == 0) {
                task->status = TASK_SUCCEEDED;
                registry->last_succeeded++;
            } else {
                task->status = TASK_FAILED;
                registry->last_failed++;
            }
        } else if (WIFSIGNALED(status)) {
            task->term_signal = WTERMSIG(status);
            task->status = TASK_FAILED;
            registry->last_failed++;
        } else {
            task->status = TASK_FAILED;
            registry->last_failed++;
        }
    }

    puts("\n===== Execution Summary =====");

    printf("Submitted: %zu | Succeeded: %zu | Failed: %zu\n",
           registry->last_submitted,
           registry->last_succeeded,
           registry->last_failed);

    for (size_t i = 0; i < registry->count; i++) {
        Task *task = &registry->items[i];

        if (task->batch_id != registry->batch_id) {
            continue;
        }

        if (task->pid == 0) {
            printf("Task %d | No Worker created | FAILED\n", task->id);
        } else {
            printf("Task %d | Worker PID %ld | %s\n",
                   task->id,
                   (long)task->pid,
                   task_status_name(task->status));
        }
    }

    return registry->last_failed == 0 ? 0 : -1;
}
