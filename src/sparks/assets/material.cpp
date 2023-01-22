#include "sparks/assets/material.h"

#include "grassland/grassland.h"
#include "sparks/assets/scene.h"
#include "sparks/assets/texture.h"
#include "sparks/util/util.h"

namespace sparks {

namespace {
std::unordered_map<std::string, MaterialType> material_name_map{
    {"lambertian", MATERIAL_TYPE_LAMBERTIAN},
    {"specular", MATERIAL_TYPE_SPECULAR},
    {"transmissive", MATERIAL_TYPE_TRANSMISSIVE},
    {"principled", MATERIAL_TYPE_PRINCIPLED},
    {"emission", MATERIAL_TYPE_EMISSION}};
}

Material::Material(Scene *scene, const tinyxml2::XMLElement *material_element)
    : Material() {
  if (!material_element) {
    return;
  }

  auto child_element = material_element->FirstChildElement("albedo");
  if (child_element) {
    base_color = StringToVec3(child_element->FindAttribute("value")->Value());
  }

  child_element = material_element->FirstChildElement("albedo_texture");
  if (child_element) {
    std::string path = child_element->FindAttribute("value")->Value();
    Texture albedo_texture(1, 1);
    if (Texture::Load(path, albedo_texture)) {
      base_color_texture_id =
          scene->AddTexture(albedo_texture, PathToFilename(path));
    }
  }

  child_element = material_element->FirstChildElement("roughness_texture");
  if (child_element) {
    std::string path = child_element->FindAttribute("value")->Value();
    Texture roughness_texture(1, 1);
    if (Texture::Load(path, roughness_texture)) {
      roughness_texture_id =
          scene->AddTexture(roughness_texture, PathToFilename(path));
      roughness = 1.0f;
    }
  }

  child_element = material_element->FirstChildElement("metallic_texture");
  if (child_element) {
    std::string path = child_element->FindAttribute("value")->Value();
    Texture metallic_texture(1, 1);
    if (Texture::Load(path, metallic_texture)) {
      metallic_texture_id =
          scene->AddTexture(metallic_texture, PathToFilename(path));
      metallic = 1.0f;
    }
  }

  child_element = material_element->FirstChildElement("normal_map");
  if (child_element) {
    std::string path = child_element->FindAttribute("value")->Value();
    Texture normal_map(1, 1);
    if (Texture::Load(path, normal_map)) {
      normal_map_id = scene->AddTexture(normal_map, PathToFilename(path));
    }
  }

#define VEC3_TERM(x)                                                  \
  child_element = material_element->FirstChildElement(#x);            \
  if (child_element) {                                                \
    x = StringToVec3(child_element->FindAttribute("value")->Value()); \
  }

#define FLOAT_TERM(x)                                              \
  child_element = material_element->FirstChildElement(#x);         \
  if (child_element) {                                             \
    x = std::stof(child_element->FindAttribute("value")->Value()); \
  }

  VEC3_TERM(base_color)
  VEC3_TERM(emission)

  FLOAT_TERM(metallic)
  FLOAT_TERM(specular)
  FLOAT_TERM(specular_tint)
  FLOAT_TERM(roughness)
  FLOAT_TERM(anisotropic)
  FLOAT_TERM(anisotropic_rotation)
  FLOAT_TERM(sheen)
  FLOAT_TERM(sheen_tint)
  FLOAT_TERM(clearcoat)
  FLOAT_TERM(clearcoat_roughness)
  FLOAT_TERM(ior)
  FLOAT_TERM(transmission)
  FLOAT_TERM(transmission_roughness)
  FLOAT_TERM(emission_strength)
  FLOAT_TERM(alpha)

  material_type =
      material_name_map[material_element->FindAttribute("type")->Value()];
}

Material::Material(const glm::vec3 &albedo) : Material() {
  base_color = albedo;
}
}  // namespace sparks
