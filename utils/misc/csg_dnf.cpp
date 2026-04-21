#include "csg_dnf.h"
#include <stack>

#ifndef DISABLE_CSGDNF
#ifndef KERNEL_SLICER

struct CSGNodeType
{
  enum Type
  {
    OR,
    AND,
    SUB,
    SCALE,
    MOVE,
    ROTATE,
    SPHERE,
    BOX,
    CYLINDER,
    TOR,
    CONE
  };
};

float get_sphere_dist(float3 p, float r)
{
  return LiteMath::length(p) - r;
}

float get_box_dist(float3 p, float3 b)
{
  float3 q = LiteMath::abs(p) - b;
  return LiteMath::length(LiteMath::max(q, float3(0.0f))) + LiteMath::min(LiteMath::max(q.x, LiteMath::max(q.y, q.z)), 0.0f);
}

float get_torus_dist(float3 p, float2 t)
{
  float2 q = float2(LiteMath::length(float2(p.x, p.z) - t.x), p.y);
  return LiteMath::length(q) - t.y;
}

float get_cone_dist(float3 p, float2 c, float h)
{
  float2 q = h * float2(c.x / c.y, p.y);
  float2 w = float2(LiteMath::length(float2(p.x, p.z)), p.y);
  float2 a = w - q * LiteMath::clamp(LiteMath::dot(w, q) / LiteMath::dot(q, q), 0.0f, 1.0f);
  float2 b = w - q * float2(LiteMath::clamp(w.x / q.x, 0.0f, 1.0f), 1.0f);

  float k = LiteMath::sign(q.y);
  float d = LiteMath::min(LiteMath::dot(a, a), LiteMath::dot(b, b));
  float s = LiteMath::max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));

  return LiteMath::sqrt(d) * LiteMath::sign(s);
}

float get_cylinder_dist(float3 p, float r, float h)
{
  float2 d = LiteMath::abs(float2(LiteMath::length(float2(p.x, p.z)), p.y)) - float2(r, h);
  return LiteMath::min(LiteMath::max(d.x, d.y), 0.0f) + LiteMath::length(LiteMath::max(d, float2(0.0f)));
}

float get_union(float d1, float d2)
{
  return LiteMath::min(d1, d2);
}

float get_subtraction(float d1, float d2)
{
  return LiteMath::max(d1, -d2);
}

float get_intersection(float d1, float d2)
{
  return LiteMath::max(d1, d2);
}

static bool is_operator(const unsigned int type)
{
  return type == CSGNodeType::OR || type == CSGNodeType::AND || type == CSGNodeType::SUB || type == CSGNodeType::SCALE || type == CSGNodeType::MOVE || type == CSGNodeType::ROTATE;
}

static bool is_group_operator(const unsigned int type)
{
  return type == CSGNodeType::OR || type == CSGNodeType::AND || type == CSGNodeType::SUB;
}

float CSGDNF::get_distance(const float3 &p) const
{
  std::stack<float> stack;
  float d = 1000;

  for (auto &block : blocks)
  {
    for (int i = 0; i < block.figures.size(); i++)
    {
      unsigned int type = block.figures[i];
      std::vector<float> params = block.params[i];

      if (type == CSGNodeType::BOX)
      {
        auto vec = float3(params[3] - params[0], params[4] - params[1], params[5] - params[2]);

        d = get_box_dist(p - 0.5f * (float3(params[0] + params[3], params[1] + params[4], params[2] + params[5])), LiteMath::abs(vec) / 2.f);
        stack.push(d);
      }
      else if (type == CSGNodeType::SPHERE)
      {
        d = get_sphere_dist(p - float3(params[1], params[2], params[3]), params[0]);
        stack.push(d);
      }
      else if (type == CSGNodeType::CYLINDER)
      {
        d = get_cylinder_dist(p - float3(params[2], params[3] + params[1] / 2.f, params[4]), params[0], params[1] / 2);
        stack.push(d);
      }
      else if (type == CSGNodeType::AND)
      {
        float d2 = stack.top();
        stack.pop();

        float d1 = stack.top();
        stack.pop();

        d = get_intersection(d1, d2);
        stack.push(d);
      }
      else if (type == CSGNodeType::SUB)
      {
        float d2 = stack.top();
        stack.pop();

        float d1 = stack.top();
        stack.pop();

        d = get_subtraction(d1, d2);
        stack.push(d);
      }
      else if (is_operator(type) && !is_group_operator(type))
      {
        int j = i + 1;
        float3 point = p;

        while (j < block.figures.size() && is_operator(block.figures[j]))
        {
          j++;
        }

        for (int k = i; k < j; k++)
        {
          if (block.figures[k] == CSGNodeType::MOVE)
          {
            point -= float3(block.params[k][0], block.params[k][1], block.params[k][2]);
          }
          else if (block.figures[k] == CSGNodeType::SCALE)
          {
            point /= block.params[k][0];
          }
          else if (block.figures[k] == CSGNodeType::ROTATE)
          {
            LiteMath::float4x4 rotate_mtrx;

            //  Rotate X
            if ((int)block.params[k][0] == 0)
            {
              rotate_mtrx = LiteMath::rotate4x4X(-block.params[k][1]);
            }
            else if ((int)block.params[k][0] == 1)
            {
              rotate_mtrx = LiteMath::rotate4x4Z(-block.params[k][1]);
            }
            else
            {
              rotate_mtrx = LiteMath::rotate4x4Y(-block.params[k][1]);
            }

            auto new_p = rotate_mtrx * LiteMath::float4(point.x, point.y, point.z, 1.f);
            point = float3(new_p.x, new_p.y, new_p.z);
          }
        }

        if (block.figures[j] == CSGNodeType::BOX)
        {
          auto vec = float3(block.params[j][3] - block.params[j][0], block.params[j][4] - block.params[j][1], block.params[j][5] - block.params[j][2]);
          d = get_box_dist(point - 0.5f * (float3(block.params[j][0] + block.params[j][3], block.params[j][1] + block.params[j][4], block.params[j][2] + block.params[j][5])), LiteMath::abs(vec) / 2.f);
        }
        else if (block.figures[j] == CSGNodeType::SPHERE)
        {
          d = get_sphere_dist(point - float3(block.params[j][1], block.params[j][2], block.params[j][3]), block.params[j][0]);
        }
        else if (block.figures[j] == CSGNodeType::CYLINDER)
        {
          d = get_cylinder_dist(point - float3(block.params[j][2], block.params[j][3] + block.params[j][1] / 2.f, block.params[j][4]), block.params[j][0], block.params[j][1] / 2);
        }

        for (int k = j - 1; k >= i; k--)
        {
          if (block.figures[k] == CSGNodeType::MOVE)
          {
          }
          else if (block.figures[k] == CSGNodeType::SCALE)
          {
            d *= block.params[k][0];
          }
          else if (block.figures[k] == CSGNodeType::ROTATE)
          {
          }
        }

        stack.push(d);

        i = j;
      }
    }
  }

  d = stack.top();
  stack.pop();

  while (!stack.empty())
  {
    d = get_union(d, stack.top());
    stack.pop();
  }

  return d;
}

void CSGDNF::save_model(const std::string &path) const
{
  std::fstream file(path, std::ios::in | std::ios::out | std::ios::trunc);

  if (!file.is_open())
  {
    printf("\nSOMETHING WENT WRONG WITH SAVING CSGDNF IN FILE\n");
    exit(-1);
  }

  for (auto &block : blocks)
  {
    for (int i = 0; i < block.figures.size(); i++)
    {
      file << block.figures[i] << std::endl;

      if (is_group_operator(block.figures[i]))
      {
        file << "None";
      }
      else
      {
        for (auto param : block.params[i])
        {
          file << param << " ";
        }
      }

      file << std::endl;
    }

    file << std::endl << std::endl;
  }

  file.close();
}

void CSGDNF::load_model(const std::string& path)
{
  std::ifstream file(path);

  if (!file.is_open())
  {
    printf("\nSOMETHING WENT WRONG WITH LOADING CSGDNF FROM FILE\n");
    exit(-1);
  }

  CSGDNFnode block;
  std::string line1, line2;

  while (std::getline(file, line1) && std::getline(file, line2))
  {
    // printf("%s\n%s\n", line1.c_str(), line2.c_str());
    if (line1.size() == 0 && line2.size() == 0 && block.figures.size() > 0)
    {
      blocks.push_back(block);
      block.figures.clear();

      for (auto &el: block.params)
      {
        el.clear();
      }

      block.params.clear();
      continue;
    }

    unsigned int type = std::atoi(line1.c_str());
    block.figures.push_back(type);

    if (is_group_operator(type))
    {
      block.params.push_back({});
    }
    else
    {
      float param = 0;
      std::vector<float> params;
      std::stringstream ss(line2);
      
      while (ss >> param)
      {
        params.push_back(param);
      }

      block.params.push_back(params);
    }
  }
}

#endif
#endif