
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

#ifdef OS2
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSQUEUES
#define INCL_DOSPROCESS
#include <os2.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include "bonnie.h"
#include "bon_io.h"
#include "bon_file.h"
#include "bon_time.h"
#include "semaphore.h"
#include <ctype.h>
#include <string.h>

void usage();

class CGlobalItems
{
public:
  bool quiet;
  bool fast;
  bool sync_bonnie;
  BonTimer timer;
  int ram;
  Semaphore sem;
  char *name;
  bool bufSync;

  CGlobalItems();
  ~CGlobalItems() { delete name; }

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
    pid_t myPid = getpid();
#endif
    sprintf(name, "%s/Bonnie.%d", path, int(myPid));
  }
  CGlobalItems(const CGlobalItems &f);
  CGlobalItems & operator =(const CGlobalItems &f);
};

CGlobalItems::CGlobalItems()
 : sem(SemKey, TestCount)
{
  quiet = false;
  fast = false;
  sync_bonnie = false;
  ram = 0;
  name = NULL;
  SetName(".");
  bufSync = false;
}

void CGlobalItems::decrement_and_wait(int nr_sem)
{
  if(sem.decrement_and_wait(nr_sem))
    exit(1);
}

int TestDirOps(int directory_size, int max_size, int min_size
             , int num_directories, CGlobalItems &globals);
int TestFileOps(int file_size, CGlobalItems &globals);

int main(int argc, char *argv[])
{
  int    file_size = DefaultFileSize;
  int    directory_size = DefaultDirectorySize;
  int    directory_max_size = DefaultDirectoryMaxSize;
  int    directory_min_size = DefaultDirectoryMinSize;
  int    num_bonnie_procs = 0;
  int    num_directories = 1;
  int    count = -1;
  char * machine = "Unknown";
  CGlobalItems globals;

  int c;
  while(-1 != (c = getopt(argc, argv, "bd:fm:n:p:qr:s:x:y")) )
  {
    switch(char(c))
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
        globals.fast = true;
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
        file_size = atoi(optarg);
      break;
      case 'x':
        count = atoi(optarg);
      break;
      case 'y':
                        /* tell procs to synchronize via previous
                           defined semaphore */
        globals.sync_bonnie = true;
      break;
    }
  }
  if(optind < argc)
    usage();

  if(num_bonnie_procs && globals.sync_bonnie)
    usage();

  if(num_bonnie_procs)
  {
    if(num_bonnie_procs == -1)
    {
      return globals.sem.clear_sem();
    }
    else
    {
      return globals.sem.create(num_bonnie_procs);
    }
  }

  if(globals.sync_bonnie)
  {
    if(globals.sem.get_semid())
      return 1;
  }

  if(file_size < 0 || directory_size < 0 || (!file_size && !directory_size) )
    usage();
  if( (directory_max_size != -1 && directory_max_size != -2)
     && (directory_max_size < directory_min_size || directory_max_size < 0
     || directory_min_size < 0) )
    usage();
  if(file_size > IOFileSize * MaxIOFiles || file_size > (1 << (31 - 20 + ChunkBits)) )
    usage();
  // if doing more than one test run then we print a header before the
  // csv format output.
  if(count > 1)
  {
    globals.timer.SetType(BonTimer::csv);
    globals.timer.PrintHeader(stdout);
  }
#ifdef OS2
    ULONG myPid = 0;
    DosQuerySysInfo(QSV_FOREGROUND_PROCESS, QSV_FOREGROUND_PROCESS
                  , &myPid, sizeof(myPid));
#else
  pid_t myPid = getpid();
#endif
  srand(myPid ^ time(NULL));
  for(; count > 0 || count == -1; count--)
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
    if(count == -1)
    {
      globals.timer.SetType(BonTimer::txt);
      rc = globals.timer.DoReport(machine, file_size, directory_size
                                , directory_max_size, directory_min_size
                                , num_directories
                                , globals.quiet ? stderr : stdout);
    }
    // print a csv version in every case
    globals.timer.SetType(BonTimer::csv);
    rc = globals.timer.DoReport(machine, file_size, directory_size
                              , directory_max_size, directory_min_size
                              , num_directories, stdout);
    if(rc) return rc;
  }
}

int
TestFileOps(int file_size, CGlobalItems &globals)
{
  if(file_size)
  {
    CFileOp file(globals.timer, file_size, globals.bufSync);
    int    num_chunks;
    int    words;
    int    buf[Chunk / IntSize];
    int    bufindex;
    int    chars[256];
    int    i;

    if(globals.ram && file_size < globals.ram * 2)
    {
      fprintf(stderr, "File size should be double RAM for good results.\n");
      return 1;
    }
    // default is we have 1M / 8K * 200 chunks = 25600
    num_chunks = Unit / Chunk * file_size;

    int rc;
    rc = file.open(globals.name, true, true);
    if(rc)
      return rc;
    globals.timer.timestamp();

    if(!globals.fast)
    {
      globals.decrement_and_wait(Putc);
      // Fill up a file, writing it a char at a time with the stdio putc() call
      if(!globals.quiet) fprintf(stderr, "Writing with putc()...");
      for(words = 0; words < num_chunks; words++)
      {
        if(file.write_block_putc() == -1)
          return 1;
      }
      fflush(NULL);
      /*
       * note that we always close the file before measuring time, in an
       *  effort to force as much of the I/O out as we can
       */
      file.close();
      globals.timer.get_delta_t(Putc);
      if(!globals.quiet) fprintf(stderr, "done\n");
    }
    /* Write the whole file from scratch, again, with block I/O */
    if(file.reopen(true))
      return 1;
    globals.decrement_and_wait(FastWrite);
    if(!globals.quiet) fprintf(stderr, "Writing intelligently...");
    for (words = 0; words < int(Chunk / IntSize); words++)
      buf[words] = 0;
    globals.timer.timestamp();
    bufindex = 0;
    // for the number of chunks of file data
    for(i = 0; i < num_chunks; i++)
    {
      // for each chunk in the Unit
      buf[bufindex]++;
      bufindex = (bufindex + 1) % (Chunk / IntSize);
      if(file.write_block(PVOID(buf)) == -1)
        return io_error("write(2)");
    }
    file.close();
    globals.timer.get_delta_t(FastWrite);
    if(!globals.quiet) fprintf(stderr, "done\n");


    /* Now read & rewrite it using block I/O.  Dirty one word in each block */
    if(file.reopen(false))
      return 1;
    if (file.seek(0, SEEK_SET) == -1)
    {
      if(!globals.quiet) fprintf(stderr, "error in lseek(2) before rewrite\n");
      return 1;
    }
    globals.decrement_and_wait(ReWrite);
    if(!globals.quiet) fprintf(stderr, "Rewriting...");
    globals.timer.timestamp();
    bufindex = 0;
    for(words = 0; words < num_chunks; words++)
    { // for each chunk in the file
      if (file.read_block(PVOID(buf)) == -1)
        return 1;
      if (bufindex == Chunk / IntSize)
        bufindex = 0;
      buf[bufindex++]++;
      if (file.seek(-1, SEEK_CUR) == -1)
        return 1;
      if (file.write_block(PVOID(buf)) == -1)
        return io_error("re write(2)");
    }
    file.close();
    globals.timer.get_delta_t(ReWrite);
    if(!globals.quiet) fprintf(stderr, "done\n");


    if(!globals.fast)
    {
      // read them all back with getc()
      if(file.reopen(false, true))
        return 1;
      for (words = 0; words < 256; words++)
        chars[words] = 0;
      globals.decrement_and_wait(Getc);
      if(!globals.quiet) fprintf(stderr, "Reading with getc()...");
      globals.timer.timestamp();

      for(words = 0; words < num_chunks; words++)
      {
        if(file.read_block_getc((char *)chars) == -1)
          return 1;
      }

      file.close();
      globals.timer.get_delta_t(Getc);
      if(!globals.quiet) fprintf(stderr, "done\n");
    }

    /* use the frequency count */
    for (words = 0; words < 256; words++)
      sprintf((char *)buf, "%d", chars[words]);

    /* Now suck it in, Chunk at a time, as fast as we can */
    if(file.reopen(false))
      return 1;
    if (file.seek(0, SEEK_SET) == -1)
      return io_error("lseek before read");
    globals.decrement_and_wait(FastRead);
    if(!globals.quiet) fprintf(stderr, "Reading intelligently...");
    globals.timer.timestamp();
    for(i = 0; i < num_chunks; i++)
    { /* per block */
      if ((words = file.read_block(PVOID(buf))) == -1)
        return io_error("read(2)");
      chars[buf[abs(buf[0]) % (Chunk / IntSize)] & 0x7f]++;
    } /* per block */
    file.close();
    globals.timer.get_delta_t(FastRead);
    if(!globals.quiet) fprintf(stderr, "done\n");

    /* use the frequency count */
    for (words = 0; words < 256; words++)
      sprintf((char *)buf, "%d", chars[words]);

    globals.timer.timestamp();
    if(file.seek_test(SeekProcCount, Seeks, globals.quiet, globals.sem))
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
  COpenTest open_test(globals.bufSync);
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
    "usage: Bonnie [-d scratch-dir] [-s size(Mb)]\n"
    "              [-n number-to-stat[:max-size[:min-size][:num-directories]]]\n"
    "              [-m machine-name]\n"
    "              [-r ram-size-in-Mb]\n"
    "              [-x number-of-tests]\n"
    "              [-q] [-f] [-b] [-p processes | -y]\n"
    "\nVersion: " BON_VERSION "\n");
  exit(1);
}

int
io_error(CPCCHAR message, bool do_exit)
{
  char buf[Chunk];

  sprintf(buf, "Bonnie: drastic I/O error (%s)", message);
  perror(buf);
  if(do_exit)
    exit(1);
  return(1);
}

#ifdef SysV
static char randseed[32];

void
srandom(int seed)
{
  sprintf(randseed, "%06d", seed);
}

long
random()
{
  return nrand48(randseed);
}
#endif

