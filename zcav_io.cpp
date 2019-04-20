#include "zcav_io.h"

#ifdef WIN32
#include <io.h>
#endif

#ifndef NON_UNIX
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

ZcavRead::~ZcavRead()
{
  delete m_name;
}

int ZcavRead::Open(bool *finished, int block_size
                 , const char *file, const char *log)
{
  m_name = strdup(file);
  m_finished = finished;
  m_block_size = block_size;

  if(strcmp(file, "-"))
  {
    m_fd = file_open(file, O_RDONLY);
    if(m_fd == -1)
    {
      fprintf(stderr, "Can't open %s\n", file);
      return 1;
    }
  }
  else
  {
    m_fd = 0;
  }
  if(strcmp(log, "-"))
  {
    m_logFile = true;
    m_log = fopen(log, "w");
    if(m_log == NULL)
    {
      fprintf(stderr, "Can't open %s\n", log);
      file_close(m_fd);
      return 1;
    }
  }
  else
  {
    m_logFile = false;
    m_log = stdout;
  }
  return 0;
}

void ZcavRead::Close()
{
  if(m_logFile)
    fclose(m_log);
  if(m_fd != 0)
    ::close(m_fd);
}

int ZcavRead::writeStatus(int fd, char c)
{
  if(write(fd, &c, 1) != 1)
  {
    fprintf(stderr, "Write channel broken\n");
    return 1;
  }
  return 0;
}

int ZcavRead::Read(int max_loops, int max_size, int writeCom)
{
  int i;
  bool exiting = false;
  for(int loops = 0; !exiting && loops < max_loops; loops++)
  {
    if(lseek(m_fd, 0, SEEK_SET))
    {
      fprintf(stderr, "Can't llseek().\n");
      writeStatus(writeCom, eSEEK);
      return 1;
    }
    // i is block index
    bool nextLoop = false;
    for(i = 0; !nextLoop && (!max_size || i < max_size) && (loops == 0 || m_times[i][0] != -1.0); i++)
    {
      if(loops == 0)
        m_times.push_back(new double[max_loops]);
      double read_time = readmegs();
      m_times[i][loops] = read_time;
      if(read_time < 0.0)
      {
        if(i == 0)
        {
          fprintf(stderr, "Input file \"%s\" too small.\n", m_name);
          writeStatus(writeCom, eSIZE);
          return 1;
        }
        nextLoop = true;
      }
      if(loops == 0)
        m_count.push_back(0);
      m_count[i]++;
    } // end loop for reading blocks
    if(exiting)
      return 1;
  } // end loop for multiple disk reads
  fprintf(m_log, "#loops: %d\n", max_loops);
  fprintf(m_log, "#block K/s time\n");
//  for(i = 0; (!max_size || i < max_size) && m_count[i]; i++)
  for(i = 0; m_times[i][0] != -1.0; i++)
  {
    printavg(i, average(m_times[i], m_count[i]), m_block_size);
  }
  writeStatus(writeCom, eEND);
  return 0;
}

void ZcavRead::printavg(int position, double avg, int block_size)
{
  double num_k = double(block_size * 1024);
  if(avg < 1.0)
    fprintf(m_log, "#%d ++++ %f\n", position * block_size, avg);
  else
    fprintf(m_log, "%d %d %f\n", position * block_size, int(num_k / avg), avg);
}

int compar(const void *a, const void *b)
{
  double *c = (double *)(a);
  double *d = (double *)(b);
  if(*c < *d) return -1;
  if(*c > *d) return 1;
  return 0;
}

// Returns the mean of the values in the array.  If the array contains
// more than 2 items then discard the highest and lowest thirds of the
// results before calculating the mean.
double average(double *array, int count)
{
  qsort(array, count, sizeof(double), compar);
  int skip = count / 3;
  int arr_items = count - (skip * 2);
  double total = 0.0;
  for(int i = skip; i < (count - skip); i++)
  {
    total += double(array[i]);
  }
  return total / double(arr_items);
}

// just like the read() system call but uses a member for the buffer and will
// not return a partial result.
ssize_t ZcavRead::readall(int count)
{
  ssize_t total = 0;
  while(total != static_cast<ssize_t>(count) )
  {
    ssize_t rc = file_read(m_fd, &m_buf[total], count - total);
    if(rc == -1 || rc == 0)
      return -1;
    total += rc;
  }
  return total;
}

// Read the specified number of megabytes of data from the fd and return the
// amount of time elapsed in seconds.
double ZcavRead::readmegs()
{
  m_dur.start();

  for(int i = 0; i < m_block_size; i++)
  {
    int rc = readall(meg);
    if(rc != meg)
      return -1.0;
  }
  return m_dur.stop();
}

