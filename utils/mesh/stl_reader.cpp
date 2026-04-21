#include "stl_reader.h"
#include "utils/common/strings.h"

#include <filesystem>
#include <fstream>
#include <cstring>

namespace cmesh4
{
  //https://www.fabbers.com/tech/STL_Format
  struct alignas(1) STLHeader
  {
    char data[80];
  };
  struct alignas(1) STLTriangleRaw
  {
    char data[50];
  };

  inline static bool is_delim(char c)
  {
    //Windows-style endline (\r\n) will also be interpreted correctly
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
  }

  class StlTokenizer
  {
  public:
    static constexpr int MAX_TOKEN_SIZE = 1024;
    char* NextToken(); 
    bool OpenFile(const std::string& path)
    {
      fs.open(path, std::ios::binary);
      return !fs.fail();
    }
    void CloseFile()
    {
      fs.close();
    }

    int cur_line = 0;
    int cur_pos  = 0;

    std::ifstream fs;
    char buffer[MAX_TOKEN_SIZE+1];
    bool eof = false;
  };

  char *StlTokenizer::NextToken()
  {
    if (eof)
    {
      printf("[LoadMeshFromStlASCII::ERROR] Failed to read token at line %d, pos %d. End of file\n", cur_line, cur_pos);
      return nullptr;
    }
    //read token to buffer
    int p = 0;
    do
    {
      buffer[p] = fs.get();
      cur_pos++;
      if (buffer[p] == '\n')
      {
        cur_line++;
        cur_pos = 0;
      }
      if (is_delim(buffer[p]))
      {
        if (p > 0)
          break;
      }
      else
        p++;
    } while (p < MAX_TOKEN_SIZE && !fs.fail());
    
    buffer[p] = '\0';

    if (fs.fail())
    {
      eof = true;
    }

    return buffer;
  }

  bool check_file(const char *path)
  {
    auto file_type = std::filesystem::status(path).type();

    if (file_type == std::filesystem::file_type::regular)
    {
      return true;
    }
    
    if (file_type == std::filesystem::file_type::not_found)
    {
      fprintf(stderr, "file not found: %s", path);
      return false;
    }
    
    fprintf(stderr, "is not a file: %s", path);
    return false;
  }

  bool check_if_ASCII(const std::string& path)
  {
    std::ifstream file(path, std::ios::binary);
    char buffer[5];
    file.read(buffer, 5);
    //pray the binary gods it is not a coincidence
    if (strncmp(buffer, "solid", 5) == 0)
      return true;
    return false;
  }

  cmesh4::SimpleMesh LoadMeshFromStlASCII(const std::string& path, bool verbose)
  {
    StlTokenizer tokenizer;
    if (!tokenizer.OpenFile(path))
      return SimpleMesh();

    cmesh4::SimpleMesh mesh;

    char *cur_token = tokenizer.NextToken();
    if (cur_token == nullptr || strncmp(cur_token, "solid", 5) != 0)
    {
      printf("[LoadMeshFromStlASCII::ERROR] Expected \"solid\" at line %d, pos %d, got %s\n", 
             tokenizer.cur_line, tokenizer.cur_pos, cur_token);
      return SimpleMesh();
    }

    std::string name = tokenizer.NextToken();
    if (name != "facet")
    tokenizer.NextToken(); //facet

    float3 normal;
    float3 v[3];
    while (strncmp(tokenizer.buffer, "facet", 5) == 0)
    {
      /*
      facet normal ni nj nk
          outer loop
              vertex v1x v1y v1z
              vertex v2x v2y v2z
              vertex v3x v3y v3z
          endloop
      endfacet
      */
      tokenizer.NextToken(); //normal
      normal.x = std::stod(tokenizer.NextToken());
      normal.y = std::stod(tokenizer.NextToken());
      normal.z = std::stod(tokenizer.NextToken());

      tokenizer.NextToken(); //outer
      tokenizer.NextToken(); //loop

      for (int i = 0; i < 3; i++)
      {
        tokenizer.NextToken(); //vertex
        v[i].x = std::stod(tokenizer.NextToken());
        v[i].y = std::stod(tokenizer.NextToken());
        v[i].z = std::stod(tokenizer.NextToken());
      }

      tokenizer.NextToken(); //endloop
      tokenizer.NextToken(); //endfacet
      tokenizer.NextToken(); //next facet or endsolid

      mesh.vNorm4f.push_back(to_float4(normal, 0));
      mesh.vNorm4f.push_back(to_float4(normal, 0));
      mesh.vNorm4f.push_back(to_float4(normal, 0));

      mesh.vPos4f.push_back(to_float4(v[0], 1));
      mesh.vPos4f.push_back(to_float4(v[1], 1));
      mesh.vPos4f.push_back(to_float4(v[2], 1));

      mesh.vTang4f.push_back(float4(1, 0, 0, 0));
      mesh.vTang4f.push_back(float4(1, 0, 0, 0));
      mesh.vTang4f.push_back(float4(1, 0, 0, 0));

      mesh.vTexCoord2f.push_back(float2(0, 0));
      mesh.vTexCoord2f.push_back(float2(0, 0));
      mesh.vTexCoord2f.push_back(float2(0, 0));

      mesh.indices.push_back(mesh.vPos4f.size() - 3);
      mesh.indices.push_back(mesh.vPos4f.size() - 2);
      mesh.indices.push_back(mesh.vPos4f.size() - 1);

      mesh.matIndices.push_back(0);
    }

    if (strncmp(tokenizer.buffer, "endsolid", 8) != 0)
    {
      printf("[LoadMeshFromStlASCII::ERROR] Expected \"endsolid\" at line %d, pos %d, got %s\n", 
             tokenizer.cur_line, tokenizer.cur_pos, tokenizer.buffer);
      return SimpleMesh();
    }

    tokenizer.NextToken(); //name after endsolid
    tokenizer.CloseFile();

    if (verbose)
    {
      printf("STL file %s has %d triangles\n", path.c_str(), (int)mesh.TrianglesNum());
      printf("STL file %s has %d vertices\n", path.c_str(), (int)mesh.TrianglesNum());
    }

    return mesh;
  }

  cmesh4::SimpleMesh LoadMeshFromStlBinary(const std::string& path, bool verbose)
  {
    std::ifstream file(path, std::ios::binary);
    STLHeader header;
    file.read((char *)&header, sizeof(STLHeader));
    uint32_t num_triangles = 0;
    file.read((char *)&num_triangles, sizeof(unsigned));
    std::vector<STLTriangleRaw> triangles(num_triangles);
    file.read((char *)triangles.data(), num_triangles * sizeof(STLTriangleRaw));

    cmesh4::SimpleMesh mesh;
    mesh.vPos4f.resize(num_triangles * 3,  float4(0, 0, 0, 1));
    mesh.vNorm4f.resize(num_triangles * 3, float4(0, 0, 0, 0));
    mesh.vTang4f.resize(num_triangles * 3, float4(1, 0, 0, 0));
    mesh.vTexCoord2f.resize(num_triangles * 3, float2(0, 0));
    mesh.indices.resize(num_triangles * 3);
    mesh.matIndices.resize(num_triangles, 0);

    for (int i = 0; i < num_triangles; i++)
    {
      auto &tri = triangles[i];
      float3 n, v1, v2, v3;
      memcpy(&n, tri.data, 12);
      memcpy(&v1, tri.data + 12, 12);
      memcpy(&v2, tri.data + 24, 12);
      memcpy(&v3, tri.data + 36, 12);
      mesh.vNorm4f[3*i + 0] = to_float4(n, 0);
      mesh.vNorm4f[3*i + 1] = to_float4(n, 0);
      mesh.vNorm4f[3*i + 2] = to_float4(n, 0);
      mesh.vPos4f[3*i + 0] = to_float4(v1, 1);
      mesh.vPos4f[3*i + 1] = to_float4(v2, 1);
      mesh.vPos4f[3*i + 2] = to_float4(v3, 1);
      mesh.indices[3*i + 0] = 3*i;
      mesh.indices[3*i + 1] = 3*i + 1;
      mesh.indices[3*i + 2] = 3*i + 2;
    }

    if (verbose)
    {
      printf("STL file %s has %d triangles\n", path.c_str(), num_triangles);
      printf("STL file %s has %d vertices\n", path.c_str(), num_triangles * 3);
    }

    return mesh;
  }
  
  cmesh4::SimpleMesh LoadMeshFromStl(const std::string& path, bool verbose)
  {
    bool is_ok = check_file(path.c_str());
    if (!is_ok)
      return SimpleMesh();

    if (check_if_ASCII(path))
      return LoadMeshFromStlASCII(path, verbose);
    else
      return LoadMeshFromStlBinary(path, verbose);
  }
}