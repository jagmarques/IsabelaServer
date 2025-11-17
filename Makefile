CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -g
INCLUDES = -Iinclude
LDFLAGS ?=
LIBS =

SRC_SERVER = server.c src/server/context.c src/server/data_store.c src/server/network.c src/server/subscription.c

SRC_CLIENT = client.c

TEST_SRCS = tests/test_registry.c

BUILD_DIR = build

SERVER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_SERVER))
CLIENT_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_CLIENT))
TEST_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEST_SRCS))

all: $(BUILD_DIR)/server $(BUILD_DIR)/client

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/src/server $(BUILD_DIR)/src/client $(BUILD_DIR)/tests

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/server: $(SERVER_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/client: $(CLIENT_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_registry: $(TEST_OBJ) $(BUILD_DIR)/src/server/context.o $(BUILD_DIR)/src/server/data_store.o $(BUILD_DIR)/src/server/subscription.o
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

.PHONY: clean test format

clean:
	rm -rf $(BUILD_DIR)
	rm -f compile_commands.json

format:
	clang-format -i src/**/*.c include/**/*.h

test: $(BUILD_DIR)/test_registry
	ISABELA_DATA_FILE=fixtures/sample_students.json $(BUILD_DIR)/test_registry
