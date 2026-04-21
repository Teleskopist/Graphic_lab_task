#include "utils/common/position_hash.h"
#include "scene_extension.h"
#include "IRenderer.h"

namespace LiteScene
{
  static constexpr uint32_t INVALID_IDX = 1u<<31u;

  using LiteMath::uint2;
  using LiteMath::uint4;

  //material that is used to indicate that is wasn't loaded properly
  MultiRendererMaterial get_error_material()
  {
    MultiRendererMaterial mat;
    mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    mat.base_color = float4(1,0,1,1);
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.texId = 0xFFFFFFFFu;

    return mat;
  }

  //material with some default values in case not all values are present or needed
  MultiRendererMaterial get_default_material()
  {
    MultiRendererMaterial mat;
    mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    mat.base_color = float4(1,1,1,1);
    mat.metallic = 0.0f;
    mat.roughness = 0.1f;
    mat.texId = 0xFFFFFFFFu;

    return mat;
  }

  MultiRendererMaterial load_material_custom(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Cannot load custom material with unknown type\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material_emissive(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    auto *mat = dynamic_cast<const LiteScene::EmissiveMaterial *>(aMat);
    if (!mat)
    {
      printf("[MultiRenderer::LoadScene] Material type_id/actual type mismatch\n");
      return get_error_material();
    }
    MultiRendererMaterial m_mat = get_default_material();
    if (std::holds_alternative<LiteScene::TextureInstance>(mat->color))
    {
      m_mat.texId = tex_remap[std::get<LiteScene::TextureInstance>(mat->color).id];
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    }
    else
    {
      auto ch = std::get<LiteScene::ColorHolder>(mat->color);
      if (ch.color)
        m_mat.base_color = ch.color.value();
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    }

    return m_mat;
  }

  MultiRendererMaterial load_material_hydra_old(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    auto *mat = dynamic_cast<const LiteScene::OldHydraMaterial *>(aMat);
    if (!mat)
    {
      printf("[MultiRenderer::LoadScene] Material type_id/actual type mismatch\n");
      return get_error_material();
    }
    MultiRendererMaterial m_mat = get_default_material();
    if (std::holds_alternative<LiteScene::TextureInstance>(mat->color))
    {
      m_mat.texId = tex_remap[std::get<LiteScene::TextureInstance>(mat->color).id];
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    }
    else
    {
      m_mat.base_color = std::get<LiteMath::float4>(mat->color);
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    }

    return m_mat;
  }

  MultiRendererMaterial load_material_gltf(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    auto *mat = dynamic_cast<const LiteScene::GltfMaterial *>(aMat);
    if (!mat)
    {
      printf("[MultiRenderer::LoadScene] Material type_id/actual type mismatch\n");
      return get_error_material();
    }
    MultiRendererMaterial m_mat = get_default_material();
    if (std::holds_alternative<LiteScene::TextureInstance>(mat->color))
    {
      m_mat.texId = tex_remap[std::get<LiteScene::TextureInstance>(mat->color).id];
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    }
    else
    {
      m_mat.base_color = to_float4(std::get<LiteMath::float3>(mat->color), 1.0f);
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    }

    if (std::holds_alternative<LiteScene::TextureInstance>(mat->glossiness_metalness_coat))
    {
      printf("[MultiRenderer::LoadScene] GlossinessMetalnessCoat texture not supported yet\n");
    }
    else
    {
      const LiteScene::GltfMaterial::GMC &gmc = std::get<LiteScene::GltfMaterial::GMC>(mat->glossiness_metalness_coat);
      if (std::holds_alternative<LiteScene::TextureInstance>(gmc.glossiness))
        printf("[MultiRenderer::LoadScene] Glossiness texture not supported yet\n");
      else
        m_mat.roughness = 1.0f - std::get<float>(gmc.glossiness);

      if (std::holds_alternative<LiteScene::TextureInstance>(gmc.metalness))
        printf("[MultiRenderer::LoadScene] Metalness texture not supported yet\n");
      else
        m_mat.metallic = std::get<float>(gmc.metalness);

      // coat is not supported here
    }

    return m_mat;
  }

  MultiRendererMaterial load_material_diffuse(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    auto *mat = dynamic_cast<const LiteScene::DiffuseMaterial *>(aMat);
    if (!mat)
    {
      printf("[MultiRenderer::LoadScene] Material type_id/actual type mismatch\n");
      return get_error_material();
    }
    MultiRendererMaterial m_mat = get_default_material();
    if (std::holds_alternative<LiteScene::TextureInstance>(mat->reflectance))
    {
      m_mat.texId = tex_remap[std::get<LiteScene::TextureInstance>(mat->reflectance).id];
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_TEXTURED;
    }
    else
    {
      m_mat.base_color = std::get<LiteScene::ColorHolder>(mat->reflectance).color.value();
      m_mat.type = MULTI_RENDER_MATERIAL_TYPE_COLORED;
    }

    //printf("loaded color %f %f %f\n", m_mat.base_color.x, m_mat.base_color.y, m_mat.base_color.z);

    return m_mat;
  }

  MultiRendererMaterial load_material_conductor(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Conductor mateiral is not supported\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material_dielectric(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Dielectric mateiral is not supported\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material_plastic(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Plastic mateiral is not supported\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material_blend(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Blend mateiral is not supported\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material_thin_film(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    printf("[MultiRenderer::LoadScene] Thin Film mateiral is not supported\n");
    return get_error_material();
  }

  MultiRendererMaterial load_material(const LiteScene::Material *aMat, std::map<uint32_t, uint32_t> &tex_remap)
  {
    MultiRendererMaterial mat = get_error_material();
    switch (aMat->type())
    {
    case LiteScene::MaterialType::CUSTOM:
      mat = load_material_custom(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::EMISSIVE:
      mat = load_material_emissive(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::HYDRA_OLD:
      mat = load_material_hydra_old(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::GLTF:
      mat = load_material_gltf(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::DIFFUSE:
      mat = load_material_diffuse(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::BLEND:
      mat = load_material_blend(aMat, tex_remap);
      break;
    case LiteScene::MaterialType::THIN_FILM:
      mat = load_material_thin_film(aMat, tex_remap);
      break;
    default:
      mat = get_error_material();
      break;
    }

    return mat;
  }
}