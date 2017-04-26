#version 300 es

uniform sampler2D texSampler;

in vec2 new_texcoords;
out vec4 out_color;

void main() {
  vec4 color = vec4(texture(texSampler, new_texcoords));
  if(color.r < 0.2f)
    out_color = vec4(1.0f, 1.0f, 1.0f, 0.0f);
  else if((color.r > 0.8) && (color.r < 0.95)) {
    out_color = vec4(0.4f, 0.4f, 0.4f, 1.0f);
  }
  else {
    out_color = vec4(color.r, color.r, color.r, 1.0f);
  }
}