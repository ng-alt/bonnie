#ifndef PORT_H
#define PORT_H




#define _LARGEFILE64_SOURCE
#ifdef _LARGEFILE64_SOURCE
#define OFF_T_PRINTF "%lld"
#else
#define OFF_T_PRINTF "%d"
#endif

#if 0
#define false 0
#define true 1
#endif

#if defined (OS2) || defined (WIN32)
#define NON_UNIX
typedef char Sync;
#ifdef OS2
typedef enum
{
  false = 0,
  true = 1
} bool;

#define INCL_DOSQUEUES
#define INCL_DOSPROCESS
#include <os2.h>

#define file_read DosRead
#define NO_SNPRINTF
#define __min min
#define __max max
typedef ULONG TIMEVAL_TYPE;
#define chdir(XX) DosSetCurrentDir(XX)
#define fsync(XX) DosResetBuffer(XX)
#define sys_rmdir(XX) DosDeleteDir(XX)
#define sys_chdir(XX) DosSetCurrentDir(XX)
#define file_findclose DosFindClose
#define file_close DosClose
#define make_directory(XX) DosCreateDir(XX, NULL)
typedef HFILE FILE_TYPE;
#define pipe(XX) DosCreatePipe(&XX[0], &XX[1], 8 * 1024)
#define sleep(XX) DosSleep((XX) * 1000)
#define exit(XX) DosExit(EXIT_THREAD, XX)
typedef ULONG pid_t;
#else
// WIN32 here
#define file_read _read
#define file_unlink _unlink
#define sys_rmdir _rmdir
#define sys_chdir _chdir
#define sys_getpid _getpid
#define file_lseek _lseek
#define file_creat _creat
#define file_open _open

typedef int ssize_t;
typedef struct _timeb TIMEVAL_TYPE;
typedef int FILE_TYPE;
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#define fsync _commit
#define make_directory _mkdir
#define sync() {}
#define HDIR long
#define achName name
typedef int pid_t;
#define sleep(XX) { Sleep((XX) * 1000); }
#endif

#else
// UNIX here
#define file_read read
#define file_unlink unlink
typedef struct timeval TIMEVAL_TYPE;
#define sys_rmdir rmdir
#define sys_chdir chdir
#define sys_getpid getpid

#ifdef _LARGEFILE64_SOURCE
#define OFF_TYPE off64_t
#define file_lseek lseek64
#define file_creat creat64
#define file_open open64
#else
#define OFF_TYPE off_t
#define file_lseek lseek
#define file_creat creat
#define file_open open
#endif

#define second_sleep sleep
#define file_close close
#define make_directory(XX) mkdir(XX, S_IRWXU)
typedef int FILE_TYPE;
#define __min min
#define __max max
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef const char * PCCHAR;
typedef char * PCHAR;
typedef PCHAR const CPCHAR;
typedef PCCHAR const CPCCHAR;
typedef void * PVOID;
typedef PVOID const CPVOID;
typedef const CPVOID CPCVOID;

#endif
typedef FILE_TYPE *PFILE_TYPE;

#ifdef NO_SNPRINTF
#define _snprintf sprintf
#else
#ifndef WIN32
#define _snprintf snprintf
#endif
#endif

#endif
