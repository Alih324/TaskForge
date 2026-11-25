# TaskForge

TaskForge is a small Linux/POSIX C application for an Operating Systems Phase 1 project. It demonstrates a Controller creating one child process per task, replacing each child with a separate Worker executable, and collecting each child's termination status.

## Phase 1 objective and features

- Create tasks interactively and keep their registry in memory for the current session.
- Run each pending task in its own Worker process; multiple Workers are launched before the Controller waits for them.
- Calculate an inclusive range sum or count primes in an inclusive range.
- Track each task's ID, type, range, Worker PID, and `PENDING`, `RUNNING`, `SUCCEEDED`, or `FAILED` status.
- Display the Controller and Worker PID/PPID and a summary of the latest batch.
- Reap every child created by a batch, including when a later `fork()` fails.

## Process roles and workflow

The **Controller** owns the task registry, menu interaction, process creation, PID tracking, wait/status handling, and summaries. The **Worker** validates its command-line task details, reports its identity, computes the requested result, and exits with status 0 on success or a nonzero status on error.

For each pending task, the Controller calls `fork()`. In the child it calls `execvp()` on the sibling `taskforge_worker` executable and passes the task ID, type, and range as arguments. The parent records the PID and continues launching the rest of the batch. It then calls `waitpid()` until every child is collected, mapping each returned PID and its exit or signal status back to the correct task. Collecting whichever Worker finishes first avoids leaving completed Workers unreaped while another task runs. No IPC is needed because task inputs are passed as arguments and results are displayed by the Workers.

## Project structure

```text
TaskForge/
├── src/
│   ├── main.c             # Controller entry point
│   ├── controller.c       # Registry and process management
│   ├── ui.c               # Interactive terminal menu and display
│   ├── worker_main.c      # Separate Worker executable entry point
│   └── worker.c           # Worker validation and calculations
├── include/
│   ├── task.h             # Shared task, status, and registry types
│   ├── controller.h
│   ├── ui.h
│   └── worker.h
├── build/                 # Generated executables (ignored by Git)
├── screenshots/           # Submission screenshots
├── Makefile
└── README.md
```

## Task types and examples

1. **Sum of range**: adds every integer from start through end. `1..100` produces `5050`.
2. **Prime count**: counts prime integers from start through end. Values below 2 are not prime; `1..100` produces `25`.

Both task types require `start <= end`. Sum overflow for the platform's `long` type is reported as a failed task instead of invoking signed overflow. Prime results use an unsigned count. Extremely large ranges may take a long time to calculate.

## System calls

| Call | Use in TaskForge |
|---|---|
| `fork()` | Creates one child for each pending task. The returned child PID is stored by the Controller. |
| `execvp()` | Replaces the child process image with the separate Worker program. Its executable path is derived from the running Controller's location, so running `build/taskforge` from another working directory still finds its sibling Worker on Linux. |
| `getpid()` | Reports the Controller or Worker process ID. |
| `getppid()` | Reports the Controller's parent and each Worker's parent, making the hierarchy visible. |
| `waitpid()` | Collects any completed child in the batch; the returned PID associates its normal exit code or signal termination with the correct task. Interrupted calls are retried. |

## Build and run

Requirements: a Linux/POSIX environment and a C compiler such as GCC or Clang.

```sh
make clean
make
make run
```

`make` creates `build/taskforge` and `build/taskforge_worker`. You can also start the Controller directly with `./build/taskforge`. The Worker is launched by the Controller and is not the interactive entry point.

## Example interaction

```text
===== TaskForge Menu =====
1. Submit a New Task
2. Run Pending Tasks
3. View All Tasks
4. View Last Execution Summary
5. Exit
Choose an option: 1
Task type:
  1. Calculate the sum of a range
  2. Count primes in a range
Select task type: 1
Start value: 1
End value: 100
Task 1 submitted: Sum of range, 1..100 [PENDING].
```

Submitting a prime task for the same range and selecting **Run Pending Tasks** starts two Workers. Their output includes distinct PIDs, the Controller PPID, and results `5050` and `25`; the Controller then prints the exit outcomes and summary.

## Input and limitations

Input is read a line at a time and parsed with `strtol()`. Invalid menu options, malformed numbers, reversed ranges, EOF, and empty task lists are handled without crashing. The task registry and execution history last only until the Controller exits. This Phase 1 application does not include persistence, a Worker Pool, or interprocess communication. Work is sequential within each Worker, although Workers in the same batch run concurrently as separate processes.

## Screenshot checklist

The `screenshots/` directory contains `phase1_execution.png` from the earlier hardcoded demo. It is retained as an existing project artifact, but it does not show the current menu and should be replaced or supplemented with genuine screenshots of this version before submission. No screenshots are generated or claimed by the code changes.

1. Controller identity and the menu, followed by submitting at least two tasks.
2. A batch with two Workers showing each Worker PID/PPID and the sum and prime results.
3. The Controller execution summary and **View All Tasks** showing final statuses.
4. Optional: invalid input and the no-pending-tasks message.
