[1mdiff --git a/include/tinycsocket.h b/include/tinycsocket.h[m
[1mindex 5036e60..b06ef0c 100644[m
[1m--- a/include/tinycsocket.h[m
[1m+++ b/include/tinycsocket.h[m
[36m@@ -29,7 +29,7 @@[m
 #ifndef TINYCSOCKET_INTERNAL_H_[m
 #define TINYCSOCKET_INTERNAL_H_[m
 [m
[31m-static const char* const TCS_VERSION_TXT = "v0.3.59";[m
[32m+[m[32mstatic const char* const TCS_VERSION_TXT = "v0.3.60";[m
 static const char* const TCS_LICENSE_TXT =[m
     "Copyright 2018 Markus Lindelöw\n"[m
     "\n"[m
[36m@@ -191,10 +191,9 @@[m [mtypedef unsigned int TcsInterfaceId; // TODO: GUID is used for in vista at newer[m
 #elif defined(TINYCSOCKET_USE_POSIX_IMPL)[m
 [m
 #if defined(TINYCSOCKET_IMPLEMENTATION) || defined(TCS_DEFINE_POSIX_MACROS)[m
[31m-// POSIX feature test macros must be set before any system header is included.[m
[31m-// Without these, glibc's <features.h> (pulled in by <stdbool.h>) locks in[m
[31m-// strict C99 defaults that hide AI_PASSIVE, struct addrinfo, struct timeval,[m
[31m-// u_short, SO_REUSEPORT, etc.[m
[32m+[m[32m// Only needed on glibc/Cygwin where -std=c99 restricts symbol visibility.[m
[32m+[m[32m// On BSD/musl, setting these RESTRICTS visibility instead of expanding it.[m
[32m+[m[32m#if defined(__linux__) || defined(__CYGWIN__)[m
 #ifndef _XOPEN_SOURCE[m
 #define _XOPEN_SOURCE 600[m
 #endif[m
[36m@@ -208,6 +207,7 @@[m [mtypedef unsigned int TcsInterfaceId; // TODO: GUID is used for in vista at newer[m
 #define _DEFAULT_SOURCE[m
 #endif[m
 #endif[m
[32m+[m[32m#endif[m
 [m
 typedef int TcsSocket;[m
 typedef unsigned int TcsInterfaceId;[m
[1mdiff --git a/src/tinycsocket_internal.h b/src/tinycsocket_internal.h[m
[1mindex a2c5b27..3c4db85 100644[m
[1m--- a/src/tinycsocket_internal.h[m
[1m+++ b/src/tinycsocket_internal.h[m
[36m@@ -23,7 +23,7 @@[m
 #ifndef TINYCSOCKET_INTERNAL_H_[m
 #define TINYCSOCKET_INTERNAL_H_[m
 [m
[31m-static const char* const TCS_VERSION_TXT = "v0.3.59";[m
[32m+[m[32mstatic const char* const TCS_VERSION_TXT = "v0.3.60";[m
 static const char* const TCS_LICENSE_TXT =[m
     "Copyright 2018 Markus Lindelöw\n"[m
     "\n"[m
[36m@@ -185,10 +185,9 @@[m [mtypedef unsigned int TcsInterfaceId; // TODO: GUID is used for in vista at newer[m
 #elif defined(TINYCSOCKET_USE_POSIX_IMPL)[m
 [m
 #if defined(TINYCSOCKET_IMPLEMENTATION) || defined(TCS_DEFINE_POSIX_MACROS)[m
[31m-// POSIX feature test macros must be set before any system header is included.[m
[31m-// Without these, glibc's <features.h> (pulled in by <stdbool.h>) locks in[m
[31m-// strict C99 defaults that hide AI_PASSIVE, struct addrinfo, struct timeval,[m
[31m-// u_short, SO_REUSEPORT, etc.[m
[32m+[m[32m// Only needed on glibc/Cygwin where -std=c99 restricts symbol visibility.[m
[32m+[m[32m// On BSD/musl, setting these RESTRICTS visibility instead of expanding it.[m
[32m+[m[32m#if defined(__linux__) || defined(__CYGWIN__)[m
 #ifndef _XOPEN_SOURCE[m
 #define _XOPEN_SOURCE 600[m
 #endif[m
[36m@@ -202,6 +201,7 @@[m [mtypedef unsigned int TcsInterfaceId; // TODO: GUID is used for in vista at newer[m
 #define _DEFAULT_SOURCE[m
 #endif[m
 #endif[m
[32m+[m[32m#endif[m
 [m
 typedef int TcsSocket;[m
 typedef unsigned int TcsInterfaceId;[m
