# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -Iinclude

# Libraries
LDFLAGS = -labsl_raw_hash_set -labsl_hash -labsl_city -labsl_low_level_hash -labsl_base -labsl_strings -labsl_raw_logging_internal -labsl_log_severity -lboost_container
LIB_DIRS = -L/usr/local/lib
INCLUDE_DIRS = -I/usr/local/include -I/usr/include/boost

# Directories
SRC_DIR = src
TEST_DIR = test
OBJ_DIR = build
INCLUDE_DIR = include

# Source files
SRCS = $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(TEST_DIR)/*.cpp)
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Output binary
TARGET = excelsior

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIB_DIRS) $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
