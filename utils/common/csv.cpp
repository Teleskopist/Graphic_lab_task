#include "csv.h"
#include "strings.h"

#include <fstream>
#include <iostream>
#include <sstream>

void save_csv(const std::string &filename, const CSVData &data, bool in_quotes)
{
  std::ofstream fs(filename);
  for (int i = 0; i < data.header.size(); i++)
  {
    if (i > 0)
      fs << ",";
    if (in_quotes)
      fs << "\"" << data.header[i] << "\"";
    else
      fs << data.header[i];
  }
  fs << "\n";

  for (int i = 0; i < data.row_count; i++)
  {
    for (int j = 0; j < data.columns.size(); j++)
    {
      if (j > 0)
        fs << ",";
      if (in_quotes)
        fs << "\"" << data.columns[j][i] << "\"";
      else
        fs << data.columns[j][i];
    }
    fs << "\n";
  }

  fs.flush();
  fs.close();
}

CSVData load_csv(const std::string &filename)
{
  CSVData data;

  std::ifstream fs(filename);
  std::string line;
  std::string default_str = "NaN";

  constexpr int STATE_BASE = 0;
  constexpr int STATE_QUOTE = 1;
  constexpr int STATE_QQ = 2;
  constexpr int STATE_SPACE = 3;
  
  char buf[8192];

  int expected_columns_count = -1;
  while (std::getline(fs, line))
  {
    line += ",";
    int col = 0;
    int n = 0;
    int p = 0;
    int state = STATE_SPACE;
    for (int i = 0; i < line.size(); i++)
    {
      if (col == data.columns.size())
        data.columns.push_back(std::vector<std::string>());
      if (state == STATE_SPACE || state == STATE_BASE || state == STATE_QQ)
      {
        if (state == STATE_SPACE)
        {
          if (line[i] == ' ')
            continue;
          else
            state = STATE_BASE;
        }
        
        if (line[i] == ',')
        {
          state = STATE_SPACE;
          buf[p] = 0;
          data.columns[col].push_back(std::string(buf));
          col++;
          p = 0;
        }
        else if (line[i] == '"')
        {
          state = STATE_QUOTE;
          if (state == STATE_QQ)
            buf[p++] = line[i];
        }
        else 
        {
          buf[p++] = line[i];
        }
      }
      else if (state == STATE_QUOTE)
      {
        if (line[i] == '"')
        {
          state = STATE_QQ;
        }
        else
        {
          buf[p++] = line[i];
        }
      }
    }

    if (expected_columns_count == -1)
    {
      expected_columns_count = col;
    }
    else
    {
      for (int i = col; i < expected_columns_count; i++)
      {
        data.columns[i].push_back(default_str);
      }
    }
  }
  data.row_count = data.columns[0].size();

  return data;
}

void print_csv(const CSVData &data, int max_rows)
{
  if (max_rows < 0)
    max_rows = data.row_count;
  
  if (data.header.size() > 0)
  {
    for (int i = 0; i < data.header.size(); i++)
      printf("%s ", data.header[i].c_str());
    printf("\n");
  }

  for (int i = 0; i < max_rows; i++)
  {
    for (int j = 0; j < data.columns.size(); j++)
    {
      printf("%s ", data.columns[j][i].c_str());
    }
    printf("\n");
  }
}