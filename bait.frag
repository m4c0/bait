#version 450

layout(push_constant) uniform upc {
  float aspect;
} pc;

layout(location = 0) in vec2 frag_coord;
layout(location = 1) in vec4 gradient;

layout(location = 0) out vec4 frag_colour;

float sd(vec2 p, vec2 a, vec2 b, float th) {
  float l = length(b - a);
  vec2 d = (b - a) / l;
  vec2 q = p - (a + b) * 0.5;
  q = mat2(d.x, -d.y, d.y, d.x) * q;
  q = abs(q) - vec2(l * 0.5, th);
  return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);    
}

void main() {
  vec2 uv = (frag_coord + 1.0) * 0.5;
  uv.x = uv.x * pc.aspect;

  float d = sd(uv, vec2(0.5, 0.2), vec2(0.6, 0.1), 0.2);

  d = abs(d);
  d = 0.001 / d;

  frag_colour = vec4(d, d, d, 1);
}
