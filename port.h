#ifndef PORT_H
#define PORT_H

#if defined(_AIX) && !defined(__GNUC__)
#define bool int
#define false 0
#define true 1
#endif

#ifdef OS2
typedef enum
{
  false = 0,
  true = 1
} bool;

#define INCL_DOSQUEUES
#include <os2.h>

#define rmdir(XX) { DosDeleteDir(XX); }
#define chdir(XX) DosSetCurrentDir(XX)
#define file_close(XX) { DosClose(XX); }
#define make_directory(XX) DosCreateDir(XX, NULL)
typedef HFILE FILE_TYPE;
#define pipe(XX) DosCreatePipe(&XX[0], &XX[1], 8 * 1024)
#define sleep(XX) DosSleep((XX) * 1000)
#define exit(XX) DosExit(EXIT_THREAD, XX)
#else
#define file_close(XX) { ::close(XX); }
#define make_directory(XX) mkdir(XX, S_IRWXU)
typedef int FILE_TYPE;
#endif

#endif


