#include "bonnie.h"
#include <stdio.h>
#include <vector>
#include <string.h>
#include <math.h>

// Maximum number of items expected on a csv line
#define MAX_ITEMS 45
#ifdef WIN32
using namespace std;
#endif
typedef vector<PCCHAR> STR_VEC;

vector<STR_VEC> data;
typedef PCCHAR * PPCCHAR;
PPCCHAR * props;

// Print the start of the HTML file
void header();
// Splits a line of text (CSV format) by commas and adds it to the list to
// process later.  Doesn't keep any pointers to the buf...
void read_in(CPCCHAR buf);
// print line in the specified line from columns start..end as a line of a
// HTML table
void print_a_line(int num, int start, int end);
// Print the end of the HTML file
void footer();
// Calculate the colors for backgrounds
void calc_vals();
// Returns a string representation of a color that maps to the value.  The
// range of values is 0..range_col and val is the value.  If reverse is set
// then low values are green and high values are red.
PCCHAR get_col(double range_col, double val, bool reverse, CPCCHAR extra);
// 0 means don't do colors, 1 means speed, 2 means CPU, 3 means latency
const int vals[MAX_ITEMS] =
  { 0,0,0,0,0,1,2,1,2,1,2,1,2,1,2,1,2,
    0,0,0,0,1,2,1,2,1,2,1,2,1,2,1,2,
    3,3,3,3,3,3,3,3,3,3,3,3 };

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
  calc_vals();
  for(i = 0; i < data.size(); i++)
  {
    printf("<TR>");
    print_a_line(i, 2, 32);
    printf("</TR>\n");
    printf("<TR>");
    print_a_line(i, 2, 2);
    printf("<TD class=\"size\" bgcolor=\"#FFFFFF\">Latency</TD><TD></TD>");
    print_a_line(i, 33, 38);
    printf("<TD COLSPAN=2></TD><TD class=\"size\" bgcolor=\"#FFFFFF\" COLSPAN=2>Latency</TD>");
    print_a_line(i, 39, 44);
    printf("</TR>\n");
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
    switch(vals[column_ind])
    {
    case 2:
      // gotta add support for CPU vals.  This means sorting on previous-col
      // divided by this column.  If a test run takes twice the CPU but does
      // three times the work it should be green!  If it does half the work but
      // uses the same CPU it should be red!
    break;
    case 1:
    case 3:
      for(unsigned int row_ind = 0; row_ind < data.size(); row_ind++)
      {
        arr[row_ind].val = 0.0;
        if(data[row_ind].size() <= column_ind
         || sscanf(data[row_ind][column_ind], "%lf", &arr[row_ind].val) == 0)
          arr[row_ind].val = 0.0;
        if(vals[column_ind] == 3 && arr[row_ind].val != 0.0)
        {
          if(strstr(data[row_ind][column_ind], "ms"))
            arr[row_ind].val *= 1000.0;
          else if(!strstr(data[row_ind][column_ind], "us"))
            arr[row_ind].val *= 1000000.0; // is !us && !ms then secs!
        }
        arr[row_ind].pos = row_ind;
      }
      qsort(arr, data.size(), sizeof(ITEM), compar);
      int col_count = -1;
      double min_col = -1.0, max_col = -1.0;
      for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
      {
        // if item is different from previous or if the first row
        // (sort_ind == 0) then increment col count
        if(sort_ind == 0 || arr[sort_ind].val != arr[sort_ind - 1].val)
        {
          if(arr[sort_ind].val != 0.0)
          {
            col_count++;
            if(min_col == -1.0)
              min_col = arr[sort_ind].val;
            else
              min_col = __min(arr[sort_ind].val, min_col);
            max_col = __max(max_col, arr[sort_ind].val);
          }
        }
        arr[sort_ind].col_ind = col_count;
      }
      // if more than 1 line has data then calculate colors
      if(col_count > 0)
      {
        double divisor = max_col / min_col;
        if(divisor < 2.0)
        {
          double mult = sqrt(2.0 / divisor);
          max_col *= mult;
          min_col /= mult;
        }
        double range_col = max_col - min_col;
        for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
        {
          if(arr[sort_ind].col_ind > -1)
          {
            bool reverse = false;
            PCCHAR extra = "";
            if(vals[column_ind] != 1)
            {
              reverse = true;
              extra = " COLSPAN=2";
            }
            props[arr[sort_ind].pos][column_ind]
                  = get_col(range_col, arr[sort_ind].val - min_col, reverse, extra);
          }
          else if(vals[column_ind] != 1)
          {
            props[arr[sort_ind].pos][column_ind] = "COLSPAN=2";
          }
        }
      }
      else
      {
        for(unsigned int sort_ind = 0; sort_ind < data.size(); sort_ind++)
        {
          if(vals[column_ind] != 1)
          {
            props[sort_ind][column_ind] = "COLSPAN=2";
          }
        }
      }
    break;
    }
  }
}

PCCHAR get_col(double range_col, double val, bool reverse, CPCCHAR extra)
{
  if(reverse)
    val = range_col - val;
  const int buf_len = 256;
  PCHAR buf = new char[buf_len];
  int green = int(255.0 * val / range_col);
  green = __min(green, 255);
  int red = 255 - green;
  _snprintf(buf, buf_len, "bgcolor=\"#%02X%02X00\"%s", red, green, extra);
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

STR_VEC split(CPCCHAR delim, CPCCHAR buf)
{
  STR_VEC arr;
  char *tmp = strdup(buf);
  while(1)
  {
    arr.push_back(tmp);
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
  STR_VEC arr = split(",", buf);
  if(strcmp(arr[0], "2") )
  {
    fprintf(stderr, "Can't process: %s\n", buf);
    free((void *)arr[0]);
    return;
  }

  data.push_back(arr);
}

void print_item(int num, int item)
{
  PCCHAR line_data = data[num][item];
  if(!line_data) line_data = "";
  if(props[num][item])
    printf("<TD %s>%s</TD>", props[num][item], line_data);
  else
    printf("<TD>%s</TD>", line_data);
}

void print_a_line(int num, int start, int end)
{
  int i;
  for(i = start; i <= end; i++)
  {
    print_item(num, i);
  }
}
