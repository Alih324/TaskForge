#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller.h"
#include "ui.h"

#define INPUT_SIZE 256

static int read_line(const char *prompt, char *buffer, size_t size)
{
    fputs(prompt, stdout);
    fflush(stdout);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else if (!feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
        puts("Input was too long; please try again.");
        buffer[0] = '\0';
    }
    return 1;
}

static int read_number(const char *prompt, long *value)
{
    char buffer[INPUT_SIZE];
    for (;;) {
        if (!read_line(prompt, buffer, sizeof(buffer))) return 0;
        char *begin = buffer;
        while (*begin == ' ' || *begin == '\t') ++begin;
        errno = 0;
        char *end;
        long parsed = strtol(begin, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
        if (begin != end && *end == '\0' && errno != ERANGE) {
            *value = parsed;
            return 1;
        }
        puts("Please enter a valid whole number.");
    }
}

static void submit_task(TaskRegistry *registry)
{
    puts("\nTask type:\n  1. Calculate the sum of a range\n  2. Count primes in a range");
    long choice;
    if (!read_number("Select task type: ", &choice)) return;
    if (choice != 1 && choice != 2) {
        puts("Invalid task type.");
        return;
    }
    long start, end;
    if (!read_number("Start value: ", &start) || !read_number("End value: ", &end)) return;
    if (start > end) {
        puts("Invalid range: the start value must not exceed the end value.");
        return;
    }
    int id = controller_submit(registry, choice == 1 ? TASK_SUM : TASK_PRIME_COUNT,
                               start, end);
    if (id < 0) {
        perror("Could not save task");
        return;
    }
    printf("Task %d submitted: %s, %ld..%ld [%s].\n", id,
           task_type_name(choice == 1 ? TASK_SUM : TASK_PRIME_COUNT), start, end,
           task_status_name(TASK_PENDING));
}

static void print_tasks(const TaskRegistry *registry)
{
    if (registry->count == 0) {
        puts("No tasks have been submitted.");
        return;
    }
    puts("\nID   Task Type          Start           End             PID       Status");
    puts("-------------------------------------------------------------------------");
    for (size_t i = 0; i < registry->count; ++i) {
        const Task *task = &registry->items[i];
        printf("%-4d %-18s %-15ld %-15ld ", task->id, task_type_name(task->type),
               task->start, task->end);
        if (task->pid == 0) printf("%-9s ", "-");
        else printf("%-9ld ", (long)task->pid);
        puts(task_status_name(task->status));
    }
}

static void print_summary(const TaskRegistry *registry)
{
    if (registry->last_submitted == 0) {
        puts("No execution batch has run yet.");
        return;
    }
    printf("Last batch: %zu submitted, %zu succeeded, %zu failed.\n",
           registry->last_submitted, registry->last_succeeded, registry->last_failed);
    for (size_t i = 0; i < registry->count; ++i) {
        const Task *task = &registry->items[i];
        if (task->batch_id != registry->batch_id) continue;
        if (task->pid != 0)
            printf("Task %d | Worker PID %ld | %s", task->id, (long)task->pid,
                   task_status_name(task->status));
        else if (task->status == TASK_FAILED)
            printf("Task %d | no Worker created | FAILED", task->id);
        else continue;
        if (task->term_signal != 0) printf(" (signal %d)", task->term_signal);
        else if (task->exit_code != 0) printf(" (exit %d)", task->exit_code);
        putchar('\n');
    }
}

int run_ui(TaskRegistry *registry)
{
    for (;;) {
        puts("\n===== TaskForge Menu =====\n"
             "1. Submit a New Task\n"
             "2. Run Pending Tasks\n"
             "3. View All Tasks\n"
             "4. View Last Execution Summary\n"
             "5. Exit");
        long choice;
        if (!read_number("Choose an option: ", &choice)) {
            puts("\nEnd of input; exiting TaskForge.");
            return 0;
        }
        switch (choice) {
        case 1: submit_task(registry); break;
        case 2: (void)controller_run_pending(registry); break;
        case 3: print_tasks(registry); break;
        case 4: print_summary(registry); break;
        case 5: puts("TaskForge closed."); return 0;
        default: puts("Invalid menu choice. Select a number from 1 to 5."); break;
        }
    }
}
