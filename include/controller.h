#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "task.h"

int controller_submit(TaskRegistry *registry, TaskType type, long start, long end);
int controller_run_pending(TaskRegistry *registry);
void controller_print_identity(void);

#endif
