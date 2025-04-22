// Command.h
#include <windows.h>
#include <string.h>
#include <stdlib.h> 

#include "cd.h"
#include "pwd.h"
#include "ls.h"
#include "cp.h"
#include "whoami.h"
#include "mkdir.h"

#include "Transport.h"
#include "Utils.h"
#include "Config.h"


#ifndef COMMAND_H
#define COMMAND_H

// Command function type
typedef char* (*CommandFunction)(const char* args);

// Initialize the command module
BOOL command_init();

// Clean up the command module
void command_cleanup();

// Execute a command
char* execute_command(const char* command_str);

// Main command processing loop
void command_loop();
#endif

