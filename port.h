#ifndef PORT_H
#define PORT_H

#include <stdio.h>



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
#define _snprintf sprintf
#define __min min
#define __max max
typedef ULONG TIMEVAL_TYPE;
#define chdir(XX) DosSetCurrentDir(XX)
#define fsync(XX) DosResetBuffer(XX)
#define _rmdir(XX) DosDeleteDir(XX)
#define _chdir(XX) DosSetCurrentDir(XX)
#define _findclose DosFindClose
#define file_close DosClose
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
#define file_close _close
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
#define _creat creat
#define _getpid getpid
#define _lseek lseek
#define _sleep sleep
#define file_close ::close
#define make_directory(XX) mkdir(XX, S_IRWXU)
typedef int FILE_TYPE;
#define __min min
#define __max max
#define _snprintf snprintf
#endif
typedef FILE_TYPE *PFILE_TYPE;
typedef FILE *PFILE;

#endif

#define EXIT_CTRL_C 5
