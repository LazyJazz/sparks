
struct GlobalUniformObject {
  mat4 projection;
  mat4 camera;
  int envmap_id;
  float envmap_offset;
  int hover_id;
  int selected_id;
};

struct EntityUniformObject {
  mat4 model;
};
