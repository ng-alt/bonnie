#ifndef BON_FILE
#define BON_FILE

#include <stdio.h>
#include "bonnie.h"
class Semaphore;
class BonTimer;

class CFileOp
{
public:
  CFileOp(BonTimer &timer, int file_size, int chunk_bits, bool sync = false);
  int open(CPCCHAR basename, bool create, bool fopen = false);
  ~CFileOp();
  int write_block_putc();
  int write_block(PVOID buf);
  int read_block_getc(char *buf);
  int read_block(PVOID buf);
  int seek(int offset, int whence);
  int doseek(long where, bool update);
  int seek_test(int threads, int seeks, bool quiet, Semaphore &sem);
  void close();
  int reopen(bool create, bool fopen = false);
  BonTimer &timer() { return m_timer; }
  int chunks() const { return m_total_chunks; }
private:
  int m_open(CPCCHAR basename, int ind, bool create);
  const int m_chunk_bits, m_chunk_size;
  int m_chunks_per_file, m_total_chunks;
  int m_last_file_chunks;
  int m_cur_pos;
  int m_file_ind;
  BonTimer &m_timer;
  FILE **m_stream;
#ifdef OS2
  HFILE *m_fd;
#else
  int *m_fd;
#endif
  int m_num_files;
  int m_file_size;
  bool m_isopen;
  char *m_name;
  CFileOp(const CFileOp &f);
  CFileOp & operator =(const CFileOp &f);
  bool m_sync;
};


#endif
