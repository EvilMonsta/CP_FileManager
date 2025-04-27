# === Общие параметры ===
TARGET = fileman
SRC_DIR = src
BUILD_DIR = build
DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES_DEBUG := $(patsubst $(SRC_DIR)/%.c, $(DEBUG_DIR)/%.o, $(SRC_FILES))
OBJ_FILES_RELEASE := $(patsubst $(SRC_DIR)/%.c, $(RELEASE_DIR)/%.o, $(SRC_FILES))

CC = gcc

CFLAGS_DEBUG = -g -ggdb -std=c11 -W -Wall -Wextra -D_GNU_SOURCE
CFLAGS_RELEASE = -std=c11 -pedantic -W -Wall -Wextra -Werror -D_GNU_SOURCE
LDFLAGS = -lncurses

.PHONY: all debug release clean

all: debug

debug: $(DEBUG_DIR)/$(TARGET)

release: $(RELEASE_DIR)/$(TARGET)

$(DEBUG_DIR)/$(TARGET): $(OBJ_FILES_DEBUG)
	$(CC) $^ -o $@ $(LDFLAGS)

$(RELEASE_DIR)/$(TARGET): $(OBJ_FILES_RELEASE)
	$(CC) $^ -o $@ $(LDFLAGS)

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@


clean:
	rm -rf $(DEBUG_DIR)/*.o $(RELEASE_DIR)/*.o
	rm -f $(DEBUG_DIR)/$(TARGET) $(RELEASE_DIR)/$(TARGET)
