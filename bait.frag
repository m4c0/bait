#version 450

layout(push_constant) uniform upc { float aspect; } pc;

layout(location = 0) in vec2 frag_coord;
layout(location = 1) in vec4 gradient;

layout(location = 0) out vec4 frag_colour;

// from a to b with THickness
vec2 dvf_rect(vec2 p, vec2 a, vec2 b, float th) {
  float l = length(b - a);
  vec2 d = (b - a) / l;
  vec2 q = p - (a + b) * 0.5;
  return mat2(d.x, -d.y, d.y, d.x) * q;
}

// b = half size
float sd_box(vec2 p, vec2 b) {
  vec2 d = abs(p) - b;
  return length(max(d, 0)) + min(max(d.x, d.y), 0);
}

float sd_rect(vec2 p, vec2 a, vec2 b, float th) {
  vec2 q = dvf_rect(p, a, b, th);
  return sd_box(q, vec2(length(b - a) * 0.5, th));
}

vec4 sq(vec2 a, vec2 b, float s) {
  b = a + s * normalize(b);

  float th = 0.2;

  vec2 dv = dvf_rect(frag_coord, a, b, th);
  float d = sd_box(dv, vec2(th));

  vec2 uv = dv * 2.0 + 0.5;

  d = 0.005 / (d - th * 0.1);
  vec3 m = mix(vec3(uv, 1), vec3(d), step(0, d));
  return vec4(m, d);
}

void main() {
  vec4 s1 = sq(vec2(0.5, 0.2), vec2(0.6, -0.1), 0.4);
  vec4 s2 = sq(vec2(-0.9, -0.1), vec2(0.6, 0.1), 0.3);

  vec3 m = s1.xyz + s2.xyz;

  frag_colour = vec4(m, 1);
}
