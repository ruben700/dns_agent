# Makefile for DNS C2 Agent

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lws2_32 -liphlpapi -ldnsapi -lshlwapi

# Source and object directories
SRCDIR = source
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c) main.c
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(OBJECTS))

# Target executable
TARGET = $(BINDIR)/dns_agent.exe

# Default target
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main.c
$(OBJDIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Clean build files
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Run the agent
run: all
	$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: all

.PHONY: all directories clean run debug
