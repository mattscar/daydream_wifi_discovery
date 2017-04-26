#version 300 es

uniform sampler2D texSampler;

in vec2 new_texcoords;
in float selected;
out vec4 out_color;

void main() {
  vec4 color = vec4(texture(texSampler, new_texcoords));
  if(color.r < 0.1f) {
    out_color = vec4(1.0f, 1.0f, 1.0f, 0.0f);
  }
  else {
    out_color = vec4(color.r, color.r, color.r, 1.0f);
  }
}