#include "strings.h"

std::vector<std::string> split(const std::string &s, char aDelim)
{
  std::string aStr = s;
  std::vector<std::string> res;
  std::string str = aStr.substr(0, aStr.find(aDelim));

  while (str.size() < aStr.size())
  {
    res.push_back(str);
    aStr = aStr.substr(aStr.find(aDelim) + 1);
    str = aStr.substr(0, aStr.find(aDelim));
  }

  res.push_back(str);
  return res;
}

std::string file_extension(const std::string& s)
{
  size_t i = s.rfind('.', s.length());
  if (i != std::string::npos)
  {
    return s.substr(i + 1, s.length() - i);
  }
  return "";
}