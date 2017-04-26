#version 300 es

in  vec3 new_color;
out vec4 out_color;

void main() {
  out_color = vec4(new_color, 1.0);
}