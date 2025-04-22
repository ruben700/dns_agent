/**
 * Config.c - Configuration for DNS C2 Agent
 * 
 * Manages agent configuration settings including C2 domain,
 * sleep intervals, agent ID, and other operational parameters.
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include "../include/Config.h"
 #include "../include/Utils.h"
 
 // Default configuration values
 #define DEFAULT_C2_DOMAIN "c2.local"
 #define DEFAULT_SLEEP_INTERVAL 10000  // 10 seconds
 #define DEFAULT_MAX_RETRIES 3
 #define DEFAULT_JITTER_PERCENT 20
 #define DEFAULT_KILL_DATE 0  // No kill date
 
 // Configuration structure
 typedef struct {
     char c2_domain[256];
     DWORD sleep_interval;
     char agent_id[64];
     DWORD max_retries;
     DWORD jitter_percent;
     DWORD kill_date;
     char working_dir[MAX_PATH];
     BOOL debug_flag;
 } AgentConfig;
 
 // Global configuration instance
 static AgentConfig config;
 static BOOL config_initialized = FALSE;
 
 /**
  * Initialize the configuration with default values
  */
 static void init_config() {
     if (!config_initialized) {
         strcpy_s(config.c2_domain, sizeof(config.c2_domain), DEFAULT_C2_DOMAIN);
         config.sleep_interval = DEFAULT_SLEEP_INTERVAL;
         sprintf_s(config.agent_id, sizeof(config.agent_id), "agent-%08x", GetTickCount());
         config.max_retries = DEFAULT_MAX_RETRIES;
         config.jitter_percent = DEFAULT_JITTER_PERCENT;
         config.kill_date = DEFAULT_KILL_DATE;
         GetCurrentDirectoryA(MAX_PATH, config.working_dir);
         config.debug_flag = FALSE;
         
         config_initialized = TRUE;
     }
 }
 
 /**
  * Get the C2 domain
  */
 const char* get_c2_domain() {
     if (!config_initialized) {
         init_config();
     }
     return config.c2_domain;
 }
 
 /**
  * Set the C2 domain
  */
 void set_c2_domain(const char* domain) {
     if (!config_initialized) {
         init_config();
     }
     if (domain != NULL) {
         strcpy_s(config.c2_domain, sizeof(config.c2_domain), domain);
     }
 }
 
 /**
  * Get the sleep interval (in milliseconds)
  */
 DWORD get_sleep_interval() {
     if (!config_initialized) {
         init_config();
     }
     
     // Apply jitter to the sleep interval
     if (config.jitter_percent > 0) {
         DWORD jitter_range = (config.sleep_interval * config.jitter_percent) / 100;
         DWORD jitter = get_random_int(0, jitter_range);
         
         // 50% chance of adding or subtracting jitter
         if (get_random_int(0, 1) == 0) {
             return config.sleep_interval - jitter;
         } else {
             return config.sleep_interval + jitter;
         }
     }
     
     return config.sleep_interval;
 }
 
 /**
  * Set the sleep interval (in milliseconds)
  */
 void set_sleep_interval(DWORD interval) {
     if (!config_initialized) {
         init_config();
     }
     config.sleep_interval = interval;
 }
 
 /**
  * Get the agent ID
  */
 const char* get_agent_id() {
     if (!config_initialized) {
         init_config();
     }
     return config.agent_id;
 }
 
 /**
  * Set the agent ID
  */
 void set_agent_id(const char* id) {
     if (!config_initialized) {
         init_config();
     }
     if (id != NULL) {
         strcpy_s(config.agent_id, sizeof(config.agent_id), id);
     }
 }
 
 /**
  * Get the maximum number of retries
  */
 DWORD get_max_retries() {
     if (!config_initialized) {
         init_config();
     }
     return config.max_retries;
 }
 
 /**
  * Set the maximum number of retries
  */
 void set_max_retries(DWORD retries) {
     if (!config_initialized) {
         init_config();
     }
     config.max_retries = retries;
 }
 
 /**
  * Get the jitter percentage (0-100)
  */
 DWORD get_jitter_percent() {
     if (!config_initialized) {
         init_config();
     }
     return config.jitter_percent;
 }
 
 /**
  * Set the jitter percentage (0-100)
  */
 void set_jitter_percent(DWORD percent) {
     if (!config_initialized) {
         init_config();
     }
     // Ensure percent is between 0 and 100
     if (percent > 100) {
         percent = 100;
     }
     config.jitter_percent = percent;
 }
 
 /**
  * Get the kill date (YYYYMMDD format, 0 for none)
  */
 DWORD get_kill_date() {
     if (!config_initialized) {
         init_config();
     }
     return config.kill_date;
 }
 
 /**
  * Set the kill date (YYYYMMDD format, 0 for none)
  */
 void set_kill_date(DWORD date) {
     if (!config_initialized) {
         init_config();
     }
     config.kill_date = date;
 }
 
 /**
  * Check if the agent should exit based on the kill date
  */
 BOOL should_exit_kill_date() {
     if (!config_initialized) {
         init_config();
     }
     
     // If no kill date is set, don't exit
     if (config.kill_date == 0) {
         return FALSE;
     }
     
     // Get current date in YYYYMMDD format
     time_t now = time(NULL);
     struct tm tm_now;
     localtime_s(&tm_now, &now);
     
     DWORD current_date = (tm_now.tm_year + 1900) * 10000 + 
                          (tm_now.tm_mon + 1) * 100 + 
                          tm_now.tm_mday;
     
     // If current date is past kill date, exit
     return (current_date >= config.kill_date);
 }
 
 /**
  * Get the working directory
  */
 const char* get_working_dir() {
     if (!config_initialized) {
         init_config();
     }
     return config.working_dir;
 }
 
 /**
  * Set the working directory
  */
 void set_working_dir(const char* dir) {
     if (!config_initialized) {
         init_config();
     }
     if (dir != NULL) {
         strcpy_s(config.working_dir, sizeof(config.working_dir), dir);
     }
 }
 
 /**
  * Get the debug flag
  */
 BOOL get_debug_flag() {
     if (!config_initialized) {
         init_config();
     }
     return config.debug_flag;
 }
 
 /**
  * Set the debug flag
  */
 void set_debug_flag(BOOL flag) {
     if (!config_initialized) {
         init_config();
     }
     config.debug_flag = flag;
 }
 
 /**
  * Load configuration from a file
  */
 BOOL load_config(const char* filename) {
     if (filename == NULL) {
         return FALSE;
     }
     
     FILE* file;
     if (fopen_s(&file, filename, "rb") != 0) {
         return FALSE;
     }
     
     // Read the configuration structure
     size_t read_size = fread(&config, sizeof(AgentConfig), 1, file);
     fclose(file);
     
     if (read_size != 1) {
         return FALSE;
     }
     
     config_initialized = TRUE;
     return TRUE;
 }
 
 /**
  * Save configuration to a file
  */
 BOOL save_config(const char* filename) {
     if (!config_initialized) {
         init_config();
     }
     
     if (filename == NULL) {
         return FALSE;
     }
     
     FILE* file;
     if (fopen_s(&file, filename, "wb") != 0) {
         return FALSE;
     }
     
     // Write the configuration structure
     size_t write_size = fwrite(&config, sizeof(AgentConfig), 1, file);
     fclose(file);
     
     return (write_size == 1);
 }
 
 /**
  * Reset configuration to defaults
  */
 void reset_config() {
     config_initialized = FALSE;
     init_config();
 }
 