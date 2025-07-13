# Simple Makefile
# root
#  |
#  +--- BIN_DIR
#  |
#  +--- *INC_DIR
#  |
#  +--- *LIB_DIR
#  |
#  +--- OBJ_DIR
#  |
#  +--- *SRC_DIR
#
#  * Mandatory
#

ifeq ($(OS), Windows_NT)
	detected_OS := Windows
	EXE_EXT := .exe
	RM := del /Q
	RMDIR := rmdir /S /Q
	MKDIR := mkdir
	PATH_SEP := \\
	PRJ_DIR := $(notdir $(CURDIR))
else
	detected_OS := $(shell uname -s)
	EXE_EXT := 
	RM := rm -f
	RMDIR := rm -rf
	MKDIR := mkdir -p
	PATH_SEP := /
	PRJ_DIR := $(shell basename "$(CURDIR)")
endif


BIN_DIR := bin
DATA_DIR := data
SRC_DIR := src
CORE_DIR := $(SRC_DIR)/core
UTILS_DIR := $(SRC_DIR)/utils
INC_DIR := $(SRC_DIR)/include
OBJ_DIR := obj

EXE := $(BIN_DIR)/robot_simulation$(EXE_EXT)

SRC := $(wildcard $(CORE_DIR)/*.c) $(wildcard $(UTILS_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))


CC       := gcc                                     # C Compiler
CPPFLAGS := -I$(INC_DIR) -MMD -MP					# C Pre-Processor flags
CFLAGS   := -Wall -Wextra -O2 -std=gnu2x -g3	    # C compiler flags
LDFLAGS  :=			            					# C linker flags
LDLIBS   := -lm	-pthread				            # Libs need

ifeq ($(detected_OS),Windows)
	LDLIBS += -lws2_32
endif

all: $(EXE)

vars:
	@echo " "
	@echo "PRJ_DIR = $(PRJ_DIR)"
	@echo "BIN_DIR = $(BIN_DIR)"
	@echo "DATA_DIR = $(DATA_DIR)"
	@echo "CORE_DIR = $(CORE_DIR)"
	@echo "UTILS_DIR = $(UTILS_DIR)"
	@echo "INC_DIR = $(INC_DIR)"
	@echo "LIB_DIR = $(LIB_DIR)"
	@echo "OBJ_DIR = $(OBJ_DIR)"
	@echo " "
	@echo "EXE = $(EXE)"
	@echo "SRC = $(SRC)"
	@echo "OBJ = $(OBJ)"
	@echo " "
	@echo "CC = $(CC)"
	@echo "CPPFLAGS = $(CPPFLAGS)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "LDLIBS = $(LDLIBS)"
	@echo " "

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR) $(OBJ_DIR):
ifeq ($(detected_OS),Windows)
	@if not exist $@ $(MKDIR) $@
else
	$(MKDIR) $@
endif

$(OBJ_DIR)/core/%.o: $(CORE_DIR)/%.c | $(OBJ_DIR)/core
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(OBJ_DIR)/utils/%.o: $(UTILS_DIR)/%.c | $(OBJ_DIR)/utils
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/core $(OBJ_DIR)/utils:
	$(MKDIR) $@

.PHONY: all clean vars help


clean:
ifeq ($(detected_OS),Windows)
	@if exist $(subst /,\,$(EXE)) del /Q "$(subst /,\,$(EXE))"
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
else
	-$(RM) $(EXE)
	-$(RMDIR) $(OBJ_DIR)
	-$(RM) $(DATA_DIR)/*.txt
endif

help:
	@echo "Available targets:"
	@echo "all - Build the project(default)"
	@echo "clean - Remove Built Files"
	@echo "vars - Show Makefile variables"
	@echo "help - Show This Message "
	@echo ""
	@echo "detected OS: $(detected_OS)"



-include $(OBJ:.o=.d)

