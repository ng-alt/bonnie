#ifndef ZCAV_IO_H
#define ZCAV_IO_H
#include <vector>

#include "bonnie.h"
#include "duration.h"
#ifdef WIN32
using namespace std;
#endif

enum results
{
  eEND = 'e',
  eBLOCK = 'b',
  eLOOP = 'l',
  eEXIT = 'z'
};

// Returns the mean of the values in the array.  If the array contains
// more than 2 items then discard the highest and lowest thirds of the
// results before calculating the mean.
double average(double *array, int count);

const int meg = 1024*1024;

class ZcavRead
{
public:
  ZcavRead(){ m_name = NULL; }
  ~ZcavRead();

  int open(bool *finished, int block_size, const char *file, const char *log);
  void close();
  int read(int max_loops, int max_size, bool wait, int readCom, int writeCom);

private:
  ssize_t readall(int count);

  // write the status to the parent thread
  int writeStatus(int fd, char c);

  // Read the m_block_count megabytes of data from the fd and return the
  // amount of time elapsed in seconds.
  double readmegs();
  void printavg(int position, double avg, int block_size);

  bool *m_finished;
  vector <double *> m_times;
  vector<int> m_count; // number of times each block has been read
  char m_buf[meg];
  int m_fd;
  FILE *m_log;
  bool m_logFile;
  int m_block_size;
  char *m_name;
  Duration m_dur;
};

#endif

