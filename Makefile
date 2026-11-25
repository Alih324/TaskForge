
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L -Iinclude

BUILD_DIR = build
CONTROLLER = $(BUILD_DIR)/taskforge
WORKER = $(BUILD_DIR)/taskforge_worker

CONTROLLER_FILES = src/main.c src/controller.c src/task.c src/ui.c
WORKER_FILES = src/worker_main.c src/worker.c src/task.c

.PHONY: all run clean

all: $(CONTROLLER) $(WORKER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CONTROLLER): $(CONTROLLER_FILES) include/task.h include/controller.h include/ui.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CONTROLLER_FILES) -o $(CONTROLLER)

$(WORKER): $(WORKER_FILES) include/task.h include/worker.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(WORKER_FILES) -o $(WORKER)

run: all
	./$(CONTROLLER)

clean:
	rm -f $(CONTROLLER) $(WORKER)
