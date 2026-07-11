CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

ROOT_DIR := $(CURDIR)
BUILD_DIR := $(ROOT_DIR)/build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

WM_TARGET := $(BIN_DIR)/turtle-wm
FILE_MANAGER_TARGET := $(BIN_DIR)/turtle-file-manager

WM_SRC_FILES := \
	ui/wm/src/main.cpp \
	ui/wm/src/window_manager.cpp \
	ui/bckg/src/background.cpp \
	ui/cursor/src/cursor.cpp \
	ui/tabs/src/w-tab.cpp \
	ui/panel/src/panel.cpp \
	ui/panel/src/turtle_button.cpp \
	ui/panel/src/calendar_button.cpp \
	ui/panel/src/browser_button.cpp \
	ui/panel/src/calculator_button.cpp \
	ui/panel/src/file_manager_button.cpp \
	ui/panel/src/battery_button.cpp \
	ui/panel/src/shutdown_button.cpp \
	ui/panel/src/panel_placeholder.cpp \
	controls/rc/src/tabs/rc1.cpp \
	session/src/session_manager.cpp

FILE_MANAGER_SRC_FILES := \
	tools/file-manager/src/main.cpp \
	tools/file-manager/src/file_system.cpp \
	tools/file-manager/src/navigation_controller.cpp \
	tools/file-manager/src/icon_loader.cpp \
	tools/file-manager/src/sidebar.cpp \
	tools/file-manager/src/directory_view.cpp \
	tools/file-manager/src/preview_panel.cpp \
	tools/file-manager/src/toolbar.cpp \
	tools/file-manager/src/file_manager_window.cpp

INCLUDE_DIRS := \
	ui/wm/include \
	ui/bckg/include \
	ui/cursor/include \
	ui/tabs/include \
	ui/panel/include \
	controls/rc/include \
	session/include \
	tools/file-manager/include

CPPFLAGS += $(foreach dir,$(INCLUDE_DIRS),-I$(ROOT_DIR)/$(dir))
CPPFLAGS += $(shell pkg-config --cflags x11 imlib2)
LDFLAGS += $(shell pkg-config --libs x11 imlib2)
FILE_MANAGER_CPPFLAGS := $(shell pkg-config --cflags gtk+-3.0 gdk-pixbuf-2.0)
FILE_MANAGER_LDFLAGS := $(shell pkg-config --libs gtk+-3.0 gdk-pixbuf-2.0) -lX11

WM_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(WM_SRC_FILES))
FILE_MANAGER_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(FILE_MANAGER_SRC_FILES))

.PHONY: all clean run

all: $(WM_TARGET) $(FILE_MANAGER_TARGET)

$(WM_TARGET): $(WM_OBJECTS) | $(BIN_DIR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(WM_OBJECTS) -o $@ $(LDFLAGS)

$(FILE_MANAGER_TARGET): $(FILE_MANAGER_OBJECTS) | $(BIN_DIR)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(FILE_MANAGER_OBJECTS) -o $@ $(FILE_MANAGER_LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/tools/file-manager/src/%.o: tools/file-manager/src/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FILE_MANAGER_CPPFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

run:
	./script/run.sh
