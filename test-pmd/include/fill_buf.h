 // *.h file
 // ...
 #ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif

 EXTERNC void fill_buf(char* buf, int buf_len);

