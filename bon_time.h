#ifndef BON_TIME_H
#define BON_TIME_H

#include "bonnie.h"
#include "duration.h"

struct report_s
{
  double CPU;
  double StartTime;
  double EndTime;
  double Latency;
};

struct delta_s
{
  double CPU;
  double Elapsed;
  double FirstStart;
  double LastStop;
  double Latency;
};

class BonTimer
{
public:
  enum RepType { csv, txt };

  BonTimer();

  void start();
  void stop_and_record(tests_t test);
  void add_delta_report(report_s &rep, tests_t test);
  int DoReport(CPCCHAR machine
             , int file_size, int char_file_size, int chunk_size
             , int directory_size
             , int max_size, int min_size, int num_directories
             , FILE *fp);
  void SetType(RepType type) { m_type = type; }
  double cpu_so_far();
  double time_so_far();
  void PrintHeader(FILE *fp);
  void Initialize();
  static double get_cur_time();
  static double get_cpu_use();

  void add_latency(tests_t test, double t);
 
private:
  int print_cpu_stat(tests_t test);
  int print_io_stat(tests_t test, int file_size);
  int print_file_stat(tests_t test);
  int print_seek_stat(tests_t test);
  int print_latency(tests_t test);

  delta_s m_delta[TestCount];
  RepType m_type;
  int m_directory_size;
  int m_chunk_size;
  FILE *m_fp;
  Duration m_dur;
  CPU_Duration m_cpu_dur;

  BonTimer(const BonTimer&);
  BonTimer &operator=(const BonTimer&);
};

#endif
