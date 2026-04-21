#include "context.h"

namespace LiteApp
{

  RasterizationContext::RasterizationContext() {}
  RasterizationContext::~RasterizationContext() {}

  void RasterizationContext::setup(const std::string &scene_path)
  {
    for (auto &i : rasterizers)
    {
      i->LoadScene(scene_path);
    }
  }

  void RasterizationContext::refresh(bool wireframe)
  {
    for (auto &r : rasterizers)
    {
      r->SetWireframe(wireframe);
      r->CreatePipeline();
    }
  }

  void RasterizationContext::add_rasterizer(std::unique_ptr<IRasterizer> rasterizer)
  {
    rasterizers.push_back(std::move(rasterizer));
    rasterizer_names_.push_back(rasterizers.back()->Name());
  }

  void RasterizationContext::clear_rasterizers()
  {
    rasterizers.resize(0);
  }

  void RasterizationContext::set_active_rasterizer(size_t i)
  {
    active_rasterizer_index = i;
  }

  const std::vector<const char *> &RasterizationContext::rasterizer_names() const
  {
    return rasterizer_names_;
  }

  IRasterizer *RasterizationContext::active_rasterizer()
  {
    if (active_rasterizer_index < rasterizers.size())
      return rasterizers[active_rasterizer_index].get();
    else
      return nullptr;
  }

}