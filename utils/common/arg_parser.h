#pragma once

#include <string>
#include <cstdlib>
#include <map>

class ConsoleArgParser
{
  enum class ArgType
  {
    FLAG,
    STRING,
    INT,
    FLOAT
  };

  public:
  ConsoleArgParser(int _argc, const char **_argv)
  {
    argc = _argc;
    argv = _argv;
  }

  bool hasFlag(const std::string &name)
  {
    arg_types[name] = ArgType::FLAG;
    for (int i=1; i<argc; i++)
    {
      if (std::string(argv[i]) == name)
        return true;
    }
    return false;
  }

  bool hasParam(const std::string &name, std::string *value)
  {
    arg_types[name] = ArgType::STRING;
    for (int i = 1; i < argc; i++)
    {
      if (std::string(argv[i]) == name)
      {
        if (i + 1 < argc)
        {
          *value = argv[i + 1];
          return true;
        }
        else
        {
          printf("String value expected after parameter %s\n", name.c_str());
          return false;
        }
      }
    }
    return false;
  }

  bool hasIntParam(const std::string &name, int *value)
  {
    arg_types[name] = ArgType::INT;
    for (int i=1; i<argc; i++)
    {
      if (std::string(argv[i]) == name)
      {
        if (i+1 < argc)
        {
          char *strend = nullptr;
          *value = strtol(argv[i+1], &strend, 10);
          if (strend == nullptr || *strend != '\0')
          {
            printf("Integer value expected after parameter %s: got\"%s\"\n", name.c_str(), argv[i+1]);
            return false;
          }
          return true;
        }
        else
        {
          printf("Integer value expected after parameter %s\n", name.c_str());
          return false;
        }
      }
    }
    return false;
  }

  bool hasFloatParam(const std::string &name, float *value)
  {
    arg_types[name] = ArgType::FLOAT;
    for (int i=1; i<argc; i++)
    {
      if (std::string(argv[i]) == name)
      {
        if (i+1 < argc)
        {
          char *strend = nullptr;
          *value = strtod(argv[i+1], &strend);
          if (strend == nullptr || *strend != '\0')
          {
            printf("Float value expected after parameter %s: got\"%s\"\n", name.c_str(), argv[i+1]);
            return false;
          }
          return true;
        }
        else
        {
          printf("Float value expected after parameter %s\n", name.c_str());
          return false;
        }
      }
    }
    return false;
  }

  void print_help()
  {
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    for (auto &it : arg_types)
    {
      printf("  %s", it.first.c_str());
      if (it.second == ArgType::FLAG)
        printf("\n");
      else if (it.second == ArgType::STRING)
        printf(" <arg>\n");
      else if (it.second == ArgType::INT)
        printf(" <int>\n");
      else if (it.second == ArgType::FLOAT)
        printf(" <float>\n");
    }
  }

private:
  int argc;
  const char **argv;
  std::map<std::string, ArgType> arg_types;
};