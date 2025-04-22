/**
 * Utils.h - Utility functions for DNS C2 Agent
 */

 #ifndef UTILS_H
 #define UTILS_H
 
 #include <windows.h>
 
 // Base64 encoding/decoding
 char* base64_encode(const unsigned char* data, size_t input_length, size_t* output_length);
 unsigned char* base64_decode(const char* data, size_t input_length, size_t* output_length);
 
 // UUID generation
 char* generate_uuid();
 
 // Time functions
 char* get_timestamp();
 
 // String manipulation
 char** str_split(const char* str, char delim, int* count);
 void free_tokens(char** tokens, int count);
 char* url_encode(const char* str);
 void str_tolower(char* str);
 void str_toupper(char* str);
 BOOL str_startswith(const char* str, const char* prefix);
 BOOL str_endswith(const char* str, const char* suffix);
 char* str_trim(const char* str);
 
 // System information
 char* get_hostname();
 char* get_username();
 char* get_current_directory();
 
 // File operations
 BOOL file_exists(const char* filename);
 BOOL directory_exists(const char* dirname);
 BOOL create_directory(const char* dirname);
 long get_file_size(const char* filename);
 unsigned char* read_file(const char* filename, size_t* size);
 BOOL write_file(const char* filename, const unsigned char* data, size_t size);
 BOOL append_to_file(const char* filename, const unsigned char* data, size_t size);
 BOOL delete_file(const char* filename);
 BOOL copy_file(const char* source, const char* destination, BOOL overwrite);
 BOOL move_file(const char* source, const char* destination);
 
 // Miscellaneous
 void sleep_ms(DWORD milliseconds);
 int get_random_int(int min, int max);
 
 #endif // UTILS_H