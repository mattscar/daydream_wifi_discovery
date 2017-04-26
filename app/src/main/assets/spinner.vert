#version 300 es

// Attribute data
in  vec2 in_coords;
out vec3 new_color;

// Uniform buffer object
uniform ubo {
  mat4 trans_matrix;
  vec2 target;
  vec2 offset[128];
  int selected_index;
};

void main(void) {

  // Rotate the incoming vertex
  vec4 new_coords = vec4(in_coords, -4.9, 1.0);

  // Apply the offset and the transformation
  new_coords = trans_matrix * new_coords;

  // Set the output coordinates
  gl_Position = new_coords;

  // Set the outgoing color
  new_color = vec3(1.0, 1.0, 1.0);
}