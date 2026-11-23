#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "controller.h"

#define WORKER_COUNT 2

int run_controller(char *program_path)
{
    pid_t worker_pids[WORKER_COUNT];
    int worker_count = 0;
    int success = 1;

    printf("===== TaskForge Controller =====\n");
    printf("Controller PID: %d\n", (int)getpid());

    fflush(stdout);

    for (int i = 0; i < WORKER_COUNT; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            success = 0;
            break;
        }

        if (pid == 0) {

            char *args[] = {
                program_path,
                "--worker",
                (i == 0) ? "sum" : "prime",
                NULL
            };

            execv(program_path, args);

            perror("execv");
            exit(1);
        }

        worker_pids[worker_count] = pid;
        worker_count++;

        printf("Created Worker PID: %d\n", (int)pid);
    }

    printf("\n===== Waiting for Workers =====\n");

    for (int i = 0; i < worker_count; i++) {

        int status;

        if (waitpid(worker_pids[i], &status, 0) == -1) {
            perror("waitpid");
            success = 0;
        }
        else if (WIFEXITED(status)) {

            int exit_status = WEXITSTATUS(status);

            printf("Worker %d finished. Exit status: %d\n",
                   (int)worker_pids[i], exit_status);

            if (exit_status != 0) {
                success = 0;
            }
        }
        else {
            printf("Worker %d did not finish normally.\n",
                   (int)worker_pids[i]);

            success = 0;
        }
    }

    if (worker_count != WORKER_COUNT) {
        success = 0;
    }

    printf("\nController finished.\n");

    return success ? 0 : 1;
}