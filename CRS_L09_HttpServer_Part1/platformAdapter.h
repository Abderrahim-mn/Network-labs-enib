//----------------------------------------------------------------------------
// Copyright (c) 2008-2016 Fabrice HARROUET (ENIB)
//
// This file is part of TransProg.
//   http://www.enib.fr/~harrouet/transprog.html
//
// TransProg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TransProg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with TransProg.  If not, see <http://www.gnu.org/licenses/>.
//----------------------------------------------------------------------------

#ifndef TRANSPROG_PLATFORM_ADAPTER_H
#define TRANSPROG_PLATFORM_ADAPTER_H 1

// #undef __STRICT_ANSI__ // prevent from disabling too many features
#define _DEFAULT_SOURCE // prevent from disabling too many features

#if defined __cplusplus && defined __clang__
  // On g++, -Wconversion does not imply -Wsign-conversion so that an
  // int may be use where a size_t is expected (vector::operator[] for
  // example) without triggering a warning.
  // Unfortunately, on clang++ it is not the case, so this warning must
  // be explicitely disabled.
# pragma clang diagnostic ignored "-Wsign-conversion"
#endif

//-------- System header inclusions ------------------------------------------

#if defined _WIN32
# define WINVER 0x601
# define _WIN32_WINNT WINVER
# if defined _MSC_VER
#   if !defined _DEBUG
#     define NDEBUG
#   endif
#   define NOMINMAX // prevent from defining min()/max() macros
#   define _CRT_SECURE_NO_WARNINGS // do not complain about standard libc
#   define _WINSOCK_DEPRECATED_NO_WARNINGS // idem about socket API
# else
#   define __USE_MINGW_ANSI_STDIO 1 // use standard printf/scanf under mingw
# endif
#endif

#if defined _MSC_VER
# include <stdint.h>
#else
# define _FILE_OFFSET_BITS 64 // large file support on 32-bit OS
# if !defined __STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS 1
# endif
# if !defined __STDC_CONSTANT_MACROS
#   define __STDC_CONSTANT_MACROS 1
# endif
# if !defined __STDC_FORMAT_MACROS
#   define __STDC_FORMAT_MACROS 1
# endif
# include <inttypes.h>
# include <sys/param.h>
# include <sys/time.h>
# include <dirent.h>
#endif

#if defined _WIN32
# include <winsock2.h> // prevent inclusion of <winsock.h> from <windows.h>
# include <windows.h>
# include <intrin.h>
# include <io.h>
# include <direct.h>
# include <process.h>
#else
# include <dlfcn.h>
# include <signal.h>
# include <sys/wait.h>
# include <sys/mman.h>
# if defined __APPLE__
#   include <AvailabilityMacros.h>
#   if defined AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER
#     include <mach/mach.h>
#     include <mach/thread_policy.h>
#   endif
# endif
# if defined __linux__
#   if !defined __USE_GNU
#     define __USE_GNU // required for CPU affinity
#     include <pthread.h>
#     include <sched.h>
#     undef __USE_GNU
#   else
#     include <pthread.h>
#     include <sched.h>
#   endif
# else
#   include <pthread.h>
# endif
# if defined __FreeBSD__
#   include <sys/cpuset.h>
# endif
# if defined __OpenBSD__
# endif
# if defined __NetBSD__
# endif
# if defined __ANDROID__ || defined __linux__
#   include <linux/sysctl.h>
# else
#   include <sys/sysctl.h>
# endif
# include <unistd.h>
# include <errno.h>
#endif

#include <limits.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined __cplusplus
# include <stdbool.h>
#endif

//-------- Various checks and adjustments ------------------------------------

#if defined _MSC_VER
# define BIG_ENDIAN    4321
# define LITTLE_ENDIAN 1234
# if defined _M_IX86 || defined _M_AMD64
#   define BYTE_ORDER LITTLE_ENDIAN // UGLY hard-coded setting!
# endif
# if !defined _DLL
#   error "_DLL undefined, use /MD when compiling!"
# endif
# if !defined _MT
#   error "_MT undefined, use /MD when compiling!"
# endif
#else
# if !defined _REENTRANT
#   error "_REENTRANT undefined, use -D_REENTRANT or -pthread when compiling!"
# endif
#endif

#if !defined BYTE_ORDER
# error "BYTE_ORDER not defined, please check platform settings"
#endif

#if defined __cplusplus
# define EXTERN_C_SYMBOL extern "C"
#else
# define EXTERN_C_SYMBOL extern
#endif

#if defined _WIN32
# define EXPORT_SYMBOL __declspec(dllexport)
# define IMPORT_SYMBOL __declspec(dllimport)
#else
# define EXPORT_SYMBOL
# define IMPORT_SYMBOL
#endif

#if defined _MSC_VER
  typedef int pid_t;
# define INLINE __inline static
# define RESTRICT __restrict
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
# define snprintf _snprintf
# define __func__ __FUNCTION__
# define STDIN_FILENO 0
# define STDOUT_FILENO 1
# define STDERR_FILENO 2
# define F_OK 0
# define X_OK 1
# define W_OK 2
# define R_OK 4
#else
# define INLINE inline static
# define RESTRICT __restrict__
#endif

#if defined _WIN32
  // The open() call is supposed to be low-level, but Windows still
  // performs end-of-line conversions by default!
  // There exists a non-standard O_BINARY flag to prevent this.
  // Since we do not want to write non-standard code, we redefine
  // the opening flags to include this O_BINARY flag.
# undef O_RDONLY
# define O_RDONLY (_O_RDONLY|_O_BINARY)
# undef O_WRONLY
# define O_WRONLY (_O_WRONLY|_O_BINARY)
# undef O_RDWR
# define O_RDWR (_O_RDWR|_O_BINARY)
#endif

#if defined _MSC_VER
  // !!! thread is not compatible with dllexport
  // # define THREAD_STORAGE __declspec(thread)
#elif !defined __APPLE__
# define THREAD_STORAGE __thread
#endif

#if !defined PATH_MAX
# if defined MAX_PATH
#   define PATH_MAX MAX_PATH
# else
#   define PATH_MAX 4096 // assumed default value
# endif
#endif

#if !defined MAP_ANONYMOUS && defined MAP_ANON
# define MAP_ANONYMOUS MAP_ANON
#endif

#if defined _WIN32
# define SYSCALL_ERRNO (GetLastError())
# define RESTART_SYSCALL(result,syscall) \
         do { (result)=(syscall); } while(0) // no restart actually!
#else
# define SYSCALL_ERRNO (errno)
# define RESTART_SYSCALL(result,syscall) \
         do { (result)=(syscall); } while(((result)<0)&&(errno==EINTR))
#endif

#define ONE_IF_ZERO(x) \
        ((x)+!(x))

#define SIZE_OF_MEMBER(struct_type,member_name) \
        (sizeof(((struct_type *)0)->member_name))

#define OFFSET_OF_MEMBER(struct_type,member_name) \
        ((int)(intptr_t)&((struct_type *)0)->member_name)

#define STRUCT_OF_MEMBER(member_addr,struct_type,member_name)        \
        ((struct_type *)(((uint8_t *)member_addr)-                   \
                         OFFSET_OF_MEMBER(struct_type,member_name)))

#define PADDED_ELEMENT(base_addr,elem_index,elem_size,elem_type) \
        ((elem_type *)(((uint8_t *)(base_addr))+(elem_index)*(elem_size)))

//-------- Sockets (optional) ------------------------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_SOCKETS

#if defined _WIN32
  typedef int socklen_t;
# define SHUT_RD SD_RECEIVE
# define SHUT_WR SD_SEND
# define SHUT_RDWR SD_BOTH
# define SOCKET_ERRNO (WSAGetLastError())
#else
# include <sys/select.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
  typedef int SOCKET;
# define INVALID_SOCKET (-1)
# define SOCKET_ERROR (-1)
# define closesocket(s) close(s)
# define SOCKET_ERRNO (errno)
#endif

#endif // not DISABLE_PLATFORM_ADAPTER_SOCKETS

//-------- Maths (optional) --------------------------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_MATHS

#if defined _WIN32
# define _USE_MATH_DEFINES
# include <math.h>
#else
# if defined __linux__
#   if defined __USE_GNU // required for sincos() and sincosf()
#     include <math.h>
#   else
#     define __USE_GNU
#     include <math.h>
#     undef __USE_GNU
#   endif
#   define HAS_SINCOS 1
# else
#   include <math.h>
# endif
# if defined __INTEL_COMPILER
#   define HAS_SINCOS 1
# endif
#endif

typedef float real32_t;
#if defined _MSC_VER
  typedef double real64_t;
#else
  typedef __attribute__((__aligned__(8))) double real64_t;
#endif

#if !defined M_PI // pi
# define M_PI 3.14159265358979323846
#endif
#if !defined M_PIf
# define M_PIf ((real32_t)M_PI)
#endif

#if !defined M_PI_2 // pi/2
# define M_PI_2 1.57079632679489661923
#endif
#if !defined M_PI_2f
# define M_PI_2f ((real32_t)M_PI_2)
#endif

#if !defined M_2PI // 2*pi
# define M_2PI 6.28318530717958647692
#endif
#if !defined M_2PIf
# define M_2PIf ((real32_t)M_2PI)
#endif

#if !defined M_180_PI // 180/pi
# define M_180_PI 57.2957795130823228646
#endif
#if !defined M_180_PIf
# define M_180_PIf ((real32_t)M_180_PI)
#endif

#if !defined M_PI_180 // pi/180
# define M_PI_180 0.01745329251994329547
#endif
#if !defined M_PI_180f
# define M_PI_180f ((real32_t)M_PI_180)
#endif

#if !defined M_SQRT2 // sqrt(2.0)
# define M_SQRT2 1.41421356237309504880
#endif
#if !defined M_SQRT2f
# define M_SQRT2f ((real32_t)M_SQRT2)
#endif

#if !defined M_SQRT1_2 // 1.0/sqrt(2.0)
# define M_SQRT1_2 0.70710678118654752440
#endif
#if !defined M_SQRT1_2f
# define M_SQRT1_2f ((real32_t)M_SQRT1_2)
#endif

#endif // not DISABLE_PLATFORM_ADAPTER_MATHS

//-------- Text formating (optional) -----------------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_FORMAT

#if defined _MSC_VER
# define PRINTF_FORMAT(formatArg,firstArg) // FIXME: not supported
#else
# if defined _WIN32
#   define PRINTF_FORMAT(formatArg,firstArg) \
           __attribute__((__format__(gnu_printf,(formatArg),(firstArg))))
# else
#   define PRINTF_FORMAT(formatArg,firstArg) \
           __attribute__((__format__(printf,(formatArg),(firstArg))))
# endif
#endif

// 64-bit integer printf/scanf format
#if defined _MSC_VER
# define F64 "I64"
#elif defined __APPLE__
# define F64 "ll"
#elif LONG_MAX==INT_MAX
# define F64 "ll"
#else
# define F64 "l"
#endif

// pointer integer printf/scanf format
#if defined __APPLE__
# define FPTR "l"
#elif INTPTR_MAX==INT64_MAX
# define FPTR F64
#elif INTPTR_MAX==INT32_MAX
# define FPTR ""
#else
# error "weird size for intptr_t!"
#endif

#endif // not DISABLE_PLATFORM_ADAPTER_FORMAT

//-------- Vectorisation hints (optional) ------------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_VECTORISATION

#if defined __INTEL_COMPILER
# define TRY_TO_VECTORISE \
         _Pragma("ivdep") \
         _Pragma("vector always")
# define DO_NOT_VECTORISE \
         _Pragma("novector")
#elif defined _MSC_VER
# define TRY_TO_VECTORISE \
         __pragma(loop(ivdep))
# define DO_NOT_VECTORISE \
         __pragma(loop(no_vector))
#elif (__GNUC__*100+__GNUC_MINOR__)>=409
# define TRY_TO_VECTORISE \
         _Pragma("GCC ivdep")
# define DO_NOT_VECTORISE \
         // FIXME: nothing to prevent vectorisation
#else
# define TRY_TO_VECTORISE
         // FIXME: nothing to ask for vectorisation
# define DO_NOT_VECTORISE
         // FIXME: nothing to prevent vectorisation
#endif

#endif // not DISABLE_PLATFORM_ADAPTER_VECTORISATION

//-------- Typed memory allocation (optional) --------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_ALLOC

#define Memory_alloc(size,type) \
        ((type *)malloc((size)*sizeof(type)))

#define Memory_initAlloc(size,type) \
        ((type *)calloc((size),sizeof(type)))

#define Memory_reAlloc(oldPtr,newSize,type) \
        ((type *)realloc((oldPtr),(newSize)*sizeof(type)))

#define Memory_free(ptr) \
        (free((ptr)))

#endif // not DISABLE_PLATFORM_ADAPTER_ALLOC

//-------- Error reporting (optional) ----------------------------------------

#if !defined DISABLE_PLATFORM_ADAPTER_ERROR

#if defined NDEBUG
# define ON_ERROR(condition,msg,action) \
         do { (void)(condition); } while(0)
#else
# define ON_ERROR(condition,msg,action)                        \
         do                                                    \
           {                                                   \
           if((condition))                                     \
             {                                                 \
             fprintf(stderr,"\n%s:%d:%s()\n  %s\n",            \
                            __FILE__,__LINE__,__func__,(msg)); \
             action;                                           \
             }                                                 \
           } while(0)
#endif

#endif // not DISABLE_PLATFORM_ADAPTER_ERROR

#endif // TRANSPROG_PLATFORM_ADAPTER_H

//----------------------------------------------------------------------------
