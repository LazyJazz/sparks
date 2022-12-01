
struct GlobalUniformObject {
  mat4 projection;
  mat4 camera;
  int envmap_id;
  float envmap_offset;
};

struct EntityUniformObject {
  mat4 model;
};
