#version 450

layout(push_constant) uniform upc {
  vec2 aa;
  vec2 bb;
  vec2 uva;
  vec2 uvb;
  vec2 scale;
};

layout(location = 0) out vec2 f_uv;

vec2 gen_pos() {
  switch (gl_VertexIndex) {
    case 0: return vec2(0, 0);
    case 1: return vec2(1, 1);
    case 2: return vec2(1, 0);
    case 3: return vec2(1, 1);
    case 4: return vec2(0, 0);
    case 5: return vec2(0, 1);
  }
}
void main() {
  vec2 pos = gen_pos();
  vec2 p = mix(aa, bb, pos) / scale;
  gl_Position = vec4(p, 0, 1);
  f_uv = mix(uva, uvb, pos) / 1024;
}
