#include "port.h"
#ifndef WIN32
#include <unistd.h>
#endif

#include "zcav_io.h"
#include "thread.h"
#ifdef NON_UNIX
#include "getopt.h"
#endif

#define TOO_MANY_LOOPS 100

void usage()
{
  fprintf(stderr
       , "Usage: zcav [-b block-size] [-c count] [-s max-size] [-w]\n"
#ifndef NON_UNIX
         "            [-u uid-to-use:gid-to-use] [-g gid-to-use]\n"
#endif
         "            [-l log-file] [-f] file-name\n"
         "            [-l log-file [-f] file-name]...\n"
         "\n"
         "File name of \"-\" means standard input\n"
         "Count is the number of times to read the data (default 1).\n"
         "Max size is the amount of data to read from each device.\n"
         "-w means to wait until all devices have read a block before reading the next.\n"
         "\n"
         "Version: " BON_VERSION "\n");
  exit(1);
}

class MultiZcav : public Thread
{
public:
  MultiZcav();
  MultiZcav(int threadNum, const MultiZcav *parent);
  virtual ~MultiZcav();

  virtual int action(PVOID param);

  int runit();

  void setFileLogNames(const char *file, const char *log)
  {
    m_fileNames.push_back(file);
    m_logNames.push_back(log);
    m_readers->push_back(new ZcavRead);
  }

  void setBlockSize(int block_size)
  {
    m_block_size = block_size;
    if(m_block_size < 1)
      usage();
  }

  void setLoops(int max_loops)
  {
    m_max_loops = max_loops;
    if(max_loops < 1 || max_loops > TOO_MANY_LOOPS)
      usage();
  }

  void setMaxSize(int max_size)
  {
    m_max_size = max_size;
    if(max_size < 1)
      usage();
  }

  void setWait() { m_wait = true; }

private:
  virtual Thread *newThread(int threadNum)
                  { return new MultiZcav(threadNum, this); }

  vector<const char *> m_fileNames, m_logNames;
  vector<ZcavRead *> *m_readers;

  int m_block_size, m_max_loops, m_max_size;
  bool m_wait;

  MultiZcav(const MultiZcav &m);
  MultiZcav & operator =(const MultiZcav &m);
};

MultiZcav::MultiZcav()
{
  m_block_size = 100;
  m_max_loops = 1;
  m_max_size = 0;
  m_wait = false;
  m_readers = new vector<ZcavRead *>;
}

MultiZcav::MultiZcav(int threadNum, const MultiZcav *parent)
 : Thread(threadNum, parent)
 , m_readers(parent->m_readers)
 , m_block_size(parent->m_block_size)
 , m_max_loops(parent->m_max_loops)
 , m_max_size(parent->m_max_size)
 , m_wait(parent->m_wait)
{
}

int MultiZcav::action(PVOID)
{
  ZcavRead *zc = (*m_readers)[getThreadNum() - 1];
  int rc = zc->read(m_max_loops, m_max_size / m_block_size, m_wait
                  , m_read, m_write);
  zc->close();
  char c = eEXIT;
  Write(&c, 1, 0);
  return rc;
}

MultiZcav::~MultiZcav()
{
  if(getThreadNum() < 1)
  {
    while(m_readers->size())
    {
      delete m_readers->back();
      m_readers->pop_back();
    }
    delete m_readers;
  }
}

int MultiZcav::runit()
{
  unsigned int i;
  unsigned int num_threads = m_fileNames.size();
  if(num_threads < 1)
    usage();
  for(i = 0; i < num_threads; i++)
  {
    if((*m_readers)[i]->open(NULL, m_block_size, m_fileNames[i], m_logNames[i]))
    {
      return 1;
    }
  }
  go(NULL, num_threads);
  char c;
  if(m_wait)
  {
    char *buf = new char[num_threads];
    bool end = false;
    while(num_threads)
    {
      bool loop = false;
      for(i = 0; i < num_threads; i++)
      {
        if(Read(&c, 1, 0) != 1) return 1;
printf("read data - threads:%d, char:%c\n", num_threads, c);
        switch (c)
        {
        case eLOOP:
          if(!end)
            loop = true;
        break;
        case eEND:
          end = true;
        break;
        case eEXIT:
          end = true;
          num_threads--;
        break;
        }
      }
      if(end)
        memset(buf, eEXIT, num_threads);
      else if(loop)
        memset(buf, eLOOP, num_threads);
      else
        memset(buf, eBLOCK, num_threads);
printf("writing:%c\n", *buf);
      if(Write(buf, (int)num_threads) != (int)num_threads)
        return 1;
    }
  }
  else while(num_threads)
  {
    if(Read(&c, 1, 0) != 1)
      printf("can't read!\n");
    if(c == eEXIT)
      num_threads--;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  MultiZcav mz;

  if(argc < 2)
    usage();

#ifndef NON_UNIX
  char *userName = NULL, *groupName = NULL;
#endif
  int c;
  const char *log = "-";
  while(-1 != (c = getopt(argc, argv, "-c:b:f:l:s:w"
#ifndef NON_UNIX
                                     "u:g:"
#endif
                          )) )
  {
    switch(char(c))
    {
      case 'b':
        mz.setBlockSize(atoi(optarg));
      break;
      case 'c':
        mz.setLoops(atoi(optarg));
      break;
      case 'l':
        log = optarg;
      break;
      case 's':
        mz.setMaxSize(atoi(optarg));
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
#endif
      break;
      case 'w':
        mz.setWait();
      break;
      case 'f':
      case char(1):
        mz.setFileLogNames(optarg, log);
        log = "-";
      break;
      default:
        usage();
    }
  }
#ifndef NON_UNIX
  if(userName || groupName)
  {
    if(bon_setugid(userName, groupName))
      return 1;
    if(userName)
      free(userName);
  }
#endif

  return mz.runit();
}


