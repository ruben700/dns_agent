/**
 * Config.h - Configuration for DNS C2 Agent
 */

 #ifndef CONFIG_H
 #define CONFIG_H
 
 #include <windows.h>
 
 // Get the C2 domain
 const char* get_c2_domain();
 
 // Set the C2 domain
 void set_c2_domain(const char* domain);
 
 // Get the sleep interval (in milliseconds)
 DWORD get_sleep_interval();
 
 // Set the sleep interval (in milliseconds)
 void set_sleep_interval(DWORD interval);
 
 // Get the agent ID
 const char* get_agent_id();
 
 // Set the agent ID
 void set_agent_id(const char* id);
 
 // Get the maximum number of retries
 DWORD get_max_retries();
 
 // Set the maximum number of retries
 void set_max_retries(DWORD retries);
 
 // Get the jitter percentage (0-100)
 DWORD get_jitter_percent();
 
 // Set the jitter percentage (0-100)
 void set_jitter_percent(DWORD percent);
 
 // Get the kill date (YYYYMMDD format, 0 for none)
 DWORD get_kill_date();
 
 // Set the kill date (YYYYMMDD format, 0 for none)
 void set_kill_date(DWORD date);
 
 // Check if the agent should exit based on the kill date
 BOOL should_exit_kill_date();
 
 // Get the working directory
 const char* get_working_dir();
 
 // Set the working directory
 void set_working_dir(const char* dir);
 
 // Get the debug flag
 BOOL get_debug_flag();
 
 // Set the debug flag
 void set_debug_flag(BOOL flag);
 
 // Load configuration from a file
 BOOL load_config(const char* filename);
 
 // Save configuration to a file
 BOOL save_config(const char* filename);
 
 // Reset configuration to defaults
 void reset_config();
 
 #endif // CONFIG_H
 
 