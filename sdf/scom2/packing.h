#pragma once
#include "scom.h"

namespace scom2
{
  struct Settings;
  void pack_SCom2(const Settings& settings, const SdfDAG &dag, SCom2Tree &out_tree);
  void unpack_SCom2(const SCom2Tree &in_tree, SdfDAG &out_dag, bool verbose = false); // for tests and debug only

  void replace_channel_links_to_values(SCom2Tree &tree);
}