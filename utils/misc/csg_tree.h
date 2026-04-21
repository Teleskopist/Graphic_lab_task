#pragma once 

#include <vector>
#include <LiteMath.h>
#include <string>

#include "csg_dnf.h"

using LiteMath::float3, LiteMath::float2;

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

struct CSGnode
{
  //  node type
  unsigned type;
  //  figure/operation params
  std::vector<float> params;
  CSGnode* left;
  CSGnode* right;

  CSGnode(const int _type, const std::vector<float> &_params, CSGnode* _left = nullptr, CSGnode* _right = nullptr) : 
    type(_type), params(_params), left(_left), right(_right) {}
  CSGnode() {}
  
  //  funcs
  static float get_sphere_dist(float3 p, float r);
  static float get_box_dist(float3 p, float3 b);
  static float get_torus_dist(float3 p, float2 t);
  static float get_cone_dist(float3 p, float2 c, float h);
  static float get_cylinder_dist(float3 p, float r, float h);
  static float get_union(float d1, float d2);
  static float get_subtraction(float d1, float d2);
  static float get_intersection(float d1, float d2);
  static float get_rotation(float3 p, float angle, char coord);
  static float get_scale(float3 p, float s);

  float get_distance(const float3 &p);

  std::string to_str() const;
};

struct CSGtree
{
  CSGnode* data;

  CSGtree() {};

  float get_distance(const float3& p);
  void load_scene(const std::string& path);
  CSGnode* build(const std::vector<std::pair<std::string, std::string>>& rpn);
private:
  void get_params(const unsigned int type, const std::string& params, std::vector<float> &p);
};

void printTree(CSGnode* node);
std::string printNodeType(const unsigned int type);

void convertCSGtree2NormalForm(CSGnode* node);
std::vector<CSGDNFnode> CSGtree2DNF(CSGtree& tree);
void CSGtree2RPN(CSGnode* node, std::vector<CSGnode*>& rpn);

#endif