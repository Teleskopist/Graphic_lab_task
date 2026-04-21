#pragma once
#include <vector>
#include <string>

struct CSVData
{
  using BaseColumn = std::vector<std::string>;

  inline int get_column_idx(const std::string &name) const
  {
    for (int i = 0; i < header.size(); i++)
      if (header[i] == name)
        return i;
    return -1;
  }
  BaseColumn &operator[](int idx) { return idx < 0 ? empty_column : columns[idx]; }
  const BaseColumn &operator[](int idx) const { return idx < 0 ? empty_column : columns[idx]; }
  BaseColumn &operator[](const std::string &name) { return operator[](get_column_idx(name)); }
  const BaseColumn &operator[](const std::string &name) const { return operator[](get_column_idx(name)); }

  std::vector<std::string> header; // column names
  std::vector<BaseColumn> columns;
  BaseColumn empty_column;
  int row_count = 0;
};

CSVData load_csv(const std::string &filename);
void save_csv(const std::string &filename, const CSVData &data, bool in_quotes = true);
void print_csv(const CSVData &data, int max_rows = -1);