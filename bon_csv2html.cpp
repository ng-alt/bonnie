#include "bonnie.h"
#include <stdio.h>
#include <vector>
#include <string.h>
#include <math.h>

#define MAX_ITEMS 33
#ifdef WIN32
using namespace std;
#endif
typedef vector<PCCHAR> STR_VEC;

vector<STR_VEC *> data;
typedef PCCHAR * PPCCHAR;
PPCCHAR * props;

void header();
void read_in(CPCCHAR buf);
void print_a_line(int num, int start, int end);
void footer();
void calc_vals();
PCCHAR get_col(double range_col, double val);
const int vals[MAX_ITEMS] =
  { 0,0,0,0,0,1,2,1,2,1,2,1,2,1,2,1,2,
    0,0,0,0,1,2,1,2,1,2,1,2,1,2,1,2 };

void usage()
{
  exit(1);
}

int main(int argc, char **argv)
{
  header();

  char buf[1024];

  FILE *fp = NULL;
  if(argc > 1)
  {
    fp = fopen(argv[1], "r");
    if(!fp)
      usage();
  }
  while(fgets(buf, sizeof(buf), fp ? fp : stdin))
  {
    buf[sizeof(buf) - 1] = '\0';
    strtok(buf, "\r\n");
    read_in(buf);
  }
//  $vals{putc} = 5;
//  $vals{put_block} = 7;
//  $vals{rewrite} = 9;
//  $vals{getc} = 11;
//  $vals{get_block} = 13;
//  $vals{seeks} = 15;
//  $vals{seq_create} = 21;
//  $vals{seq_stat} = 23;
//  $vals{seq_del} = 25;
//  $vals{ran_create} = 27;
//  $vals{ran_stat} = 29;
//  $vals{ran_del} = 31;

  props = new PPCCHAR[data.size()];
  unsigned int i;
  for(i = 0; i < data.size(); i++)
  {
    props[i] = new PCCHAR[MAX_ITEMS];
    props[i][0] = NULL;
    props[i][1] = NULL;
    props[i][2] = "bgcolor=\"#FFFFFF\" class=\"rowheader\"><FONT SIZE=+1";
    props[i][3] = "class=\"size\" bgcolor=\"#FFFFFF\"";
    int j;
    for(j = 4; j < MAX_ITEMS; j++)
    {
      if( (j >= 17 && j <= 20) || j == 4 )
      {
        props[i][j] = "class=\"size\" bgcolor=\"#FFFFFF\"";
      }
      else
      {
//        props[i][j] = "bgcolor=\"#FFFFFF\"";
        props[i][j] = NULL;
      }
    }
  }
  if(data.size() > 1)
  {
    calc_vals();
  }
  for(i = 0; i < data.size(); i++)
  {
    print_a_line(i, 2, 32);
  }
  footer();
  return 0;
}

typedef struct { double val; int pos; int col_ind; } ITEM;
typedef ITEM * PITEM;

int compar(const void *a, const void *b)
{
  double a1 = PITEM(a)->val;
  double b1 = PITEM(b)->val;
  if(a1 < b1)
    return -1;
  if(a1 > b1)
    return 1;
  return 0;
}

void calc_vals()
{
  ITEM *arr = new ITEM[data.size()];
  for(unsigned int column_ind = 0; column_ind < MAX_ITEMS; column_ind++)
  {
    if(vals[column_ind] == 1)
    {
      unsigned int row_ind;
      for(row_ind = 0; row_ind < data.size(); row_ind++)
      {
        if((*data[row_ind]).size() <= column_ind
         || sscanf((*data[row_ind])[column_ind], "%lf", &arr[row_ind].val) == 0)
          arr[row_ind].val = 0.0;
        arr[row_ind].pos = row_ind;
      }
      qsort(arr, data.size(), sizeof(ITEM), compar);
      int ind = -1;
      double min_col = -1.0, max_col = -1.0;
      for(row_ind = 0; row_ind < data.size(); row_ind++)
      {
        // if item is different from previous then increment col index
        if(row_ind == 0 || arr[row_ind].val != arr[row_ind - 1].val)
        {
          if(arr[row_ind].val != 0.0)
          {
            ind++;
            if(min_col == -1.0)
              min_col = arr[row_ind].val;
            else
              min_col = __min(arr[row_ind].val, min_col);
            max_col = __max(max_col, arr[row_ind].val);
          }
        }
        arr[row_ind].col_ind = ind;
      }
      if(ind > 0)
      {
        double divisor = max_col / min_col;
        if(divisor < 2.0)
        {
          double mult = sqrt(2.0 / divisor);
          max_col *= mult;
          min_col /= mult;
        }
        double range_col = max_col - min_col;
        for(row_ind = 0; row_ind < data.size(); row_ind++)
        {
          if(arr[row_ind].col_ind > -1)
            props[row_ind][column_ind]
                          = get_col(range_col, arr[row_ind].val - min_col);
        }
      }
    }
  }
}

PCCHAR get_col(double range_col, double val)
{
  const int buf_len = 18;
  PCHAR buf = new char[buf_len];
  int green = int(255.0 * val / range_col);
  green = __min(green, 255);
  int red = 255 - green;
  _snprintf(buf, buf_len, "bgcolor=\"#%02X%02X00\"", green, red);
  buf[buf_len - 1] = '\0';
  return buf;
}

void heading(const char * const head);

void header()
{
  printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
"<HTML>"
"<HEAD><TITLE>Bonnie++ Benchmark results</TITLE>"
"<STYLE type=\"text/css\">"
"TD.header {text-align: center; backgroundcolor: \"#CCFFFF\" }"
"TD.rowheader {text-align: center; backgroundcolor: \"#CCCFFF\" }"
"TD.size {text-align: center; backgroundcolor: \"#CCCFFF\" }"
"TD.ksec {text-align: center; fontstyle: italic }"
"</STYLE>"
"<BODY>"
"<TABLE ALIGN=center BORDER=3 CELLPADDING=2 CELLSPACING=1>"
"<TR><TD COLSPAN=3 class=\"header\"><FONT SIZE=+1><B>"
"Version " BON_VERSION
"</B></FONT></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Sequential Output</B></FONT></TD>"
"<TD COLSPAN=4 class=\"header\"><FONT SIZE=+2><B>Sequential Input</B></FONT></TD>"
"<TD COLSPAN=2 ROWSPAN=2 class=\"header\"><FONT SIZE=+2><B>Random<BR>Seeks</B></FONT></TD>"
"<TD COLSPAN=4 class=\"header\"></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Sequential Create</B></FONT></TD>"
"<TD COLSPAN=6 class=\"header\"><FONT SIZE=+2><B>Random Create</B></FONT></TD>"
"</TR>"
"<TR><TD></TD>"
"<TD>Size</TD><TD>Chunk Size</TD>");
  heading("Per Char"); heading("Block"); heading("Rewrite");
  heading("Per Char"); heading("Block");
  printf("<TD>Num Files</TD><TD>Max Size</TD><TD>Min Size</TD><TD>Num Dirs</TD>");
  heading("Create"); heading("Read"); heading("Delete");
  heading("Create"); heading("Read"); heading("Delete");
  printf("</TR>");

  printf("<TR><TD COLSPAN=3></TD>");

  int i;
  CPCCHAR ksec_form = "<TD class=\"ksec\"><FONT SIZE=-2>%s/sec</FONT></TD>"
                      "<TD class=\"ksec\"><FONT SIZE=-2>%% CPU</FONT></TD>";
  for(i = 0; i < 5; i++)
  {
    printf(ksec_form, "K");
  }
  printf(ksec_form, "");
  printf("<TD COLSPAN=4></TD>");
  for(i = 0; i < 6; i++)
  {
    printf(ksec_form, "");
  }
  printf("</TR>\n");
}

void heading(const char * const head)
{
  printf("<TD COLSPAN=2>%s</TD>", head);
}

void footer()
{
  printf("</TABLE>\n</BODY></HTML>\n");
}

STR_VEC *split(CPCCHAR delim, CPCCHAR buf)
{
  STR_VEC *arr = new STR_VEC;
  char *tmp = strdup(buf);
  while(1)
  {
    arr->push_back(tmp);
    tmp = strstr(tmp, delim);
    if(!tmp)
      break;
    *tmp = '\0';
    tmp += strlen(delim);
  }
  return arr;
}

void read_in(CPCCHAR buf)
{
  STR_VEC *arr = split(",", buf);
  if(strcmp((*arr)[0], "2") )
  {
    fprintf(stderr, "Can't process: %s\n", buf);
    free((void *)(*arr)[0]);
    delete arr;
    return;
  }

//printf("str:%s,%s,%s,%s\n", (*arr)[2], (*arr)[3], (*arr)[4], (*arr)[5]);
  data.push_back(arr);
}

void print_a_line(int num, int start, int end)
{
  STR_VEC *arr = data[num];

  printf("<TR>");
  int i;
  for(i = start; i < end; i++)
  {
    PCCHAR line_data = (*arr)[i];
    if(!line_data) line_data = "";
    if(props[num][i])
      printf("<TD %s>%s</TD>", props[num][i], line_data);
    else
      printf("<TD>%s</TD>", line_data);
  }
  printf("</TR>\n");
}
