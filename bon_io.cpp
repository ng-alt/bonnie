#include <fcntl.h>
#include <sys/types.h>
#ifdef OS2
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#include <os2.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

#include "bonnie.h"
#include "bon_io.h"
#include "semaphore.h"
#include "bon_time.h"

#include "forkit.h"

CFileOp::~CFileOp()
{
  close();
  if(m_name)
  {
    int len = strlen(m_name);
    for(int i = 0; i < m_num_files; i++)
    {
      sprintf(&m_name[len], ".%03d", i);
      unlink(m_name);
    }
    delete m_name;
  }
}

void seeker(Fork *f, PVOID param, int unused)
{
  struct report_s seeker_report;
  CFileOp *file = (CFileOp *)param;
  int num_chunks = file->chunks();
  if(file->reopen(false))
    exit(1);
  char ticket;
  int rc;
  int lseek_count = 0;
  file->timer().timestamp();
  while((rc = f->Read(&ticket, 1, 0)) == 1 && ticket)
  {
    bool update;
    if( (lseek_count++ % UpdateSeek) == 0)
      update = true;
    else
      update = false;
    if(file->doseek(rand() % num_chunks, update) )
      exit(1);
  }
  if(rc != 1)
  {
    fprintf(stderr, "Can't read ticket.\n");
    exit(1);
  }
  file->close();
  file->timer().get_delta_report(seeker_report);
  if(f->Write(&seeker_report, sizeof(seeker_report)) != sizeof(seeker_report))
  {
    fprintf(stderr, "Can't write report.\n");
    exit(1);
  }
}

int CFileOp::seek_test(int threads, int seeks, bool quiet, Semaphore &sem)
{
  char   seek_tickets[seeks + threads];
  int next;
  for (next = 0; next < seeks; next++)
    seek_tickets[next] = 1;
  for ( ; next < (seeks + threads); next++)
    seek_tickets[next] = 0;
  Fork f;
  f.go(seeker, this, threads);

  sleep(5);
  if(sem.decrement_and_wait(Lseek))
    return 1;
  if(!quiet) fprintf(stderr, "start 'em...");
  if(f.Write(seek_tickets, sizeof(seek_tickets)) != int(sizeof(seek_tickets)) )
  {
    fprintf(stderr, "Can't write tickets.\n");
    return 1;
  }
  for (next = 0; next < threads; next++)
  { /* for each child */
    struct report_s seeker_report;

    int rc;
    if((rc = f.Read(&seeker_report, sizeof(seeker_report), 0))
        != sizeof(seeker_report))
    {
      fprintf(stderr, "Can't read from pipe, expected %d, got %d.\n"
                    , int(sizeof(seeker_report)), rc);
      return 1;
    }

    /*
     * each child writes back its CPU, start & end times.  The elapsed time
     *  to do all the seeks is the time the first child started until the
     *  time the last child stopped
     */
    m_timer.add_delta_report(seeker_report, Lseek);
#ifdef OS2
    TID status = 0;
    if(DosWaitThread(&status, DCWW_WAIT))
#else
    int status = 0;
    if(wait(&status) == -1)
#endif
      return io_error("wait");
    if(!quiet) fprintf(stderr, "done...");
  } /* for each child */
  if(!quiet) fprintf(stderr, "\n");
  return 0;
}

int CFileOp::seek(int offset, int whence)
{
  switch(whence)
  {
    case SEEK_SET:
      m_file_ind = offset / m_chunks_per_file;
      m_cur_pos = offset % m_chunks_per_file;
    break;
    case SEEK_CUR:
      m_cur_pos += offset;
      while(m_cur_pos < 0)
      {
        m_file_ind--;
        m_cur_pos += m_chunks_per_file;
      }
      while(m_cur_pos >= m_chunks_per_file)
      {
        m_file_ind++;
        m_cur_pos -= m_chunks_per_file;
      }
    break;
    default:
      fprintf(stderr, "unsupported seek option\n");
      return -1;
  }
  if(m_file_ind == m_num_files)
  {
    m_file_ind--;
    m_cur_pos += m_chunks_per_file;
  }
  else
  {
    if(m_file_ind < 0 || m_file_ind >= m_num_files
       || (m_file_ind == m_num_files - 1 && m_cur_pos >= m_last_file_chunks))
    {
      fprintf(stderr, "Bad seek offset\n");
      return -1;
    }
    int rc;
    if(m_fd)
    {
#ifdef OS2
      unsigned long actual;
      rc = DosSetFilePtr(m_fd[m_file_ind], m_cur_pos << m_chunk_bits, FILE_BEGIN, &actual);
      if(rc != 0) rc = -1;
#else
      rc = lseek(m_fd[m_file_ind], m_cur_pos << m_chunk_bits, SEEK_SET);
#endif
    }
    else
    {
      rc = fseek(m_stream[m_file_ind], m_cur_pos << m_chunk_bits, SEEK_SET);
    }

    if(rc == -1)
      fprintf(stderr, "Error in lseek to %d\n", (m_cur_pos << m_chunk_bits));
    else
      rc = 0;
    return rc;
  }
  return 0;
}

int CFileOp::read_block(PVOID buf)
{
  assert(m_fd);
#ifdef OS2
  unsigned long actual;
  int rc = DosRead(m_fd[m_file_ind], buf, m_chunk_size, &actual);
  if(rc)
    rc = -1;
  else
    rc = actual;
#else
  int rc = ::read(m_fd[m_file_ind], buf, m_chunk_size);
#endif
  m_cur_pos++;
  if(m_cur_pos >= m_chunks_per_file)
  {
    if(seek(0, SEEK_CUR) == -1)
    {
      fprintf(stderr, "Error in seek(0)\n");
      return -1;
    }
  }
  if(rc == -1)
  {
    io_error("re-write read");
  }
  else if(rc != m_chunk_size)
  {
    fprintf(stderr, "Can't read a full block, only got %d bytes.\n", rc);
    rc = -1;
  }
  return rc;
}

int CFileOp::read_block_getc(char *buf)
{
  assert(m_stream);
  int next;
  for(int i = 0; i < m_chunk_size; i++)
  {
    if ((next = getc(m_stream[m_file_ind])) == EOF)
    {
      fprintf(stderr, "Can't getc(3)\n");
      return -1;
    }
    /* just to fool optimizers */
    buf[next]++;
  }

  m_cur_pos++;
  if(m_cur_pos >= m_chunks_per_file)
  {
    if(seek(0, SEEK_CUR) == -1)
      return -1;
  }
  return 0;
}

int CFileOp::write_block(PVOID buf)
{
  assert(m_fd);
#ifdef OS2
  unsigned long actual;
  int rc = DosWrite(m_fd[m_file_ind], buf, m_chunk_size, &actual);
  if(rc)
    rc = -1;
  else
    rc = 0;
  if(actual != m_chunk_size)
    rc = -1;
#else
  int rc = ::write(m_fd[m_file_ind], buf, m_chunk_size);
  if(rc != m_chunk_size)
  {
    fprintf(stderr, "Can't write block.\n");
    return -1;
  }
#endif
  m_cur_pos++;
  if(m_cur_pos >= m_chunks_per_file)
  {
    if(seek(0, SEEK_CUR) == -1)
      return -1;
  }
  return rc;
}

int CFileOp::write_block_putc()
{
  assert(m_stream);
  for(int i = 0; i < m_chunk_size; i++)
  {
    if (putc(i & 0x7f, m_stream[m_file_ind]) == EOF)
    {
      fprintf(stderr, "Can't putc() - disk full?\n");
      return -1;
    }
  }
  m_cur_pos++;
  if(m_cur_pos >= m_chunks_per_file)
  {
    if(seek(0, SEEK_CUR) == -1)
      return -1;
  }
  return 0;
}

int CFileOp::open(CPCCHAR basename, bool create, bool fopen)
{
  m_name = new char[strlen(basename) + 8];
  strcpy(m_name, basename);
  return reopen(create, fopen);
}

CFileOp::CFileOp(BonTimer &timer, int file_size, int chunk_bits, bool sync)
 : m_chunk_bits(chunk_bits)
 , m_chunk_size(1 << m_chunk_bits)
 , m_chunks_per_file(Unit / m_chunk_size * IOFileSize)
 , m_timer(timer)
 , m_stream(NULL)
 , m_fd(NULL)
 , m_isopen(false), m_name(NULL)
 , m_sync(sync)
{
  m_file_size = file_size;
  m_total_chunks = Unit / m_chunk_size * file_size;
  m_last_file_chunks = m_total_chunks % m_chunks_per_file;
  m_num_files = m_file_size / IOFileSize + 1;
  if(m_last_file_chunks == 0)
  {
    m_last_file_chunks = m_chunks_per_file;
    m_num_files--;
  }
}

typedef FILE * PFILE;

int CFileOp::reopen(bool create, bool fopen)
{
  int i;
  m_cur_pos = 0;
  m_file_ind = 0;

  if(m_isopen) close();

  m_isopen = true;
  if(fopen)
  {
    m_stream = new PFILE[m_num_files];
    for(i = 0; i < m_num_files; i++)
      m_stream[i] = NULL;
  }
  else
  {
#ifdef OS2
    m_fd = new HFILE[m_num_files];
#else
    m_fd = new int[m_num_files];
#endif
    for(i = 0; i < m_num_files; i++)
      m_fd[i] = -1;
  }
  int len = strlen(m_name);
  for(i = 0; i < m_num_files; i++)
  {
    sprintf(&m_name[len], ".%03d", i);
    if(m_open(m_name, i, create))
      return 1;
  }
  m_name[len] = '\0';
  return 0;
}

int CFileOp::m_open(CPCCHAR basename, int ind, bool create)
{
#ifdef OS2
  ULONG createFlag;
#else
  int flags;
#endif
  char *fopen_mode;
  if(create)
  { /* create from scratch */
#ifndef OS2
    unlink(basename);
#endif
    fopen_mode = "w+";
#ifdef OS2
    createFlag = OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS;
#else
    flags = O_RDWR | O_CREAT | O_EXCL;
#endif
  }
  else
  {
    fopen_mode = "r+";
#ifdef OS2
    createFlag = OPEN_ACTION_OPEN_IF_EXISTS;
#else
    flags = O_RDWR;
#endif
  }
  if(m_fd)
  {
#ifdef OS2
    ULONG action = 0;
    ULONG rc = DosOpen(basename, &m_fd[ind], &action, 0, FILE_NORMAL, createFlag
                     , OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE
                     , NULL);
    if(rc)
      m_fd[ind] = -1;
#else
    m_fd[ind] = ::open(basename, flags, S_IRUSR | S_IWUSR);
#endif
  }
  else
  {
    m_stream[ind] = fopen(basename, fopen_mode);
  }

  if( (m_fd && m_fd[ind] == -1) || (m_stream && m_stream[ind] == NULL) )
  {
    fprintf(stderr, "Can't open file %s\n", basename);
    return -1;
  }
  return 0;
}

void CFileOp::close()
{
  if(!m_isopen)
    return;
  for(int i = 0; i < m_num_files; i++)
  {
    if(m_sync)
      fflush(NULL);
    if(m_stream && m_stream[i]) fclose(m_stream[i]);
    if(m_fd && m_fd[i] != -1)
    {
      if(m_sync)
      {
        if(fsync(m_fd[i]))
          fprintf(stderr, "Can't sync files.\n");
      }
      file_close(m_fd[i]);
    }
  }
  m_isopen = false;
  delete m_fd;
  delete m_stream;
  m_fd = NULL;
  m_stream = NULL;
}


/*
 * Do a typical-of-something random I/O.  Any serious application that
 *  has a random I/O bottleneck is going to be smart enough to operate
 *  in a page mode, and not stupidly pull individual words out at
 *  odd offsets.  To keep the cache from getting too clever, some
 *  pages must be updated.  However an application that updated each of
 *  many random pages that it looked at is hard to imagine.
 * However, it would be wrong to put the update percentage in as a
 *  parameter - the effect is too nonlinear.  Need a profile
 *  of what Oracle or Ingres or some such actually does.
 * Be warned - there is a *sharp* elbow in this curve - on a 1-Mb file,
 *  most substantial unix systems show >2000 random I/Os per second -
 *  obviously they've cached the whole thing and are just doing buffer
 *  copies.
 */
int
CFileOp::doseek(long where, bool update)
{
  assert(m_fd);
  int   buf[m_chunk_size / IntSize];
  int   size;

  if (seek(where, SEEK_SET) == -1)
    return io_error("lseek in doseek");
  if ((size = read_block(PVOID(buf))) == -1)
    return io_error("read in doseek");

  /* every so often, update a block */
  if (update)
  { /* update this block */

    /* touch a word */
    buf[(int(rand()) % (size/IntSize - 2)) + 1]--;
    if(seek(where, SEEK_SET) == -1)
      return io_error("lseek in doseek update");
    if (write_block(PVOID(buf)) == -1)
      return io_error("write in doseek");
    if(m_sync)
    {
      if(fsync(m_fd[m_file_ind]))
        fprintf(stderr, "Can't sync file.\n");
    }
  } /* update this block */
  return 0;
}

