#version 450

layout(push_constant) uniform upc {
  vec2 aa;
  vec2 bb;
  vec2 uva;
  vec2 uvb;
  vec2 scale;
};

layout(location = 0) in vec2 pos;

layout(location = 0) out vec2 f_uv;

void main() {
  vec2 p = mix(aa, bb, pos) / scale;
  gl_Position = vec4(p, 0, 1);
  f_uv = mix(uva, uvb, pos) / 1024;
}
