#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "bonnie.h"

// Read the specified number of megabytes of data from the fd and return the
// amount of time elapsed in seconds.
double readmegs(int fd, int size, char *buf);

// Returns the mean of the values in the array.  If the array contains
// more than 2 items then discard the highest and lowest thirds of the
// results before calculating the mean.
double average(double *array, int count);
void printavg(int position, double avg, int block_size);

const int meg = 1024*1024;
const int max_blocks = 10000;
typedef double *PDOUBLE;

void usage()
{
  printf("Usage: zcav file-name [count] [block-size]\n"
         "File name of \"-\" means standard input\n"
         "Count is the number of times to read the data (default 1).\n"
         "Version: " BON_VERSION "\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  double **times = NULL;
  int count[max_blocks];
  int block_size = 100;

  if(argc < 2 || argc > 4)
    usage();
  int max_loops = 1;

  if(argc >= 3)
    max_loops = atoi(argv[2]);
  if(argc == 4)
    block_size = atoi(argv[3]);

  if(max_loops < 1 || block_size < 1)
    usage();

  int i;
  if(max_loops > 1)
  {
    times = new PDOUBLE[max_loops];
    for(i = 0; i < max_loops; i++)
      times[i] = new double[max_blocks];
    for(i = 0; i < max_blocks; i++)
    {
      times[0][i] = 0.0;
      count[i] = 0;
    }
  }
  char *buf = new char[meg];
  int fd;
  if(strcmp(argv[1], "-"))
  {
    fd = open(argv[1], O_RDONLY);
    if(fd == -1)
    {
      printf("Can't open %s\n", argv[1]);
      return 1;
    }
  }
  else
  {
    fd = 0;
  }
  if(max_loops > 1)
  {
    for(int loops = 0; loops < max_loops; loops++)
    {
      if(lseek(fd, 0, SEEK_SET))
      {
        printf("Can't llseek().\n");
        return 1;
      }
      for(i = 0; times[0][i] != -1.0; i += block_size)
      {
        double read_time = readmegs(fd, block_size, buf);
        times[loops][i] += read_time;
        if(read_time < 0.0)
        {
          if(i == 0)
          {
            fprintf(stderr, "Input file too small.\n");
            return 1;
          }
          times[0][i] = -1.0;
          break;
        }
        count[i]++;
      }
    }
    printf("loops: %d\n", max_loops);
    double *item_times = new double[max_loops];
    for(i = 0; count[i]; i += block_size)
    {
      for(int j = 0; j < max_loops; j++)
        item_times[j] = times[j][i];
      printavg(i, average(item_times, count[i]), block_size);
    }
  }
  else
  {
    for(i = 0; 1; i += block_size)
    {
      double read_time = readmegs(fd, block_size, buf);
      if(read_time < 0.0)
        break;
      printavg(i, read_time, block_size);
    }
    if(i == 0)
    {
      fprintf(stderr, "Input file too small.\n");
      return 1;
    }
  }
  return 0;
}

void printavg(int position, double avg, int block_size)
{
  double num_k = double(block_size * 1024);
  if(avg < 1.0)
    printf("%d %f \n", position, avg);
  else
    printf("%d %f %d\n", position, avg, int(num_k / avg));
}

int compar(const void *a, const void *b)
{
  double *c = static_cast<double *>(a);
  double *d = static_cast<double *>(b);
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
ssize_t readall(int fd, char *buf, size_t count)
{
  ssize_t total = 0;
  while(total != static_cast<ssize_t>(count) )
  {
    ssize_t rc = read(fd, &buf[total], count - total);
    if(rc == -1 || rc == 0)
      return -1;
    total += rc;
  }
  return total;
}

// Read the specified number of megabytes of data from the fd and return the
// amount of time elapsed in seconds.
double readmegs(int fd, int size, char *buf)
{
  struct timeval tp;
 
  if (gettimeofday(&tp, static_cast<struct timezone *>(NULL)) == -1)
  {
    printf("Can't get time.\n");
    return -1.0;
  }
  double start = double(tp.tv_sec) +
    (double(tp.tv_usec) / 1000000.0);

  for(int i = 0; i < size; i++)
  {
    int rc = readall(fd, buf, meg);
    if(rc != meg)
      return -1.0;
  }
  if (gettimeofday(&tp, static_cast<struct timezone *>(NULL)) == -1)
  {
    printf("Can't get time.\n");
    return -1.0;
  }
  return (double(tp.tv_sec) + (double(tp.tv_usec) / 1000000.0))
        - start;
}

