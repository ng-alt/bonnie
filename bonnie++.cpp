
/*
 * COPYRIGHT NOTICE:
 * Copyright (c) Tim Bray, 1990.
 * Copyright (c) Russell Coker, 1999.  I have updated the program, added
 * support for >2G on 32bit machines, and tests for file creation.
 * Licensed under the GPL version 2.0.
 * DISCLAIMER:
 * This program is provided AS IS with no warranty of any kind, and
 * The author makes no representation with respect to the adequacy of this
 *  program for any particular purpose or with respect to its adequacy to
 *  produce any particular result, and
 * The author shall not be liable for loss or damage arising out of
 *  the use of this program regardless of how sustained, and
 * In no event shall the author be liable for special, direct, indirect
 *  or consequential damage, loss, costs or fees or expenses of any
 *  nature or kind.
 */

#include <stdlib.h>
#ifdef OS2
#define INCL_DOSMISC
#endif

#include "bonnie.h"

#ifdef NON_UNIX
typedef char Sync;
#include "getopt.h"
#endif

#ifdef OS2
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSQUEUES
#define INCL_DOSPROCESS
#include <os2.h>
#endif

#ifdef WIN32
#include <process.h>
#include <windows.h>
#endif

#ifndef NON_UNIX
#include <algo.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>
#include "sync.h"
#endif

#include <time.h>
#include "bon_io.h"
#include "bon_file.h"
#include "bon_time.h"
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

void usage();

class CGlobalItems
{
public:
  bool quiet;
  int char_io_size;
  bool sync_bonnie;
  BonTimer timer;
  int ram;
  Sync *syn;
  char *name;
  bool bufSync;
  int  chunk_bits;
  int chunk_size() const { return m_chunk_size; }
  bool *doExit;
  void set_chunk_size(int size)
    { delete m_buf; m_buf = new char[size]; m_chunk_size = size; }

  char *buf() { return m_buf; }

  CGlobalItems(bool *exitFlag);
  ~CGlobalItems() { delete name; delete m_buf;
#ifndef NON_UNIX
                    delete syn;
#endif
                  }

  void decrement_and_wait(int nr_sem);

  void SetName(CPCCHAR path)
  {
    delete name;
    name = new char[strlen(path) + 15];
#ifdef OS2
    ULONG myPid = 0;
    DosQuerySysInfo(QSV_FOREGROUND_PROCESS, QSV_FOREGROUND_PROCESS
                  , &myPid, sizeof(myPid));
#else
    pid_t myPid = _getpid();
#endif
    sprintf(name, "%s/Bonnie.%d", path, int(myPid));
  }

#ifndef NON_UNIX
  void setSync(SYNC_TYPE type, int semKey = 0, int num_tests = 0)
  {
    syn = new Sync(type, semKey, num_tests);
  }
#endif

private:
  int m_chunk_size;
  char *m_buf;


  CGlobalItems(const CGlobalItems &f);
  CGlobalItems & operator =(const CGlobalItems &f);
};

CGlobalItems::CGlobalItems(bool *exitFlag)
 : quiet(false)
 , char_io_size(DefaultCharIO)
 , sync_bonnie(false)
 , timer()
 , ram(0)
 , syn(NULL)
 , name(NULL)
 , bufSync(false)
 , chunk_bits(DefaultChunkBits)
 , doExit(exitFlag)
 , m_chunk_size(DefaultChunkSize)
 , m_buf(new char[m_chunk_size])
{
  SetName(".");
}

void CGlobalItems::decrement_and_wait(int nr_sem)
{
#ifndef NON_UNIX
  if(syn->decrement_and_wait(nr_sem))
    exit(1);
#endif
}

int TestDirOps(int directory_size, int max_size, int min_size
             , int num_directories, CGlobalItems &globals);
int TestFileOps(int file_size, CGlobalItems &globals);

static bool exitNow;

#ifndef NON_UNIX
void ctrl_c_handler(int sig)
{
  if(sig == SIGXCPU)
    fprintf(stderr, "Exceeded CPU usage.\n");
  else if(sig == SIGXFSZ)
    fprintf(stderr, "exceeded file storage limits.\n");
  exitNow = true;
}
#endif

int main(int argc, char *argv[])
{
  int    file_size = DefaultFileSize;
  int    directory_size = DefaultDirectorySize;
  int    directory_max_size = DefaultDirectoryMaxSize;
  int    directory_min_size = DefaultDirectoryMinSize;
  int    num_bonnie_procs = 0;
  int    num_directories = 1;
  int    test_count = -1;
  const char * machine = NULL;
#ifndef NON_UNIX
  char *userName = NULL, *groupName = NULL;
#endif
  CGlobalItems globals(&exitNow);
  bool setSize = false;

  exitNow = false;

#ifndef NON_UNIX
  struct sigaction sa;
  sa.sa_sigaction = NULL;
  sa.sa_handler = ctrl_c_handler;
  sa.sa_flags = SA_RESETHAND;
  if(sigaction(SIGINT, &sa, NULL)
   || sigaction(SIGXCPU, &sa, NULL)
   || sigaction(SIGXFSZ, &sa, NULL))
  {
    printf("Can't handle SIGINT.\n");
    return 1;
  }
  sa.sa_handler = SIG_IGN;
  if(sigaction(SIGHUP, &sa, NULL))
  {
    printf("Can't handle SIGHUP.\n");
    return 1;
  }
#endif

#ifdef _SC_PHYS_PAGES
  int page_size = sysconf(_SC_PAGESIZE);
  int num_pages = sysconf(_SC_PHYS_PAGES);
  if(page_size != -1 && num_pages != -1)
  {
    globals.ram = page_size/1024 * (num_pages/1024);
  }
#endif
#ifdef WIN32
  MEMORYSTATUS ms;
  GlobalMemoryStatus(&ms);
  globals.ram = ms.dwTotalPhys / 1024 / 1024;
#endif

  int int_c;
  while(-1 != (int_c = getopt(argc, argv, "bd:f::g:m:n:p:qr:s:u:x:y:")) )
  {
    switch(char(int_c))
    {
      case '?':
      case ':':
        usage();
      break;
      case 'b':
        globals.bufSync = true;
      break;
      case 'd':
        globals.SetName(optarg);
      break;
      case 'f':
        if(optarg)
          globals.char_io_size = atoi(optarg);
        else
          globals.char_io_size = 0;
      break;
      case 'm':
        machine = optarg;
      break;
      case 'n':
        sscanf(optarg, "%d:%d:%d:%d", &directory_size
                     , &directory_max_size, &directory_min_size
                     , &num_directories);
      break;
      case 'p':
        num_bonnie_procs = atoi(optarg);
                        /* Set semaphore to # of bonnie++ procs
                           to synchronize */
      break;
      case 'q':
        globals.quiet = true;
      break;
      case 'r':
        globals.ram = atoi(optarg);
      break;
      case 's':
      {
        char *sbuf = _strdup(optarg);
        char *size = strtok(sbuf, ":");
        file_size = atoi(size);
        char c = size[strlen(size) - 1];
        if(c == 'g' || c == 'G')
          file_size *= 1024;
        size = strtok(NULL, "");
        if(size)
        {
          int tmp = atoi(size);
          c = size[strlen(size) - 1];
          if(c == 'k' || c == 'K')
            tmp *= 1024;
          globals.set_chunk_size(tmp);
        }
        setSize = true;
      }
      break;
#ifndef NON_UNIX
      case 'g':
        if(groupName)
          usage();
        groupName = optarg;
      break;
      case 'u':
      {
        if(userName)
          usage();
        userName = strdup(optarg);
        int i;
        for(i = 0; userName[i] && userName[i] != ':'; i++);
        if(userName[i] == ':')
        {
          if(groupName)
            usage();
          userName[i] = '\0';
          groupName = &userName[i + 1];
        }
      }
      break;
#endif
      case 'x':
        test_count = atoi(optarg);
      break;
#ifndef NON_UNIX
      case 'y':
                        /* tell procs to synchronize via previous
                           defined semaphore */
        switch(tolower(optarg[0]))
        {
        case 's':
          globals.setSync(eSem, SemKey, TestCount);
        break;
        case 'p':
          globals.setSync(ePrompt);
        break;
        }
        globals.sync_bonnie = true;
      break;
#endif
    }
  }
#ifndef NON_UNIX
  if(!globals.syn)
    globals.setSync(eNone);
#endif
  if(optind < argc)
    usage();

  if(globals.ram && !setSize)
  {
    if(file_size < (globals.ram * 2))
      file_size = globals.ram * 2;
    // round up to the nearest gig
    if(file_size % 1024 > 512)
      file_size = file_size + 1024 - (file_size % 1024);
  }
  globals.char_io_size = __min(file_size, globals.char_io_size);
  globals.char_io_size = __max(0, globals.char_io_size);

  if(machine == NULL)
  {
#ifdef WIN32
    char utsBuf[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if(GetComputerName(utsBuf, &size))
      machine = _strdup(utsBuf);
#else
#ifdef OS2
    machine = "OS/2";
#else
    struct utsname utsBuf;
    if(uname(&utsBuf) != -1)
      machine = utsBuf.nodename;
#endif
#endif
  }

#ifndef NON_UNIX
  if(userName || groupName)
  {
    if(bon_setugid(userName, groupName))
      return 1;
    if(userName)
      free(userName);
  }
  else if(geteuid() == 0)
  {
    fprintf(stderr, "You must use the \"-u\" switch when running as root.\n");
    usage();
  }
#endif

  if(num_bonnie_procs && globals.sync_bonnie)
    usage();

#ifndef NON_UNIX
  if(num_bonnie_procs)
  {
    if(num_bonnie_procs == -1)
    {
      return globals.syn->clear_sem();
    }
    else
    {
      return globals.syn->create(num_bonnie_procs);
    }
  }

  if(globals.sync_bonnie)
  {
    if(globals.syn->get_semid())
      return 1;
  }
#endif

  if(file_size < 0 || directory_size < 0 || (!file_size && !directory_size) )
    usage();
  if(globals.chunk_size() < 256 || globals.chunk_size() > Unit)
    usage();
  int i;
  globals.chunk_bits = 0;
  for(i = globals.chunk_size(); i > 1; i = i >> 1, globals.chunk_bits++);
  if(1 << globals.chunk_bits != globals.chunk_size())
    usage();

  if( (directory_max_size != -1 && directory_max_size != -2)
     && (directory_max_size < directory_min_size || directory_max_size < 0
     || directory_min_size < 0) )
    usage();
  if(file_size > IOFileSize * MaxIOFiles || file_size > (1 << (31 - 20 + globals.chunk_bits)) )
    usage();
  // if doing more than one test run then we print a header before the
  // csv format output.
  if(test_count > 1)
  {
    globals.timer.SetType(BonTimer::csv);
    globals.timer.PrintHeader(stdout);
  }
  pid_t myPid = 0;
#ifdef OS2
    DosQuerySysInfo(QSV_FOREGROUND_PROCESS, QSV_FOREGROUND_PROCESS
                  , &myPid, sizeof(myPid));
#else
  myPid = _getpid();
#endif
  srand(myPid ^ time(NULL));
  for(; test_count > 0 || test_count == -1; test_count--)
  {
    globals.timer.Initialize();
    int rc;
    rc = TestFileOps(file_size, globals);
    if(rc) return rc;
    rc = TestDirOps(directory_size, directory_max_size, directory_min_size
                  , num_directories, globals);
    if(rc) return rc;
    // if we are only doing one test run then print a plain-text version of
    // the results before printing a csv version.
    if(test_count == -1)
    {
      globals.timer.SetType(BonTimer::txt);
      rc = globals.timer.DoReport(machine
                    , file_size, globals.char_io_size, globals.chunk_size()
                    , directory_size
                    , directory_max_size, directory_min_size, num_directories
                    , globals.quiet ? stderr : stdout);
    }
    // print a csv version in every case
    globals.timer.SetType(BonTimer::csv);
    rc = globals.timer.DoReport(machine
                    , file_size, globals.char_io_size, globals.chunk_size()
                    , directory_size
                    , directory_max_size, directory_min_size, num_directories
                    , stdout);
    if(rc) return rc;
  }
  return 0;
}

int
TestFileOps(int file_size, CGlobalItems &globals)
{
  if(file_size)
  {
    CFileOp file(globals.timer, file_size, globals.chunk_bits, globals.bufSync);
    int    num_chunks;
    int    words;
    char  *buf = globals.buf();
    int    bufindex;
    int    i;

    if(globals.ram && file_size < globals.ram * 2)
    {
      fprintf(stderr
            , "File size should be double RAM for good results, RAM is %dM.\n"
            , globals.ram);
      return 1;
    }
    // default is we have 1M / 8K * 200 chunks = 25600
    num_chunks = Unit / globals.chunk_size() * file_size;
    int char_io_chunks = Unit / globals.chunk_size() * globals.char_io_size;

    int rc;
    rc = file.open(globals.name, true, true);
    if(rc)
      return rc;
    if(exitNow)
      return EXIT_CTRL_C;
    Duration dur;

    globals.timer.start();
    if(char_io_chunks)
    {
      dur.reset();
      globals.decrement_and_wait(Putc);
      // Fill up a file, writing it a char at a time with the stdio putc() call
      if(!globals.quiet) fprintf(stderr, "Writing with putc()...");
      for(words = 0; words < char_io_chunks; words++)
      {
        dur.start();
        if(file.write_block_putc() == -1)
          return 1;
        dur.stop();
        if(exitNow)
          return EXIT_CTRL_C;
      }
      fflush(NULL);
      /*
       * note that we always close the file before measuring time, in an
       *  effort to force as much of the I/O out as we can
       */
      file.close();
      globals.timer.stop_and_record(Putc);
      globals.timer.add_latency(Putc, dur.getMax());
      if(!globals.quiet) fprintf(stderr, "done\n");
    }
    /* Write the whole file from scratch, again, with block I/O */
    if(file.reopen(true))
      return 1;
    dur.reset();
    globals.decrement_and_wait(FastWrite);
    if(!globals.quiet) fprintf(stderr, "Writing intelligently...");
    memset(buf, 0, globals.chunk_size());
    globals.timer.start();
    bufindex = 0;
    // for the number of chunks of file data
    for(i = 0; i < num_chunks; i++)
    {
      if(exitNow)
        return EXIT_CTRL_C;
      // for each chunk in the Unit
      buf[bufindex]++;
      bufindex = (bufindex + 1) % globals.chunk_size();
      dur.start();
      if(file.write_block(PVOID(buf)) == -1)
        return io_error("write(2)");
      dur.stop();
    }
    file.close();
    globals.timer.stop_and_record(FastWrite);
    globals.timer.add_latency(FastWrite, dur.getMax());
    if(!globals.quiet) fprintf(stderr, "done\n");


    /* Now read & rewrite it using block I/O.  Dirty one word in each block */
    if(file.reopen(false))
      return 1;
    if (file.seek(0, SEEK_SET) == -1)
    {
      if(!globals.quiet) fprintf(stderr, "error in lseek(2) before rewrite\n");
      return 1;
    }
    dur.reset();
    globals.decrement_and_wait(ReWrite);
    if(!globals.quiet) fprintf(stderr, "Rewriting...");
    globals.timer.start();
    bufindex = 0;
    for(words = 0; words < num_chunks; words++)
    { // for each chunk in the file
      dur.start();
      if (file.read_block(PVOID(buf)) == -1)
        return 1;
      bufindex = bufindex % globals.chunk_size();
      buf[bufindex]++;
      bufindex++;
      if (file.seek(-1, SEEK_CUR) == -1)
        return 1;
      if (file.write_block(PVOID(buf)) == -1)
        return io_error("re write(2)");
      dur.stop();
      if(exitNow)
        return EXIT_CTRL_C;
    }
    file.close();
    globals.timer.stop_and_record(ReWrite);
    globals.timer.add_latency(ReWrite, dur.getMax());
    if(!globals.quiet) fprintf(stderr, "done\n");

    if(char_io_chunks)
    {
      // read them all back with getc()
      if(file.reopen(false, true))
        return 1;
      dur.reset();
      globals.decrement_and_wait(Getc);
      if(!globals.quiet) fprintf(stderr, "Reading with getc()...");
      globals.timer.start();

      for(words = 0; words < char_io_chunks; words++)
      {
        dur.start();
        if(file.read_block_getc(buf) == -1)
          return 1;
        dur.stop();
        if(exitNow)
          return EXIT_CTRL_C;
      }

      file.close();
      globals.timer.stop_and_record(Getc);
      globals.timer.add_latency(Getc, dur.getMax());
      if(!globals.quiet) fprintf(stderr, "done\n");
    }

    /* Now suck it in, Chunk at a time, as fast as we can */
    if(file.reopen(false))
      return 1;
    if (file.seek(0, SEEK_SET) == -1)
      return io_error("lseek before read");
    dur.reset();
    globals.decrement_and_wait(FastRead);
    if(!globals.quiet) fprintf(stderr, "Reading intelligently...");
    globals.timer.start();
    for(i = 0; i < num_chunks; i++)
    { /* per block */
      dur.start();
      if ((words = file.read_block(PVOID(buf))) == -1)
        return io_error("read(2)");
      dur.stop();
      if(exitNow)
        return EXIT_CTRL_C;
    } /* per block */
    file.close();
    globals.timer.stop_and_record(FastRead);
    globals.timer.add_latency(FastRead, dur.getMax());
    if(!globals.quiet) fprintf(stderr, "done\n");

    globals.timer.start();
    if(file.seek_test(globals.quiet, *globals.syn))
      return 1;

    /*
     * Now test random seeks; first, set up for communicating with children.
     * The object of the game is to do "Seeks" lseek() calls as quickly
     *  as possible.  So we'll farm them out among SeekProcCount processes.
     *  We'll control them by writing 1-byte tickets down a pipe which
     *  the children all read.  We write "Seeks" bytes with val 1, whichever
     *  child happens to get them does it and the right number of seeks get
     *  done.
     * The idea is that since the write() of the tickets is probably
     *  atomic, the parent process likely won't get scheduled while the
     *  children are seeking away.  If you draw a picture of the likely
     *  timelines for three children, it seems likely that the seeks will
     *  overlap very nicely with the process scheduling with the effect
     *  that there will *always* be a seek() outstanding on the file.
     * Question: should the file be opened *before* the fork, so that
     *  all the children are lseeking on the same underlying file object?
     */
  }
  return 0;
}

int
TestDirOps(int directory_size, int max_size, int min_size
         , int num_directories, CGlobalItems &globals)
{
  COpenTest open_test(globals.chunk_size(), globals.bufSync, globals.doExit);
  if(!directory_size)
  {
    return 0;
  }
  // if directory_size (in K) * data per file*2 > ram << 10 (IE memory /1024)
  // then the storage of file names will take more than half RAM and there
  // won't be enough RAM to have Bonnie++ paged in and to have a reasonable
  // meta-data cache.
  if(globals.ram
      && directory_size * MaxDataPerFile * 2 * num_directories
                  > (globals.ram << 10))
  {
    fprintf(stderr, "When testing %dK * %d of files in %d MB of RAM the system is likely to\n"
            "start paging Bonnie++ data and the test will give suspect\n"
            "results, use less files or install more RAM for this test.\n"
           , directory_size, num_directories, globals.ram);
    return 1;
  }
  if(directory_size * num_directories * MaxDataPerFile > (1 << 20))
  {
    fprintf(stderr, "Not enough ram to test with %dK * %d files.\n"
                  , directory_size, num_directories);
    return 1;
  }
  globals.decrement_and_wait(CreateSeq);
  if(!globals.quiet) fprintf(stderr, "Create files in sequential order...");
  if(open_test.create(globals.name, globals.timer, directory_size
                    , max_size, min_size, num_directories, false))
    return 1;
  globals.decrement_and_wait(StatSeq);
  if(!globals.quiet) fprintf(stderr, "done.\nStat files in sequential order...");
  if(open_test.stat_sequential(globals.timer))
    return 1;
  globals.decrement_and_wait(DelSeq);
  if(!globals.quiet) fprintf(stderr, "done.\nDelete files in sequential order...");
  if(open_test.delete_sequential(globals.timer))
    return 1;
  if(!globals.quiet) fprintf(stderr, "done.\n");

  globals.decrement_and_wait(CreateRand);
  if(!globals.quiet) fprintf(stderr, "Create files in random order...");
  if(open_test.create(globals.name, globals.timer, directory_size
                    , max_size, min_size, num_directories, true))
    return 1;
  globals.decrement_and_wait(StatRand);
  if(!globals.quiet) fprintf(stderr, "done.\nStat files in random order...");
  if(open_test.stat_random(globals.timer))
    return 1;
  globals.decrement_and_wait(DelRand);
  if(!globals.quiet) fprintf(stderr, "done.\nDelete files in random order...");
  if(open_test.delete_random(globals.timer))
    return 1;
  if(!globals.quiet) fprintf(stderr, "done.\n");
  return 0;
}

void
usage()
{
  fprintf(stderr,
    "usage: Bonnie [-d scratch-dir] [-s size(Mb)[:chunk-size(b)]]\n"
    "              [-n number-to-stat[:max-size[:min-size][:num-directories]]]\n"
    "              [-m machine-name]\n"
    "              [-r ram-size-in-Mb]\n"
    "              [-x number-of-tests] [-u uid-to-use:gid-to-use] [-g gid-to-use]\n"
    "              [-q] [-f] [-b] [-p processes | -y]\n"
    "\nVersion: " BON_VERSION "\n");
  exit(1);
}

int
io_error(CPCCHAR message, bool do_exit)
{
  char buf[1024];

  sprintf(buf, "Bonnie: drastic I/O error (%s)", message);
  perror(buf);
  if(do_exit)
    exit(1);
  return(1);
}

