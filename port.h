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

#define _read DosRead
#define _strdup strdup
#define NO_SNPRINTF
#define __min min
#define __max max
typedef ULONG TIMEVAL_TYPE;
#define chdir(XX) DosSetCurrentDir(XX)
#define fsync(XX) DosResetBuffer(XX)
#define _rmdir(XX) DosDeleteDir(XX)
#define _chdir(XX) DosSetCurrentDir(XX)
#define _findclose DosFindClose
#define _close DosClose
#define make_directory(XX) DosCreateDir(XX, NULL)
typedef HFILE FILE_TYPE;
#define pipe(XX) DosCreatePipe(&XX[0], &XX[1], 8 * 1024)
#define sleep(XX) DosSleep((XX) * 1000)
#define exit(XX) DosExit(EXIT_THREAD, XX)
typedef ULONG pid_t;
#else
// WIN32 here
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
#define _read read
#define _unlink unlink
#define _strdup strdup
typedef struct timeval TIMEVAL_TYPE;
#define _rmdir rmdir
#define _chdir chdir
#define _getpid getpid

#ifdef _LARGEFILE64_SOURCE
#define OFF_TYPE off64_t
#define _lseek lseek64
#define _creat creat64
#define _open open64
#else
#define OFF_TYPE off_t
#define _lseek lseek
#define _creat creat
#define _open ::open
#endif

#define _sleep sleep
#define _close ::close
#define make_directory(XX) mkdir(XX, S_IRWXU)
typedef int FILE_TYPE;
#define __min min
#define __max max
#endif
typedef FILE_TYPE *PFILE_TYPE;

#ifdef NO_SNPRINTF
#define _snprintf sprintf
#else
#ifndef WIN32
#define _snprintf snprintf
#endif
#endif

#define EXIT_CTRL_C 5

typedef const char * PCCHAR;
typedef char * PCHAR;
typedef PCHAR const CPCHAR;
typedef PCCHAR const CPCCHAR;
typedef void * PVOID;
typedef PVOID const CPVOID;
typedef const CPVOID CPCVOID;


#endif
