#include "csg_tree.h"
#include <fstream>
#include <cstdio>
#include <stack>
#include <sstream>

float CSGnode::get_sphere_dist(float3 p, float r)
{
  return LiteMath::length(p) - r;
}

float CSGnode::get_box_dist(float3 p, float3 b)
{
  float3 q = LiteMath::abs(p) - b;
  return LiteMath::length(LiteMath::max(q, float3(0.0f))) + LiteMath::min(LiteMath::max(q.x, LiteMath::max(q.y, q.z)), 0.0f);
}

float CSGnode::get_torus_dist(float3 p, float2 t)
{
  float2 q = float2(LiteMath::length(float2(p.x, p.z) - t.x), p.y);
  return LiteMath::length(q) - t.y;
}

float CSGnode::get_cone_dist(float3 p, float2 c, float h)
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

float CSGnode::get_cylinder_dist(float3 p, float r, float h)
{
  float2 d = LiteMath::abs(float2(LiteMath::length(float2(p.x, p.z)), p.y)) - float2(r, h);
  return LiteMath::min(LiteMath::max(d.x, d.y), 0.0f) + LiteMath::length(LiteMath::max(d, float2(0.0f)));
}

float CSGnode::get_union(float d1, float d2)
{
  return LiteMath::min(d1, d2);
}

float CSGnode::get_subtraction(float d1, float d2)
{
  return LiteMath::max(d1, -d2);
}

float CSGnode::get_intersection(float d1, float d2)
{
  return LiteMath::max(d1, d2);
}

float CSGnode::get_rotation(float3 p, float angle, char coord)
{
  return 0;
}

float CSGnode::get_scale(float3 p, float s)
{
  return 0;
}

std::string CSGnode::to_str() const
{
  if (type == CSGNodeType::OR)
  {
    return "OR";
  }
  else if (type == CSGNodeType::AND)
  {
    return "AND";
  }
  else if (type == CSGNodeType::SUB)
  {
    return "SUB";
  }
  else if (type == CSGNodeType::SCALE)
  {
    return "SCALE";
  }
  else if (type == CSGNodeType::MOVE)
  {
    return "MOVE";
  }
  else if (type == CSGNodeType::ROTATE)
  {
    return "ROTATE";
  }
  else if (type == CSGNodeType::BOX)
  {
    return "BOX";
  }
  else if (type == CSGNodeType::CYLINDER)
  {
    return "CYLINDER";
  }
  else if (type == CSGNodeType::SPHERE)
  {
    return "SPHERE";
  }
  else
  {
    printf("\n%d is not a node type\n", type);
    exit(-1);
  }

  return "";
}

std::string printNodeType(const unsigned int type)
{
  if (type == CSGNodeType::OR)
  {
    return "OR";
  }
  else if (type == CSGNodeType::AND)
  {
    return "AND";
  }
  else if (type == CSGNodeType::SUB)
  {
    return "SUB";
  }
  else if (type == CSGNodeType::SCALE)
  {
    return "SCALE";
  }
  else if (type == CSGNodeType::MOVE)
  {
    return "MOVE";
  }
  else if (type == CSGNodeType::ROTATE)
  {
    return "ROTATE";
  }
  else if (type == CSGNodeType::BOX)
  {
    return "BOX";
  }
  else if (type == CSGNodeType::CYLINDER)
  {
    return "CYLINDER";
  }
  else if (type == CSGNodeType::SPHERE)
  {
    return "SPHERE";
  }
  else
  {
    printf("\n%d is not a node type\n", type);
    exit(-1);
  }

  return "";
}

static bool is_operator(const unsigned int type)
{
  return type == CSGNodeType::OR || type == CSGNodeType::AND || type == CSGNodeType::SUB || type == CSGNodeType::SCALE || type == CSGNodeType::MOVE || type == CSGNodeType::ROTATE;
}

static bool is_group_operator(const unsigned int type)
{
  return type == CSGNodeType::OR || type == CSGNodeType::AND || type == CSGNodeType::SUB;
}

unsigned int get_node_name_id(const std::string &value)
{
  if (value == "union")
  {
    return CSGNodeType::OR;
  }
  else if (value == "intersection")
  {
    return CSGNodeType::AND;
  }
  else if (value == "difference")
  {
    return CSGNodeType::SUB;
  }
  else if (value == "scale")
  {
    return CSGNodeType::SCALE;
  }
  else if (value == "translate")
  {
    return CSGNodeType::MOVE;
  }
  else if (value == "rotate")
  {
    return CSGNodeType::ROTATE;
  }
  else if (value == "box")
  {
    return CSGNodeType::BOX;
  }
  else if (value == "cylinder")
  {
    return CSGNodeType::CYLINDER;
  }
  else if (value == "sphere")
  {
    return CSGNodeType::SPHERE;
  }
  else
  {
    printf("\n%s is not a node type\n", value.c_str());
    exit(-1);
  }
}

void printTree(CSGnode *node)
{
  if (node == nullptr)
  {
    return;
  }

  printf("%s: ", node->to_str().c_str());
  for (auto p : node->params)
  {
    printf("%f ", p);
  }
  printf("\n");

  printTree(node->left);
  printTree(node->right);
}

void CSGtree::get_params(const unsigned int type, const std::string& params, std::vector<float> &p)
{
  if (is_group_operator(type))
  {
    return;
  }

  std::stringstream ss(params);
  float val = 0;

  while (ss >> val)
  {
    p.push_back(val);
  } 

  if (type == CSGNodeType::MOVE)
  {
    p[1] = -p[1];
    std::swap(p[1], p[2]);
    // std::swap(p[2], p[0]);
  }
  else if (type == CSGNodeType::BOX)
  {
    p[1] = -p[1];
    p[4] = -p[4];
    std::swap(p[1], p[2]);
    std::swap(p[4], p[5]);
  }
  else if (type == CSGNodeType::CYLINDER)
  {
    p[3] = -p[3];
    std::swap(p[3], p[4]);
  }
  else if (type == CSGNodeType::SPHERE)
  {
    p[2] = -p[2];
    std::swap(p[2], p[3]);
  }
}

void CSGtree::load_scene(const std::string& path)
{
  std::ifstream f(path);

  if (!f.is_open())
  {
    printf("FILE WAS NOT OPENED!!!\n");
    return;
  }

  std::string line1, line2;
  std::vector<std::pair<std::string, std::string>> scene_data;

  while (std::getline(f, line1) && std::getline(f, line2))
  {
    if (is_group_operator(get_node_name_id(line1)))
    {
      scene_data.push_back({line1, ""});
    }
    else
    {
      scene_data.push_back({line1, line2});
    }
  }

  std::reverse(scene_data.begin(), scene_data.end());

  this->data = build(scene_data);
}

CSGnode* CSGtree::build(const std::vector<std::pair<std::string, std::string>> &rpn)
{
  std::stack<CSGnode*> stack;

  for (const auto& [value, params] : rpn)
  {
    std::vector<float> d;
    auto node_name_id = get_node_name_id(value);

    get_params(node_name_id, params, d);

    if (is_operator(node_name_id))
    {
      CSGnode *left = stack.top();
      CSGnode *right = nullptr;

      stack.pop();

      if (is_group_operator(node_name_id))
      {
        right = stack.top();
        stack.pop();
      }

      CSGnode* newNode = new CSGnode(node_name_id, d);
      newNode->left = left;
      newNode->right = right;

      stack.push(newNode);
    }
    else
    {
      stack.push(new CSGnode(node_name_id, d));
    }
  }

  return stack.top();
}

float CSGnode::get_distance(const float3& p)
{
  if (type == CSGNodeType::OR)
  {
    return get_union(left->get_distance(p), right->get_distance(p));
  }
  else if (type == CSGNodeType::AND)
  {
    return get_intersection(left->get_distance(p), right->get_distance(p));
  }
  else if (type == CSGNodeType::SUB)
  {
    return get_subtraction(left->get_distance(p), right->get_distance(p));
  }
  else if (type == CSGNodeType::SCALE)
  {
    return left->get_distance(p / params[0]) * params[0];
  }
  else if (type == CSGNodeType::MOVE)
  {
    return left->get_distance(p - float3(params[0], params[1], params[2]));
  }
  else if (type == CSGNodeType::ROTATE)
  {
    LiteMath::float4x4 rotate_mtrx;

    //  Rotate X
    if ((int)params[0] == 0)
    {
      rotate_mtrx = LiteMath::rotate4x4X(-params[1]);
    }
    else if ((int)params[0] == 1)
    {
      rotate_mtrx = LiteMath::rotate4x4Z(-params[1]);
    }
    else
    {
      rotate_mtrx = LiteMath::rotate4x4Y(-params[1]);
    }

    auto new_p = rotate_mtrx * LiteMath::float4(p.x, p.y, p.z, 1.f);
    return left->get_distance(float3(new_p.x, new_p.y, new_p.z));
  }
  else if (type == CSGNodeType::SPHERE)
  {
    return get_sphere_dist(p - float3(params[1], params[2], params[3]), params[0]);
  }
  else if (type == CSGNodeType::BOX)
  {
    auto vec = float3(params[3] - params[0], params[4] - params[1], params[5] - params[2]);
    return get_box_dist(p - 0.5f * (float3(params[0] + params[3], params[1] + params[4], params[2] + params[5])), LiteMath::abs(vec) / 2.f);
  }
  else if (type == CSGNodeType::CYLINDER)
  {
    return get_cylinder_dist(p - float3(params[2], params[3] + params[1] / 2.f, params[4]), params[0], params[1] / 2);
  }

  return 0;
}

float CSGtree::get_distance(const float3 &p)
{
  return data->get_distance(p);
}

void convertCSGtree2NormalForm(CSGnode* node)
{
  if (node == nullptr || !is_group_operator(node->type))
  {
    return;
  }

  //  skip all variants, where node->type == OR, and process each another variation

  constexpr unsigned SUB = CSGNodeType::SUB;
  constexpr unsigned AND = CSGNodeType::AND;
  constexpr unsigned OR = CSGNodeType::OR;

  bool node_finished = false;
  
  while (!node_finished)
  {
    bool match = true;

    while (match)
    {
      #define T (node)
      #define L (node->left)
      #define R (node->right)

      match = false;

      if (T->type == SUB && R->type == OR)
      {
        // printf("1:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());

        auto x = L;
        auto y = R->left;
        auto z = R->right;

        // printf("%s %s\n", y->to_str().c_str(), z->to_str().c_str());

        R->type = SUB;
        R->left = x;
        R->right = y;

        T->type = SUB;
        T->left = R;
        T->right = z;

        match = true;

        // printf("1:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == AND && R->type == OR)
      {
        // printf("2:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L;
        auto y = R->left;
        auto z = R->right;

        CSGnode* new_L = new CSGnode(AND, {}, x, y);
        
        R->type = AND;
        R->left = x;
        R->right = z;

        T->type = OR;
        T->left = new_L;
        T->right = R;

        match = true;
        // printf("2:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == SUB && R->type == AND)
      {
        // printf("3:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L;
        auto y = R->left;
        auto z = R->right;

        CSGnode* new_L = new CSGnode(SUB, {}, x, y);

        R->type = SUB;
        R->left = x;
        R->right = z;

        T->type = OR;
        T->left = new_L;
        T->right = R;

        match = true;
        // printf("3:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == AND && R->type == AND)
      {
        // printf("4:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L;
        auto y = R->left;
        auto z = R->right;

        R->type = AND;
        R->left = x;
        R->right = y;

        T->type = AND;
        T->left = R;
        T->right = z;

        match = true;
        // printf("4:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == SUB && R->type == SUB)
      {
        // printf("5:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L;
        auto y = R->left;
        auto z = R->right;

        CSGnode* new_l = new CSGnode(SUB, {}, x, y);

        R->type = AND;
        R->left = x;
        R->right = z;

        T->type = OR;
        T->left = new_l;
        T->right = R;

        match = true;
        // printf("5:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == AND && R->type == SUB)
      {
        // printf("6:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L;
        auto y = R->left;
        auto z = R->right;

        R->type = AND;
        R->left = x;
        R->right = y;

        T->type = SUB;
        T->left = R;
        T->right = z;

        match = true;
        // printf("6:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == SUB && L->type == OR)
      {
        // printf("7:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L->left;
        auto y = L->right;
        auto z = R;

        CSGnode* new_right = new CSGnode(SUB, {}, y, z);

        L->type = SUB;
        L->left = x;
        L->right = z;

        T->type = OR;
        T->left = L;
        T->right = new_right;

        match = true;
        // printf("7:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == AND && L->type == OR)
      {
        // printf("8:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L->left;
        auto y = L->right;
        auto z = R;

        CSGnode* new_right = new CSGnode(AND, {}, y, z);

        L->type = AND;
        L->left = x;
        L->right = z;

        T->type = OR;
        T->left = L;
        T->right = new_right;

        match = true;
        // printf("8:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else if (T->type == AND && L->type == SUB)
      {
        // printf("9:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
        auto x = L->left;
        auto y = L->right;
        auto z = R;

        L->type = AND;
        L->left = x;
        L->right = z;

        T->type = SUB;
        T->left = L;
        T->right = y;

        match = true;
        // printf("9:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
      else
      {
        // printf("10:\n%s %s %s\n", node->left->to_str().c_str(), node->to_str().c_str(), node->right->to_str().c_str());
      }
    }

    convertCSGtree2NormalForm(node->left);
      
    node_finished = (T->type == OR) || (!is_group_operator(L->type) && !is_group_operator(R->type)) || (!is_group_operator(L->type) && R->type != OR) || (!is_group_operator(R->type) && L->type != OR);
  }
  
  convertCSGtree2NormalForm(node->right);
}

void CSGtree2RPN(CSGnode* node, std::vector<CSGnode*>& rpn)
{
  if (node == nullptr)
  {
    return;
  }

  CSGtree2RPN(node->left, rpn);
  CSGtree2RPN(node->right, rpn);

  rpn.push_back(node);
}

std::vector<CSGDNFnode> CSGtree2DNF(CSGtree& tree)
{
  std::vector<CSGDNFnode> result;
  std::vector<CSGnode*> rpn;
  std::stack<CSGDNFnode> stack;

  convertCSGtree2NormalForm(tree.data);
  CSGtree2RPN(tree.data, rpn);

  for (auto token : rpn)
  {
    if (!is_operator(token->type))
    {
      CSGDNFnode tmp;
      tmp.figures.push_back(token->type);
      tmp.params.push_back(token->params);

      stack.push(tmp);
    }
    else if (!is_group_operator(token->type))
    {
      auto last = stack.top();
      stack.pop();

      int ind = last.figures.size() - 1;

      while (ind >= 0 && is_operator(last.figures[ind]))
      {
        ind--;
      }

      if (ind >= 1)
      {
        ind--;
      }

      last.figures.emplace(last.figures.begin() + ind, token->type);
      last.params.emplace(last.params.begin() + ind, token->params);

      stack.push(last);
    }
    else
    {
      auto right = stack.top();
      stack.pop();

      auto left = stack.top();
      stack.pop();

      if (token->type != CSGNodeType::OR)
      {
        CSGDNFnode new_node;

        for (int i = 0; i < left.figures.size(); i++)
        {
          new_node.figures.push_back(left.figures[i]);
          new_node.params.push_back(left.params[i]);
        }

        for (int i = 0; i < right.figures.size(); i++)
        {
          new_node.figures.push_back(right.figures[i]);
          new_node.params.push_back(right.params[i]);
        }

        new_node.figures.push_back(token->type);
        new_node.params.push_back({});

        stack.push(new_node);
      }
      else
      {
        if (left.figures.size() > 0)
        {
          result.push_back(left);
        }

        if (right.figures.size() > 0)
        {
          result.push_back(right);
        }

        CSGDNFnode tmp;
        stack.push(tmp);
      }
    }
  }

  while (stack.size() > 0)
  {
    auto last = stack.top();
    stack.pop();

    if (last.figures.size() > 0)
    {
      result.push_back(last);
    }
  }

  return result;
}