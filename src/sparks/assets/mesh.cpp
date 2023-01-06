#include "sparks/assets/mesh.h"

#include "fstream"
#include "iomanip"
#include "iostream"
#include "unordered_map"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace sparks {

Mesh::Mesh(const Mesh &mesh) : Mesh(mesh.vertices_, mesh.indices_) {
}

Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices) {
  vertices_ = vertices;
  indices_ = indices;
}

Mesh Mesh::Cube(const glm::vec3 &center, const glm::vec3 &size) {
  return {{}, {}};
}

Mesh Mesh::Sphere(const glm::vec3 &center, float radius) {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  auto pi = glm::radians(180.0f);
  const auto precision = 60;
  const auto inv_precision = 1.0f / float(precision);
  std::vector<glm::vec2> circle;
  for (int i = 0; i <= precision * 2; i++) {
    float omega = inv_precision * float(i) * pi;
    circle.emplace_back(-std::sin(omega), -std::cos(omega));
  }
  for (int i = 0; i <= precision; i++) {
    float theta = inv_precision * float(i) * pi;
    float sin_theta = std::sin(theta);
    float cos_theta = std::cos(theta);
    int i_1 = i - 1;
    for (int j = 0; j <= 2 * precision; j++) {
      auto normal = glm::vec3{circle[j].x * sin_theta, cos_theta,
                              circle[j].y * sin_theta};
      vertices.push_back(
          Vertex(normal * radius + center, normal,
                 {float(j) * inv_precision * 0.5f, float(i) * inv_precision}));
      if (i) {
        int j1 = j + 1;
        if (j == 2 * precision) {
          j1 = 0;
        }
        indices.push_back(i * (2 * precision + 1) + j1);
        indices.push_back(i * (2 * precision + 1) + j);
        indices.push_back(i_1 * (2 * precision + 1) + j);
        indices.push_back(i * (2 * precision + 1) + j1);
        indices.push_back(i_1 * (2 * precision + 1) + j);
        indices.push_back(i_1 * (2 * precision + 1) + j1);
      }
    }
  }
  return {vertices, indices};
}

AxisAlignedBoundingBox Mesh::GetAABB(const glm::mat4 &transform) const {
  if (vertices_.empty()) {
    return {};
  }
  auto it = vertices_.begin();
  AxisAlignedBoundingBox result(
      glm::vec3{transform * glm::vec4{it->position, 1.0f}});
  it++;
  while (it != vertices_.end()) {
    result |= {glm::vec3{transform * glm::vec4{it->position, 1.0f}}};
    it++;
  }
  return result;
}

float Mesh::TraceRay(const glm::vec3 &origin,
                     const glm::vec3 &direction,
                     float t_min,
                     HitRecord *hit_record) const {
  float result = -1.0f;
  for (int i = 0; i < indices_.size(); i += 3) {
    int j = i + 1, k = i + 2;
    const auto &v0 = vertices_[indices_[i]];
    const auto &v1 = vertices_[indices_[j]];
    const auto &v2 = vertices_[indices_[k]];

    glm::mat3 A = glm::mat3(v1.position - v0.position,
                            v2.position - v0.position, -direction);
    if (std::abs(glm::determinant(A)) < 1e-9f) {
      continue;
    }
    A = glm::inverse(A);
    auto uvt = A * (origin - v0.position);
    auto &t = uvt.z;
    if (t < t_min || (result > 0.0f && t > result)) {
      continue;
    }
    auto &u = uvt.x;
    auto &v = uvt.y;
    auto w = 1.0f - u - v;
    auto position = origin + t * direction;
    if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
      result = t;
      if (hit_record) {
        auto geometry_normal = glm::normalize(
            glm::cross(v2.position - v0.position, v1.position - v0.position));
        if (glm::dot(geometry_normal, direction) < 0.0f) {
          hit_record->position = position;
          hit_record->geometry_normal = geometry_normal;
          hit_record->normal = v0.normal * w + v1.normal * u + v2.normal * v;
          hit_record->tangent =
              v0.tangent * w + v1.tangent * u + v2.tangent * v;
          hit_record->tex_coord =
              v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
          hit_record->front_face = true;
        } else {
          hit_record->position = position;
          hit_record->geometry_normal = -geometry_normal;
          hit_record->normal = -(v0.normal * w + v1.normal * u + v2.normal * v);
          hit_record->tangent =
              -(v0.tangent * w + v1.tangent * u + v2.tangent * v);
          hit_record->tex_coord =
              v0.tex_coord * w + v1.tex_coord * u + v2.tex_coord * v;
          hit_record->front_face = false;
        }
      }
    }
  }
  return result;
}

void Mesh::WriteObjFile(const std::string &file_path) const {
  std::ofstream file(file_path);
  if (file) {
    file.setf(std::ios::fixed);
    file.precision(7);
    for (auto &vertex : vertices_) {
      file << "v " << vertex.position.x << ' ' << vertex.position.y << ' '
           << vertex.position.z << "\n";
      file << "vn " << vertex.normal.x << ' ' << vertex.normal.y << ' '
           << vertex.normal.z << "\n";
      file << "vt " << vertex.tex_coord.x << ' ' << vertex.tex_coord.y << "\n";
    }
    for (auto i = 0; i < indices_.size(); i += 3) {
      file << "f " << indices_[i] + 1 << "/" << indices_[i] + 1 << "/"
           << indices_[i] + 1 << " " << indices_[i + 1] + 1 << "/"
           << indices_[i + 1] + 1 << "/" << indices_[i + 1] + 1 << " "
           << indices_[i + 2] + 1 << "/" << indices_[i + 2] + 1 << "/"
           << indices_[i + 2] + 1 << "\n";
    }
    file.close();
  }
}
std::vector<Vertex> Mesh::GetVertices() const {
  return vertices_;
}
std::vector<uint32_t> Mesh::GetIndices() const {
  return indices_;
}

bool Mesh::LoadObjFile(const std::string &obj_file_path, Mesh &mesh) {
  tinyobj::ObjReaderConfig reader_config;
  reader_config.mtl_search_path = "./";  // Path to material files

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(obj_file_path, reader_config)) {
    if (!reader.Error().empty()) {
      LAND_WARN("[Load obj, ERROR]: {}", reader.Error());
    }
    return false;
  }

  if (!reader.Warning().empty()) {
    LAND_WARN("{}", reader.Warning());
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();
  auto &materials = reader.GetMaterials();

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  // Loop over shapes
  for (size_t s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

      // Loop over vertices in the face.
      std::vector<Vertex> face_vertices;
      for (size_t v = 0; v < fv; v++) {
        Vertex vertex{};
        // access to vertex
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
        tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
        tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
        vertex.position = {vx, vy, vz};
        // Check if `normal_index` is zero or positive. negative = no normal
        // data
        if (idx.normal_index >= 0) {
          tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
          tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
          tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
          vertex.normal = {nx, ny, nz};
        } else {
          vertex.normal = {0.0f, 0.0f, 0.0f};
        }

        // Check if `texcoord_index` is zero or positive. negative = no texcoord
        // data
        if (idx.texcoord_index >= 0) {
          tinyobj::real_t tx =
              attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
          tinyobj::real_t ty =
              attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
          vertex.tex_coord = {tx, ty};
        }
        face_vertices.push_back(vertex);
      }

      for (int i = 1; i < face_vertices.size() - 1; i++) {
        Vertex v0 = face_vertices[0];
        Vertex v1 = face_vertices[i];
        Vertex v2 = face_vertices[i + 1];
        auto geometry_normal = glm::normalize(
            glm::cross(v2.position - v0.position, v1.position - v0.position));
        if (v0.normal == glm::vec3{0.0f, 0.0f, 0.0f}) {
          v0.normal = geometry_normal;
        } else if (glm::dot(geometry_normal, v0.normal) < 0.0f) {
          v0.normal = -v0.normal;
        }
        if (v1.normal == glm::vec3{0.0f, 0.0f, 0.0f}) {
          v1.normal = geometry_normal;
        } else if (glm::dot(geometry_normal, v1.normal) < 0.0f) {
          v1.normal = -v1.normal;
        }
        if (v2.normal == glm::vec3{0.0f, 0.0f, 0.0f}) {
          v2.normal = geometry_normal;
        } else if (glm::dot(geometry_normal, v2.normal) < 0.0f) {
          v2.normal = -v2.normal;
        }
        indices.push_back(vertices.size());
        indices.push_back(vertices.size() + 1);
        indices.push_back(vertices.size() + 2);
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
      }

      index_offset += fv;
    }
  }
  mesh = Mesh(vertices, indices);
  mesh.MergeVertices();
  return true;
}

void Mesh::MergeVertices() {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::unordered_map<Vertex, uint32_t, VertexHash> vertex_index_map;
  auto index_func = [&vertices, &vertex_index_map](const Vertex &v) {
    if (vertex_index_map.count(v)) {
      return vertex_index_map.at(v);
    }
    uint32_t res = vertices.size();
    vertex_index_map[v] = res;
    vertices.push_back(v);
    return res;
  };
  for (auto ind : indices_) {
    indices.push_back(index_func(vertices_[ind]));
  }
  vertices_ = vertices;
  indices_ = indices;
}

const char *Mesh::GetDefaultEntityName() {
  return "Mesh";
}

Mesh::Mesh(const tinyxml2::XMLElement *element) {
  std::string mesh_type{};
  auto element_type = element->FindAttribute("type");
  if (element_type) {
    mesh_type = element_type->Value();
  }

  if (mesh_type == "sphere") {
    glm::vec3 center{0.0f};
    float radius{1.0f};

    auto child_element = element->FirstChildElement("center");
    if (child_element) {
      center = StringToVec3(child_element->FindAttribute("value")->Value());
    }

    child_element = element->FirstChildElement("radius");
    if (child_element) {
      radius = std::stof(child_element->FindAttribute("value")->Value());
    }

    *this = Mesh::Sphere(center, radius);
  } else if (mesh_type == "obj") {
    Mesh::LoadObjFile(
        element->FirstChildElement("filename")->FindAttribute("value")->Value(),
        *this);
  } else {
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};
    for (auto vertex_element = element->FirstChildElement("vertex");
         vertex_element;
         vertex_element = vertex_element->NextSiblingElement("vertex")) {
      Vertex vertex{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}};
      auto attribute = vertex_element->FindAttribute("position");
      if (attribute) {
        vertex.position = StringToVec3(attribute->Value());
      }
      attribute = vertex_element->FindAttribute("normal");
      if (attribute) {
        vertex.normal = StringToVec3(attribute->Value());
      }
      attribute = vertex_element->FindAttribute("tex_coord");
      if (attribute) {
        vertex.tex_coord = StringToVec2(attribute->Value());
      }
      vertices.push_back(vertex);
    }

    for (auto index_element = element->FirstChildElement("index");
         index_element;
         index_element = index_element->NextSiblingElement("index")) {
      uint32_t index =
          std::stoul(index_element->FindAttribute("value")->Value());
      indices.push_back(index);
    }

    for (int i = 0; i < indices.size(); i += 3) {
      auto v0 = vertices[indices[i]];
      auto v1 = vertices[indices[i + 1]];
      auto v2 = vertices[indices[i + 2]];
      auto geom_normal = glm::normalize(
          glm::cross(v1.position - v0.position, v2.position - v0.position));
      if (glm::length(v0.normal) < 0.5f) {
        v0.normal = geom_normal;
      }
      if (glm::length(v1.normal) < 0.5f) {
        v1.normal = geom_normal;
      }
      if (glm::length(v2.normal) < 0.5f) {
        v2.normal = geom_normal;
      }
      vertices_.push_back(v0);
      vertices_.push_back(v1);
      vertices_.push_back(v2);
      indices_.push_back(i);
      indices_.push_back(i + 1);
      indices_.push_back(i + 2);
    }
    MergeVertices();
  }
}

}  // namespace sparks
