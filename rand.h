#ifndef RAND_H
#define RAND_H

#include "port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

class Rand
{
public:
  Rand() : m_arr(NULL) , m_size(0) , m_ind(0) { }

  bool seedFile(CPCCHAR name);

  void seedNum(UINT num);

  int getNum()
  {
    if(m_arr)
    {
      if(m_ind >= m_size)
        m_ind = 0;
      else
        m_ind++;
      return m_arr[m_ind];
    }
    else
      return rand();
  }

  int getSize() { return m_size; }

  string getSeed() { return m_name; }

private:
  int *m_arr;
  int m_size;
  int m_ind;
  string m_name;

  Rand(const Rand &t);
  Rand & operator =(const Rand &t);
};

#endif
