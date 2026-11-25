
CC ?= cc

CPPFLAGS ?= -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

BUILD_DIR := build
CONTROLLER := $(BUILD_DIR)/taskforge
WORKER := $(BUILD_DIR)/taskforge_worker

CONTROLLER_SOURCES := src/main.c src/controller.c src/task.c src/ui.c
WORKER_SOURCES := src/worker_main.c src/worker.c src/task.c
HEADERS := include/task.h include/controller.h include/worker.h include/ui.h

.PHONY: all run clean

all: $(CONTROLLER) $(WORKER)

$(BUILD_DIR):
	mkdir -p $@

$(CONTROLLER): $(CONTROLLER_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CONTROLLER_SOURCES) -o $@

$(WORKER): $(WORKER_SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WORKER_SOURCES) -o $@

run: all
	./$(CONTROLLER)

clean:
	rm -f $(CONTROLLER) $(WORKER)
