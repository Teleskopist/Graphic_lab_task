#pragma once
#include <vector>

namespace sdf_converter
{
  static constexpr unsigned TYPE_UNKNOWN           = 0 << 24;
  static constexpr unsigned TYPE_ISOLATED          = 1 << 24;
  static constexpr unsigned TYPE_WIRE_EDGE         = 2 << 24;
  static constexpr unsigned TYPE_WIRE              = 3 << 24;
  static constexpr unsigned TYPE_PLANE_CORNER      = 4 << 24;
  static constexpr unsigned TYPE_BULGE             = 5 << 24;
  static constexpr unsigned TYPE_PLANE_EDGE        = 6 << 24;
  static constexpr unsigned TYPE_PLANE_EDGE_BEND   = 7 << 24;
  static constexpr unsigned TYPE_VOLUME_CORNER     = 8 << 24;
  static constexpr unsigned TYPE_PLANE             = 9 << 24;
  static constexpr unsigned TYPE_PLANE_BEND        = 10 << 24;
  static constexpr unsigned TYPE_VOLUME_EDGE       = 11 << 24;
  static constexpr unsigned TYPE_TWO_PLANES        = 12 << 24;
  static constexpr unsigned TYPE_VOLUME            = 13 << 24;
  static constexpr unsigned TYPE_VOLUME_EDGE_INV_5 = 14 << 24;
  static constexpr unsigned TYPE_VOLUME_EDGE_INV_6 = 15 << 24;
  static constexpr unsigned TYPE_VOLUME_CORNER_INV = 16 << 24;
  static constexpr unsigned TYPE_TWO_SURFACES      = 17 << 24;
  static constexpr unsigned TYPE_VOLUME_INSIDE     = 18 << 24;
  static constexpr unsigned TYPE_UNDECIDED_N5      = 19 << 24;
  static constexpr unsigned TYPE_UNDECIDED_N61     = 20 << 24;
  static constexpr unsigned TYPE_UNDECIDED_N62     = 21 << 24;
  static constexpr unsigned TYPE_UNDECIDED_N63     = 22 << 24;
  static constexpr unsigned TYPE_UNDECIDED_N64     = 23 << 24;

  static std::vector<const char *> type_names =
      {
          "TYPE_UNKNOWN",
          "TYPE_ISOLATED",
          "TYPE_WIRE_EDGE",
          "TYPE_WIRE",
          "TYPE_PLANE_CORNER",
          "TYPE_BULGE",
          "TYPE_PLANE_EDGE",
          "TYPE_PLANE_EDGE_BEND",
          "TYPE_VOLUME_CORNER",
          "TYPE_PLANE",
          "TYPE_PLANE_BEND",
          "TYPE_VOLUME_EDGE",
          "TYPE_TWO_PLANES",
          "TYPE_VOLUME",
          "TYPE_VOLUME_EDGE_INV_5",
          "TYPE_VOLUME_EDGE_INV_6",
          "TYPE_VOLUME_CORNER_INV",
          "TYPE_TWO_SURFACES",
          "TYPE_VOLUME_INSIDE",
          "TYPE_UNDECIDED_N5",
          "TYPE_UNDECIDED_N61",
          "TYPE_UNDECIDED_N62",
          "TYPE_UNDECIDED_N63",
          "TYPE_UNDECIDED_N64"};

  static constexpr unsigned NODE_UNKNOWN = 0 << 20;
  static constexpr unsigned NODE_EMPTY   = 1 << 20;
  static constexpr unsigned NODE_VOLUME  = 2 << 20;
  static constexpr unsigned NODE_SURFACE = 3 << 20;

  static constexpr unsigned NO_BRICKS  = 0 << 16;
  static constexpr unsigned ONE_BRICK  = 1 << 16;
  static constexpr unsigned TWO_BRICKS = 2 << 16;

  static constexpr unsigned SIGNS_EMPTY          = 0b00000000'00000000;
  static constexpr unsigned SIGNS_TWO_SURFACES_X = 0b00001111'11110000;
  static constexpr unsigned SIGNS_TWO_SURFACES_Y = 0b00110011'11001100;
  static constexpr unsigned SIGNS_TWO_SURFACES_Z = 0b01010101'10101010;

  //if node has this code, it will be processed as it was before codes we introduces
  static constexpr unsigned CODE_UNDECIDED = 0u;

  // code is 32-bit descriptor of an octree node with the following structure
  // | 8               | 4            | 4          | 8          | 8          |  - bits
  // | node class      | node type    | surf count | surf2 signs| surf1 signs|  - fields
  // | TYPE_PLANE_BEND | NODE_SURFACE | TWO_BRICKS | 0b11110000 | 0b00001111 |  - example

  static unsigned extract_type(unsigned code) { return code & 0xFF000000u; }
  static bool     extract_is_empty(unsigned code) { return (code & 0x00F00000u) == NODE_EMPTY; }
  static bool extract_is_surface(unsigned code) { return (code & 0x00F00000u) == NODE_SURFACE; }
  static bool extract_is_decided(unsigned code) { return extract_type(code) < TYPE_UNDECIDED_N5; }
  static unsigned extract_count(unsigned code) { return (code & 0x000F0000u) >> 16; }
  static unsigned extract_signs_1(unsigned code) { return (code & 0x000000FFu); }
  static unsigned extract_signs_2(unsigned code) { return (code & 0x0000FF00u) >> 8; }
}