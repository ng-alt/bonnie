#ifdef OS2
#define INCL_DOSFILEMGR
#define INCL_BASE
#define INCL_DOSMISC
#endif

#include <stdlib.h>

#include "bon_time.h"
#include "duration.h"
#include <time.h>
#include <string.h>

#ifdef WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#endif

#ifndef NON_UNIX
#include <sys/time.h>
#include <unistd.h>
#include <algo.h>
#endif

void BonTimer::start()
{
  m_dur.start();
  m_cpu_dur.start();
}
void BonTimer::stop_and_record(tests_t test)
{
  m_delta[test].Elapsed = m_dur.stop();
  m_delta[test].CPU = m_cpu_dur.stop();
}

void BonTimer::add_delta_report(report_s &rep, tests_t test)
{
  if(m_delta[test].CPU == 0.0)
  {
    m_delta[test].FirstStart = rep.StartTime;
    m_delta[test].LastStop = rep.EndTime;
  }
  else
  {
    m_delta[test].FirstStart = __min(m_delta[test].FirstStart, rep.StartTime);
    m_delta[test].LastStop = __max(m_delta[test].LastStop, rep.EndTime);
  }
  m_delta[test].CPU += rep.CPU;
  m_delta[test].Elapsed = m_delta[test].LastStop - m_delta[test].FirstStart;
  m_delta[test].Latency = __max(m_delta[test].Latency, rep.Latency);
}

BonTimer::BonTimer()
 : m_type(txt)
{
  Initialize();
}

void
BonTimer::Initialize()
{
  for(int i = 0; i < TestCount; i++)
  {
    m_delta[i].CPU = 0.0;
    m_delta[i].Elapsed = 0.0;
    m_delta[i].Latency = 0.0;
  }
}

void
BonTimer::add_latency(tests_t test, double t)
{
  m_delta[test].Latency = __max(m_delta[test].Latency, t);
}

int BonTimer::print_cpu_stat(tests_t test)
{
#ifndef WIN32
  if(m_delta[test].Elapsed == 0.0)
  {
#endif
    if(m_type == txt)
      fprintf(m_fp, "    ");
    else
      fprintf(m_fp, ",");
    return 0;
#ifndef WIN32
  }
  if(m_delta[test].Elapsed < MinTime)
  {
    if(m_type == txt)
      fprintf(m_fp, " +++");
    else
      fprintf(m_fp, ",+++");
    return 0;
  }
  double cpu = m_delta[test].CPU / m_delta[test].Elapsed * 100.0;
  if(m_type == txt)
  {
      fprintf(m_fp, " %3d", (int)cpu);
  }
  else
    fprintf(m_fp, ",%d", (int)cpu);
  return 0;
#endif
}

int BonTimer::print_io_stat(tests_t test, int file_size)
{
  int stat = int(double(file_size) / (m_delta[test].Elapsed / 1024.0));
  if(m_type == txt)
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, "      ");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, " +++++");
    else
      fprintf(m_fp, " %5d", stat);

  }
  else
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, ",");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, ",+++++");
    else
      fprintf(m_fp, ",%d", stat);
  }
  print_cpu_stat(test);
  return 0;
}

int BonTimer::print_seek_stat(tests_t test)
{
  double seek_stat = double(Seeks) / m_delta[test].Elapsed;
  if(m_type == txt)
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, "      ");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, " +++++");
    else
      fprintf(m_fp, " %5.1f", seek_stat);
  }
  else
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, ",");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, ",+++++");
    else
      fprintf(m_fp, ",%.1f", seek_stat);
  }
  print_cpu_stat(test);
  return 0;
}

int BonTimer::print_file_stat(tests_t test)
{
  int stat = int(double(m_directory_size) * double(DirectoryUnit)
                / m_delta[test].Elapsed);
  if(m_type == txt)
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, "      ");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, " +++++");
    else
      fprintf(m_fp, " %5d", stat);
  }
  else
  {
    if(m_delta[test].Elapsed == 0.0)
      fprintf(m_fp, ",");
    else if(m_delta[test].Elapsed < MinTime)
      fprintf(m_fp, ",+++++");
    else
      fprintf(m_fp, ",%d", stat);
  }
  print_cpu_stat(test);
  return 0;
}

int BonTimer::print_latency(tests_t test)
{
  char buf[10];
  if(m_delta[test].Latency == 0.0)
  {
    buf[0] = '\0';
  }
  else
  {
    if(m_delta[test].Latency > 99.999999)
      _snprintf(buf
#ifndef OS2
, sizeof(buf)
#endif
              , "%ds", int(m_delta[test].Latency));
    else if(m_delta[test].Latency > 0.099999)
      _snprintf(buf
#ifndef OS2
, sizeof(buf)
#endif
              , "%dms", int(m_delta[test].Latency * 1000.0));
    else
      _snprintf(buf
#ifndef OS2
, sizeof(buf)
#endif
              , "%dus", int(m_delta[test].Latency * 1000000.0));
  }
  if(m_type == txt)
  {
    fprintf(m_fp, " %9s", buf);
  }
  else
  {
    fprintf(m_fp, ",%s", buf);
  }
  return 0;
}

void
BonTimer::PrintHeader(FILE *fp)
{
  fprintf(fp, "format_version,bonnie_version,name,file_size,chunk_size,putc,putc_cpu,put_block,put_block_cpu,rewrite,rewrite_cpu,getc,getc_cpu,get_block,get_block_cpu,seeks,seeks_cpu");
  fprintf(fp, ",num_files,max_size,min_size,num_dirs,seq_create,seq_create_cpu,seq_stat,seq_stat_cpu,seq_del,seq_del_cpu,ran_create,ran_create_cpu,ran_stat,ran_stat_cpu,ran_del,ran_del_cpu");
  fprintf(fp, ",putc_latency,put_block_latency,rewrite_latency,getc_latency,get_block_latency,seeks_latency,seq_create_latency,seq_stat_latency,seq_del_latency,ran_create_latency,ran_stat_latency,ran_del_latency");
  fprintf(fp, "\n");
  fflush(NULL);
}

int
BonTimer::DoReport(CPCCHAR machine
                 , int file_size, int char_file_size, int chunk_size
                 , int directory_size
                 , int max_size, int min_size, int num_directories
                 , FILE *fp)
{
  int i;
  m_fp = fp;
  m_directory_size = directory_size;
  m_chunk_size = chunk_size;
  const int txt_machine_size = 20;
  char separator = ':';
  if(m_type == csv)
    separator = ',';
  if(file_size)
  {
    if(m_type == txt)
    {
      fprintf(m_fp, "Version %5s       ", BON_VERSION);
      fprintf(m_fp,
        "------Sequential Output------ --Sequential Input- --Random-\n");
      fprintf(m_fp, "                    ");
      fprintf(m_fp,
        "-Per Chr- --Block-- -Rewrite- -Per Chr- --Block-- --Seeks--\n");
      if(m_chunk_size == DefaultChunkSize)
        fprintf(m_fp, "Machine        Size ");
      else
        fprintf(m_fp, "Machine   Size:chnk ");
      fprintf(m_fp, "K/sec %%CP K/sec %%CP K/sec %%CP K/sec %%CP K/sec ");
      fprintf(m_fp, "%%CP  /sec %%CP\n");
    }
    char size_buf[1024];
    if(file_size % 1024 == 0)
      sprintf(size_buf, "%dG", file_size / 1024);
    else
      sprintf(size_buf, "%dM", file_size);
    char *tmp = size_buf + strlen(size_buf);
    if(m_chunk_size != DefaultChunkSize)
    {
      if(m_chunk_size >= 1024)
        sprintf(tmp, "%c%dk", separator, m_chunk_size / 1024);
      else
        sprintf(tmp, "%c%d", separator, m_chunk_size);
    }
    else if(m_type == csv)
    {
      sprintf(tmp, "%c", separator);
    }
    char buf[4096];
    if(m_type == txt)
    {
      // copy machine name to buf
      //
      _snprintf(buf
#ifndef OS2
, txt_machine_size - 1
#endif
              , "%s                  ", machine);
      buf[txt_machine_size - 1] = '\0';
      // set cur to point to a byte past where we end the machine name
      // size of the buf - size of the new data - 1 for the space - 1 for the
      // terminating zero on the string
      char *cur = &buf[txt_machine_size - strlen(size_buf) - 2];
      *cur = ' '; // make cur a space
      cur++; // increment to where we store the size
      strcpy(cur, size_buf);  // copy the size in
      fputs(buf, m_fp);
    }
    else
    {
      printf("2," BON_VERSION ",%s,%s", machine, size_buf);
    }
    for(i = Putc; i < Lseek; i++)
    {
      if(i == Putc || i == Getc)
        print_io_stat(tests_t(i), char_file_size);
      else
        print_io_stat(tests_t(i), file_size);
    }
    print_seek_stat(Lseek);
    if(m_type == txt)
    {
      fprintf(m_fp, "\nLatency          ");
      for(i = Putc; i <= Lseek; i++)
        print_latency(tests_t(i));
      fprintf(m_fp, "\n");
    }
  }
  else if(m_type == csv)
  {
    fprintf(m_fp, "2," BON_VERSION ",%s,,,,,,,,,,,,,,", machine);
  }

  if(m_directory_size)
  {
    char buf[128];
    char *tmp;
    sprintf(buf, "%d", m_directory_size);
    tmp = &buf[strlen(buf)];
    if(m_type == csv)
    {
      if(max_size == -1)
      {
        sprintf(tmp, ",link,");
      }
      else if(max_size == -2)
      {
        sprintf(tmp, ",symlink,");
      }
      else if(max_size)
      {
        sprintf(tmp, ",%d,%d", max_size, min_size);
      }
      else
      {
        sprintf(tmp, ",,");
      }
    }
    else
    {
      if(max_size == -1)
      {
        sprintf(tmp, ":link");
      }
      else if(max_size == -2)
      {
        sprintf(tmp, ":symlink");
      }
      else if(max_size)
      {
        sprintf(tmp, ":%d:%d", max_size, min_size);
      }
    }
    tmp = &buf[strlen(buf)];
    if(num_directories > 1)
    {
      if(m_type == txt)
        sprintf(tmp, "/%d", num_directories);
      else
        sprintf(tmp, ",%d", num_directories);
    }
    else if(m_type == csv)
    {
       sprintf(tmp, ",");
    }
    if(m_type == txt)
    {
      if(file_size)
        fprintf(m_fp, "                    ");
      else
        fprintf(m_fp, "Version %5s       ", BON_VERSION);
      fprintf(m_fp,
        "------Sequential Create------ --------Random Create--------\n");
      if(file_size)
        fprintf(m_fp, "                    ");
      else
        fprintf(m_fp, "%-19.19s ", machine);
      fprintf(m_fp,
           "-Create-- --Read--- -Delete-- -Create-- --Read--- -Delete--\n");
      if(min_size)
      {
        fprintf(m_fp, "files:max:min       ");
      }
      else
      {
        if(max_size)
          fprintf(m_fp, "files:max           ");
        else
          fprintf(m_fp, "              files ");
      }
      fprintf(m_fp, " /sec %%CP  /sec %%CP  /sec %%CP  /sec %%CP  /sec ");
      fprintf(m_fp, "%%CP  /sec %%CP\n");
      fprintf(m_fp, "%19s", buf);
    }
    else
    {
      fprintf(m_fp, ",%s", buf);
    }
    for(i = CreateSeq; i < TestCount; i++)
      print_file_stat(tests_t(i));
    if(m_type == txt)
    {
      fprintf(m_fp, "\nLatency          ");
      for(i = CreateSeq; i < TestCount; i++)
        print_latency(tests_t(i));
    }
  }
  else if(m_type == csv)
  {
    fprintf(m_fp, ",,,,,,,,,,,,,,,,");
  }
  if(m_type == csv)
  {
    for(i = Putc; i < TestCount; i++)
      print_latency(tests_t(i));
  }
  fprintf(m_fp, "\n");
  fflush(stdout);
  return 0;
}

