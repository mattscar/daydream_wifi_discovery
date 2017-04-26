#version 300 es

in vec2 in_coords;
in vec2 in_texcoords;
out vec2 new_texcoords;
out float selected;

// Uniform buffer object
uniform ubo {
  mat4 trans_matrix;
  vec2 target;
  vec2 offset[128];
  int selected_index;
};

void main(void) {

  // Transform the incoming vertex
  vec4 new_coords = vec4(in_coords + offset[gl_InstanceID], -5.0, 1.0);
  new_coords = trans_matrix * new_coords;

  // Set the output coordinates
  gl_Position = new_coords;

  // Set texture coordinates
  new_texcoords = in_texcoords;

  // Determine if the host is selected
  selected = float(selected_index - gl_InstanceID);
}
