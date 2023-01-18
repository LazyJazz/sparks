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

  base_color = glm::vec3{1.0f};

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

#define FLOAT_TERM(x)                                              \
  child_element = material_element->FirstChildElement(#x);         \
  if (child_element) {                                             \
    x = std::stof(child_element->FindAttribute("value")->Value()); \
  }
  FLOAT_TERM(metallic);
  FLOAT_TERM(specular);
  FLOAT_TERM(specular_tint);
  FLOAT_TERM(roughness);
  FLOAT_TERM(anisotropic);
  FLOAT_TERM(anisotropic_rotation);
  FLOAT_TERM(sheen);
  FLOAT_TERM(sheen_tint);
  FLOAT_TERM(clearcoat);
  FLOAT_TERM(clearcoat_roughness);
  FLOAT_TERM(ior);
  FLOAT_TERM(transmission);
  FLOAT_TERM(transmission_roughness);

  child_element = material_element->FirstChildElement("emission");
  if (child_element) {
    emission = StringToVec3(child_element->FindAttribute("value")->Value());
  }

  child_element = material_element->FirstChildElement("emission_strength");
  if (child_element) {
    emission_strength =
        std::stof(child_element->FindAttribute("value")->Value());
  }

  child_element = material_element->FirstChildElement("alpha");
  if (child_element) {
    alpha = std::stof(child_element->FindAttribute("value")->Value());
  }

  material_type =
      material_name_map[material_element->FindAttribute("type")->Value()];
}

Material::Material(const glm::vec3 &albedo) : Material() {
  base_color = albedo;
}
}  // namespace sparks
