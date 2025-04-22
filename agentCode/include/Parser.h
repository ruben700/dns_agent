/**
 * Parser.h - Command parsing for DNS C2 Agent
 */

 #ifndef PARSER_H
 #define PARSER_H
 
 #include <windows.h>
 
 // Parse a command string
 BOOL parse_command(const char* command_str, char** command, char** args);
 
 // Parse arguments into an array of strings
 BOOL parse_args(const char* args, int* argc, char*** argv);
 
 // Free the arguments array
 void free_args(int argc, char** argv);
 
 #endif // PARSER_H
 