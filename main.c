#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller.h"
#include "worker.h"

int main(int argc, char *argv[])
{
    if (argc == 1) {
        return run_controller(argv[0]);
    }

    if (strcmp(argv[1], "--worker") == 0) {
        return run_worker(argc, argv);
    }

    printf("Invalid command.\n");
    printf("Usage: ./taskforge\n");

    return EXIT_FAILURE;
}