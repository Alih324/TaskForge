#include <stdio.h>

#include "controller.h"
#include "task.h"
#include "ui.h"

int main(void)
{
    TaskRegistry registry;
    controller_print_identity();
    if (controller_init(&registry) != 0) {
        fputs("TaskForge: could not initialize task registry.\n", stderr);
        return 1;
    }

    int result = run_ui(&registry);
    controller_destroy(&registry);
    return result;
}
