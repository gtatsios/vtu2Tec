# Compiler
CC := g++

# Compiler flags
CFLAGS := -Wall -Wextra -std=c++11 -Wno-sign-compare

# Directories
SRC_DIR := src
INCLUDE_DIR := include
TEST_DIR := test
BUILD_DIR := build
TEST_BUILD_DIR := $(BUILD_DIR)/test

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))

# Library
LIB := $(BUILD_DIR)/libvtu2Tec.a

# Targets
.PHONY: all clean

all: $(LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB): $(OBJS) | $(BUILD_DIR)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE_DIR)

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE_DIR)

test: $(LIB) $(TEST_OBJS) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TEST_BUILD_DIR)/test $(TEST_OBJS) $(LIB) -I$(INCLUDE_DIR)
	cp -r $(TEST_DIR)/*.vtu $(TEST_BUILD_DIR)

$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.a $(BUILD_DIR)/test
	rm -rf $(TEST_BUILD_DIR)


