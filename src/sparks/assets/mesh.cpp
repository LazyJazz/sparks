#include "sparks/assets/mesh.h"

#include "fstream"
#include "iomanip"
#include "iostream"

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

}  // namespace sparks
