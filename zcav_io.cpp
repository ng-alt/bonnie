#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "zcav_io.h"

ZcavRead::~ZcavRead()
{
  delete m_name;
}

int ZcavRead::open(bool *finished, int block_size
                 , const char *file, const char *log)
{
  m_name = strdup(file);
  m_finished = finished;
  m_block_size = block_size;

  if(strcmp(file, "-"))
  {
    m_fd = ::open(file, O_RDONLY);
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
      ::close(m_fd);
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

void ZcavRead::close()
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

int ZcavRead::read(int max_loops, int max_size, bool wait
                 , int readCom, int writeCom)
{
  int i;
  bool exiting = false;
  for(int loops = 0; !exiting && loops < max_loops; loops++)
  {
    if(lseek(m_fd, 0, SEEK_SET))
    {
      fprintf(stderr, "Can't llseek().\n");
      writeStatus(writeCom, eEND);
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
          writeStatus(writeCom, eEND);
          return 1;
        }
        m_times[i][0] = -1.0;
        break;
      }
      if(wait)
      {
        if((!max_size || (i+1) < max_size)
           && (loops == 0 || m_times[i+1][0] != -1.0))
        {
          if(writeStatus(writeCom, eBLOCK))
            return 1;
        }
      }
      if(loops == 0)
        m_count.push_back(0);
      m_count[i]++;
      if(wait)
      {
        char c;
        if(::read(readCom, &c, 1) != 1)
        {
          fprintf(stderr, "Read comms channel broken\n");
          return 1;
        }
printf("thread got:%c\n", c);
        switch (c)
        {
        case eEXIT:
          exiting = true;
          nextLoop = true;
        break;
        case eLOOP:
          nextLoop = true;
        break;
        case eBLOCK:
        break;
        default:
          printf("unexpected character %c\n", c);
        }
      }
    } // end loop for reading blocks
    if(!exiting && wait && writeStatus(writeCom, eLOOP))
      return 1;
printf("got here!\n");
  } // end loop for multiple disk reads
  fprintf(m_log, "#loops: %d\n", max_loops);
  fprintf(m_log, "#block K/s time\n");
//  for(i = 0; (!max_size || i < max_size) && m_count[i]; i++)
  for(i = 0; (unsigned)i < m_count.size(); i++)
  {
    printavg(i, average(m_times[i], m_count[i]), m_block_size);
  }
  return writeStatus(writeCom, eEND);
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
    total += array[i];
  }
  return total / arr_items;
}

// just like the read() system call but takes a (char *) and will not return
// a partial result.
ssize_t ZcavRead::readall(int count)
{
  ssize_t total = 0;
  while(total != static_cast<ssize_t>(count) )
  {
    ssize_t rc = ::read(m_fd, &m_buf[total], count - total);
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
  struct timeval tp;
 
  if (gettimeofday(&tp, static_cast<struct timezone *>(NULL)) == -1)
  {
    fprintf(stderr, "Can't get time.\n");
    return -1.0;
  }
  double start = double(tp.tv_sec) +
    (double(tp.tv_usec) / 1000000.0);

  for(int i = 0; i < m_block_size; i++)
  {
    int rc = readall(meg);
    if(rc != meg)
      return -1.0;
  }
  if (gettimeofday(&tp, static_cast<struct timezone *>(NULL)) == -1)
  {
    fprintf(stderr, "Can't get time.\n");
    return -1.0;
  }
  return (double(tp.tv_sec) + (double(tp.tv_usec) / 1000000.0))
        - start;
}

