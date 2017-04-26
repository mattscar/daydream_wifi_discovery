#version 300 es

uniform sampler2D texSampler;

in float new_color;
out vec4 out_color;

void main() {
  out_color = vec4(new_color, new_color, new_color, 1.0);
}