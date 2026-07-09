CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

ROOT_DIR := $(CURDIR)
BUILD_DIR := $(ROOT_DIR)/build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

TARGET := $(BIN_DIR)/turtle-wm

SRC_FILES := \
	ui/wm/src/main.cpp \
	ui/wm/src/window_manager.cpp \
	ui/bckg/src/background.cpp \
	ui/cursor/src/cursor.cpp \
	ui/tabs/src/w-tab.cpp \
	controls/rc/src/tabs/rc1.cpp \
	session/src/session_manager.cpp

INCLUDE_DIRS := \
	ui/wm/include \
	ui/bckg/include \
	ui/cursor/include \
	ui/tabs/include \
	controls/rc/include \
	session/include

CPPFLAGS += $(foreach dir,$(INCLUDE_DIRS),-I$(ROOT_DIR)/$(dir))
CPPFLAGS += $(shell pkg-config --cflags x11 imlib2)
LDFLAGS += $(shell pkg-config --libs x11 imlib2)

OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

run:
	./script/run.sh
